#include "seal/seal.h"
#include "fhe_image.h"
#include "jpge.h"
#include "stb_image.c"

using namespace seal;

auto start = std::chrono::steady_clock::now();
auto diff = std::chrono::steady_clock::now() - start;
void fhe_jpg(std::vector<Ciphertext> &y,
             std::vector<Ciphertext> &cb,
             std::vector<Ciphertext> &cr,
             int width, 
             int height,
             Evaluator &evaluator,
             FractionalEncoder &encoder,
             Encryptor &encryptor);
void save_computed_blocks_interleaved_ycc(std::vector<std::vector<Ciphertext>> &y,
                                          std::vector<std::vector<Ciphertext>> &cb,
                                          std::vector<std::vector<Ciphertext>> &cr);
int main(int argc, char** argv) {

    // Read encryption parameters from file
    int WIDTH = 0, HEIGHT = 0, CHANNELS=0;
    std::ifstream paramfile;
    paramfile.open("../keys/params.txt");
    paramfile >> WIDTH;
    paramfile >> HEIGHT;
    paramfile >> CHANNELS;
    std::cout << WIDTH << " " << HEIGHT << " Channels: " << CHANNELS << std::endl;
    assert(CHANNELS != 0);
    assert(CHANNELS == 3);
    assert(WIDTH != 0);
    assert(HEIGHT != 0);
    paramfile.close();


    // Encryption Parameters
    EncryptionParameters params;
    params.set_poly_modulus("1x^8192 + 1");
    params.set_coeff_modulus(coeff_modulus_128(COEFF_MODULUS));
    params.set_plain_modulus(PLAIN_MODULUS);
    SEALContext context(params);
    print_parameters(context);


    // Generate keys
    std::ifstream pkfile, skfile;
    pkfile.open("../keys/pubkey.txt");
    skfile.open("../keys/seckey.txt");
    start = std::chrono::steady_clock::now(); 
    PublicKey public_key;
    SecretKey secret_key;
    public_key.load(pkfile);
    secret_key.load(skfile);
    diff = std::chrono::steady_clock::now() - start; 
    std::cout << "Key Load Time: ";
    std::cout << chrono::duration<double, milli>(diff).count() << " ms" << std::endl;
    pkfile.close(); skfile.close();    


    // Encrytor and decryptor setup
    Encryptor encryptor(context, public_key);
    Evaluator evaluator(context);

    // FOR DEBUGGING ONLY!
    Decryptor decryptor(context, secret_key);


    FractionalEncoder encoder(context.plain_modulus(), context.poly_modulus(), N_NUMBER_COEFFS, N_FRACTIONAL_COEFFS, POLY_BASE);

    std::ifstream myfile;
    myfile.open("../image/nothingpersonnel.txt");
    start = std::chrono::steady_clock::now(); 
    std::vector<Ciphertext> red, green, blue;
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            Ciphertext c;
            c.load(myfile); red.push_back(c);
            c.load(myfile); green.push_back(c);
            c.load(myfile); blue.push_back(c);
        }
    }
    myfile.close();
    diff = std::chrono::steady_clock::now() - start; 
    std::cout << "Ciphertext load time: ";
    std::cout << chrono::duration<double, milli>(diff).count() << " ms" << std::endl;




    // CONVERT RGB INTO YCC
    for (int i = 0; i < red.size(); i++) {
        rgb_to_ycc_fhe(red[i], green[i], blue[i], evaluator, encoder, encryptor);
    }


    // for (int i = 0; i < red.size() && i < 64; i++) {
    //     Plaintext p1, p2, p3;
    //     decryptor.decrypt(red[i], p1);
    //     decryptor.decrypt(green[i], p2);
    //     decryptor.decrypt(blue[i], p3);
    //     std::cout << "[" << encoder.decode(p1) << " " << encoder.decode(p2) << " " << encoder.decode(p3) << "] ";
    //     if ((i+1) % WIDTH == 0) std::cout << std::endl;
    // }




    if (CHANNELS == 3) {
        fhe_jpg(red, green, blue, WIDTH, HEIGHT, evaluator, encoder, encryptor);
    }

    
    
    

    // Actually run the FHE calculations necessary, only supporting 3channel for now
    // At this point, red->Y, green->Cb, blue->Cr
    
    

    
    return 0;
}

void fhe_jpg(std::vector<Ciphertext> &y,
             std::vector<Ciphertext> &cb,
             std::vector<Ciphertext> &cr,
             int width, 
             int height,
             Evaluator &evaluator,
             FractionalEncoder &encoder,
             Encryptor &encryptor) {
    // Perform DCT and quantization
    start = std::chrono::steady_clock::now(); 
    std::vector<std::vector<Ciphertext>> y_blocks = split_image_eight_block(y, width, height);
    std::vector<std::vector<Ciphertext>> cb_blocks = split_image_eight_block(cb, width, height);
    std::vector<std::vector<Ciphertext>> cr_blocks = split_image_eight_block(cr, width, height);
    for (int i = 0; i < y_blocks.size(); i++) {
        encrypted_dct(y_blocks[i], evaluator, encoder, encryptor);
        // quantize_fhe(y_blocks[i], S_STD_LUM_QUANT, evaluator, encoder, encryptor);
        encrypted_dct(cb_blocks[i], evaluator, encoder, encryptor);
        // quantize_fhe(cb_blocks[i], S_STD_CROMA_QUANT, evaluator, encoder, encryptor);
        encrypted_dct(cr_blocks[i], evaluator, encoder, encryptor);
        // quantize_fhe(cr_blocks[i], S_STD_CROMA_QUANT, evaluator, encoder, encryptor);
    }
    diff = std::chrono::steady_clock::now() - start; 
    std::cout << "DCT/QUANT calculation time: ";
    std::cout << chrono::duration<double, milli>(diff).count() << " ms" << std::endl;

    save_computed_blocks_interleaved_ycc(y_blocks, cb_blocks, cr_blocks); 
}

// We want to store the output as blocks of Y Cb Cr interleaved by block
// This is why Nico is moving out
void save_computed_blocks_interleaved_ycc(std::vector<std::vector<Ciphertext>> &y,
                                          std::vector<std::vector<Ciphertext>> &cb,
                                          std::vector<std::vector<Ciphertext>> &cr) {
    std::ofstream myfile;
    myfile.open("../image/zoop.txt");
    start = std::chrono::steady_clock::now(); 
    for (int i = 0; i < y.size(); i++) {
        for (int j = 0; j < y[i].size(); j++) {
            y[i][j].save(myfile);
        }
        for (int j = 0; j < cb[i].size(); j++) {
            cb[i][j].save(myfile);
        }
        for (int j = 0; j < cr[i].size(); j++) {
            cr[i][j].save(myfile);
        }
    }
    myfile.close();
    diff = std::chrono::steady_clock::now() - start; 
    std::cout << "Ciphertext write time: ";
    std::cout << chrono::duration<double, milli>(diff).count() << " ms" << std::endl;
}
