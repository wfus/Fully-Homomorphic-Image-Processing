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


#include "seal/seal.h"

using namespace seal;

auto start = std::chrono::steady_clock::now();

const std::vector<int> S_ZAG = { 0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63 };
const std::vector<double> S_STD_LUM_QUANT = { 16,11,12,14,12,10,16,14,13,14,18,17,16,19,24,40,26,24,22,22,24,49,35,37,29,40,58,51,61,60,57,51,56,55,64,72,92,78,64,68,87,69,55,56,80,109,81,87,95,98,103,104,103,62,77,113,121,112,100,120,92,101,103,99 };
const std::vector<double> S_STD_CROMA_QUANT = { 17,18,18,24,21,24,47,26,26,47,99,66,56,66,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99 };


void raymond_average();
void dct(double *data);
std::vector<double> read_image(std::string fname);

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
        std::cout << std::setw(5) << im[i] << " ";
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

void dct_blocks(std::vector<std::vector<double>> &blocks) {
    for (int a = 0; a < blocks.size(); a++) {
        dct(&blocks[a][0]);
    }
} 


void raymond_average() {
    std::vector<double> im = read_image("../image/kung.txt");
    print_image(im, 16, 16);
    std::vector<std::vector<double>> blocks = split_image_eight_block(im, 16, 16);
    dct_blocks(blocks);
    print_blocks(blocks);
    return;
}


// Forward DCT
void dct(double *data)
{
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