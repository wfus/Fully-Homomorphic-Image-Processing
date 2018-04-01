#include "seal/seal.h"
#include "fhe_image.h"
#include "jpge.h"
#include "stb_image.c"

using namespace seal;

auto start = std::chrono::steady_clock::now();
void fhe_jpg(std::vector<Ciphertext> &raw_data, 
             int width, 
             int height,
             Evaluator &evaluator,
             FractionalEncoder &encoder,
             Encryptor &encryptor);

const std::vector<double> S_STD_LUM_QUANT = { 16,11,12,14,12,10,16,14,13,14,18,17,16,19,24,40,26,24,22,22,24,49,35,37,29,40,58,51,61,60,57,51,56,55,64,72,92,78,64,68,87,69,55,56,80,109,81,87,95,98,103,104,103,62,77,113,121,112,100,120,92,101,103,99 };
const std::vector<double> S_STD_CROMA_QUANT = { 17,18,18,24,21,24,47,26,26,47,99,66,56,66,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99 };

int main(int argc, char** argv) {

    // Read encryption parameters from file
    int WIDTH = 0, HEIGHT = 0;
    std::ifstream paramfile;
    paramfile.open("../keys/params.txt");
    paramfile >> WIDTH;
    paramfile >> HEIGHT;
    std::cout << WIDTH << " " << HEIGHT << std::endl;
    paramfile.close();


    // Encryption Parameters
    EncryptionParameters params;
    params.set_poly_modulus("1x^8192 + 1");
    params.set_coeff_modulus(coeff_modulus_128(2048));
    params.set_plain_modulus(1 << 14);
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
    auto diff = std::chrono::steady_clock::now() - start; 
    std::cout << "Key Load Time: ";
    std::cout << chrono::duration<double, milli>(diff).count() << " ms" << std::endl;
    pkfile.close(); skfile.close();    


    // Encrytor and decryptor setup
    Encryptor encryptor(context, public_key);
    Evaluator evaluator(context);

    // FOR DEBUGGING ONLY!
    Decryptor decryptor(context, secret_key);

    // Base + Number of coefficients used for encoding past the decimal point (both pos and neg)
    // Example: if poly_base = 11, and N_FRACTIONAL_COEFFS=3, then we will have 
    // a1 * 11^-1 + a2 * 11^-2 + a3 * 11^-3
    const int POLY_BASE = 11;
    const int N_FRACTIONAL_COEFFS = 3;  
    const int N_NUMBER_COEFFS = 10;

    FractionalEncoder encoder(context.plain_modulus(), context.poly_modulus(), N_NUMBER_COEFFS, N_FRACTIONAL_COEFFS, POLY_BASE);

    std::ifstream myfile;
    myfile.open("../image/nothingpersonnel.txt");
    start = std::chrono::steady_clock::now(); 
    std::vector<Ciphertext> nothingpersonnel;
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            Ciphertext c;
            c.load(myfile);
            nothingpersonnel.push_back(c);
        }
    }
    myfile.close();
    diff = std::chrono::steady_clock::now() - start; 
    std::cout << "Ciphertext load time: ";
    std::cout << chrono::duration<double, milli>(diff).count() << " ms" << std::endl;
   
    /*
    for (int i = 0; i < nothingpersonnel.size(); i++) {
        Plaintext p;
        decryptor.decrypt(nothingpersonnel[i], p);
        std::cout << encoder.decode(p) << " ";
        if ((i+1) % WIDTH == 0) std::cout << std::endl;
    }
    */

    // Actually run the FHE calculations necessary...
    start = std::chrono::steady_clock::now(); 
    fhe_jpg(nothingpersonnel, WIDTH, HEIGHT, evaluator, encoder, encryptor);
    diff = std::chrono::steady_clock::now() - start; 
    std::cout << "DCT/QUANT calculation time: ";
    std::cout << chrono::duration<double, milli>(diff).count() << " ms" << std::endl;
    
    
    return 0;
}

void fhe_jpg(std::vector<Ciphertext> &raw_data, 
             int width, 
             int height,
             Evaluator &evaluator,
             FractionalEncoder &encoder,
             Encryptor &encryptor) {
    std::cout << "Got here" << std::endl;
    std::vector<std::vector<Ciphertext>> blocks = split_image_eight_block(raw_data, width, height);
    std::cout << "Got here" << std::endl;
    for (int i = 0; i < blocks.size(); i++) {
        encrypted_dct(blocks[i], evaluator, encoder, encryptor);
        quantize_fhe(blocks[i], S_STD_LUM_QUANT, evaluator, encoder, encryptor);
    }
}
