#include "seal/seal.h"
#include "fhe_image.h"
#include "jpge.h"
#include "stb_image.c"

using namespace seal;

auto start = std::chrono::steady_clock::now();
auto diff = std::chrono::steady_clock::now() - start;
void fhe_jpg(std::vector<Ciphertext> &raw_data, 
             int width, 
             int height,
             Evaluator &evaluator,
             FractionalEncoder &encoder,
             Encryptor &encryptor);

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
    params.set_coeff_modulus(coeff_modulus_128(2048));
    params.set_plain_modulus(1 << 12);
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


    for (int i = 0; i < red.size(); i++) {
        Plaintext p1, p2, p3;
        decryptor.decrypt(red[i], p1);
        decryptor.decrypt(green[i], p2);
        decryptor.decrypt(blue[i], p3);
        std::cout << "[" << encoder.decode(p1) << " " << encoder.decode(p2) << " " << encoder.decode(p3) << "] ";
        if ((i+1) % WIDTH == 0) std::cout << std::endl;
    }

    // Actually run the FHE calculations necessary...
    //fhe_jpg(nothingpersonnel, WIDTH, HEIGHT, evaluator, encoder, encryptor);
    
    

    
    return 0;
}

void fhe_jpg(std::vector<Ciphertext> &raw_data, 
             int width, 
             int height,
             Evaluator &evaluator,
             FractionalEncoder &encoder,
             Encryptor &encryptor) {
    // Perform DCT and quantization
    start = std::chrono::steady_clock::now(); 
    std::vector<std::vector<Ciphertext>> blocks = split_image_eight_block(raw_data, width, height);
    for (int i = 0; i < blocks.size(); i++) {
        encrypted_dct(blocks[i], evaluator, encoder, encryptor);
        quantize_fhe(blocks[i], S_STD_LUM_QUANT, evaluator, encoder, encryptor);
    }
    diff = std::chrono::steady_clock::now() - start; 
    std::cout << "DCT/QUANT calculation time: ";
    std::cout << chrono::duration<double, milli>(diff).count() << " ms" << std::endl;

    // Write output
    std::ofstream myfile;
    myfile.open("../image/zoop.txt");
    start = std::chrono::steady_clock::now(); 
    for (int i = 0; i < blocks.size(); i++) {
        for (int j = 0; j < blocks[i].size(); j++) {
            blocks[i][j].save(myfile);
        }
    }
    myfile.close();
    diff = std::chrono::steady_clock::now() - start; 
    std::cout << "Ciphertext write time: ";
    std::cout << chrono::duration<double, milli>(diff).count() << " ms" << std::endl;

}
