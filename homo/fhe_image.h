#pragma once

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <thread>
#include <mutex>
#include <random>
#include <limits>
#include <fstream>
#include <cmath>


#include "seal/seal.h"
using namespace seal;


inline void quantize_fhe(std::vector<Ciphertext> &data, 
                            const std::vector<double> &quant,
                            Evaluator &evaluator, 
                            FractionalEncoder &encoder, 
                            Encryptor &encryptor) {
    for (int i = 0; i < 64; i++) {
        evaluator.multiply_plain(data[i], encoder.encode(1/quant[i]));
    }
}