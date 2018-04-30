#include "seal/seal.h"
#include "fhe_image.h"
#include "fhe_resize.h"
#include "jpge.h"
#include "cxxopts.h"

using namespace seal;

auto start = std::chrono::steady_clock::now();
auto diff = std::chrono::steady_clock::now() - start;


int main(int argc, const char** argv) {
    
    // Read encryption parameters from file
    int original_width = 0, original_height = 0, channels = 0;
    std::ifstream paramfile;
    paramfile.open("./keys/params.txt");
    paramfile >> original_width;
    paramfile >> original_height;
    paramfile >> channels;
    assert(channels == 3); assert(original_width != 0); assert(original_height != 0);
    paramfile.close();
    
    std::string ctext_infile("./image/nothingpersonnel.txt");
    std::string ctext_outfile("./image/zoop.txt");
    bool bicubic = false;
    bool verbose = false;
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
            ("f,file", "Filename for input file to be resized", cxxopts::value<std::string>())
            ("bicubic", "Use bicubic interpolation instead of linear", cxxopts::value<bool>(bicubic))
            ("v,verbose", "Verbose logging output", cxxopts::value<bool>(verbose))
            ("o,outfile", "Filename for homomorphic ciphertext to be saved to", cxxopts::value<std::string>())
            ("ncoeff", "Number of coefficients for integer portion of encoding", cxxopts::value<int>())
            ("fcoeff", "Number of coefficients for fractional portion of encoding", cxxopts::value<int>())
            ("cmod", "Coefficient Modulus for polynomial encoding", cxxopts::value<int>())
            ("pmod", "Plaintext modulus", cxxopts::value<int>())
            ("w,width", "width of resized image", cxxopts::value<int>())
            ("h,height", "height of resized image", cxxopts::value<int>())
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
    PublicKey public_key;
    SecretKey secret_key;
    public_key.load(pkfile);
    secret_key.load(skfile);
    pkfile.close(); skfile.close();


    // Encrytor and decryptor setup
    Encryptor encryptor(context, public_key);
    Evaluator evaluator(context);

    // FOR DEBUGGING ONLY!
    Decryptor decryptor(context, secret_key);

    FractionalEncoder encoder(context.plain_modulus(), context.poly_modulus(), n_number_coeffs, n_fractional_coeffs, n_poly_base);

    std::ofstream outfile;
    outfile.open(ctext_outfile.c_str()); 
    
    String op = bicubic ? "Cubic," : "Linear,";
    std::cout << op;
    int INTER_TYPE = bicubic ? BICUBIC : BILINEAR;
    SImageData resize_im;
    ResizeImage(
        ctext_infile,
        original_width,
        original_height,
        resize_im,
        resized_width,
        resized_height,
        INTER_TYPE,
        evaluator,
        encoder,
        encryptor
    );
    std::cout << std::endl;

    for (int i = 0; i < resize_im.pixels.size(); i++) {
        for (int j = 0; j < 3; j++) {
            Ciphertext c = resize_im.pixels[i][j];
            c.save(outfile);
        }
    }
    outfile.close();
    return 0;
}