#ifndef FHE_RESIZE_H
#define FHE_RESIZE_H

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <cmath>

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

inline void CubicHermite(Ciphertext &result, Ciphertext &A, Ciphertext &B, Ciphertext &C, Ciphertext &D, Ciphertext &t,
                            Evaluator &evaluator, 
                            FractionalEncoder &encoder, 
                            Encryptor &encryptor) {
    Ciphertext a, b, c;
    Ciphertext boaz1(A); evaluator.multiply_plain(boaz1, encoder.encode(2.0)); Ciphertext boaz2(boaz1); evaluator.sub_plain(boaz2, encoder.encode(0.0)); Ciphertext boaz3(B); evaluator.multiply_plain(boaz3, encoder.encode(3.0)); Ciphertext boaz4(boaz3); evaluator.multiply_plain(boaz4, encoder.encode(2.0)); Ciphertext boaz5(boaz2); evaluator.add(boaz5, boaz4); Ciphertext boaz6(C); evaluator.multiply_plain(boaz6, encoder.encode(3.0)); Ciphertext boaz7(boaz6); evaluator.multiply_plain(boaz7, encoder.encode(2.0)); Ciphertext boaz8(boaz5); evaluator.sub(boaz8, boaz7); Ciphertext boaz9(D); evaluator.multiply_plain(boaz9, encoder.encode(2.0)); Ciphertext boaz10(boaz8); evaluator.add(boaz10, boaz9); a = boaz10;
    Ciphertext boaz11(B); evaluator.multiply_plain(boaz11, encoder.encode(5.0)); Ciphertext boaz12(boaz11); evaluator.multiply_plain(boaz12, encoder.encode(2.0)); Ciphertext boaz13(A); evaluator.sub(boaz13, boaz12); Ciphertext boaz14(C); evaluator.multiply_plain(boaz14, encoder.encode(2.0)); Ciphertext boaz15(boaz13); evaluator.add(boaz15, boaz14); Ciphertext boaz16(D); evaluator.multiply_plain(boaz16, encoder.encode(2.0)); Ciphertext boaz17(boaz15); evaluator.sub(boaz17, boaz16); b = boaz17;
    Ciphertext boaz18(A); evaluator.multiply_plain(boaz18, encoder.encode(2.0)); Ciphertext boaz19(boaz18); evaluator.sub_plain(boaz19, encoder.encode(0.0)); Ciphertext boaz20(C); evaluator.multiply_plain(boaz20, encoder.encode(2.0)); Ciphertext boaz21(boaz19); evaluator.add(boaz21, boaz20); c = boaz21;
    Ciphertext boaz22(a); evaluator.multiply(boaz22, t); Ciphertext boaz23(boaz22); evaluator.multiply(boaz23, t); Ciphertext boaz24(boaz23); evaluator.multiply(boaz24, t); Ciphertext boaz25(b); evaluator.multiply(boaz25, t); Ciphertext boaz26(boaz25); evaluator.multiply(boaz26, t); Ciphertext boaz27(boaz24); evaluator.add(boaz27, boaz26); Ciphertext boaz28(c); evaluator.multiply(boaz28, t); Ciphertext boaz29(boaz27); evaluator.add(boaz29, boaz28); Ciphertext boaz30(boaz29); evaluator.add(boaz30, B); result = boaz30;
    return;
}


inline void Lerp(Ciphertext &result, Ciphertext &A, Ciphertext &B, Ciphertext &t,
                    Evaluator &evaluator, 
                    FractionalEncoder &encoder, 
                    Encryptor &encryptor) {
    Ciphertext boaz1(t); evaluator.sub_plain(boaz1, encoder.encode(1.0)); Ciphertext boaz2(A); evaluator.multiply(boaz2, boaz1); Ciphertext boaz3(B); evaluator.multiply(boaz3, t); Ciphertext boaz4(boaz2); evaluator.add(boaz4, boaz3); result = boaz4;
    return;
}

struct SImageData
{
    long width;
    long height;
    std::vector<std::vector<Ciphertext>> pixels;
};
 
inline std::vector<Ciphertext> GetPixelClamped (const SImageData& image, int x, int y)
{
    CLAMP(x, 0, image.width - 1);
    CLAMP(y, 0, image.height - 1);    
    return image.pixels[x * image.width + y];
}

