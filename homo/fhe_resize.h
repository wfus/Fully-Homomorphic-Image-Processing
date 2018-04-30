#ifndef FHE_RESIZE_H
#define FHE_RESIZE_H

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <cmath>
#include <fstream>

#include "seal/seal.h"
#include <opencv2/opencv.hpp>

#define BILINEAR 0
#define BICUBIC 1

using namespace seal;
using namespace cv;

#define CLAMP(v, min, max) if (v < min) { v = min; } else if (v > max) { v = max; } 


void resize_image_opencv(const char* im_name, int new_width, int new_height) {
    Mat image;
    Mat resized_image;
    image = imread(im_name, IMREAD_COLOR);
    Size size(new_width, new_height);
    resize(image, resized_image, size, 0, 0, INTER_LINEAR);
    namedWindow("Result", WINDOW_AUTOSIZE);
    imshow("Result", resized_image);
    waitKey(0);
}


void compare_resize_opencv(const char* im_name, int new_width, int new_height, bool bicubic,
                           std::vector<uint8_t> &interleaved) {
    // Convert interleaved to BGR to be the same as OPENCV, rip

    std::vector<uint8_t> bgr_interleaved;
    for (int i = 0; i < interleaved.size(); i += 3) {
        bgr_interleaved.push_back(interleaved[i+2]);
        bgr_interleaved.push_back(interleaved[i+1]);
        bgr_interleaved.push_back(interleaved[i+0]);
    }
    Mat image;
    Mat resized_image;
    image = imread(im_name, IMREAD_COLOR);
    Size size(new_width, new_height);
    auto INTERPOLATION_FLAG = bicubic ? INTER_CUBIC : INTER_LINEAR;
    resize(image, resized_image, size, 0, 0, INTERPOLATION_FLAG);

    // Compare bgr_interleaved with RESIZE_PICTURE
    int running_error = 0;
    for (int i = 0; i < new_height; i++) {
        for (int j = 0; j < new_width; j++) {
            Vec3b bgr_pixel = resized_image.at<Vec3b>(i, j);
            uint8_t blue = bgr_interleaved[i*new_width*3 + j*3 + 0];
            uint8_t green = bgr_interleaved[i*new_width*3 + j*3 + 1];
            uint8_t red = bgr_interleaved[i*new_width*3 + j*3 + 2];
            running_error += (bgr_pixel.val[0]-blue)*(bgr_pixel.val[0]-blue);
            running_error += (bgr_pixel.val[1]-green)*(bgr_pixel.val[1]-green);
            running_error += (bgr_pixel.val[2]-red)*(bgr_pixel.val[2]-red);
        }
    }
    double average_error = ((double) running_error) / (3 * new_width * new_height);
    double rms_error = std::sqrt(average_error);
    std::cout << "RMSError," << rms_error << ',' << std::endl;
}



void show_image(const char* im_name) {
    Mat image;
    image = imread(im_name, 1);

    if (!image.data) {
        std::cout << "No image data..." << std::endl;
        return;
    }
    namedWindow("Display Image", WINDOW_AUTOSIZE);
    imshow("Display Image", image);
    waitKey(0);
}

void show_image_rgb(int width, int height, std::vector<uint8_t> &interleaved) {
    std::vector<uint8_t> bgr_interleaved;
    for (int i = 0; i < interleaved.size(); i += 3) {
        bgr_interleaved.push_back(interleaved[i+2]);
        bgr_interleaved.push_back(interleaved[i+1]);
        bgr_interleaved.push_back(interleaved[i+0]);
    }
    uint8_t* bgr_array = &bgr_interleaved[0];
    Mat image(height, width, CV_8UC3, bgr_array);
    namedWindow("result", WINDOW_AUTOSIZE);
    imshow("result", image);
    waitKey(0);
}

