#include "seal/seal.h"
#include "fhe_image.h"
// #include "fhe_decode.h"
#include "cxxopts.h"

using namespace seal;

auto start = std::chrono::steady_clock::now();
auto diff = std::chrono::steady_clock::now() - start; 
const bool VERBOSE = true;

int main(int argc, const char** argv) {
    bool recieving = false;
    bool sending = false;
    bool bicubic = false;
    std::string test_filename("./image/barak.jpg");
    std::string ctext_outfile("./image/nothingpersonnel.txt");
    std::string ctext_infile("./image/zoop.txt");
    std::string test_output("./image/test_out.txt");
    int n_number_coeffs = N_NUMBER_COEFFS;
    int n_fractional_coeffs = N_FRACTIONAL_COEFFS;
    int n_poly_base = POLY_BASE;
    int plain_modulus = PLAIN_MODULUS;
    int coeff_modulus = COEFF_MODULUS;
    int dbc = DBC;
    int resized_width = 0;
    int resized_height = 0;
    bool verbose = false;

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
        const int requested_composition = 1;
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
        paramfile << actual_composition << " ";
        paramfile << plain_modulus << " ";
        

        // Generate keys
        // and save them to file
        std::ofstream pkfile, skfile, ekfile;
        pkfile.open("./keys/pubkey.txt");
        skfile.open("./keys/seckey.txt");
        ekfile.open("./keys/evalkey.txt");
        // start = std::chrono::steady_clock::now(); 
        KeyGenerator keygen(context);
        auto public_key = keygen.public_key();
        auto secret_key = keygen.secret_key();
        EvaluationKeys ev_key;
        keygen.generate_evaluation_keys(dbc, ev_key);
        public_key.save(pkfile);
        secret_key.save(skfile);
        ev_key.save(ekfile);
        // diff = std::chrono::steady_clock::now() - start; 
        // std::cout << "KeyGen: ";
        // std::cout << chrono::duration<double, milli>(diff).count() << " ms" << std::endl;
        pkfile.close(); skfile.close(); ekfile.close();


        // Encrytor and decryptor setup
        Encryptor encryptor(context, public_key);
        Evaluator evaluator(context);
        Decryptor decryptor(context, secret_key);

        // Base + Number of coefficients used for encoding past the decimal point (both pos and neg)
        // Example: if poly_base = 11, and N_FRACTIONAL_COEFFS=3, then we will have 
        // a1 * 11^-1 + a2 * 11^-2 + a3 * 11^-3
        FractionalEncoder encoder(context.plain_modulus(), context.poly_modulus(), n_number_coeffs, n_fractional_coeffs, n_poly_base);

        std::vector<double> red, green, blue; 
        
        for (int i = 0; i < width * height * actual_composition; i+=3) {
            red.push_back((double) image_data[i]);
            green.push_back((double) image_data[i+1]);
            blue.push_back((double) image_data[i+2]);
        }
        std::vector<std::vector<std::vector<double>>> blocks;
        blocks.push_back(split_image_eight_block(red, width, height));
        blocks.push_back(split_image_eight_block(green, width, height));
        blocks.push_back(split_image_eight_block(blue, width, height));

        std::ofstream myfile;
        myfile.open(ctext_outfile.c_str());
        start = std::chrono::steady_clock::now(); 
        Ciphertext c;
        double curr;
        int count;
        int pairs;
        curr = image_data[0];
        count = 1;
        pairs = 1;
        for (int i = 1; i < width * height; i++) {
            if (image_data[i] == curr) {
                count++;
            } else {
                encryptor.encrypt(encoder.encode(curr), c);
                c.save(myfile);
                encryptor.encrypt(encoder.encode(count), c);
                c.save(myfile);
                curr = image_data[i];
                count = 1;
                pairs++;
            }
        }
        encryptor.encrypt(encoder.encode(curr), c);
        c.save(myfile);
        encryptor.encrypt(encoder.encode(count), c);
        c.save(myfile);
        paramfile << pairs << std::endl;

        
       
        // std::cout << width << " " << height << std::endl;

       
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


        // Get keys
        std::ifstream pkfile, skfile;
        pkfile.open("./keys/pubkey.txt");
        skfile.open("./keys/seckey.txt");
        // start = std::chrono::steady_clock::now(); 
        PublicKey public_key;
        SecretKey secret_key;
        public_key.load(pkfile);
        secret_key.load(skfile);
        // diff = std::chrono::steady_clock::now() - start; 
        // std::cout << "Key Load Time: ";
        // std::cout << chrono::duration<double, milli>(diff).count() << " ms" << std::endl;
        pkfile.close(); skfile.close();    

        Encryptor encryptor(context, public_key);
        Evaluator evaluator(context);
        Decryptor decryptor(context, secret_key);

        FractionalEncoder encoder(context.plain_modulus(), context.poly_modulus(), n_number_coeffs, n_fractional_coeffs, n_poly_base);

        // Read in the image as RGB interleaved...
        // Assuming that there are three channels
        std::ifstream instream;
        // std::cout << "Loading Ciphertexts now..." << std::endl; 
        instream.open(ctext_infile.c_str());
        instream.close();
    }
    return 0;
}
