#include "seal/seal.h"
#include "fhe_image.h"
#include "jpge.h"
#include "stb_image.c"


using namespace seal;

auto start = std::chrono::steady_clock::now();
const bool VERBOSE = true;

std::vector<double> read_image(std::string fname);

int main()
{
    const char* test_filename = "../image/kung.jpg";
    const int requested_composition = 3;
    int width = 0, height = 0, actual_composition = 0;
    uint8_t *image_data = stbi_load(test_filename, &width, &height, &actual_composition, requested_composition);
    std::cout << width << " x " << height << std::endl;
    print_image(image_data,  width, height);


    // Encryption Parameters
    EncryptionParameters params;
    params.set_poly_modulus("1x^32768 + 1");
    params.set_coeff_modulus(coeff_modulus_128(2048));
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
    
    std::ofstream myfile;
    myfile.open("../image/nothingpersonnel.txt");
    start = std::chrono::steady_clock::now(); 
    for (int i = 0; i < width * height; i++) {
        Ciphertext c;
        double conv = (double)(image_data[i]);
        std::cout<< conv <<std::endl;
        encryptor.encrypt(encoder.encode(conv), c);
        c.save(myfile);
    }


    myfile.close();
    return 0;
}
