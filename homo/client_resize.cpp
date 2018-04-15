#include "seal/seal.h"
#include "fhe_image.h"
#include "fhe_resize.h"
#include "stb_image.c"
#include "cxxopts.h"

using namespace seal;

auto start = std::chrono::steady_clock::now();
auto diff = std::chrono::steady_clock::now() - start; 
const bool VERBOSE = true;

std::vector<double> read_image(std::string fname);

int main(int argc, const char** argv) {
    bool recieving = false;
    bool sending = false;
    std::string test_filename("./image/test.jpg");
    std::string ctext_outfile("./image/nothingpersonnel.txt");
    std::string ctext_infile("./image/zoop.txt");
    int n_number_coeffs = N_NUMBER_COEFFS;
    int n_fractional_coeffs = N_FRACTIONAL_COEFFS;
    int n_poly_base = POLY_BASE;
    int plain_modulus = PLAIN_MODULUS;
    int coeff_modulus = COEFF_MODULUS;
    int resized_width = 0;
    int resized_height = 0;

    try {
        cxxopts::Options options(argv[0], "Options for Client-Side FHE");
        options.positional_help("[optional args]").show_positional_help();

        options.add_options()
            ("r,recieve", "Is the client currently decrypting results", cxxopts::value<bool>(recieving))
            ("s,send", "Is the client currently encrypting raw image", cxxopts::value<bool>(sending))
            ("f,file", "Filename for input file to be resized", cxxopts::value<std::string>())
            ("o,outfile", "Filename for homomorphic ciphertext to be saved to", cxxopts::value<std::string>())
            ("c,cfile", "Filename for ciphertext result file to be checked for correctness", cxxopts::value<std::string>())
            ("ncoeff", "Number of coefficients for integer portion of encoding", cxxopts::value<int>())
            ("fcoeff", "Number of coefficients for fractional portion of encoding", cxxopts::value<int>())
            ("cmod", "Coefficient Modulus for polynomial encoding", cxxopts::value<int>())
            ("pmod", "Plaintext modulus", cxxopts::value<int>())
            ("width", "width of resized image", cxxopts::value<int>())
            ("height", "height of resized image", cxxopts::value<int>())
            ("base", "Polynomial base used for fractional encoding (essentially a number base)", cxxopts::value<int>())
            ("help", "Print help");

        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help({"", "Group"}) << std::endl;
            exit(0);
        }
        if (!recieving && !sending) {
            std::cout << "Please either toggle sending or recieving by using the flags: " << std::endl;
            std::cout << "\t--send or --recieve" << std::endl;
            std::cout << options.help({"", "Group"}) << std::endl;
            exit(0);
        }
        if (result.count("file")) test_filename = result["file"].as<std::string>();
        if (result.count("outfile")) ctext_outfile = result["outfile"].as<std::string>();
        if (result.count("cfile")) ctext_infile = result["cfile"].as<std::string>();
        if (result.count("ncoeff")) n_number_coeffs = result["ncoeff"].as<int>(); 
        if (result.count("fcoeff")) n_fractional_coeffs = result["fcoeff"].as<int>(); 
        if (result.count("pmod")) plain_modulus = result["pmod"].as<int>(); 
        if (result.count("cmod")) coeff_modulus = result["cmod"].as<int>(); 
        if (result.count("n_poly_base")) n_poly_base = result["base"].as<int>(); 
        
        
        if (result.count("width")) { 
            resized_width = result["width"].as<int>(); 
        } else {
            std::cout << "Please enter the width/height of the resized image..." << std::endl;
            std::cout << "\t Use --help to see the options." << std::endl;
        }
        if (result.count("height")) { 
            resized_height = result["height"].as<int>(); 
        } else {
            std::cout << "Please enter the width/height of the resized image..." << std::endl;
            std::cout << "\t Use --help to see the options." << std::endl;
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
        int channels = requested_composition;
        // std::vector<uint8_t> original_image;
        // for (int i = 0; i < width * height * channels; i++) {
        //     original_image.push_back(image_data[i]);
        // }
        // show_image_rgb(width, height, original_image);

        // Encryption Parameters
        EncryptionParameters params;
        params.set_poly_modulus("1x^8192 + 1");
        params.set_coeff_modulus(coeff_modulus_128(coeff_modulus));
        params.set_plain_modulus(plain_modulus);
        SEALContext context(params);
        print_parameters(context);

        std::ofstream paramfile; 
        paramfile.open("./keys/params.txt");
        paramfile << width << " ";
        paramfile << height << " ";
        paramfile << actual_composition << " ";
        paramfile << plain_modulus << std::endl;
        paramfile.close();

        // Generate keys
        // and save them to file
        std::ofstream pkfile, skfile;
        pkfile.open("./keys/pubkey.txt");
        skfile.open("./keys/seckey.txt");
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

        // Write to ciphertext as RGBRGBRGBRGB row by row. 
        std::ofstream myfile;
        myfile.open(ctext_outfile.c_str());
        std::cout << width << " " << height << std::endl;
        start = std::chrono::steady_clock::now(); 
        Ciphertext c;
        for (int i = 0; i < width * height * channels; i++) {
            encryptor.encrypt(encoder.encode(image_data[i]), c);
            c.save(myfile); 
            if (i % 100 == 0) std::cout << "Encoded "<< i << " intensities..." << std::endl;
        }
        diff = std::chrono::steady_clock::now() - start; 
        std::cout << "EncryptWrite: ";
        std::cout << chrono::duration<double, milli>(diff).count() << std::endl;
        std::cout << "EncryptWritePerPixel: " << chrono::duration<double, milli>(diff).count()/(width*height) << std::endl;
        myfile.close();
    }

    /******************************************************************************
     * Recieving,      SERVER --> CLIENT                                          *
     ******************************************************************************/
    else
    {
        // Encryption Parameters
        EncryptionParameters params;

        params.set_poly_modulus("1x^8192 + 1");
        params.set_coeff_modulus(coeff_modulus_128(coeff_modulus));
        params.set_plain_modulus(plain_modulus);
        SEALContext context(params);
        print_parameters(context);


        // Get keys
        std::ifstream pkfile, skfile;
        pkfile.open("./keys/pubkey.txt");
        skfile.open("./keys/seckey.txt");
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
        Decryptor decryptor(context, secret_key);

        FractionalEncoder encoder(context.plain_modulus(), context.poly_modulus(), n_number_coeffs, n_fractional_coeffs, n_poly_base);

        // Read in the image as RGB interleaved...
        // Assuming that there are three channels
        std::ifstream instream;
        std::cout << "Loading Ciphertexts now..." << std::endl; 
        instream.open(ctext_infile.c_str());
        std::vector<uint8_t> decrypted_image;
        Plaintext p;
        Ciphertext c;
        for (int i = 0; i < resized_width * resized_height * 3; i++) {
            c.load(instream);
            decryptor.decrypt(c, p);
            std::cout << i << '\t' << encoder.decode(p) << std::endl;
            uint8_t pixel = (uint8_t) encoder.decode(p);
            CLAMP(pixel, 0, 255)
            decrypted_image.push_back(pixel);
        }
        instream.close();

        // Display our decrypted image!
        show_image_rgb(resized_width, resized_height, decrypted_image);
        save_image_rgb(resized_width, resized_height, decrypted_image);
    }
    return 0;
}
