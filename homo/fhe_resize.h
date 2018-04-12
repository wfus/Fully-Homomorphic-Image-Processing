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

#endif