// THIS ONLY WORKS FOR UNIX
#ifdef linux
    void save_image_rgb(int width, int height, std::vector<uint8_t> &interleaved, std::string fname) {
        std::vector<uint8_t> bgr_interleaved;
        for (int i = 0; i < interleaved.size(); i += 3) {
            bgr_interleaved.push_back(interleaved[i+2]);
            bgr_interleaved.push_back(interleaved[i+1]);
            bgr_interleaved.push_back(interleaved[i+0]);
        }
        uint8_t* bgr_array = &bgr_interleaved[0];
        Mat image(height, width, CV_8UC3, bgr_array);

        vector<int> compression_params;
        compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
        compression_params.push_back(9);

        imwrite(fname.c_str(), image, compression_params);
    }
#endif


void show_image_rgb(int width, int height, 
                    std::vector<uint8_t> &red, 
                    std::vector<uint8_t> &green, 
                    std::vector<uint8_t> &blue) {
    std::vector<uint8_t> combined_rgb;
    for (int i = 0; i < red.size(); i++) {
        combined_rgb.push_back(blue[i]);
        combined_rgb.push_back(green[i]);
        combined_rgb.push_back(red[i]);
    }
    uint8_t* rgb_array = &combined_rgb[0]; 
    // Format for creating an opencv MAT:
    // CV_[The number of bits per item][Signed or Unsigned][Type Prefix]C[The channel number]
    Mat image(height, width, CV_8UC3, rgb_array); // create an image of all blue
    if (!image.data) {
        std::cout << "No image data..." << std::endl;
        return;
    }
    namedWindow("Display Image", WINDOW_AUTOSIZE);
    imshow("Display Image", image);
    waitKey(0);
}

inline void Cubic(Ciphertext &result, Ciphertext &A, Ciphertext &B, Ciphertext &C, Ciphertext &D, Ciphertext &t,
                            Evaluator &evaluator, 
                            FractionalEncoder &encoder, 
                            Encryptor &encryptor) {
    auto start = std::chrono::steady_clock::now();
    Ciphertext a, b, c, d;
    Ciphertext boaz1(A); 
    Ciphertext boaz2(B); evaluator.multiply_plain(boaz2, encoder.encode(3)); 
    evaluator.sub(boaz2, boaz1);
    Ciphertext boaz3(C); evaluator.multiply_plain(boaz3, encoder.encode(3)); 
    evaluator.sub(boaz2, boaz3); 
    Ciphertext boaz4(D);
    evaluator.add(boaz2, boaz4);
    a = boaz2;

    Ciphertext boaz5(A); evaluator.multiply_plain(boaz5, encoder.encode(2));
    Ciphertext boaz6(B); evaluator.multiply_plain(boaz6, encoder.encode(5));
    evaluator.sub(boaz5, boaz6); 
    Ciphertext boaz7(C); evaluator.multiply_plain(boaz7, encoder.encode(4)); 
    evaluator.add(boaz5, boaz7);
    Ciphertext boaz8(D);
    evaluator.sub(boaz5, boaz8); 
    b = boaz5;

    Ciphertext boaz9(A); 
    Ciphertext boaz10(C);
    evaluator.sub(boaz10, boaz9);
    c = boaz10;

    d = B;

    Ciphertext t2(t); evaluator.square(t2);
    Ciphertext t3(t); evaluator.multiply(t3, t);

    evaluator.multiply(a, t3);
    evaluator.multiply(b, t2);
    evaluator.multiply(c, t); 
        
    evaluator.add(a, b);
    evaluator.add(a, c);
    evaluator.multiply_plain(a, encoder.encode(0.5));
    evaluator.add(a, d);
    result = a;
    auto diff = std::chrono::steady_clock::now() - start;
    std::cout << chrono::duration<double, milli>(diff).count() << ',';
    return;
}

