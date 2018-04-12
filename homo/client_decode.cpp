#include "seal/seal.h"
#include "fhe_decode.h"
#include "fhe_image.h"
#include "cxxopts.h"
#include "stb_image.c"

using namespace seal;

auto start = std::chrono::steady_clock::now();
auto diff = std::chrono::steady_clock::now() - start; 
const bool VERBOSE = true;

static int decode_jpeg_image_fhe(jpeg *j);
static int process_marker_fhe(jpeg *z, int m);

/*
typedef struct
{
   #ifdef STBI_SIMD
   unsigned short dequant2[4][64];
   #endif
   stbi *s;
   huffman huff_dc[4];
   huffman huff_ac[4];
   uint8 dequant[4][64];

// sizes for components, interleaved MCUs
   int img_h_max, img_v_max;
   int img_mcu_x, img_mcu_y;
   int img_mcu_w, img_mcu_h;

// definition of jpeg image component
   struct
   {
      int id;
      int h,v;
      int tq;
      int hd,ha;
      int dc_pred;

      int x,y,w2,h2;
      uint8 *data;
      void *raw_data;
      uint8 *linebuf;
   } img_comp[4];

   uint32         code_buffer; // jpeg entropy-coded buffer
   int            code_bits;   // number of valid bits
   unsigned char  marker;      // marker seen while filling entropy buffer
   int            nomore;      // flag if we saw a marker so must stop

   int scan_n, order[4];
   int restart_interval, todo;
} jpeg;
*/

int main(int argc, const char** argv) {
    bool recieving = false;
    bool sending = false;
    std::string test_filename("../image/boazbarak.jpg");
    std::string input_filename("../image/zoop.txt");
    int n_number_coeffs = N_NUMBER_COEFFS;
    int n_fractional_coeffs = N_FRACTIONAL_COEFFS;
    int n_poly_base = POLY_BASE;

    try {
        cxxopts::Options options(argv[0], "Options for Client-Side FHE");
        options.positional_help("[optional args]").show_positional_help();

        options.add_options()
            ("r,recieve", "Is the client currently decrypting results", cxxopts::value<bool>(recieving))
            ("s,send", "Is the client currently encrypting raw image", cxxopts::value<bool>(sending))
            ("f,file", "Filename for input file to be decompressed", cxxopts::value<std::string>())
            ("c,cfile", "Filename for ciphertext result file to be checked for correctness", cxxopts::value<std::string>())
            ("ncoeff", "Number of coefficients for integer portion of encoding", cxxopts::value<int>())
            ("fcoeff", "Number of coefficients for fractional portion of encoding", cxxopts::value<int>())
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
        if (result.count("cfile")) input_filename = result["cfile"].as<std::string>();
        if (result.count("ncoeff")) n_number_coeffs = result["ncoeff"].as<int>(); 
        if (result.count("fcoeff")) n_fractional_coeffs = result["fcoeff"].as<int>(); 
        if (result.count("n_poly_base")) n_poly_base = result["base"].as<int>(); 
    } 
    catch (const cxxopts::OptionException& e) {
        std::cout << "error parsing options: " << e.what() << std::endl;
        exit(1);
    }

    if (sending) {
        
        int requested_composition = 3; // requested number of channels
        int actual_composition = 0, width = 0, height = 0;


        // Load in JPEG image we will be working with
        FILE *f = fopen(test_filename.c_str(), "rb");
        unsigned char *result;
        if (!f) {
            std::cout << "Unable to open file... Exiting..." << std::endl;
            exit(1);
        } 
        jpeg j; stbi s; j.s = &s;
        start_file(&s,f);
        load_jpeg_image(&j, &width, &height, &actual_composition, requested_composition);
        fclose(f);



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
        FractionalEncoder encoder(
            context.plain_modulus(), 
            context.poly_modulus(), 
            n_number_coeffs, 
            n_fractional_coeffs, 
            n_poly_base);

    }
    else
    {
        const char* infile = input_filename.c_str();

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
        FractionalEncoder encoder(context.plain_modulus(), 
            context.poly_modulus(), 
            n_number_coeffs, 
            n_fractional_coeffs, 
            n_poly_base);



    }
    return 0;
}


static int decode_jpeg_image_fhe(jpeg *j)
{
   int m;
   j->restart_interval = 0;
   if (!decode_jpeg_header(j, SCAN_load)) return 0;
   m = get_marker(j);
   while (!EOI(m)) {
      if (SOS(m)) {
         if (!process_scan_header(j)) return 0;
         if (!parse_entropy_coded_data(j)) return 0;
         if (j->marker == MARKER_none ) {
            // handle 0s at the end of image data from IP Kamera 9060
            while (!at_eof(j->s)) {
               int x = get8(j->s);
               if (x == 255) {
                  j->marker = get8u(j->s);
                  break;
               } else if (x != 0) {
                  return 0;
               }
            }
            // if we reach eof without hitting a marker, get_marker() below will fail and we'll eventually return 0
         }
      } else {
         if (!process_marker(j, m)) return 0;
      }
      m = get_marker(j);
   }
   return 1;
}


static int process_marker_fhe(jpeg *z, int m)
{
   int L;
   switch (m) {
      case MARKER_none: // no marker found
         std::cout << "expected marker - Corrupt JPEG" << std::endl;
         exit(1);

      case 0xC2: // SOF - progressive
         std::cout << "progressive jpeg - JPEG format not supported (progressive)" << std::endl;
         exit(1);

      case 0xDD: // DRI - specify restart interval
         if (get16(z->s) != 4)  {
             std::cout << "bad DRI len - Corrupt JPEG" << std::endl;
             exit(1);
         }
         z->restart_interval = get16(z->s);
         return 1;

      case 0xDB: // DQT - define quantization table
         L = get16(z->s)-2;
         while (L > 0) {
            int q = get8(z->s); // guessing that this gets the next uint8_t
            int p = q >> 4;     // checking that q is not larger than 16 
            int t = q & 15,i;   // guessing this is number of channels
            if (p != 0) std::cout << "bad DQT type - Corrupt JPEG" << std::endl;
            if (t > 3) std::cout << "bad DQT table - Corrupt JPEG" << std::endl;
            for (i=0; i < 64; ++i)
               z->dequant[t][dezigzag[i]] = get8u(z->s);
            #ifdef STBI_SIMD
            for (i=0; i < 64; ++i)
               z->dequant2[t][i] = z->dequant[t][i];
            #endif
            L -= 65;
         }
         return L==0;

      case 0xC4: // DHT - define huffman table
         L = get16(z->s)-2;
         while (L > 0) {
            uint8 *v;
            int sizes[16],i,m=0;
            int q = get8(z->s);
            int tc = q >> 4;
            int th = q & 15;
            if (tc > 1 || th > 3) {
                std::cout << "bad DHT header - Corrupt JPEG" << std::endl;
                exit(1);
            }
            for (i=0; i < 16; ++i) {
               sizes[i] = get8(z->s);
               m += sizes[i];
            }
            L -= 17;
            if (tc == 0) {
               if (!build_huffman(z->huff_dc+th, sizes)) return 0;
               v = z->huff_dc[th].values;
            } else {
               if (!build_huffman(z->huff_ac+th, sizes)) return 0;
               v = z->huff_ac[th].values;
            }
            for (i=0; i < m; ++i)
               v[i] = get8u(z->s);
            L -= m;
         }
         return L==0;
   }
   // check for comment block or APP blocks
   if ((m >= 0xE0 && m <= 0xEF) || m == 0xFE) {
      skip(z->s, get16(z->s)-2);
      return 1;
   }
   return 0;
}
