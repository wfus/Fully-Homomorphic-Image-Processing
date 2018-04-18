#ifndef FHE_IMAGE_H 
#define FHE_IMAGE_H

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#include <cmath>


#include "seal/seal.h"
#include "jo_jpeg.h"
#include "stb_image.h"


using namespace seal;


// Encrytion params
const int POLY_BASE = 2;
const int N_FRACTIONAL_COEFFS = 100;  
const int N_NUMBER_COEFFS = 100;

const int PLAIN_MODULUS = 1 << 14;
const int COEFF_MODULUS = 8192;

// Constants for jpg processing

const int QUALITY = 0;

const unsigned char s_ZigZag[] = { 0,1,5,6,14,15,27,28,2,4,7,13,16,26,29,42,3,8,12,17,25,30,41,43,9,11,18,24,31,40,44,53,10,19,23,32,39,45,52,54,20,22,33,38,46,51,55,60,21,34,37,47,50,56,59,61,35,36,48,49,57,58,62,63 };

const unsigned char std_dc_luminance_nrcodes[] = {0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0};
const unsigned char std_dc_luminance_values[] = {0,1,2,3,4,5,6,7,8,9,10,11};
const unsigned char std_ac_luminance_nrcodes[] = {0,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d};
const unsigned char std_ac_luminance_values[] = {
    0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,
    0x23,0x42,0xb1,0xc1,0x15,0x52,0xd1,0xf0,0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,0x17,0x18,0x19,0x1a,0x25,0x26,0x27,0x28,
    0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,
    0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
    0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,
    0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,
    0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa
};
const unsigned char std_dc_chrominance_nrcodes[] = {0,0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0};
const unsigned char std_dc_chrominance_values[] = {0,1,2,3,4,5,6,7,8,9,10,11};
const unsigned char std_ac_chrominance_nrcodes[] = {0,0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77};
const unsigned char std_ac_chrominance_values[] = {
    0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,
    0xa1,0xb1,0xc1,0x09,0x23,0x33,0x52,0xf0,0x15,0x62,0x72,0xd1,0x0a,0x16,0x24,0x34,0xe1,0x25,0xf1,0x17,0x18,0x19,0x1a,0x26,
    0x27,0x28,0x29,0x2a,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,
    0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x82,0x83,0x84,0x85,0x86,0x87,
    0x88,0x89,0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,
    0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,
    0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa
};
// Huffman tables
const unsigned short YDC_HT[256][2] = { {0,2},{2,3},{3,3},{4,3},{5,3},{6,3},{14,4},{30,5},{62,6},{126,7},{254,8},{510,9}};
const unsigned short UVDC_HT[256][2] = { {0,2},{1,2},{2,2},{6,3},{14,4},{30,5},{62,6},{126,7},{254,8},{510,9},{1022,10},{2046,11}};
const unsigned short YAC_HT[256][2] = { 
    {10,4},{0,2},{1,2},{4,3},{11,4},{26,5},{120,7},{248,8},{1014,10},{65410,16},{65411,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {12,4},{27,5},{121,7},{502,9},{2038,11},{65412,16},{65413,16},{65414,16},{65415,16},{65416,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {28,5},{249,8},{1015,10},{4084,12},{65417,16},{65418,16},{65419,16},{65420,16},{65421,16},{65422,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {58,6},{503,9},{4085,12},{65423,16},{65424,16},{65425,16},{65426,16},{65427,16},{65428,16},{65429,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {59,6},{1016,10},{65430,16},{65431,16},{65432,16},{65433,16},{65434,16},{65435,16},{65436,16},{65437,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {122,7},{2039,11},{65438,16},{65439,16},{65440,16},{65441,16},{65442,16},{65443,16},{65444,16},{65445,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {123,7},{4086,12},{65446,16},{65447,16},{65448,16},{65449,16},{65450,16},{65451,16},{65452,16},{65453,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {250,8},{4087,12},{65454,16},{65455,16},{65456,16},{65457,16},{65458,16},{65459,16},{65460,16},{65461,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {504,9},{32704,15},{65462,16},{65463,16},{65464,16},{65465,16},{65466,16},{65467,16},{65468,16},{65469,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {505,9},{65470,16},{65471,16},{65472,16},{65473,16},{65474,16},{65475,16},{65476,16},{65477,16},{65478,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {506,9},{65479,16},{65480,16},{65481,16},{65482,16},{65483,16},{65484,16},{65485,16},{65486,16},{65487,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {1017,10},{65488,16},{65489,16},{65490,16},{65491,16},{65492,16},{65493,16},{65494,16},{65495,16},{65496,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {1018,10},{65497,16},{65498,16},{65499,16},{65500,16},{65501,16},{65502,16},{65503,16},{65504,16},{65505,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {2040,11},{65506,16},{65507,16},{65508,16},{65509,16},{65510,16},{65511,16},{65512,16},{65513,16},{65514,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {65515,16},{65516,16},{65517,16},{65518,16},{65519,16},{65520,16},{65521,16},{65522,16},{65523,16},{65524,16},{0,0},{0,0},{0,0},{0,0},{0,0},
    {2041,11},{65525,16},{65526,16},{65527,16},{65528,16},{65529,16},{65530,16},{65531,16},{65532,16},{65533,16},{65534,16},{0,0},{0,0},{0,0},{0,0},{0,0}
};
const unsigned short UVAC_HT[256][2] = { 
    {0,2},{1,2},{4,3},{10,4},{24,5},{25,5},{56,6},{120,7},{500,9},{1014,10},{4084,12},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {11,4},{57,6},{246,8},{501,9},{2038,11},{4085,12},{65416,16},{65417,16},{65418,16},{65419,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {26,5},{247,8},{1015,10},{4086,12},{32706,15},{65420,16},{65421,16},{65422,16},{65423,16},{65424,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {27,5},{248,8},{1016,10},{4087,12},{65425,16},{65426,16},{65427,16},{65428,16},{65429,16},{65430,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {58,6},{502,9},{65431,16},{65432,16},{65433,16},{65434,16},{65435,16},{65436,16},{65437,16},{65438,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {59,6},{1017,10},{65439,16},{65440,16},{65441,16},{65442,16},{65443,16},{65444,16},{65445,16},{65446,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {121,7},{2039,11},{65447,16},{65448,16},{65449,16},{65450,16},{65451,16},{65452,16},{65453,16},{65454,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {122,7},{2040,11},{65455,16},{65456,16},{65457,16},{65458,16},{65459,16},{65460,16},{65461,16},{65462,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {249,8},{65463,16},{65464,16},{65465,16},{65466,16},{65467,16},{65468,16},{65469,16},{65470,16},{65471,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {503,9},{65472,16},{65473,16},{65474,16},{65475,16},{65476,16},{65477,16},{65478,16},{65479,16},{65480,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {504,9},{65481,16},{65482,16},{65483,16},{65484,16},{65485,16},{65486,16},{65487,16},{65488,16},{65489,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {505,9},{65490,16},{65491,16},{65492,16},{65493,16},{65494,16},{65495,16},{65496,16},{65497,16},{65498,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {506,9},{65499,16},{65500,16},{65501,16},{65502,16},{65503,16},{65504,16},{65505,16},{65506,16},{65507,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {2041,11},{65508,16},{65509,16},{65510,16},{65511,16},{65512,16},{65513,16},{65514,16},{65515,16},{65516,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {16352,14},{65517,16},{65518,16},{65519,16},{65520,16},{65521,16},{65522,16},{65523,16},{65524,16},{65525,16},{0,0},{0,0},{0,0},{0,0},{0,0},
    {1018,10},{32707,15},{65526,16},{65527,16},{65528,16},{65529,16},{65530,16},{65531,16},{65532,16},{65533,16},{65534,16},{0,0},{0,0},{0,0},{0,0},{0,0}
};
const int YQT[] = {16,11,10,16,24,40,51,61,12,12,14,19,26,58,60,55,14,13,16,24,40,57,69,56,14,17,22,29,51,87,80,62,18,22,37,56,68,109,103,77,24,35,55,64,81,104,113,92,49,64,78,87,103,121,120,101,72,92,95,98,112,100,103,99};
const int UVQT[] = {17,18,24,47,99,99,99,99,18,21,26,66,99,99,99,99,24,26,56,99,99,99,99,99,47,66,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99};
const float aasf[] = { 1.0f * 2.828427125f, 1.387039845f * 2.828427125f, 1.306562965f * 2.828427125f, 1.175875602f * 2.828427125f, 1.0f * 2.828427125f, 0.785694958f * 2.828427125f, 0.541196100f * 2.828427125f, 0.275899379f * 2.828427125f };
const unsigned char head0[] = { 0xFF,0xD8,0xFF,0xE0,0,0x10,'J','F','I','F',0,1,1,0,0,1,0,1,0,0,0xFF,0xDB,0,0x84,0 };



const int BLOCK_SIZE = 8;

template <class T>
std::vector<std::vector<T>> split_image_eight_block(std::vector<T> &im, int w, int h) {
    std::vector<std::vector<T>> lst;
    for (int j = 0; j < h; j += BLOCK_SIZE) {
        for (int i = 0; i < w; i += BLOCK_SIZE) {   
            std::vector<T> new_lst;
            for (int k = 0; k < BLOCK_SIZE; k++) {
                for (int l = 0; l < BLOCK_SIZE; l++) {
                    int index = (j+k)*w + i + l;
                    new_lst.push_back(im[index]);
                }
            }
            lst.push_back(new_lst);
        }
    }
    return lst;
}

void print_image(uint8_t *im, int w, int h) {
    std::cout << "Printing Image dim: (" << w << "," << h << ")" << std::endl;
    for (int i = 0; i < w*h; i++) {
        printf("%.2x ", im[i]);
        if ((i + 1) % w == 0) std::cout << std::endl;
    }
} 

template <class T>
void print_image(std::vector<T> &im, int w, int h) {
    std::cout << "Printing Image dim: (" << w << "," << h << ")" << std::endl;
    for (int i = 0; i < im.size(); i++) {
        std::cout << std::setw(11) << im[i] << " ";
        if ((i + 1) % w == 0) std::cout << std::endl;
    }
} 

template <class T>
void print_blocks(std::vector<std::vector<T>> &blocks) {
    for (int a = 0; a < blocks.size(); a++) {
        std::cout << "Printing block " << a << std::endl;
        for (int i = 0; i < blocks[a].size(); i++) {
            std::cout << std::setw(11) << blocks[a][i] << " ";
            if ((i + 1) % BLOCK_SIZE == 0) std::cout << std::endl;
        }
        std::cout << "---------------" << std::endl;
    }
}

std::vector<double> read_image(std::string fname) {
    int w, h;
    std::vector<double> im; 
    std::ifstream myfile;
        
    myfile.open(fname.c_str());
    myfile >> w;
    myfile >> h;
    // std::cout << "Read in " << fname << "with dimensions: " << w << " x " << h << std::endl;

    float tmp;
    for (int i = 0; i < w*h; i++) {
        myfile >> tmp;
        im.push_back(tmp);
    }   
    return im; 
}

std::vector<double> read_image(std::string fname, int* w, int *h) {

    std::vector<double> im; 
    std::ifstream myfile;
        
    myfile.open(fname.c_str());
    myfile >> *w;
    myfile >> *h;
    // std::cout << "Read in " << fname << "with dimensions: " << w << " x " << h << std::endl;

    float tmp;
    for (int i = 0; i < (*w) * (*h); i++) {
        myfile >> tmp;
        im.push_back(tmp);
    }   
    return im; 
}


/* Recieves a 8x8 box of pixels, in ciphertext form. 
 * Data should be laid out in a 64 element vector with 
 * rows first then columns
 */
void encrypted_dct(std::vector<Ciphertext> &data,
                   Evaluator &evaluator, 
                   FractionalEncoder &encoder, 
                   Encryptor &encryptor) {
    auto start = std::chrono::steady_clock::now();
    Ciphertext z1, z2, z3, z4, z5; 
    Ciphertext tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp10, tmp11, tmp12, tmp13;
    Ciphertext tmp14;
    Ciphertext cons1;
    int curr_index = 0;
    for (int c = 0; c < BLOCK_SIZE; c++) {
        Ciphertext boaz1(data[curr_index+0]); evaluator.add(boaz1, data[curr_index+7]); tmp0 = boaz1;
        Ciphertext boaz2(data[curr_index+0]); evaluator.sub(boaz2, data[curr_index+7]); tmp7 = boaz2;
        Ciphertext boaz3(data[curr_index+1]); evaluator.add(boaz3, data[curr_index+6]); tmp1 = boaz3;
        Ciphertext boaz4(data[curr_index+1]); evaluator.sub(boaz4, data[curr_index+6]); tmp6 = boaz4;
        Ciphertext boaz5(data[curr_index+2]); evaluator.add(boaz5, data[curr_index+5]); tmp2 = boaz5;
        Ciphertext boaz6(data[curr_index+2]); evaluator.sub(boaz6, data[curr_index+5]); tmp5 = boaz6;
        Ciphertext boaz7(data[curr_index+3]); evaluator.add(boaz7, data[curr_index+4]); tmp3 = boaz7;
        Ciphertext boaz8(data[curr_index+3]); evaluator.sub(boaz8, data[curr_index+4]); tmp4 = boaz8;
        Ciphertext boaz9(tmp0); evaluator.add(boaz9, tmp3); tmp10 = boaz9;
        Ciphertext boaz10(tmp0); evaluator.sub(boaz10, tmp3); tmp13 = boaz10;
        Ciphertext boaz11(tmp1); evaluator.add(boaz11, tmp2); tmp11 = boaz11;
        Ciphertext boaz12(tmp1); evaluator.sub(boaz12, tmp2); tmp12 = boaz12;
        Ciphertext boaz13(tmp10); evaluator.add(boaz13, tmp11); data[curr_index+0] = boaz13;
        Ciphertext boaz14(tmp10); evaluator.sub(boaz14, tmp11); data[curr_index+4] = boaz14;
        Ciphertext boaz15(tmp12); evaluator.add(boaz15, tmp13); Ciphertext boaz16(boaz15); evaluator.multiply_plain(boaz16, encoder.encode(0.541196100)); z1 = boaz16;
        Ciphertext boaz17(tmp13); evaluator.multiply_plain(boaz17, encoder.encode(0.765366865)); Ciphertext boaz18(z1); evaluator.add(boaz18, boaz17); data[curr_index+2] = boaz18;
        Ciphertext boaz19(tmp12); evaluator.multiply_plain(boaz19, encoder.encode(-1.847759065)); Ciphertext boaz20(z1); evaluator.add(boaz20, boaz19); data[curr_index+6] = boaz20;
        Ciphertext boaz21(tmp4); evaluator.add(boaz21, tmp7); z1 = boaz21;
        Ciphertext boaz22(tmp5); evaluator.add(boaz22, tmp6); z2 = boaz22;
        Ciphertext boaz23(tmp4); evaluator.add(boaz23, tmp6); z3 = boaz23;
        Ciphertext boaz24(tmp5); evaluator.add(boaz24, tmp7); z4 = boaz24;
        Ciphertext boaz25(z3); evaluator.add(boaz25, z4); Ciphertext boaz26(boaz25); evaluator.multiply_plain(boaz26, encoder.encode(1.175875602)); z5 = boaz26;
        Ciphertext boaz27(tmp4); evaluator.multiply_plain(boaz27, encoder.encode(0.298631336)); tmp4 = boaz27;
        Ciphertext boaz28(tmp5); evaluator.multiply_plain(boaz28, encoder.encode(2.053119869)); tmp5 = boaz28;
        Ciphertext boaz29(tmp6); evaluator.multiply_plain(boaz29, encoder.encode(3.072711026)); tmp6 = boaz29;
        Ciphertext boaz30(tmp7); evaluator.multiply_plain(boaz30, encoder.encode(1.501321110)); tmp7 = boaz30;
        Ciphertext boaz31(z1); evaluator.multiply_plain(boaz31, encoder.encode(-0.899976223)); z1 = boaz31;
        Ciphertext boaz32(z2); evaluator.multiply_plain(boaz32, encoder.encode(-2.562915447)); z2 = boaz32;
        Ciphertext boaz33(z3); evaluator.multiply_plain(boaz33, encoder.encode(-1.961570560)); z3 = boaz33;
        Ciphertext boaz34(z4); evaluator.multiply_plain(boaz34, encoder.encode(-0.390180644)); z4 = boaz34;
        Ciphertext boaz35(z3); evaluator.add(boaz35, z5); z3 = boaz35;
        Ciphertext boaz36(z4); evaluator.add(boaz36, z5); z4 = boaz36;
        Ciphertext boaz37(tmp4); evaluator.add(boaz37, z1); Ciphertext boaz38(boaz37); evaluator.add(boaz38, z3); data[curr_index+7] = boaz38;
        Ciphertext boaz39(tmp5); evaluator.add(boaz39, z2); Ciphertext boaz40(boaz39); evaluator.add(boaz40, z4); data[curr_index+5] = boaz40;
        Ciphertext boaz41(tmp6); evaluator.add(boaz41, z2); Ciphertext boaz42(boaz41); evaluator.add(boaz42, z3); data[curr_index+3] = boaz42;
        Ciphertext boaz43(tmp7); evaluator.add(boaz43, z1); Ciphertext boaz44(boaz43); evaluator.add(boaz44, z4); data[curr_index+1] = boaz44;
        curr_index += BLOCK_SIZE;
    }
    curr_index = 0; 
    for (int c = 0; c < BLOCK_SIZE; c++) {
        Ciphertext boaz45(data[curr_index+0]); evaluator.add(boaz45, data[curr_index+56]); tmp0 = boaz45;
        Ciphertext boaz46(data[curr_index+0]); evaluator.sub(boaz46, data[curr_index+56]); tmp7 = boaz46;
        Ciphertext boaz47(data[curr_index+8]); evaluator.add(boaz47, data[curr_index+48]); tmp1 = boaz47;
        Ciphertext boaz48(data[curr_index+8]); evaluator.sub(boaz48, data[curr_index+48]); tmp6 = boaz48;
        Ciphertext boaz49(data[curr_index+16]); evaluator.add(boaz49, data[curr_index+40]); tmp2 = boaz49;
        Ciphertext boaz50(data[curr_index+16]); evaluator.sub(boaz50, data[curr_index+40]); tmp5 = boaz50;
        Ciphertext boaz51(data[curr_index+24]); evaluator.add(boaz51, data[curr_index+32]); tmp3 = boaz51;
        Ciphertext boaz52(data[curr_index+24]); evaluator.sub(boaz52, data[curr_index+32]); tmp4 = boaz52;
        Ciphertext boaz53(tmp0); evaluator.add(boaz53, tmp3); tmp10 = boaz53;
        Ciphertext boaz54(tmp0); evaluator.sub(boaz54, tmp3); tmp13 = boaz54;
        Ciphertext boaz55(tmp1); evaluator.add(boaz55, tmp2); tmp11 = boaz55;
        Ciphertext boaz56(tmp1); evaluator.sub(boaz56, tmp2); tmp12 = boaz56;
        Ciphertext boaz57(tmp10); evaluator.add(boaz57, tmp11); Ciphertext boaz58(boaz57); evaluator.multiply_plain(boaz58, encoder.encode(0.125)); data[curr_index+0] = boaz58;
        Ciphertext boaz59(tmp10); evaluator.sub(boaz59, tmp11); Ciphertext boaz60(boaz59); evaluator.multiply_plain(boaz60, encoder.encode(0.125)); data[curr_index+32] = boaz60;
        Ciphertext boaz61(tmp12); evaluator.add(boaz61, tmp13); Ciphertext boaz62(boaz61); evaluator.multiply_plain(boaz62, encoder.encode(0.541196100)); z1 = boaz62;
        Ciphertext boaz63(tmp13); evaluator.multiply_plain(boaz63, encoder.encode(0.765366865)); Ciphertext boaz64(z1); evaluator.add(boaz64, boaz63); Ciphertext boaz65(boaz64); evaluator.multiply_plain(boaz65, encoder.encode(0.125)); data[curr_index+16] = boaz65;
        Ciphertext boaz66(tmp12); evaluator.multiply_plain(boaz66, encoder.encode(-1.847759065)); Ciphertext boaz67(z1); evaluator.add(boaz67, boaz66); Ciphertext boaz68(boaz67); evaluator.multiply_plain(boaz68, encoder.encode(0.125)); data[curr_index+48] = boaz68;
        Ciphertext boaz69(tmp4); evaluator.add(boaz69, tmp7); z1 = boaz69;
        Ciphertext boaz70(tmp5); evaluator.add(boaz70, tmp6); z2 = boaz70;
        Ciphertext boaz71(tmp4); evaluator.add(boaz71, tmp6); z3 = boaz71;
        Ciphertext boaz72(tmp5); evaluator.add(boaz72, tmp7); z4 = boaz72;
        Ciphertext boaz73(z3); evaluator.add(boaz73, z4); Ciphertext boaz74(boaz73); evaluator.multiply_plain(boaz74, encoder.encode(1.175875602)); z5 = boaz74;
        Ciphertext boaz75(tmp4); evaluator.multiply_plain(boaz75, encoder.encode(0.298631336)); tmp4 = boaz75;
        Ciphertext boaz76(tmp5); evaluator.multiply_plain(boaz76, encoder.encode(2.053119869)); tmp5 = boaz76;
        Ciphertext boaz77(tmp6); evaluator.multiply_plain(boaz77, encoder.encode(3.072711026)); tmp6 = boaz77;
        Ciphertext boaz78(tmp7); evaluator.multiply_plain(boaz78, encoder.encode(1.501321110)); tmp7 = boaz78;
        Ciphertext boaz79(z1); evaluator.multiply_plain(boaz79, encoder.encode(-0.899976223)); z1 = boaz79;
        Ciphertext boaz80(z2); evaluator.multiply_plain(boaz80, encoder.encode(-2.562915447)); z2 = boaz80;
        Ciphertext boaz81(z3); evaluator.multiply_plain(boaz81, encoder.encode(-1.961570560)); z3 = boaz81;
        Ciphertext boaz82(z4); evaluator.multiply_plain(boaz82, encoder.encode(-0.390180644)); z4 = boaz82;
        Ciphertext boaz83(z3); evaluator.add(boaz83, z5); z3 = boaz83;
        Ciphertext boaz84(z4); evaluator.add(boaz84, z5); z4 = boaz84;
        Ciphertext boaz85(tmp4); evaluator.add(boaz85, z1); Ciphertext boaz86(boaz85); evaluator.add(boaz86, z3); Ciphertext boaz87(boaz86); evaluator.multiply_plain(boaz87, encoder.encode(0.125)); data[curr_index+56] = boaz87;
        Ciphertext boaz88(tmp5); evaluator.add(boaz88, z2); Ciphertext boaz89(boaz88); evaluator.add(boaz89, z4); Ciphertext boaz90(boaz89); evaluator.multiply_plain(boaz90, encoder.encode(0.125)); data[curr_index+40] = boaz90;
        Ciphertext boaz91(tmp6); evaluator.add(boaz91, z2); Ciphertext boaz92(boaz91); evaluator.add(boaz92, z3); Ciphertext boaz93(boaz92); evaluator.multiply_plain(boaz93, encoder.encode(0.125)); data[curr_index+24] = boaz93;
        Ciphertext boaz94(tmp7); evaluator.add(boaz94, z1); Ciphertext boaz95(boaz94); evaluator.add(boaz95, z4); Ciphertext boaz96(boaz95); evaluator.multiply_plain(boaz96, encoder.encode(0.125)); data[curr_index+8]= boaz96;
        curr_index++;
    }
    auto diff = std::chrono::steady_clock::now() - start;
    std::cout << chrono::duration<double, milli>(diff).count() << ',';
    return;
}

/* Recieves a 8x8 box of pixels, in ciphertext form. 
 * Data should be laid out in a 64 element vector with 
 * rows first then columns. Divides each element by quant
 */
inline void quantize_fhe(std::vector<Ciphertext> &data, 
                            const std::vector<double> &quant,
                            Evaluator &evaluator, 
                            FractionalEncoder &encoder, 
                            Encryptor &encryptor) {
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 64; i++) {
        evaluator.multiply_plain(data[i], encoder.encode(1/quant[i]));
    }
    auto diff = std::chrono::steady_clock::now() - start;
    std::cout << chrono::duration<double, milli>(diff).count() << ',';
}

/* Recieves three ciphertexts, corresponding to R, G, B. Does 
 * an operation in place that maps R->Y,  
 */
void rgb_to_ycc_fhe(Ciphertext &r,
                           Ciphertext &g,
                           Ciphertext &b,
                           Evaluator &evaluator, 
                           FractionalEncoder &encoder, 
                           Encryptor &encryptor) {
    auto start = std::chrono::steady_clock::now();
    Ciphertext boaz1(r); evaluator.multiply_plain(boaz1, encoder.encode(0.299)); Ciphertext boaz2(g); evaluator.multiply_plain(boaz2, encoder.encode(0.587)); Ciphertext boaz3(boaz1); evaluator.add(boaz3, boaz2); Ciphertext boaz4(b); evaluator.multiply_plain(boaz4, encoder.encode(0.114)); Ciphertext boaz5(boaz3); evaluator.add(boaz5, boaz4); Ciphertext boaz6(boaz5); evaluator.sub_plain(boaz6, encoder.encode(128.0)); Ciphertext y(boaz6);
    Ciphertext boaz7(r); evaluator.multiply_plain(boaz7, encoder.encode(-0.168736)); Ciphertext boaz8(g); evaluator.multiply_plain(boaz8, encoder.encode(0.331264)); Ciphertext boaz9(boaz7); evaluator.sub(boaz9, boaz8); Ciphertext boaz10(b); evaluator.multiply_plain(boaz10, encoder.encode(0.5)); Ciphertext boaz11(boaz9); evaluator.add(boaz11, boaz10); Ciphertext u (boaz11);
    Ciphertext boaz12(r); evaluator.multiply_plain(boaz12, encoder.encode(0.5)); Ciphertext boaz13(g); evaluator.multiply_plain(boaz13, encoder.encode(0.418688)); Ciphertext boaz14(boaz12); evaluator.sub(boaz14, boaz13); Ciphertext boaz15(b); evaluator.multiply_plain(boaz15, encoder.encode(0.081312)); Ciphertext boaz16(boaz14); evaluator.sub(boaz16, boaz15); Ciphertext v (boaz16);
    r = y;
    g = u;
    b = v;
    auto diff = std::chrono::steady_clock::now() - start;
    std::cout << chrono::duration<double, milli>(diff).count() << ',';
}





void writeBits(FILE *fp, int &bitBuf, int &bitCnt, const unsigned short *bs) {
	bitCnt += bs[1];
	bitBuf |= bs[0] << (24 - bitCnt);
	while(bitCnt >= 8) {
		unsigned char c = (bitBuf >> 16) & 255;
		putc(c, fp);
		if(c == 255) {
			putc(0, fp);
		}
		bitBuf <<= 8;
		bitCnt -= 8;
	}
}

void calcBits(int val, unsigned short bits[2]) {
	int tmp1 = val < 0 ? -val : val;
	val = val < 0 ? val-1 : val;
	bits[1] = 1;
	while(tmp1 >>= 1) {
		++bits[1];
	}
	bits[0] = val & ((1<<bits[1])-1);
}

int processBlock(FILE *fp, int &bitBuf, int &bitCnt, int* DU, float *fdtbl, int DC, const unsigned short HTDC[256][2], const unsigned short HTAC[256][2]){
    const unsigned short EOB[2] = { HTAC[0x00][0], HTAC[0x00][1] };
	const unsigned short M16zeroes[2] = { HTAC[0xF0][0], HTAC[0xF0][1] };
    // Encode DC
	int diff = DU[0] - DC; 
	if (diff == 0) {
		writeBits(fp, bitBuf, bitCnt, HTDC[0]);
	} else {
		unsigned short bits[2];
		calcBits(diff, bits);
		writeBits(fp, bitBuf, bitCnt, HTDC[bits[1]]);
		writeBits(fp, bitBuf, bitCnt, bits);
	}
	// Encode ACs
	int end0pos = 63;
	for(; (end0pos>0)&&(DU[end0pos]==0); --end0pos) {
	}
	// end0pos = first element in reverse order !=0
	if(end0pos == 0) {
		writeBits(fp, bitBuf, bitCnt, EOB);
		return DU[0];
	}
	for(int i = 1; i <= end0pos; ++i) {
		int startpos = i;
		for (; DU[i]==0 && i<=end0pos; ++i) {
		}
		int nrzeroes = i-startpos;
		if ( nrzeroes >= 16 ) {
			int lng = nrzeroes>>4;
			for (int nrmarker=1; nrmarker <= lng; ++nrmarker)
				writeBits(fp, bitBuf, bitCnt, M16zeroes);
			nrzeroes &= 15;
		}
		unsigned short bits[2];
		calcBits(DU[i], bits);
		writeBits(fp, bitBuf, bitCnt, HTAC[(nrzeroes<<4)+bits[1]]);
		writeBits(fp, bitBuf, bitCnt, bits);
	}
	if(end0pos != 63) {
		writeBits(fp, bitBuf, bitCnt, EOB);
	}
    return DU[0];
}

// Forward DCT, regular no encryption
inline void dct(double *data) {
    double z1, z2, z3, z4, z5, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp10, tmp11, tmp12, tmp13, *data_ptr;
    data_ptr = data;
    for (int c=0; c < 8; c++) {
        tmp0 = data_ptr[0] + data_ptr[7];
        tmp7 = data_ptr[0] - data_ptr[7];
        tmp1 = data_ptr[1] + data_ptr[6];
        tmp6 = data_ptr[1] - data_ptr[6];
        tmp2 = data_ptr[2] + data_ptr[5];
        tmp5 = data_ptr[2] - data_ptr[5];
        tmp3 = data_ptr[3] + data_ptr[4];
        tmp4 = data_ptr[3] - data_ptr[4];
        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;
        data_ptr[0] = tmp10 + tmp11;
        data_ptr[4] = tmp10 - tmp11;
        z1 = (tmp12 + tmp13) * 0.541196100;
        data_ptr[2] = z1 + tmp13 * 0.765366865;
        data_ptr[6] = z1 + tmp12 * - 1.847759065;
        z1 = tmp4 + tmp7;
        z2 = tmp5 + tmp6;
        z3 = tmp4 + tmp6;
        z4 = tmp5 + tmp7;
        z5 = (z3 + z4) * 1.175875602;
        tmp4 *= 0.298631336;
        tmp5 *= 2.053119869;
        tmp6 *= 3.072711026;
        tmp7 *= 1.501321110;
        z1 *= -0.899976223;
        z2 *= -2.562915447;
        z3 *= -1.961570560;
        z4 *= -0.390180644;
        z3 += z5;
        z4 += z5;
        data_ptr[7] = tmp4 + z1 + z3;
        data_ptr[5] = tmp5 + z2 + z4;
        data_ptr[3] = tmp6 + z2 + z3;
        data_ptr[1] = tmp7 + z1 + z4;
        data_ptr += 8;
    }

    data_ptr = data;

    for (int c=0; c < 8; c++) {
        tmp0 = data_ptr[8*0] + data_ptr[8*7];
        tmp7 = data_ptr[8*0] - data_ptr[8*7];
        tmp1 = data_ptr[8*1] + data_ptr[8*6];
        tmp6 = data_ptr[8*1] - data_ptr[8*6];
        tmp2 = data_ptr[8*2] + data_ptr[8*5];
        tmp5 = data_ptr[8*2] - data_ptr[8*5];
        tmp3 = data_ptr[8*3] + data_ptr[8*4];
        tmp4 = data_ptr[8*3] - data_ptr[8*4];
        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;
        data_ptr[8*0] = (tmp10 + tmp11) / 8.0;
        data_ptr[8*4] = (tmp10 - tmp11) / 8.0;
        z1 = (tmp12 + tmp13) * 0.541196100;
        data_ptr[8*2] = (z1 + tmp13 * 0.765366865) / 8.0;
        data_ptr[8*6] = (z1 + tmp12 * -1.847759065) / 8.0;
        z1 = tmp4 + tmp7;
        z2 = tmp5 + tmp6;
        z3 = tmp4 + tmp6;
        z4 = tmp5 + tmp7;
        z5 = (z3 + z4) * 1.175875602;
        tmp4 *= 0.298631336;
        tmp5 *= 2.053119869;
        tmp6 *= 3.072711026;
        tmp7 *= 1.501321110;
        z1 *= -0.899976223;
        z2 *= -2.562915447;
        z3 *= -1.961570560;
        z4 *= -0.390180644;
        z3 += z5;
        z4 += z5;
        data_ptr[8*7] = (tmp4 + z1 + z3) / 8.0;
        data_ptr[8*5] = (tmp5 + z2 + z4) / 8.0;
        data_ptr[8*3] = (tmp6 + z2 + z3) / 8.0;
        data_ptr[8*1] = (tmp7 + z1 + z4) / 8.0;
        data_ptr++;
    }
}


void dct_blocks(std::vector<std::vector<double>> &blocks) {
    for (int a = 0; a < blocks.size(); a++) {
        dct(&blocks[a][0]);
    }
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

void compare_jpeg_jojpeg(const char* original_image, const char* output_image, const char* jo_image) {
    int width, height, composition;
    uint8_t *image_data = stbi_load(original_image, &width, &height, &composition, 3);
    jo_write_jpg(jo_image, image_data, width, height, 3, QUALITY);
    uint8_t *output_data = stbi_load(output_image, &width, &height, &composition, 3);
    uint8_t *jo_data = stbi_load(jo_image, &width, &height, &composition, 3);
    int running_error = 0;
    for (int i = 0; i < width * height * composition; i++) {
        std::cout << jo_data[i] << '\t' << output_data[i] << std::endl;
        running_error += (output_data[i] - jo_data[i]) * (output_data[i] - jo_data[i]);
    }
    double average_error = ((double) running_error) / (width * height * composition);
    double rms_error = std::sqrt(average_error);
    std::cout << "RMSError," << rms_error << ',' << std::endl;
}

#endif