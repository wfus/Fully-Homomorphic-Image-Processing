#ifndef FHE_DECODE_H
#define FHE_DECODE_H

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


int precision = 8;
double cosine[106];
int coefficients[4096];


#endif