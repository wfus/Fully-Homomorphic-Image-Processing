#include "seal/seal.h"
#include "fhe_image.h"
#include "jpge.h"
#include "stb_image.c"
#include "cxxopts.h"

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
void save_three_blocks_interleaved_ycc(std::ofstream &file,
                                       std::vector<Ciphertext> &y,
                                       std::vector<Ciphertext> &cb,
                                       std::vector<Ciphertext> &cr);



int main(int argc, const char** argv) {

    // Read encryption parameters from file
    int WIDTH = 0, HEIGHT = 0, channels = 0;
    std::ifstream paramfile;
    paramfile.open("./keys/params.txt");
    paramfile >> WIDTH;
    paramfile >> HEIGHT;
    paramfile >> channels;
    // std::cout << original_width << " " << original_height << std::endl;
    assert(channels == 3);
    paramfile.close();
    
    std::string ctext_infile("./image/nothingpersonnel.txt");
    std::string ctext_outfile("./image/zoop.txt");
    bool verbose = false;
    int n_number_coeffs = N_NUMBER_COEFFS;
    int n_fractional_coeffs = N_FRACTIONAL_COEFFS;
    int n_poly_base = POLY_BASE;
    int plain_modulus = PLAIN_MODULUS;
    int coeff_modulus = COEFF_MODULUS;


    try {
        cxxopts::Options options(argv[0], "Options for Client-Side FHE");
        options.positional_help("[optional args]").show_positional_help();

        options.add_options()
            ("f,file", "Filename for input file to be resized", cxxopts::value<std::string>())
            ("v,verbose", "Verbose logging output", cxxopts::value<bool>(verbose))
            ("o,outfile", "Filename for homomorphic ciphertext to be saved to", cxxopts::value<std::string>())
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

        if (result.count("file")) ctext_infile = result["file"].as<std::string>();
        if (result.count("outfile")) ctext_outfile = result["outfile"].as<std::string>();
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



    // Encryption Parameters
    EncryptionParameters params;
    char poly_mod[16];
    snprintf(poly_mod, 16, "1x^%i + 1", coeff_modulus);
    params.set_poly_modulus(poly_mod);
    params.set_coeff_modulus(coeff_modulus_128(coeff_modulus));
    params.set_plain_modulus(plain_modulus);
    SEALContext context(params);
    // print_parameters(context);


    // Generate keys
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
  
    // Encrytor and decryptor setup
    Encryptor encryptor(context, public_key);
    Evaluator evaluator(context);

    // FOR DEBUGGING ONLY!
    Decryptor decryptor(context, secret_key);

    FractionalEncoder encoder(context.plain_modulus(), context.poly_modulus(), n_number_coeffs, n_fractional_coeffs, n_poly_base);

    int num_blocks = WIDTH*HEIGHT/64;
    std::ofstream outfile;
    outfile.open(ctext_outfile.c_str()); 
    std::ifstream myfile;
    myfile.open(ctext_infile.c_str());
    Ciphertext c;

    // Load three blocks at a time for memory reasons.
    // Do the DCT on these three blocks, and then save the three blocks to file
    start = std::chrono::steady_clock::now(); 
    int count = 0;
    for (int curr_block = 0; curr_block < num_blocks; curr_block++) {
        // Load in only three blocks, important to only do three to not get OOM
        std::vector<Ciphertext> red, green, blue;  
        for (int i = 0; i < BLOCK_SIZE*BLOCK_SIZE; i++) {
            c.load(myfile); red.push_back(c); 
        } 
        for (int i = 0; i < BLOCK_SIZE*BLOCK_SIZE; i++) {
            c.load(myfile); green.push_back(c); 
        } 
        for (int i = 0; i < BLOCK_SIZE*BLOCK_SIZE; i++) {
            c.load(myfile); blue.push_back(c); 
        } 
       
        // CONVERT RGB INTO YCC in place
        std::cout << "RGBYCC,";
        for (int i = 0; i < red.size(); i++) {
            rgb_to_ycc_fhe(red[i], green[i], blue[i], evaluator, encoder, encryptor);
        }
        std::cout << std::endl;
        std::cout << "DCT,";
        encrypted_dct(red, evaluator, encoder, encryptor);
        encrypted_dct(green, evaluator, encoder, encryptor);
        encrypted_dct(blue, evaluator, encoder, encryptor);
        save_three_blocks_interleaved_ycc(outfile, red, green, blue);
        std::cout << std::endl;
        // std::cout << "Finished " << curr_block << " block triples..." << std::endl;
    }
    // std::cout << "DCT and File IO: ";
    // diff = std::chrono::steady_clock::now() - start; 
    // std::cout << chrono::duration<double, milli>(diff).count() << " ms" << std::endl;
    
    myfile.close();
    outfile.close();  
    return 0;

     // for (int i = 0; i < red.size() && i < 64; i++) {
    //     Plaintext p1, p2, p3;
    //     decryptor.decrypt(red[i], p1);
    //     decryptor.decrypt(green[i], p2);
    //     decryptor.decrypt(blue[i], p3);
    //     std::cout << "[" << encoder.decode(p1) << " " << encoder.decode(p2) << " " << encoder.decode(p3) << "] ";
    //     if ((i+1) % WIDTH == 0) std::cout << std::endl;
    // }
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
// This should take in three blocks, one Y block, one Cb block, and one Cr block
void save_three_blocks_interleaved_ycc(std::ofstream &myfile,
                                       std::vector<Ciphertext> &y,
                                       std::vector<Ciphertext> &cb,
                                       std::vector<Ciphertext> &cr) {
    for (int i = 0; i < y.size(); i++)  { y[i].save(myfile);  }
    for (int i = 0; i < cb.size(); i++) { cb[i].save(myfile); }
    for (int i = 0; i < cr.size(); i++) { cr[i].save(myfile); }
}



// We want to store the output as blocks of Y Cb Cr interleaved by block
// This is why Nico is moving out
void save_computed_blocks_interleaved_ycc(std::vector<std::vector<Ciphertext>> &y,
                                          std::vector<std::vector<Ciphertext>> &cb,
                                          std::vector<std::vector<Ciphertext>> &cr) {
    std::ofstream myfile;
    myfile.open("./image/zoop.txt");
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