inline void Linear(Ciphertext &result, Ciphertext &A, Ciphertext &B, Ciphertext &t,
                   Evaluator &evaluator, 
                   FractionalEncoder &encoder, 
                   Encryptor &encryptor) {
    auto start = std::chrono::steady_clock::now();
    Ciphertext boaz1(t); evaluator.negate(boaz1); evaluator.add_plain(boaz1, encoder.encode(1.0)); 
    evaluator.multiply(boaz1, A); 
    Ciphertext boaz2(B); evaluator.multiply(boaz2, t); 
    evaluator.add(boaz1, boaz2); 
    result = boaz1;
    auto diff = std::chrono::steady_clock::now() - start;
    std::cout << chrono::duration<double, milli>(diff).count() << ',';
    return;
}


struct SImageData
{
    int width;
    int height;
    int start;
    std::vector<std::vector<Ciphertext>> pixels;
};
 
inline std::vector<Ciphertext> GetPixelClamped (const SImageData& image, int x, int y)
{
    CLAMP(x, 0, image.width - 1);
    CLAMP(y, 0, image.height - 1);    
    return image.pixels[y * image.width + x - image.start];
}

inline void SampleLinear (std::vector<Ciphertext> &ret, const SImageData& image, float x, float y,
                    Evaluator &evaluator, 
                    FractionalEncoder &encoder, 
                    Encryptor &encryptor)
{
    // calculate coordinates -> also need to offset by half a pixel to keep image from shifting down and left half a pixel
    int xint = int(x);
    Ciphertext xfract;
    encryptor.encrypt(encoder.encode(x - floor(x)), xfract);
    
    int yint = int(y);
    Ciphertext yfract;
    encryptor.encrypt(encoder.encode(y - floor(y)), yfract);

    // get pixels
    auto p00 = GetPixelClamped(image, xint + 0, yint + 0);
    auto p10 = GetPixelClamped(image, xint + 1, yint + 0);
    auto p01 = GetPixelClamped(image, xint + 0, yint + 1);
    auto p11 = GetPixelClamped(image, xint + 1, yint + 1);
 
    // interpolate bi-linearly!
    Ciphertext col0, col1;
    
    for (int i = 0; i < 3; ++i)
    {
        Linear(col0, p00[i], p10[i], xfract, evaluator, encoder, encryptor);
        Linear(col1, p01[i], p11[i], xfract, evaluator, encoder, encryptor);
        Linear(ret[i], col0, col1, yfract, evaluator, encoder, encryptor);
    }
    return; 
}

inline void SampleBicubic (std::vector<Ciphertext> &ret, const SImageData& image, float x, float y,
                    Evaluator &evaluator, 
                    FractionalEncoder &encoder, 
                    Encryptor &encryptor)
{
    // calculate coordinates -> also need to offset by half a pixel to keep image from shifting down and left half a pixel
    int xint = int(x);
    Ciphertext xfract;
    encryptor.encrypt(encoder.encode(x - floor(x)), xfract);
 
    int yint = int(y);
    Ciphertext yfract;
    encryptor.encrypt(encoder.encode(y - floor(y)), yfract);

    // 1st row
    auto p00 = GetPixelClamped(image, xint - 1, yint - 1);
    auto p10 = GetPixelClamped(image, xint + 0, yint - 1);
    auto p20 = GetPixelClamped(image, xint + 1, yint - 1);
    auto p30 = GetPixelClamped(image, xint + 2, yint - 1);
 
    // 2nd row
    auto p01 = GetPixelClamped(image, xint - 1, yint + 0);
    auto p11 = GetPixelClamped(image, xint + 0, yint + 0);
    auto p21 = GetPixelClamped(image, xint + 1, yint + 0);
    auto p31 = GetPixelClamped(image, xint + 2, yint + 0);
 
    // 3rd row
    auto p02 = GetPixelClamped(image, xint - 1, yint + 1);
    auto p12 = GetPixelClamped(image, xint + 0, yint + 1);
    auto p22 = GetPixelClamped(image, xint + 1, yint + 1);
    auto p32 = GetPixelClamped(image, xint + 2, yint + 1);
 
    // 4th row
    auto p03 = GetPixelClamped(image, xint - 1, yint + 2);
    auto p13 = GetPixelClamped(image, xint + 0, yint + 2);
    auto p23 = GetPixelClamped(image, xint + 1, yint + 2);
    auto p33 = GetPixelClamped(image, xint + 2, yint + 2);
 
    // interpolate bi-cubically!
    // Clamp the values since the curve can put the value below 0 or above 255
    
    Ciphertext col0, col1, col2, col3;
    for (int i = 0; i < 3; ++i)
    {
        Cubic(col0, p00[i], p10[i], p20[i], p30[i], xfract, evaluator, encoder, encryptor);
        Cubic(col1, p01[i], p11[i], p21[i], p31[i], xfract, evaluator, encoder, encryptor);
        Cubic(col2, p02[i], p12[i], p22[i], p32[i], xfract, evaluator, encoder, encryptor);
        Cubic(col3, p03[i], p13[i], p23[i], p33[i], xfract, evaluator, encoder, encryptor);
        Cubic(ret[i], col0, col1, col2, col3, yfract, evaluator, encoder, encryptor);
    }
    return;
}