void SampleLinear (std::vector<Ciphertext> &ret, const SImageData& image, float u, float v,
                    Evaluator &evaluator, 
                    FractionalEncoder &encoder, 
                    Encryptor &encryptor)
{
    // calculate coordinates -> also need to offset by half a pixel to keep image from shifting down and left half a pixel
    float x = (u * image.width) - 0.5;
    int xint = int(x);
    Ciphertext xfract;
    encryptor.encrypt(encoder.encode(x - floor(x)), xfract);
    // float xfract = x - floor(x);
 
    float y = (v * image.height) - 0.5;
    int yint = int(y);
    Ciphertext yfract;
    encryptor.encrypt(encoder.encode(y - floor(y)), yfract);
    // float yfract = y - floor(y);

    // get pixels
    auto p00 = GetPixelClamped(image, xint + 0, yint + 0);
    auto p10 = GetPixelClamped(image, xint + 1, yint + 0);
    auto p01 = GetPixelClamped(image, xint + 0, yint + 1);
    auto p11 = GetPixelClamped(image, xint + 1, yint + 1);
 
    // interpolate bi-linearly!
    Ciphertext col0, col1;

    // need to add encryption of xfract, yfract
    for (int i = 0; i < 3; ++i)
    {
        Lerp(col0, p00[i], p10[i], xfract, evaluator, encoder, encryptor);
        Lerp(col1, p01[i], p11[i], xfract, evaluator, encoder, encryptor);
        Lerp(ret[i], col0, col1, yfract, evaluator, encoder, encryptor);
    }
    
    return; 
}

void SampleBicubic (std::vector<Ciphertext> &ret, const SImageData& image, float u, float v,
                    Evaluator &evaluator, 
                    FractionalEncoder &encoder, 
                    Encryptor &encryptor)
{
    // calculate coordinates -> also need to offset by half a pixel to keep image from shifting down and left half a pixel
    float x = (u * image.width) - 0.5;
    int xint = int(x);
    Ciphertext xfract;
    encryptor.encrypt(encoder.encode(x - floor(x)), xfract);
    // float xfract = x - floor(x);
 
    float y = (v * image.height) - 0.5;
    int yint = int(y);
    Ciphertext yfract;
    encryptor.encrypt(encoder.encode(y - floor(y)), yfract);
    // float yfract = y - floor(y);
 
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
    // need to add encryption of xfract, yfract
    for (int i = 0; i < 3; ++i)
    {
        CubicHermite(col0, p00[i], p10[i], p20[i], p30[i], xfract, evaluator, encoder, encryptor);
        CubicHermite(col1, p01[i], p11[i], p21[i], p31[i], xfract, evaluator, encoder, encryptor);
        CubicHermite(col2, p02[i], p12[i], p22[i], p32[i], xfract, evaluator, encoder, encryptor);
        CubicHermite(col3, p03[i], p13[i], p23[i], p33[i], xfract, evaluator, encoder, encryptor);
        CubicHermite(ret[i], col0, col1, col2, col3, yfract, evaluator, encoder, encryptor);
    }
    return;
}


void ResizeImage (const SImageData &srcImage, SImageData &destImage, int dest_width, int dest_height, int inter,
                    Evaluator &evaluator, 
                    FractionalEncoder &encoder, 
                    Encryptor &encryptor)
{
    std::vector<std::vector<Ciphertext>> dest_cpixels;
    destImage.width = dest_width;
    destImage.height = dest_height;
    destImage.pixels = dest_cpixels;
    for (int y = 0; y < destImage.height; ++y)
    {
        float v = float(y) / float(destImage.height - 1);
        for (int x = 0; x < destImage.width; ++x)
        {
            float u = float(x) / float(destImage.width - 1);
            std::vector<Ciphertext> sample(3);
            if (inter == BILINEAR)
                SampleLinear(sample, srcImage, u, v, evaluator, encoder, encryptor);
            else if (inter == BICUBIC)
                SampleBicubic(sample, srcImage, u, v, evaluator, encoder, encryptor);
            destImage.pixels.push_back(sample);
        }
    }
}


#endif