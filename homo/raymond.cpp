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

const std::vector<int> S_ZAG = { 0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63 };
const std::vector<double> S_STD_LUM_QUANT = { 16,11,12,14,12,10,16,14,13,14,18,17,16,19,24,40,26,24,22,22,24,49,35,37,29,40,58,51,61,60,57,51,56,55,64,72,92,78,64,68,87,69,55,56,80,109,81,87,95,98,103,104,103,62,77,113,121,112,100,120,92,101,103,99 };
const std::vector<double> S_STD_CROMA_QUANT = { 17,18,18,24,21,24,47,26,26,47,99,66,56,66,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99 };

void dct(double *data); 
void raymond_average();
std::vector<double> read_image(std::string fname);
void encrypted_dct(std::vector<Ciphertext> &data,
                   Evaluator &evaluator, 
                   FractionalEncoder &encoder, 
                   Encryptor &encryptor);

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

void dct_blocks(std::vector<std::vector<double>> &blocks) {
    for (int a = 0; a < blocks.size(); a++) {
        dct(&blocks[a][0]);
    }
} 

int main()
{
    std::cout << "\nTotal memory allocated by global memory pool: "
              << (MemoryPoolHandle::Global().alloc_byte_count() >> 20) 
              << " MB" << std::endl;
    
    raymond_average();

    return 0;
}

std::vector<double> read_image(std::string fname) {
    int w, h;
    std::vector<double> im; 
    std::ifstream myfile;
        
    myfile.open(fname.c_str());
    myfile >> w;
    myfile >> h;
    std::cout << "Read in " << fname << "with dimensions: " << w << " x " << h << std::endl;

    float tmp;
    for (int i = 0; i < w*h; i++) {
        myfile >> tmp;
        im.push_back(tmp);
    }   
    return im; 
}

const int BLOCK_SIZE = 8;
std::vector<std::vector<double>> split_image_eight_block(std::vector<double> im, int w, int h) {
    std::vector<std::vector<double>> lst;
    for (int i = 0; i < w; i += BLOCK_SIZE) {
        for (int j = 0; j < h; j += BLOCK_SIZE) {
            std::vector<double> new_lst;
            for (int k = 0; k < BLOCK_SIZE; k++)
            for (int l = 0; l < BLOCK_SIZE; l++) {
                int index = (j+k)*w + i + l;
                new_lst.push_back(im[index]);
            }
            lst.push_back(new_lst);
        }
    }
    return lst;
}

void print_image(std::vector<double> &im, int w, int h) {
    std::cout << "Printing Image dim: (" << w << "," << h << ")" << std::endl;
    for (int i = 0; i < im.size(); i++) {
        std::cout << std::setw(11) << im[i] << " ";
        if ((i + 1) % w == 0) std::cout << std::endl;
    }
} 


void print_blocks(std::vector<std::vector<double>> &blocks) {
    for (int a = 0; a < blocks.size(); a++) {
        std::cout << "Printing block " << a << std::endl;
        for (int i = 0; i < blocks[a].size(); i++) {
            std::cout << std::setw(11) << blocks[a][i] << " ";
            if ((i + 1) % BLOCK_SIZE == 0) std::cout << std::endl;
        }
        std::cout << "---------------" << std::endl;
    }
} 



void raymond_average() {
    std::vector<double> im = read_image("../image/kung.txt");
    print_image(im, 16, 16);
    std::vector<std::vector<double>> blocks = split_image_eight_block(im, 16, 16);
    print_blocks(blocks);

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
    encrypted_dct(encoded_blocks[0], evaluator, encoder, encryptor);
    encrypted_dct(encoded_blocks[1], evaluator, encoder, encryptor);
    encrypted_dct(encoded_blocks[2], evaluator, encoder, encryptor);
    encrypted_dct(encoded_blocks[3], evaluator, encoder, encryptor);
    diff = std::chrono::steady_clock::now() - start; 
    std::cout << "DCT 1 Round: ";
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

/*
// Forward DCT
static void dct(dct_t *data)
{
    dct_t z1, z2, z3, z4, z5, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp10, tmp11, tmp12, tmp13, *data_ptr;

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
        tmp0 = data_ptr[0] + data_ptr[56];
        tmp7 = data_ptr[0] - data_ptr[56];
        tmp1 = data_ptr[8] + data_ptr[48];
        tmp6 = data_ptr[8] - data_ptr[48];
        tmp2 = data_ptr[16] + data_ptr[40];
        tmp5 = data_ptr[16] - data_ptr[40];
        tmp3 = data_ptr[24] + data_ptr[32];
        tmp4 = data_ptr[24] - data_ptr[32];
        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;
        data_ptr[0] = (tmp10 + tmp11) * 0.125;
        data_ptr[32] = (tmp10 - tmp11) * 0.125;
        z1 = (tmp12 + tmp13) * 0.541196100;
        data_ptr[16] = (z1 + tmp13 * 0.765366865) * 0.125;
        data_ptr[48] = (z1 + tmp12 * -1.847759065) * 0.125;
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
        data_ptr[56] = (tmp4 + z1 + z3) * 0.125;
        data_ptr[40] = (tmp5 + z2 + z4) * 0.125;
        data_ptr[24] = (tmp6 + z2 + z3) * 0.125;
        data_ptr[8] = (tmp7 + z1 + z4) * 0.125;
        data_ptr++;
    }
}
*/

/* Recieves a 8x8 box of pixels, in ciphertext form. 
 * Data should be laid out in a 64 element vector with 
 * rows first then columns
 */
void encrypted_dct(std::vector<Ciphertext> &data,
                   Evaluator &evaluator, 
                   FractionalEncoder &encoder, 
                   Encryptor &encryptor) {
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
    return;
}



// Forward DCT, regular no encryption
void dct(double *data) {
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