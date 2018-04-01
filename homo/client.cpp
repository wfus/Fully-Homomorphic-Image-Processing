#include "seal/seal.h"
#include "fhe_image.h"
#include "jpge.h"
#include "stb_image.c"


using namespace seal;

auto start = std::chrono::steady_clock::now();
auto diff = std::chrono::steady_clock::now() - start; 
const bool VERBOSE = true;

std::vector<double> read_image(std::string fname);

int main(int argc, char** argv) {
    bool sending = true;
    if (argc >= 2) {
        sending = false;
    }
    if (sending) {
        const char* test_filename = "../image/boaz.jpeg";
        const int requested_composition = 3;
        int width = 0, height = 0, actual_composition = 0;
        uint8_t *image_data = stbi_load(test_filename, &width, &height, &actual_composition, requested_composition);
        std::cout << width << " x " << height << std::endl;
        print_image(image_data,  width, height);

        // Encryption Parameters
        EncryptionParameters params;
        params.set_poly_modulus("1x^8192 + 1");
        params.set_coeff_modulus(coeff_modulus_128(2048));
        params.set_plain_modulus(1 << 12);
        SEALContext context(params);
        print_parameters(context);

        std::ofstream paramfile; 
        paramfile.open("../keys/params.txt");
        paramfile << width << " ";
        paramfile << height << " ";
        paramfile << (1 << 14) << std::endl;
        paramfile.close();


        // Generate keys
        std::ofstream pkfile, skfile;
        pkfile.open("../keys/pubkey.txt");
        skfile.open("../keys/seckey.txt");
        start = std::chrono::steady_clock::now(); 
        KeyGenerator keygen(context);
        auto public_key = keygen.public_key();
        auto secret_key = keygen.secret_key();
        public_key.save(pkfile);
        secret_key.save(skfile);
        diff = std::chrono::steady_clock::now() - start; 
        std::cout << "KeyGen: ";
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
        
        std::ofstream myfile;
        myfile.open("../image/nothingpersonnel.txt");
        std::cout << width << " " << height << std::endl;
        start = std::chrono::steady_clock::now(); 
        for (int i = 0; i < width * height; i++) {
            Ciphertext c;
            double conv = (double)(image_data[i]);
            encryptor.encrypt(encoder.encode(conv), c);
            c.save(myfile);
            if (i % 100 == 0) std::cout << i << std::endl;
        }
        diff = std::chrono::steady_clock::now() - start; 
        std::cout << "Ciphertext write: ";
        std::cout << chrono::duration<double, milli>(diff).count() << " ms" << std::endl;
        myfile.close();
    }
    else
    {
        // We are recieving the results and then doing the actual compression
        // Note that it is impossible to do compression with purely FHE, 
        // it is possible to do decompression though.
        int width, height;
        std::ifstream paramfile; 
        paramfile.open("../keys/params.txt");
        paramfile >> width;
        paramfile >> height;
        paramfile.close();
        int block_pix = BLOCK_SIZE * BLOCK_SIZE;
        int num_blocks = (width * height) / block_pix;
        if (num_blocks % block_pix > 0) {
            num_blocks += 1;
        }

        std::ofstream myfile;
        myfile.open("../image/zoop.txt");
        start = std::chrono::steady_clock::now(); 
        for (int i = 0; i < num_blocks; i++) {
            double block[block_pix];
            for (int j = 0; j < block_pix; j++) {
                // TODO
            }
        }

    }

    return 0;
}
