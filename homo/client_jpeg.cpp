#include "seal/seal.h"
#include "fhe_image.h"
#include "cxxopts.h"

using namespace seal;

auto start = std::chrono::steady_clock::now();
auto diff = std::chrono::steady_clock::now() - start; 
const bool VERBOSE = true;


int main(int argc, const char** argv) {
    bool recieving = false;
    bool sending = false;
    std::string test_filename("./image/test.jpg");
    std::string ctext_outfile("./image/nothingpersonnel.txt");
    std::string ctext_infile("./image/zoop.txt");
    std::string test_output("./image/test_out.jpg");
    int n_number_coeffs = N_NUMBER_COEFFS;
    int n_fractional_coeffs = N_FRACTIONAL_COEFFS;
    int n_poly_base = POLY_BASE;
    int plain_modulus = PLAIN_MODULUS;
    int coeff_modulus = COEFF_MODULUS;
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
        paramfile << actual_composition << " ";
        paramfile << plain_modulus << std::endl;
        paramfile.close();

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
        myfile.open(ctext_outfile.c_str());
        start = std::chrono::steady_clock::now(); 
        Ciphertext c;
        double conv;
        std::cout << "Encryption,";
        for (int i = 0; i < red_blocks.size(); i++) {
            for (int j = 0; j < red_blocks[i].size(); j++) {
                conv = red_blocks[i][j];
                start = std::chrono::steady_clock::now();
                encryptor.encrypt(encoder.encode(conv), c);
                diff = std::chrono::steady_clock::now() - start; 
                std::cout << chrono::duration<double, milli>(diff).count() << ',';
                c.save(myfile);
            }
            for (int j = 0; j < green_blocks[i].size(); j++) {
                conv = green_blocks[i][j];
                start = std::chrono::steady_clock::now();
                encryptor.encrypt(encoder.encode(conv), c);
                diff = std::chrono::steady_clock::now() - start; 
                std::cout << chrono::duration<double, milli>(diff).count() << ',';
                c.save(myfile);
            }
            for (int j = 0; j < blue_blocks[i].size(); j++) {
                conv = blue_blocks[i][j];
                start = std::chrono::steady_clock::now();
                encryptor.encrypt(encoder.encode(conv), c);
                diff = std::chrono::steady_clock::now() - start; 
                std::cout << chrono::duration<double, milli>(diff).count() << ',';
                c.save(myfile);
            }
        }
        std::cout << std::endl;
        myfile.close();
    }
    else
    {
        // We are recieving the results and then doing the actual compression
        // Note that it is very difficult to do compression with purely FHE, 
        // it is possible to do decompression though.
        const char* infile = ctext_infile.c_str();
        const char* outfile = test_output.c_str();

        

        // Read encryption parameters from file
        int WIDTH = 0, HEIGHT = 0;
        std::ifstream paramfile;
        paramfile.open("./keys/params.txt");
        paramfile >> WIDTH;
        paramfile >> HEIGHT;
        paramfile.close();


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
        PublicKey public_key;
        SecretKey secret_key;
        public_key.load(pkfile);
        secret_key.load(skfile);
        pkfile.close(); skfile.close();    

        Encryptor encryptor(context, public_key);
        Evaluator evaluator(context);
        Decryptor decryptor(context, secret_key);

        // Base + Number of coefficients used for encoding past the decimal point (both pos and neg)
        // Example: if poly_base = 11, and N_FRACTIONAL_COEFFS=3, then we will have 
        // a1 * 11^-1 + a2 * 11^-2 + a3 * 11^-3
        FractionalEncoder encoder(context.plain_modulus(), context.poly_modulus(), n_number_coeffs, n_fractional_coeffs, n_poly_base);

        int block_pix = BLOCK_SIZE * BLOCK_SIZE;
        int num_blocks = (WIDTH / BLOCK_SIZE) * (HEIGHT  / BLOCK_SIZE);

        unsigned char YTable[64], UVTable[64];
        for(int i = 0; i < 64; ++i) {
            int yti = (YQT[i]*QUALITY+50)/100;
            YTable[s_ZigZag[i]] = yti < 1 ? 1 : yti > 255 ? 255 : yti;
            int uvti  = (UVQT[i]*QUALITY+50)/100;
            UVTable[s_ZigZag[i]] = uvti < 1 ? 1 : uvti > 255 ? 255 : uvti;
        }

        float fdtbl_Y[64], fdtbl_UV[64];
        for(int row = 0, k = 0; row < 8; ++row) {
            for(int col = 0; col < 8; ++col, ++k) {
                fdtbl_Y[k]  = 1 / (YTable [s_ZigZag[k]] * aasf[row] * aasf[col]);
                fdtbl_UV[k] = 1 / (UVTable[s_ZigZag[k]] * aasf[row] * aasf[col]);
            }
        }

        FILE *fp = fopen(outfile, "wb");

        // Write Headers
        fwrite(head0, sizeof(head0), 1, fp);
        fwrite(YTable, sizeof(YTable), 1, fp);
        putc(1, fp);
        fwrite(UVTable, sizeof(UVTable), 1, fp);
        const unsigned char head1[] = { 0xFF,0xC0,0,0x11,8,HEIGHT>>8,HEIGHT&0xFF,WIDTH>>8,WIDTH&0xFF,3,1,0x11,0,2,0x11,1,3,0x11,1,0xFF,0xC4,0x01,0xA2,0 };
        fwrite(head1, sizeof(head1), 1, fp);
        fwrite(std_dc_luminance_nrcodes+1, sizeof(std_dc_luminance_nrcodes)-1, 1, fp);
        fwrite(std_dc_luminance_values, sizeof(std_dc_luminance_values), 1, fp);
        putc(0x10, fp); // HTYACinfo
        fwrite(std_ac_luminance_nrcodes+1, sizeof(std_ac_luminance_nrcodes)-1, 1, fp);
        fwrite(std_ac_luminance_values, sizeof(std_ac_luminance_values), 1, fp);
        putc(1, fp); // HTUDCinfo
        fwrite(std_dc_chrominance_nrcodes+1, sizeof(std_dc_chrominance_nrcodes)-1, 1, fp);
        fwrite(std_dc_chrominance_values, sizeof(std_dc_chrominance_values), 1, fp);
        putc(0x11, fp); // HTUACinfo
        fwrite(std_ac_chrominance_nrcodes+1, sizeof(std_ac_chrominance_nrcodes)-1, 1, fp);
        fwrite(std_ac_chrominance_values, sizeof(std_ac_chrominance_values), 1, fp);
        static const unsigned char head2[] = { 0xFF,0xDA,0,0xC,3,1,0,2,0x11,3,0x11,0,0x3F,0 };
        fwrite(head2, sizeof(head2), 1, fp);


        std::ifstream myfile;
        myfile.open(infile);
        int bitBuf=0, bitCnt=0, DCY=0, DCU=0, DCV=0;
        double v = 0;
        std::cout << "Decryption,";
        for (int i = 0; i < num_blocks; i++) {
            int block_zz[3][block_pix];
            for (int k = 0; k < 3; k++) {
                for (int j = 0; j < block_pix; j++) {
                    Ciphertext c;
                    c.load(myfile);
                    Plaintext p;
                    start = std::chrono::steady_clock::now();
                    decryptor.decrypt(c, p);
                    diff = std::chrono::steady_clock::now() - start; 
                    std::cout << chrono::duration<double, milli>(diff).count() << ',';
                    v = encoder.decode(p);
                    block_zz[k][s_ZigZag[j]] = (int)(v < 0 ? ceilf(v - 0.5f) : floorf(v + 0.5f));
                }
            }
            DCY = processBlock(fp, bitBuf, bitCnt, block_zz[0], fdtbl_Y, DCY, YDC_HT, YAC_HT);
            DCU = processBlock(fp, bitBuf, bitCnt, block_zz[1], fdtbl_UV, DCU, UVDC_HT, UVAC_HT);
            DCV = processBlock(fp, bitBuf, bitCnt, block_zz[2], fdtbl_UV, DCV, UVDC_HT, UVAC_HT);
        }
        std::cout << std::endl;
        // Write ending sequence
        static const unsigned short fillBits[] = {0x7F, 7};
	    writeBits(fp, bitBuf, bitCnt, fillBits);
        putc(0xFF, fp);
        putc(0xD9, fp);
        myfile.close();
        fclose(fp);
        
        // compare FHE jpeg to jo_jpeg
        compare_jpeg_jojpeg(test_filename.c_str(), test_output.c_str(), "./image/jo_image.jpg");
    }

    return 0;
}
