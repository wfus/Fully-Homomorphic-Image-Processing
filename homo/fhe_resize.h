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

using namespace seal;
using namespace cv;

#define CLAMP(v, min, max) if (v < min) { v = min; } else if (v > max) { v = max; } 


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

void show_image_rgb() {
    return;
}

inline void CubicHermite(std::vector<Ciphertext> &r, 
                            std::vector<Ciphertext> A, 
                            std::vector<Ciphertext> B, 
                            std::vector<Ciphertext> C, 
                            std::vector<Ciphertext> D,
                            std::vector<Ciphertext> t,
                            Evaluator &evaluator, 
                            FractionalEncoder &encoder, 
                            Encryptor &encryptor) {
    Ciphertext a, b, c, result
    Ciphertext boaz1(A); evaluator.multiply_plain(boaz1, encoder.encode(2.0)); Ciphertext boaz2(boaz1); evaluator.sub_plain(boaz2, encoder.encode(0.0)); Ciphertext boaz3(B); evaluator.multiply_plain(boaz3, encoder.encode(3.0)); Ciphertext boaz4(boaz3); evaluator.multiply_plain(boaz4, encoder.encode(2.0)); Ciphertext boaz5(boaz2); evaluator.add(boaz5, boaz4); Ciphertext boaz6(C); evaluator.multiply_plain(boaz6, encoder.encode(3.0)); Ciphertext boaz7(boaz6); evaluator.multiply_plain(boaz7, encoder.encode(2.0)); Ciphertext boaz8(boaz5); evaluator.sub(boaz8, boaz7); Ciphertext boaz9(D); evaluator.multiply_plain(boaz9, encoder.encode(2.0)); Ciphertext boaz10(boaz8); evaluator.add(boaz10, boaz9); a = boaz10;
    Ciphertext boaz11(B); evaluator.multiply_plain(boaz11, encoder.encode(5.0)); Ciphertext boaz12(boaz11); evaluator.multiply_plain(boaz12, encoder.encode(2.0)); Ciphertext boaz13(A); evaluator.sub(boaz13, boaz12); Ciphertext boaz14(C); evaluator.multiply_plain(boaz14, encoder.encode(2.0)); Ciphertext boaz15(boaz13); evaluator.add(boaz15, boaz14); Ciphertext boaz16(D); evaluator.multiply_plain(boaz16, encoder.encode(2.0)); Ciphertext boaz17(boaz15); evaluator.sub(boaz17, boaz16); b = boaz17;
    Ciphertext boaz18(A); evaluator.multiply_plain(boaz18, encoder.encode(2.0)); Ciphertext boaz19(boaz18); evaluator.sub_plain(boaz19, encoder.encode(0.0)); Ciphertext boaz20(C); evaluator.multiply_plain(boaz20, encoder.encode(2.0)); Ciphertext boaz21(boaz19); evaluator.add(boaz21, boaz20); c = boaz21;
    Ciphertext boaz22(a); evaluator.multiply(boaz22, t); Ciphertext boaz23(boaz22); evaluator.multiply(boaz23, t); Ciphertext boaz24(boaz23); evaluator.multiply(boaz24, t); Ciphertext boaz25(b); evaluator.multiply(boaz25, t); Ciphertext boaz26(boaz25); evaluator.multiply(boaz26, t); Ciphertext boaz27(boaz24); evaluator.add(boaz27, boaz26); Ciphertext boaz28(c); evaluator.multiply(boaz28, t); Ciphertext boaz29(boaz27); evaluator.add(boaz29, boaz28); Ciphertext boaz30(boaz29); evaluator.add(boaz30, B); result = boaz30;
    *r = result;
    return ;
}


inline void LERP(std::vector<Ciphertext> &r, 
                            std::vector<Ciphertext> A, 
                            std::vector<Ciphertext> B, 
                            std::vector<Ciphertext> t,
                            Evaluator &evaluator, 
                            FractionalEncoder &encoder, 
                            Encryptor &encryptor) {
    Ciphertext result;
    Ciphertext boaz1(t); evaluator.sub_plain(boaz1, encoder.encode(1.0)); Ciphertext boaz2(A); evaluator.multiply(boaz2, boaz1); Ciphertext boaz3(B); evaluator.multiply(boaz3, t); Ciphertext boaz4(boaz2); evaluator.add(boaz4, boaz3); result = boaz4;
    *r = result;
    return ;
}