void ResizeImage (String infile_str, int original_width, int original_height, 
                    SImageData &destImage, int dest_width, int dest_height, int inter,
                    Evaluator &evaluator, 
                    FractionalEncoder &encoder, 
                    Encryptor &encryptor)
{
    std::ifstream infile;
    infile.open(infile_str.c_str());

    SImageData srcImage;
    std::vector<std::vector<Ciphertext>> src_cpixels;
    srcImage.width = original_width;
    srcImage.height = original_height;
    srcImage.start = 0;
    srcImage.pixels = src_cpixels;
    
    int init_rows = 0;
    Ciphertext c;
    if (inter == BILINEAR) {
        init_rows = 2;
    } else if (inter == BICUBIC) {
        init_rows = 4;
    }
    int read = 0;
    for (int i = 0; i < original_width * init_rows; i++) {
        std::vector<Ciphertext> pixel;
        c.load(infile);
        pixel.push_back(c);
        c.load(infile);
        pixel.push_back(c);
        c.load(infile);
        pixel.push_back(c);
        srcImage.pixels.push_back(pixel);
        read ++;
    }

    std::vector<std::vector<Ciphertext>> dest_cpixels;
    destImage.width = dest_width;
    destImage.height = dest_height;
    destImage.start = 0;
    destImage.pixels = dest_cpixels;
    
    for (int y = 0; y < destImage.height; ++y){
        float v = float(y) / float(destImage.height - 1) * float(srcImage.height) - 0.5;
        int new_start = min(int(v) - init_rows / 2 + 1, srcImage.height - init_rows) * srcImage.width; 
        if (new_start > srcImage.start) { 
            for (int i = srcImage.start + init_rows * srcImage.width; i < new_start; i++) {
                for (int j = 0; j < 3; j++) {
                    c.load(infile);
                }
            }

            std::vector<std::vector<Ciphertext>> new_pixels;
            int present = 0;
            for (int i = new_start - srcImage.start; i < init_rows * srcImage.width; i++) {
                new_pixels.push_back(srcImage.pixels[i]);
                present++;
            }
            for (int i = present; i < init_rows * srcImage.width; i++) {
                std::vector<Ciphertext> pixel;
                c.load(infile);
                pixel.push_back(c);
                c.load(infile);
                pixel.push_back(c);
                c.load(infile);
                pixel.push_back(c);
                new_pixels.push_back(pixel);
                read++;
            }
            srcImage.start = new_start;
            srcImage.pixels = new_pixels;
        }

        for (int x = 0; x < destImage.width; ++x) {
            float u = float(x) / float(destImage.width - 1) * float(srcImage.width) - 0.5;
            std::vector<Ciphertext> sample(3);
            if (inter == BILINEAR) {
                SampleLinear(sample, srcImage, u, v, evaluator, encoder, encryptor);
            } else if (inter == BICUBIC) {
                SampleBicubic(sample, srcImage, u, v, evaluator, encoder, encryptor);
            }
            destImage.pixels.push_back(sample);
        }
    }
}




#endif