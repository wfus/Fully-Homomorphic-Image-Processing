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
    std::vector<double> im = read_image("image/kung.txt");
    print_image(im, 16, 16);
    std::vector<std::vector<double>> blocks = split_image_eight_block(im, 16, 16);
    print_blocks(blocks);

    EncryptionParameters params;
    params.set_poly_modulus("1x^32768 + 1");
    params.set_coeff_modulus(coeff_modulus_128(2048));
    params.set_plain_modulus(1 << 8);
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


    FractionalEncoder encoder(context.plain_modulus(), context.poly_modulus(), 64, 32, 3);

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
    
    std::cout << "Noise budget in result: "
        << decryptor.invariant_noise_budget(encoded_blocks[0][0]) 
        << " bits" << std::endl;

    
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
    print_blocks(decoded_blocks);
    diff = std::chrono::steady_clock::now() - start; 
    std::cout << "Decrypting blocks: ";
    std::cout << chrono::duration<double, milli>(diff).count() << " ms" << std::endl;
    
    
    
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
        Ciphertext goodmorning457(data[curr_index+0]); evaluator.add(goodmorning457, data[curr_index+7]); tmp0 = goodmorning457;
        Ciphertext goodmorning458(data[curr_index+0]); evaluator.sub(goodmorning458, data[curr_index+7]); tmp7 = goodmorning458;
        Ciphertext goodmorning459(data[curr_index+1]); evaluator.add(goodmorning459, data[curr_index+6]); tmp1 = goodmorning459;
        Ciphertext goodmorning460(data[curr_index+1]); evaluator.sub(goodmorning460, data[curr_index+6]); tmp6 = goodmorning460;
        Ciphertext goodmorning461(data[curr_index+2]); evaluator.add(goodmorning461, data[curr_index+5]); tmp2 = goodmorning461;
        Ciphertext goodmorning462(data[curr_index+2]); evaluator.sub(goodmorning462, data[curr_index+5]); tmp5 = goodmorning462;
        Ciphertext goodmorning463(data[curr_index+3]); evaluator.add(goodmorning463, data[curr_index+4]); tmp3 = goodmorning463;
        Ciphertext goodmorning464(data[curr_index+3]); evaluator.sub(goodmorning464, data[curr_index+4]); tmp4 = goodmorning464;
        Ciphertext goodmorning465(tmp0); evaluator.add(goodmorning465, tmp3); tmp10 = goodmorning465;
        Ciphertext goodmorning466(tmp0); evaluator.sub(goodmorning466, tmp3); tmp13 = goodmorning466;
        Ciphertext goodmorning467(tmp1); evaluator.add(goodmorning467, tmp2); tmp11 = goodmorning467;
        Ciphertext goodmorning468(tmp1); evaluator.sub(goodmorning468, tmp2); tmp12 = goodmorning468;
        Ciphertext goodmorning469(tmp10); evaluator.add(goodmorning469, tmp11); data[curr_index+0] = goodmorning469;
        Ciphertext goodmorning470(tmp10); evaluator.sub(goodmorning470, tmp11); data[curr_index+4] = goodmorning470;
        Ciphertext goodmorning471(tmp12); evaluator.add(goodmorning471, tmp13); Ciphertext goodmorning472(goodmorning471); evaluator.multiply_plain(goodmorning472, encoder.encode(0.541196100)); z1 = goodmorning472;
        Ciphertext goodmorning473(tmp13); evaluator.multiply_plain(goodmorning473, encoder.encode(0.765)); Ciphertext goodmorning474(z1); evaluator.add(goodmorning474, goodmorning473); data[curr_index+2] = goodmorning474;
        Ciphertext manual0(tmp12); evaluator.multiply_plain(manual0, encoder.encode(-1.847)); Ciphertext manual1(z1); evaluator.add(manual1, manual0); data[curr_index+6] = manual1;
        Ciphertext goodmorning475(tmp4); evaluator.add(goodmorning475, tmp7); z1 = goodmorning475;
        Ciphertext goodmorning476(tmp5); evaluator.add(goodmorning476, tmp6); z2 = goodmorning476;
        Ciphertext goodmorning477(tmp4); evaluator.add(goodmorning477, tmp6); z3 = goodmorning477;
        Ciphertext goodmorning478(tmp5); evaluator.add(goodmorning478, tmp7); z4 = goodmorning478;
        Ciphertext goodmorning479(z3); evaluator.add(goodmorning479, z4); Ciphertext goodmorning480(goodmorning479); evaluator.multiply_plain(goodmorning480, encoder.encode(1.175875602)); z5 = goodmorning480;
        Ciphertext goodmorning481(tmp4); evaluator.multiply_plain(goodmorning481, encoder.encode(0.298)); tmp4 = goodmorning481;
        Ciphertext goodmorning482(tmp5); evaluator.multiply_plain(goodmorning482, encoder.encode(2.053)); tmp5 = goodmorning482;
        Ciphertext goodmorning483(tmp6); evaluator.multiply_plain(goodmorning483, encoder.encode(3.072)); tmp6 = goodmorning483;
        Ciphertext goodmorning484(tmp7); evaluator.multiply_plain(goodmorning484, encoder.encode(1.501)); tmp7 = goodmorning484;
        evaluator.multiply_plain(z1, encoder.encode(-0.899));
        evaluator.multiply_plain(z2, encoder.encode(-2.562));
        evaluator.multiply_plain(z3, encoder.encode(-1.961));
        evaluator.multiply_plain(z4, encoder.encode(-0.390));
        Ciphertext goodmorning485(z3); evaluator.add(goodmorning485, z5); z3 = goodmorning485;
        Ciphertext goodmorning486(z4); evaluator.add(goodmorning486, z5); z4 = goodmorning486;
        Ciphertext goodmorning487(tmp4); evaluator.add(goodmorning487, z1); Ciphertext goodmorning488(goodmorning487); evaluator.add(goodmorning488, z3); data[curr_index+7] = goodmorning488;
        Ciphertext goodmorning489(tmp5); evaluator.add(goodmorning489, z2); Ciphertext goodmorning490(goodmorning489); evaluator.add(goodmorning490, z4); data[curr_index+5] = goodmorning490;
        Ciphertext goodmorning491(tmp6); evaluator.add(goodmorning491, z2); Ciphertext goodmorning492(goodmorning491); evaluator.add(goodmorning492, z3); data[curr_index+3] = goodmorning492;
        Ciphertext goodmorning493(tmp7); evaluator.add(goodmorning493, z1); Ciphertext goodmorning494(goodmorning493); evaluator.add(goodmorning494, z4); data[curr_index+1] = goodmorning494; 

        curr_index += BLOCK_SIZE;
    }
    curr_index = 0; 

    for (int c = 0; c < BLOCK_SIZE; c++) {
        Ciphertext goodmorning1(data[curr_index+0]); evaluator.add(goodmorning1, data[curr_index+56]); tmp0 = goodmorning1;
        Ciphertext goodmorning2(data[curr_index+0]); evaluator.sub(goodmorning2, data[curr_index+56]); tmp7 = goodmorning2;
        Ciphertext goodmorning3(data[curr_index+8]); evaluator.add(goodmorning3, data[curr_index+48]); tmp1 = goodmorning3;
        Ciphertext goodmorning4(data[curr_index+8]); evaluator.sub(goodmorning4, data[curr_index+48]); tmp6 = goodmorning4;
        Ciphertext goodmorning5(data[curr_index+16]); evaluator.add(goodmorning5, data[curr_index+40]); tmp2 = goodmorning5;
        Ciphertext goodmorning6(data[curr_index+16]); evaluator.sub(goodmorning6, data[curr_index+40]); tmp5 = goodmorning6;
        Ciphertext goodmorning7(data[curr_index+24]); evaluator.add(goodmorning7, data[curr_index+32]); tmp3 = goodmorning7;
        Ciphertext goodmorning8(data[curr_index+24]); evaluator.sub(goodmorning8, data[curr_index+32]); tmp4 = goodmorning8;
        Ciphertext goodmorning9(tmp0); evaluator.add(goodmorning9, tmp3); tmp10 = goodmorning9;
        Ciphertext goodmorning10(tmp0); evaluator.sub(goodmorning10, tmp3); tmp13 = goodmorning10;
        Ciphertext goodmorning11(tmp1); evaluator.add(goodmorning11, tmp2); tmp11 = goodmorning11;
        Ciphertext goodmorning12(tmp1); evaluator.sub(goodmorning12, tmp2); tmp12 = goodmorning12;
        Ciphertext goodmorning13(tmp10); evaluator.add(goodmorning13, tmp11); Ciphertext goodmorning14(goodmorning13); evaluator.multiply_plain(goodmorning14, encoder.encode(0.125)); data[curr_index+0] = goodmorning14;
        Ciphertext goodmorning15(tmp10); evaluator.sub(goodmorning15, tmp11); Ciphertext goodmorning16(goodmorning15); evaluator.multiply_plain(goodmorning16, encoder.encode(0.125)); data[curr_index+32] = goodmorning16;
        Ciphertext goodmorning17(tmp12); evaluator.add(goodmorning17, tmp13); Ciphertext goodmorning18(goodmorning17); evaluator.multiply_plain(goodmorning18, encoder.encode(0.541)); z1 = goodmorning18;
        Ciphertext goodmorning19(tmp13); evaluator.multiply_plain(goodmorning19, encoder.encode(0.765)); Ciphertext goodmorning20(z1); evaluator.add(goodmorning20, goodmorning19); Ciphertext goodmorning21(goodmorning20); evaluator.multiply_plain(goodmorning21, encoder.encode(0.125)); data[curr_index+16] = goodmorning21;
        Ciphertext manual19(tmp12); evaluator.multiply_plain(manual19, encoder.encode(-1.847)); Ciphertext manual20(z1); evaluator.add(manual20, manual19); Ciphertext manual21(manual20); evaluator.multiply_plain(manual21, encoder.encode(0.125)); data[curr_index+48] = manual21;
        Ciphertext goodmorning22(tmp4); evaluator.add(goodmorning22, tmp7); z1 = goodmorning22;
        Ciphertext goodmorning23(tmp5); evaluator.add(goodmorning23, tmp6); z2 = goodmorning23;
        Ciphertext goodmorning24(tmp4); evaluator.add(goodmorning24, tmp6); z3 = goodmorning24;
        Ciphertext goodmorning25(tmp5); evaluator.add(goodmorning25, tmp7); z4 = goodmorning25;
        Ciphertext goodmorning26(z3); evaluator.add(goodmorning26, z4); Ciphertext goodmorning27(goodmorning26); evaluator.multiply_plain(goodmorning27, encoder.encode(1.175875602)); z5 = goodmorning27;
        Ciphertext goodmorning28(tmp4); evaluator.multiply_plain(goodmorning28, encoder.encode(0.298)); tmp4 = goodmorning28;
        Ciphertext goodmorning29(tmp5); evaluator.multiply_plain(goodmorning29, encoder.encode(2.053)); tmp5 = goodmorning29;
        Ciphertext goodmorning30(tmp6); evaluator.multiply_plain(goodmorning30, encoder.encode(3.072)); tmp6 = goodmorning30;
        Ciphertext goodmorning31(tmp7); evaluator.multiply_plain(goodmorning31, encoder.encode(1.501)); tmp7 = goodmorning31;
        evaluator.multiply_plain(z1, encoder.encode(-0.899));
        evaluator.multiply_plain(z2, encoder.encode(-2.562));
        evaluator.multiply_plain(z3, encoder.encode(-1.961));
        evaluator.multiply_plain(z4, encoder.encode(-0.390));
        Ciphertext goodmorning32(z3); evaluator.add(goodmorning32, z5); z3 = goodmorning32;
        Ciphertext goodmorning33(z4); evaluator.add(goodmorning33, z5); z4 = goodmorning33;
        Ciphertext goodmorning34(tmp4); evaluator.add(goodmorning34, z1); Ciphertext goodmorning35(goodmorning34); evaluator.add(goodmorning35, z3); Ciphertext goodmorning36(goodmorning35); evaluator.multiply_plain(goodmorning36, encoder.encode(0.125)); data[curr_index+56] = goodmorning36;
        Ciphertext goodmorning37(tmp5); evaluator.add(goodmorning37, z2); Ciphertext goodmorning38(goodmorning37); evaluator.add(goodmorning38, z4); Ciphertext goodmorning39(goodmorning38); evaluator.multiply_plain(goodmorning39, encoder.encode(0.125)); data[curr_index+40] = goodmorning39;
        Ciphertext goodmorning40(tmp6); evaluator.add(goodmorning40, z2); Ciphertext goodmorning41(goodmorning40); evaluator.add(goodmorning41, z3); Ciphertext goodmorning42(goodmorning41); evaluator.multiply_plain(goodmorning42, encoder.encode(0.125)); data[curr_index+24] = goodmorning42;
        Ciphertext goodmorning43(tmp7); evaluator.add(goodmorning43, z1); Ciphertext goodmorning44(goodmorning43); evaluator.add(goodmorning44, z4); Ciphertext goodmorning45(goodmorning44); evaluator.multiply_plain(goodmorning45, encoder.encode(0.125)); data[curr_index+8] = goodmorning45;
        curr_index++;
    }
    return;
}
/*
void encrypted_dct(std::vector<Ciphertext> &data,
                   Evaluator &evaluator, 
                   FractionalEncoder &encoder, 
                   Encryptor &encryptor) {
    Ciphertext z1, z2, z3, z4, z5; 
    Ciphertext tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp10, tmp11, tmp12, tmp13;
    Ciphertext tmp14;
    Ciphertext cons1;
    encryptor.encrypt(encoder.encode(0.5441), cons1); 
    int curr_index = 0;
    for (int c = 0; c < BLOCK_SIZE; c++) {
        tmp0 = Ciphertext(data[curr_index+0]); evaluator.add(tmp0, data[curr_index+7]);
        tmp7 = Ciphertext(data[curr_index+0]); evaluator.sub(tmp7, data[curr_index+7]);
        tmp1 = Ciphertext(data[curr_index+1]); evaluator.add(tmp1, data[curr_index+6]);
        tmp6 = Ciphertext(data[curr_index+1]); evaluator.sub(tmp6, data[curr_index+6]);
        tmp2 = Ciphertext(data[curr_index+2]); evaluator.add(tmp2, data[curr_index+5]);
        tmp5 = Ciphertext(data[curr_index+2]); evaluator.sub(tmp5, data[curr_index+5]);
        tmp3 = Ciphertext(data[curr_index+3]); evaluator.add(tmp3, data[curr_index+4]);
        tmp4 = Ciphertext(data[curr_index+3]); evaluator.sub(tmp4, data[curr_index+4]);

        tmp10 = Ciphertext(tmp0); evaluator.add(tmp10, tmp3);
        tmp13 = Ciphertext(tmp0); evaluator.sub(tmp13, tmp3);
        tmp11 = Ciphertext(tmp1); evaluator.add(tmp11, tmp2);
        tmp12 = Ciphertext(tmp1); evaluator.sub(tmp12, tmp2);

        data[curr_index+0] = tmp10; evaluator.add(data[curr_index+0], tmp11);
        data[curr_index+4] = tmp10; evaluator.sub(data[curr_index+4], tmp11);
        tmp14 = tmp12; evaluator.add(tmp14, tmp13);  
        z1 = tmp14; evaluator.multiply(z1, cons1);

        evaluator.multiply_plain(tmp13, encoder.encode(0.7653));
        evaluator.multiply_plain(tmp12, encoder.encode(-1.8477));
        data[curr_index+2] = z1; evaluator.add(data[curr_index+2], tmp13);
        data[curr_index+6] = z1; evaluator.sub(data[curr_index+6], tmp12);

        z1 = tmp4; evaluator.add(z1, tmp7);
        z2 = tmp5; evaluator.add(z2, tmp6);
        z3 = tmp4; evaluator.add(z3, tmp6);
        z4 = tmp5; evaluator.add(z4, tmp7);
        z5 = z3; evaluator.add(z5, z4); evaluator.multiply_plain(z5, encoder.encode(1.1758));

        evaluator.multiply_plain(tmp4, encoder.encode(0.298)); 
        evaluator.multiply_plain(tmp5, encoder.encode(2.053)); 
        evaluator.multiply_plain(tmp6, encoder.encode(3.072)); 
        evaluator.multiply_plain(tmp7, encoder.encode(1.501)); 

    
        evaluator.multiply_plain(z1, encoder.encode(-0.899)); 
        evaluator.multiply_plain(z2, encoder.encode(-2.562)); 
        evaluator.multiply_plain(z3, encoder.encode(-1.961)); 
        evaluator.multiply_plain(z4, encoder.encode(-0.390)); 

        evaluator.add(z3, z5);
        evaluator.add(z4, z5);

        data[curr_index+7] = tmp4; evaluator.add(data[curr_index+7], z1); evaluator.add(data[curr_index+7], z3);
        data[curr_index+5] = tmp5; evaluator.add(data[curr_index+5], z2); evaluator.add(data[curr_index+5], z4);
        data[curr_index+3] = tmp6; evaluator.add(data[curr_index+3], z2); evaluator.add(data[curr_index+3], z3);
        data[curr_index+1] = tmp7; evaluator.add(data[curr_index+1], z1); evaluator.add(data[curr_index+1], z4);

        curr_index += BLOCK_SIZE;
    }
    curr_index = 0; 

    for (int c = 0; c < BLOCK_SIZE; c++) {
        tmp0 = data[c + 0]; evaluator.add(tmp0, data[c + 56]);
        tmp7 = data[c + 0]; evaluator.add(tmp7, data[c + 56]);
        tmp1 = data[c + 8]; evaluator.add(tmp1, data[c + 48]);
        tmp6 = data[c + 8]; evaluator.add(tmp6, data[c + 48]);
        tmp2 = data[c + 16]; evaluator.add(tmp2, data[c + 40]);
        tmp5 = data[c + 16]; evaluator.add(tmp5, data[c + 40]);
        tmp3 = data[c + 24]; evaluator.add(tmp3, data[c + 32]);
        tmp4 = data[c + 24]; evaluator.add(tmp4, data[c + 32]);



    }
    return;
}*/
