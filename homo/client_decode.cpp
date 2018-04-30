#include "seal/seal.h"
#include "fhe_image.h"
#include "fhe_resize.h"
#include "fhe_decode.h"
#include "cxxopts.h"

using namespace seal;

auto start = std::chrono::steady_clock::now();
auto diff = std::chrono::steady_clock::now() - start; 
const bool VERBOSE = true;

int main(int argc, const char** argv) {
    bool recieving = false;
    bool sending = false;
    bool bicubic = false;
    std::string test_filename("./image/encode_test.png");
    std::string ctext_outfile("./image/nothingpersonnel.txt");
    std::string ctext_infile("./image/zoop.txt");
    std::string test_output("./image/out.png");
    int n_number_coeffs = N_NUMBER_COEFFS;
    int n_fractional_coeffs = N_FRACTIONAL_COEFFS;
    int n_poly_base = POLY_BASE;
    int plain_modulus = PLAIN_MODULUS;
    int coeff_modulus = COEFF_MODULUS;
    int dbc = DBC;
    int resized_width = 0;
    int resized_height = 0;
    bool verbose = false;
    int width = 8;
    int height = 8;

     try {
        cxxopts::Options options(argv[0], "Options for Client-Side FHE");
        options.positional_help("[optional args]").show_positional_help();

        options.add_options()
            ("r,recieve", "Is the client currently decrypting results", cxxopts::value<bool>(recieving))
            ("s,send", "Is the client currently encrypting raw image", cxxopts::value<bool>(sending))
            ("v,verbose", "Verbose logging output", cxxopts::value<bool>(verbose))
            ("f,file", "Filename for input file to be resized", cxxopts::value<std::string>())
            ("c,coutfile", "Filename for ciphertext to be saved to", cxxopts::value<std::string>())
            ("i,cinfile", "Filename for ciphertext to be received", cxxopts::value<std::string>())
            ("o,outfile", "Filename for result image", cxxopts::value<std::string>())
            ("ncoeff", "Number of coefficients for integer portion of encoding", cxxopts::value<int>())
            ("fcoeff", "Number of coefficients for fractional portion of encoding", cxxopts::value<int>())
            ("cmod", "Coefficient Modulus for polynomial encoding", cxxopts::value<int>())
            ("pmod", "Plaintext modulus", cxxopts::value<int>())
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
        if (result.count("coutfile")) ctext_outfile = result["coutfile"].as<std::string>();
        if (result.count("cinfile")) ctext_infile = result["cinfile"].as<std::string>();
        if (result.count("outfile")) test_output = result["outfile"].as<std::string>();
        if (result.count("ncoeff")) n_number_coeffs = result["ncoeff"].as<int>(); 
        if (result.count("fcoeff")) n_fractional_coeffs = result["fcoeff"].as<int>(); 
        if (result.count("pmod")) plain_modulus = result["pmod"].as<int>(); 
        if (result.count("cmod")) coeff_modulus = result["cmod"].as<int>(); 
        if (result.count("n_poly_base")) n_poly_base = result["base"].as<int>(); 
    } 
    catch (const cxxopts::OptionException& e) {
        std::cout << "error parsing options: " << e.what() << std::endl;
        exit(1);
    }

    if (sending) {

        const int requested_composition = 3;
        int width = 0, height = 0, actual_composition = 0;
        uint8_t *image_data = stbi_load(test_filename.c_str(), &width, &height, &actual_composition, requested_composition);
        // Encryption Parameters
        EncryptionParameters params;
        char poly_mod[16];
        snprintf(poly_mod, 16, "1x^%i + 1", coeff_modulus);
        params.set_poly_modulus(poly_mod);
        params.set_coeff_modulus(coeff_modulus_128(coeff_modulus));
        params.set_plain_modulus(plain_modulus);
        SEALContext context(params);
        // print_parameters(context);


        std::ofstream paramfile; 
        paramfile.open("./keys/params.txt");  
        paramfile << width << " ";
        paramfile << height << " ";      

        // Generate keys
        // and save them to file
        std::ofstream pkfile, skfile;
        pkfile.open("./keys/pubkey.txt");
        skfile.open("./keys/seckey.txt");
        KeyGenerator keygen(context);
        auto public_key = keygen.public_key();
        auto secret_key = keygen.secret_key();
        public_key.save(pkfile);
        secret_key.save(skfile);
        pkfile.close(); skfile.close();


        // Encrytor and decryptor setup
        Encryptor encryptor(context, public_key);
        Evaluator evaluator(context);
        Decryptor decryptor(context, secret_key);

        // Base + Number of coefficients used for encoding past the decimal point (both pos and neg)
        // Example: if poly_base = 11, and N_FRACTIONAL_COEFFS=3, then we will have 
        // a1 * 11^-1 + a2 * 11^-2 + a3 * 11^-3
        FractionalEncoder encoder(context.plain_modulus(), context.poly_modulus(), n_number_coeffs, n_fractional_coeffs, n_poly_base);

        std::ofstream myfile;
        myfile.open(ctext_outfile.c_str());
        Ciphertext c;
        int curr, count, index, pairs;
        for (int i = 0; i < 3; i++) {
            curr = image_data[i];
            count = 1;
            pairs = 1;
            for (int j = 1; j < width * height; j++) {
                index = 3 * j + i;
                if (image_data[index] == curr) {
                    count++;
                } else {
                    encryptor.encrypt(encoder.encode(curr), c);
                    c.save(myfile);
                    encryptor.encrypt(encoder.encode(count), c);
                    c.save(myfile);
                    curr = image_data[index];
                    count = 1;
                    pairs++;
                }
            }
            encryptor.encrypt(encoder.encode(curr), c);
            c.save(myfile);
            encryptor.encrypt(encoder.encode(count), c);
            c.save(myfile);
            paramfile << pairs << " ";
        }
        paramfile << std::endl;
       
        paramfile.close();
        myfile.close();

    }

    /******************************************************************************
     * Recieving,      SERVER --> CLIENT                                          *
     ******************************************************************************/
    else
    {
        // Encryption Parameters
        EncryptionParameters params;

        char poly_mod[16];
        snprintf(poly_mod, 16, "1x^%i + 1", coeff_modulus);
        params.set_poly_modulus(poly_mod);
        params.set_coeff_modulus(coeff_modulus_128(coeff_modulus));
        params.set_plain_modulus(plain_modulus);
        SEALContext context(params);
        // print_parameters(context);

        int width, height;
        std::ifstream paramfile; 
        paramfile.open("./keys/params.txt");
        paramfile >> width;
        paramfile >> height;
        paramfile.close();

        // Get keys
        std::ifstream pkfile, skfile;
        pkfile.open("./keys/pubkey.txt");
        skfile.open("./keys/seckey.txt");
        PublicKey public_key;
        SecretKey secret_key;
        public_key.load(pkfile);
        secret_key.load(skfile);
        pkfile.close(); skfile.close();    

        Encryptor encryptor(context, public_key);
        Evaluator evaluator(context);
        Decryptor decryptor(context, secret_key);

        FractionalEncoder encoder(context.plain_modulus(), context.poly_modulus(), n_number_coeffs, n_fractional_coeffs, n_poly_base);

        std::ifstream instream;
        instream.open(ctext_infile.c_str());
        std::vector<uint8_t> decrypted_image;
        Plaintext p;
        Ciphertext c;
        for (int i = 0; i < width * height * 3; i++) {
            c.load(instream);
            decryptor.decrypt(c, p);
            int pixel = encoder.decode(p);
            CLAMP(pixel, 0, 255)
            decrypted_image.push_back((uint8_t) pixel);
        }
        instream.close();
        
        #ifdef linux
            save_image_rgb(width, height, decrypted_image, test_output);
        #else
            show_image_rgb(width, height, decrypted_image);
        #endif
    }
    return 0;
}
