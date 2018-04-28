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

/* We want to find the Fourier decomposition of a step function at b_1 and b_2, 
 * and then 0 everywhere else from 0 to 64. 
 * 1             ________
 *              |        |
 *              |        |
 * 0  __________|        |_____________________
 *   0         b1        b2                   64
 * We will shift the origin to be at (b1 + b2)/2 and then calculate the Fourier 
 * decomposition there for the coefficients. 
 * 
 * Returns a vector of coefficients and sine stuff
 * First term is b/64
 * Other terms will be (2/(k Pi)) Sin(k b pi/64) Cos(k x pi/64) 
 * Degree will just be the number of terms of approximation
 * Will return the coefficients without the B!
 */
void calculate_coefficients(std::vector<double> coeff
                            std::vector<double> sincoeff,
                            std::vector<double> coscoeff,
                            int degree) {
    for (int i = 0; i <= degree; i++) {
        if (i == 0) {
            coeff.push_back(1.0/64.0);
        }
        else {
            coeff.push_back(2.0/(i * M_PI))
        }
    }
}



#endif