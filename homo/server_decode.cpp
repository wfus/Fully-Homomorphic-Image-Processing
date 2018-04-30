#include "seal/seal.h"
#include "fhe_image.h"
#include "fhe_decode.h"
#include "cxxopts.h"

using namespace seal;

auto start = std::chrono::steady_clock::now();
auto diff = std::chrono::steady_clock::now() - start;


int main(int argc, const char** argv) {
    
    // Read encryption parameters from file
    int pairs[3];
    int width, height;
    std::ifstream paramfile;
    paramfile.open("./keys/params.txt");
    paramfile >> width;
    paramfile >> height;
    for (int i = 0; i < 3; i++) {
        paramfile >> pairs[i];
    }
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
    int degree = 12;
    double delta = 0.5;
    int order = 64;


    try {
        cxxopts::Options options(argv[0], "Options for Client-Side FHE");
        options.positional_help("[optional args]").show_positional_help();

        options.add_options()
            ("f,file", "Filename for input file to be resized", cxxopts::value<std::string>())
            ("v,verbose", "Verbose logging output", cxxopts::value<bool>(verbose))
            ("o,outfile", "Filename for homomorphic ciphertext to be saved to", cxxopts::value<std::string>())
            ("delta", "Delta for step function size", cxxopts::value<double>())
            ("order", "Order of approximation for step function approximation Fourier series", cxxopts::value<int>())
            ("degree", "Terms for step function approximation", cxxopts::value<int>())
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
        if (result.count("degree")) degree = result["degree"].as<int>();  
        if (result.count("order")) order = result["order"].as<int>(); 
        if (result.count("delta")) delta = result["delta"].as<double>(); 
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
    std::ifstream myfile;
    myfile.open(ctext_infile.c_str()); 
    Ciphertext c, index, elem, count;
    std::vector<std::vector<Ciphertext>> res;
    for (int i = 0; i < 3; i++) {
        encryptor.encrypt(encoder.encode(0), index);
        std::vector<Ciphertext> channel;
        for (int j = 0; j < width * height; j++) {
            encryptor.encrypt(encoder.encode(0), c);
            channel.push_back(c);
        }
        for (int j = 0; j < pairs[i]; j++) {
            std::vector<Ciphertext> run;
            elem.load(myfile);
            count.load(myfile);
            approximated_step(elem, index, count, order, degree, delta, width, height, run, evaluator, encoder, encryptor, decryptor);
            for (int k = 0; k < width * height; k++) {
                evaluator.add(channel[k], run[k]);
            }
            evaluator.add(index, count);
        }
        res.push_back(channel);
    }
    for (int i = 0; i < width * height; i++) {
        for (int j = 0; j < 3; j++) {
            res[j][i].save(outfile);
        }
    }
    myfile.close();
    outfile.close();
    return 0;
}