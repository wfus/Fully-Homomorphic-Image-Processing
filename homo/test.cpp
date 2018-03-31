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

auto start = std::chrono::steady_clock::now();
const bool VERBOSE = false;


void print_parameters(const SEALContext &context) {
    std::cout << "/ Encryption parameters:" << std::endl;
    std::cout << "| poly_modulus: " << context.poly_modulus().to_string() << std::endl;

    /*
    Print the size of the true (product) coefficient modulus
    */
    std::cout << "| coeff_modulus size: " 
        << context.total_coeff_modulus().significant_bit_count() << " bits" << std::endl;

    std::cout << "| plain_modulus: " << context.plain_modulus().value() << std::endl;
    std::cout << "\\ noise_standard_deviation: " << context.noise_standard_deviation() << std::endl;
    std::cout << std::endl;
}

bool ciphertexts_equal(Ciphertext &c1, Ciphertext &c2) {
    std::cout << c1.size() << " " << c2.size() << std::endl;
    std::cout << "----------------------------" << std::endl;
    if (c1.size() != c2.size()) return false;
    for (int i = 0; i < c1.size(); i++) {
        std::cout << c1[i] << " - " << c2[i] << std::endl;
        if (c1[i] != c2[i])
            return false;
    }
    return true;
}
 

int main()
{
    EncryptionParameters params;
    params.set_poly_modulus("1x^32768 + 1");
    params.set_coeff_modulus(coeff_modulus_192(2048));
    params.set_plain_modulus(1 << 14);
    SEALContext context(params);
    print_parameters(context);


    // Generate keys
    start = std::chrono::steady_clock::now(); 
    KeyGenerator keygen(context);
    auto public_key = keygen.public_key();
    auto secret_key = keygen.secret_key();
    auto diff = std::chrono::steady_clock::now() - start; 
    std::cout << "KeyGen: ";
    std::cout << chrono::duration<double, milli>(diff).count() << " ms" << std::endl;
    
    // Encrytor and decryptor setup
    Encryptor encryptor(context, public_key);
    Evaluator evaluator(context);
    Decryptor decryptor(context, secret_key);


    // Base + Number of coefficients used for encoding past the decimal point (both pos and neg)
    // Example: if poly_base = 11, and N_FRACTIONAL_COEFFS=3, then we will have 
    // a1 * 11^-1 + a2 * 11^-2 + a3 * 11^-3
    const int POLY_BASE = 11;
    const int N_FRACTIONAL_COEFFS = 3;  
    const int N_NUMBER_COEFFS = 10;

    FractionalEncoder encoder(context.plain_modulus(), context.poly_modulus(), N_NUMBER_COEFFS, N_FRACTIONAL_COEFFS, POLY_BASE); 

    Ciphertext a1, a2, a3;
    encryptor.encrypt(encoder.encode(0), a1);
    encryptor.encrypt(encoder.encode(0), a2);
    encryptor.encrypt(encoder.encode(1.1), a3);
    if (ciphertexts_equal(a1, a2)) {
        std::cout << "GOOD MORNING" << std::endl;
    }
    if (ciphertexts_equal(a1, a3)) {
        std::cout << "BAD MORNING" << std::endl;
    }
    return 0;
}
