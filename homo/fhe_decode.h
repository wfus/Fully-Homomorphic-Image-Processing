#ifndef FHE_DECODE_H
#define FHE_DECODE_H

#include "seal/seal.h"
#include <opencv2/opencv.hpp>

using namespace seal;
using namespace cv;


int precision = 8;
double cosine[106];
int coefficients[4096];


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
 */
void calculate_coefficients(int b1, int b2) {

}



#endif