struct SImageData
{
    long width;
    long height;
    std::vector<std::vector<Ciphertext>> pixels;
};
 
inline Ciphertext GetPixelClamped (const SImageData& image, int x, int y)
{
    CLAMP(x, 0, image.width - 1);
    CLAMP(y, 0, image.height - 1);    
    return image.pixels[x * image.width + y];
}

void SampleLinear (std::vector<Ciphertext> &ret, const SImageData& image, float u, float v)
{
    // calculate coordinates -> also need to offset by half a pixel to keep image from shifting down and left half a pixel
    float x = (u * image.width) - 0.5f;
    int xint = int(x);
    float xfract = x - floor(x);
 
    float y = (v * image.height) - 0.5f;
    int yint = int(y);
    float yfract = y - floor(y);
 
    // get pixels
    auto p00 = GetPixelClamped(image, xint + 0, yint + 0);
    auto p10 = GetPixelClamped(image, xint + 1, yint + 0);
    auto p01 = GetPixelClamped(image, xint + 0, yint + 1);
    auto p11 = GetPixelClamped(image, xint + 1, yint + 1);
 
    // interpolate bi-linearly!
    for (int i = 0; i < 3; ++i)
    {
        Ciphertext col0 = Lerp(p00[i], p10[i], xfract);
        Ciphertext col1 = Lerp(p01[i], p11[i], xfract);
        ret[i] = Lerp(col0, col1, yfract);
    }
    return ; 
}

std::array<uint8, 3> SampleBicubic (const SImageData& image, float u, float v)
{
    // calculate coordinates -> also need to offset by half a pixel to keep image from shifting down and left half a pixel
    float x = (u * image.width) - 0.5;
    int xint = int(x);
    float xfract = x - floor(x);
 
    float y = (v * image.height) - 0.5;
    int yint = int(y);
    float yfract = y - floor(y);
 
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
    std::vector<Ciphertext> ret;
    
    for (int i = 0; i < 3; ++i)
    {
        Ciphertext col0 = CubicHermite(p00[i], p10[i], p20[i], p30[i], xfract);
        Ciphertext col1 = CubicHermite(p01[i], p11[i], p21[i], p31[i], xfract);
        Ciphertext col2 = CubicHermite(p02[i], p12[i], p22[i], p32[i], xfract);
        Ciphertext col3 = CubicHermite(p03[i], p13[i], p23[i], p33[i], xfract);
        float value = CubicHermite(col0, col1, col2, col3, yfract);
        ret[i] = value;
    }
    return ret;
}

void ResizeImage (const SImageData &srcImage, SImageData &destImage, float scale, int degree)
{
    destImage.m_width = long(float(srcImage.m_width)*scale);
    destImage.m_height = long(float(srcImage.m_height)*scale);
    destImage.m_pitch = destImage.m_width * 3;
    if (destImage.m_pitch & 3)
    {
        destImage.m_pitch &= ~3;
        destImage.m_pitch += 4;
    }
    destImage.m_pixels.resize(destImage.m_pitch*destImage.m_height);
 
    uint8 *row = &destImage.m_pixels[0];
    for (int y = 0; y < destImage.m_height; ++y)
    {
        uint8 *destPixel = row;
        float v = float(y) / float(destImage.m_height - 1);
        for (int x = 0; x < destImage.m_width; ++x)
        {
            float u = float(x) / float(destImage.m_width - 1);
            std::array<uint8, 3> sample;
 
            if (degree == 0)
                sample = SampleNearest(srcImage, u, v);
            else if (degree == 1)
                sample = SampleLinear(srcImage, u, v);
            else if (degree == 2)
                sample = SampleBicubic(srcImage, u, v);
 
            destPixel[0] = sample[0];
            destPixel[1] = sample[1];
            destPixel[2] = sample[2];
            destPixel += 3;
        }
        row += destImage.m_pitch;
    }
}

#endif