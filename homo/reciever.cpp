#include "seal/seal.h"
#include "fhe_image.h"
#include "jpge.h"
#include "stb_image.c"

using namespace seal;

auto start = std::chrono::steady_clock::now();

int main(int argc, char** argv) {

    // Read encryption parameters from file
    int WIDTH = 0, HEIGHT = 0;
    std::ifstream paramfile;
    paramfile.open("../keys/param.txt");
    paramfile >> WIDTH;
    paramfile >> HEIGHT;
    paramfile.close();


    // Encryption Parameters
    EncryptionParameters params;
    params.set_poly_modulus("1x^4096 + 1");
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
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        Ciphertext c;
        c.load(myfile);
        if (i % 100 == 0) std::cout << i << std::endl;
        nothingpersonnel.push_back(c);
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
    return 0;
}
