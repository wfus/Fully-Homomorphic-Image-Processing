#include "seal/seal.h"
#include "fhe_decode.h"
#include "fhe_image.h"
#include "cxxopts.h"
#include "stb_image.c"

using namespace seal;

auto start = std::chrono::steady_clock::now();
auto diff = std::chrono::steady_clock::now() - start; 
const bool VERBOSE = true;


int main(int argc, const char** argv) {
    bool recieving = false;
    bool sending = false;
    std::string test_filename("../image/boazbarak.jpg");

    try {
        cxxopts::Options options(argv[0], "Options for Client-Side FHE");
        options.positional_help("[optional args]").show_positional_help();

        options.add_options()
            ("r,recieve", "Is the client currently decrypting results", cxxopts::value<bool>(recieving))
            ("s,send", "Is the client currently encrypting raw image", cxxopts::value<bool>(sending))
            ("f,file", "Filename for input file to be homomorphically encrypted", cxxopts::value<std::string>())
            ("help", "Print help");

        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help({"", "Group"}) << std::endl;
            exit(0);
        }
        if (result.count("file")) {
            test_filename = result["file"].as<std::string>();
        } 
    } 
    catch (const cxxopts::OptionException& e) {
        std::cout << "error parsing options: " << e.what() << std::endl;
        exit(1);
    }

    if (sending) {
        const int requested_composition = 3;
        int width = 0, height = 0, actual_composition = 0;
        uint8_t *image_data = stbi_load(test_filename.c_str(), &width, &height, &actual_composition, requested_composition);
        // The image will be interleaved r g b r g b ...
        std::cout << width << " x " << height << " Channels: " << actual_composition << std::endl;

        //jo_write_jpg("../image/boazbarak.jpg", image_data, width, height, 3, 100);


        // Encryption Parameters
        EncryptionParameters params;
        params.set_poly_modulus("1x^8192 + 1");
        params.set_coeff_modulus(coeff_modulus_128(COEFF_MODULUS));
        params.set_plain_modulus(PLAIN_MODULUS);
        SEALContext context(params);
        print_parameters(context);

        std::ofstream paramfile; 
        paramfile.open("../keys/params.txt");
        paramfile << width << " ";
        paramfile << height << " ";
        paramfile << actual_composition << " ";
        paramfile << PLAIN_MODULUS << std::endl;
        paramfile.close();


        // Generate keys
        // and save them to file
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
        FractionalEncoder encoder(context.plain_modulus(), context.poly_modulus(), N_NUMBER_COEFFS, N_FRACTIONAL_COEFFS, POLY_BASE);

        // Write the ciphertext to file block by block - since most times
        // ciphertext doesn't fit directly in RAM
        // Write RED 8x8 BLOCK, GREEN 8x8 BLOCK, then BLUE 8x8 BLOCK
        // Repeats until the entire image is converted to ciphertext. 
        std::vector<double> red, green, blue; 
        
        for (int i = 0; i < width * height * actual_composition; i+=3) {
            red.push_back((double) image_data[i]);
            green.push_back((double) image_data[i+1]);
            blue.push_back((double) image_data[i+2]);
        }
        std::vector<std::vector<double>> red_blocks, green_blocks, blue_blocks;
        red_blocks = split_image_eight_block(red, width, height);
        green_blocks = split_image_eight_block(green, width, height);
        blue_blocks = split_image_eight_block(blue, width, height);

        std::ofstream myfile;
        myfile.open("../image/nothingpersonnel.txt");
        std::cout << width << " " << height << std::endl;
        start = std::chrono::steady_clock::now(); 
        Ciphertext c;
        double conv;
        for (int i = 0; i < red_blocks.size(); i++) {
            for (int j = 0; j < red_blocks[i].size(); j++) {
                conv = red_blocks[i][j];
                encryptor.encrypt(encoder.encode(conv), c);
                c.save(myfile);
            }
            for (int j = 0; j < green_blocks[i].size(); j++) {
                conv = green_blocks[i][j];
                encryptor.encrypt(encoder.encode(conv), c);
                c.save(myfile);
            }
            for (int j = 0; j < blue_blocks[i].size(); j++) {
                conv = blue_blocks[i][j];
                encryptor.encrypt(encoder.encode(conv), c);
                c.save(myfile);
            }
            if (i % 10 == 0) std::cout << "Encoded "<< i << " blocks..." << std::endl;
        }
        diff = std::chrono::steady_clock::now() - start; 
        std::cout << "Ciphertext write: ";
        std::cout << chrono::duration<double, milli>(diff).count() << " ms" << std::endl;
        myfile.close();
    }
    else
    {
        const char* infile = "../image/zoop.txt";
        const char* outfile = "../image/new_boazzzzzz.jpg";

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
        params.set_coeff_modulus(coeff_modulus_128(COEFF_MODULUS));
        params.set_plain_modulus(PLAIN_MODULUS);
        SEALContext context(params);
        print_parameters(context);


        // Get keys
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

        Encryptor encryptor(context, public_key);
        Evaluator evaluator(context);
        // FOR DEBUGGING ONLY!
        Decryptor decryptor(context, secret_key);
        FractionalEncoder encoder(context.plain_modulus(), context.poly_modulus(), N_NUMBER_COEFFS, N_FRACTIONAL_COEFFS, POLY_BASE);
    }
    return 0;
}
