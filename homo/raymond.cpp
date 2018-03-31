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
#include "fhe_image.h"

using namespace seal;

auto start = std::chrono::steady_clock::now();
const bool VERBOSE = true;

const std::vector<int> S_ZAG = { 0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63 };
const std::vector<double> S_STD_LUM_QUANT = { 16,11,12,14,12,10,16,14,13,14,18,17,16,19,24,40,26,24,22,22,24,49,35,37,29,40,58,51,61,60,57,51,56,55,64,72,92,78,64,68,87,69,55,56,80,109,81,87,95,98,103,104,103,62,77,113,121,112,100,120,92,101,103,99 };
const std::vector<double> S_STD_CROMA_QUANT = { 17,18,18,24,21,24,47,26,26,47,99,66,56,66,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99 };

void raymond_average();
void print_parameters(const SEALContext &context);


int main()
{
    std::cout << "\nTotal memory allocated by global memory pool: "
              << (MemoryPoolHandle::Global().alloc_byte_count() >> 20) 
              << " MB" << std::endl;
    
    raymond_average();

    return 0;
}


void raymond_average() {
    std::vector<double> im = read_image("../image/kung.txt");
    std::vector<std::vector<double>> blocks = split_image_eight_block(im, 16, 16);
    print_blocks(blocks);

    // Encryption Parameters
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

    start = std::chrono::steady_clock::now(); 
    // We will encode all of the numbers and encrypt them to check if a number is below 0
    // without having to resort to bitvector AND/XOR and have the user encrypt twice :( 


    diff = std::chrono::steady_clock::now() - start; 
    std::cout << "Encoding less than 1: ";
    std::cout << chrono::duration<double, milli>(diff).count() << " ms" << std::endl;

    
    start = std::chrono::steady_clock::now(); 
    std::vector<std::vector<Ciphertext>> encoded_blocks;
    for (int i = 0; i < blocks.size(); i++) {
        std::vector<Ciphertext> encoded_block;
        for (int j = 0; j < blocks[i].size(); j++) {
            Ciphertext c;
            encryptor.encrypt(encoder.encode(blocks[i][j]), c);
            encoded_block.push_back(c);
        }
        encoded_blocks.push_back(encoded_block);
    }
    diff = std::chrono::steady_clock::now() - start; 
    std::cout << "Encoding: ";
    std::cout << chrono::duration<double, milli>(diff).count() << " ms" << std::endl;

    std::cout << "Noise budget in result: "
        << decryptor.invariant_noise_budget(encoded_blocks[0][0]) 
        << " bits" << std::endl;

    start = std::chrono::steady_clock::now();
    for (int i = 0; i < encoded_blocks.size(); i++) {
        encrypted_dct(encoded_blocks[i], evaluator, encoder, encryptor);
        quantize_fhe(encoded_blocks[i], S_STD_LUM_QUANT, evaluator, encoder, encryptor);
    }
    diff = std::chrono::steady_clock::now() - start; 
    std::cout << "DCT + QUANT Rounds: ";
    std::cout << chrono::duration<double, milli>(diff).count() << " ms" << std::endl;

    if (VERBOSE) {
        for (int i = 0; i < encoded_blocks.size(); i++) {
            for (int j = 0; j < encoded_blocks[0].size(); j++) {
                std::cout << "Noise budget in result: "
                << decryptor.invariant_noise_budget(encoded_blocks[0][0]) 
                << " bits" << std::endl;
            }
        } 
    }




    // DECODING THE RESULT
    start = std::chrono::steady_clock::now(); 
    std::vector<std::vector<double>> decoded_blocks;
    std::cout << "Decoding result..." << std::endl;
    for (int j = 0; j < encoded_blocks.size(); j++) {
        std::vector<double> decoded_block;
        for (int i = 0; i < encoded_blocks[j].size(); i++) {
            Plaintext p;
            decryptor.decrypt(encoded_blocks[j][i], p);
            decoded_block.push_back(encoder.decode(p));
        }
        decoded_blocks.push_back(decoded_block);
    }
    if (VERBOSE) print_blocks(decoded_blocks);
    diff = std::chrono::steady_clock::now() - start; 
    std::cout << "Decrypting blocks: ";
    std::cout << chrono::duration<double, milli>(diff).count() << " ms" << std::endl;


    // CALCULATE THE ACTUAL DCTS
    im = read_image("../image/kung.txt");
    std::vector<std::vector<double>> reg_blocks = split_image_eight_block(im, 16, 16);
    dct_blocks(reg_blocks);

    // FIND THE DIFFS
    if (VERBOSE) {
        const double NOISE_EPSILON = 0.5; // Amount of noise we will consider "the same"
        std::cout<<std::endl<<std::endl<<"DIFFS BETWEEN HOMO AND REG" <<std::endl;
        for (int i = 0; i < reg_blocks.size(); i++) {
            for (int j = 0; j < reg_blocks[0].size(); j++) {
                reg_blocks[i][j] -= decoded_blocks[i][j];
                if (reg_blocks[i][j] < NOISE_EPSILON && reg_blocks[i][j] > -NOISE_EPSILON) {
                    reg_blocks[i][j] = 0;
                }
            }
        }
        print_blocks(reg_blocks);
    }
    
    return;
}

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