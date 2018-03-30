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
#include "jpge.h"

using namespace seal;

auto start = std::chrono::steady_clock::now();

const std::vector<int> S_ZAG = { 0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63 };
const std::vector<double> S_STD_LUM_QUANT = { 16,11,12,14,12,10,16,14,13,14,18,17,16,19,24,40,26,24,22,22,24,49,35,37,29,40,58,51,61,60,57,51,56,55,64,72,92,78,64,68,87,69,55,56,80,109,81,87,95,98,103,104,103,62,77,113,121,112,100,120,92,101,103,99 };
const std::vector<double> S_STD_CROMA_QUANT = { 17,18,18,24,21,24,47,26,26,47,99,66,56,66,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99 };


void raymond_average();
std::vector<unsigned char> read_image(std::string fname);

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

std::vector<unsigned char> read_image(std::string fname) {
    int w, h;
    std::vector<unsigned char> im; 
    std::ifstream myfile;
        
    myfile.open(fname.c_str());
    myfile >> w;
    myfile >> h;
    std::cout << "Read in " << fname << "with dimensions: " << w << " x " << h << std::endl;

    int tmp;
    for (int i = 0; i < w*h; i++) {
        myfile >> tmp;
        im.push_back((unsigned char) tmp);
    }   
    return im; 
}

void print_image(std::vector<unsigned char> &im, int w, int h) {
    std::cout << "Printing Image dim: (" << w << "," << h << ")" << std::endl;
    for (int i = 0; i < im.size(); i++) {
        std::cout << std::setw(5) << (int) im[i] << " ";
        if ((i + 1) % w == 0) std::cout << std::endl;
    }
} 

void raymond_average() {
    int width = 16;
    int height = 16;
    std::vector<unsigned char> im = read_image("../image/kung.txt");
    print_image(im, width, height);
    int buf_size = 1024;
    void *pBuf = malloc(buf_size);
    jpge::params params;
    params.m_quality = 90;
    params.m_subsampling = jpge::Y_ONLY;
    jpge::compress_image_to_jpeg_file_in_memory(pBuf, buf_size, width, height, 1, &im[0], params);
    std::ofstream myfile;
    unsigned char* buf = (unsigned char*) pBuf;
    myfile.open("blunt.jpg");
    for (int i = 0; i < buf_size; i++) {
        myfile << buf[i];
    }
    return;
}