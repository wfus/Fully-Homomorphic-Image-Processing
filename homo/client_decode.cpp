#include "seal/seal.h"
#include "fhe_decode.h"
#include "fhe_image.h"
#include "cxxopts.h"

using namespace seal;

auto start = std::chrono::steady_clock::now();
auto diff = std::chrono::steady_clock::now() - start; 
const bool VERBOSE = true;

static uint8 *load_jpeg_image_fhe(jpeg *z, int *out_x, int *out_y, int *comp, int req_comp);
static int decode_jpeg_image_fhe(jpeg *j);
static int process_marker_fhe(jpeg *z, int m);
static int parse_entropy_coded_data_fhe(jpeg *z);
static int decode_block_fhe(jpeg *j, short data[64], huffman *hdc, huffman *hac, int b);
static int decode_fhe(jpeg *j, huffman *h);

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
        std::cout << "Loading in the JPEG..." << std::endl;
        load_jpeg_image_fhe(&j, &width, &height, &actual_composition, requested_composition);
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
         std::cout << "Called SOS on JPEG decoding..." <<std::endl;
         if (!process_scan_header(j)) return 0;
         if (!parse_entropy_coded_data_fhe(j)) return 0;
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
         if (!process_marker_fhe(j, m)) return 0;
      }
      m = get_marker(j);
   }
   return 1;
}

static uint8 *load_jpeg_image_fhe(jpeg *z, int *out_x, int *out_y, int *comp, int req_comp)
{
   int n, decode_n;
   // validate req_comp
   if (req_comp < 0 || req_comp > 4) {
       std::cout << "bad req_comp - Internal error" << std::endl;
       exit(1);
   }
   z->s->img_n = 0;

   // load a jpeg image from whichever source
   if (!decode_jpeg_image_fhe(z)) { 
       cleanup_jpeg(z); 
       std::cout << "Problem Decoding JPEG" << std::endl;
       exit(1);
   }

   // determine actual number of components to generate
   n = req_comp ? req_comp : z->s->img_n;

   if (z->s->img_n == 3 && n < 3)
      decode_n = 1;
   else
      decode_n = z->s->img_n;

   // resample and color-convert
   {
      int k;
      uint i,j;
      uint8 *output;
      uint8 *coutput[4];

      stbi_resample res_comp[4];

      for (k=0; k < decode_n; ++k) {
         stbi_resample *r = &res_comp[k];

         // allocate line buffer big enough for upsampling off the edges
         // with upsample factor of 4
         z->img_comp[k].linebuf = (uint8 *) malloc(z->s->img_x + 3);
         if (!z->img_comp[k].linebuf) { cleanup_jpeg(z); return epuc("outofmem", "Out of memory"); }

         r->hs      = z->img_h_max / z->img_comp[k].h;
         r->vs      = z->img_v_max / z->img_comp[k].v;
         r->ystep   = r->vs >> 1;
         r->w_lores = (z->s->img_x + r->hs-1) / r->hs;
         r->ypos    = 0;
         r->line0   = r->line1 = z->img_comp[k].data;

         if      (r->hs == 1 && r->vs == 1) r->resample = resample_row_1;
         else if (r->hs == 1 && r->vs == 2) r->resample = resample_row_v_2;
         else if (r->hs == 2 && r->vs == 1) r->resample = resample_row_h_2;
         else if (r->hs == 2 && r->vs == 2) r->resample = resample_row_hv_2;
         else                               r->resample = resample_row_generic;
      }

      // can't error after this so, this is safe
      output = (uint8 *) malloc(n * z->s->img_x * z->s->img_y + 1);
      if (!output) { cleanup_jpeg(z); return epuc("outofmem", "Out of memory"); }

      // now go ahead and resample
      for (j=0; j < z->s->img_y; ++j) {
         uint8 *out = output + n * z->s->img_x * j;
         for (k=0; k < decode_n; ++k) {
            stbi_resample *r = &res_comp[k];
            int y_bot = r->ystep >= (r->vs >> 1);
            coutput[k] = r->resample(z->img_comp[k].linebuf,
                                     y_bot ? r->line1 : r->line0,
                                     y_bot ? r->line0 : r->line1,
                                     r->w_lores, r->hs);
            if (++r->ystep >= r->vs) {
               r->ystep = 0;
               r->line0 = r->line1;
               if (++r->ypos < z->img_comp[k].y)
                  r->line1 += z->img_comp[k].w2;
            }
         }
         if (n >= 3) {
            uint8 *y = coutput[0];
            if (z->s->img_n == 3) {
               #ifdef STBI_SIMD
               stbi_YCbCr_installed(out, y, coutput[1], coutput[2], z->s.img_x, n);
               #else
               YCbCr_to_RGB_row(out, y, coutput[1], coutput[2], z->s->img_x, n);
               #endif
            } else
               for (i=0; i < z->s->img_x; ++i) {
                  out[0] = out[1] = out[2] = y[i];
                  out[3] = 255; // not used if n==3
                  out += n;
               }
         } else {
            uint8 *y = coutput[0];
            if (n == 1)
               for (i=0; i < z->s->img_x; ++i) out[i] = y[i];
            else
               for (i=0; i < z->s->img_x; ++i) *out++ = y[i], *out++ = 255;
         }
      }
      cleanup_jpeg(z);
      *out_x = z->s->img_x;
      *out_y = z->s->img_y;
      if (comp) *comp  = z->s->img_n; // report original components, not output
      return output;
   }
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
            std::cout << "--- BEGINBLOCK ---" << std::endl;
            for (i=0; i < 64; ++i) {
                int next = get8u(z->s);
                std::cout << next << " ";
                if ((i+1) % 8 == 0) std::cout << std::endl;
                z->dequant[t][dezigzag[i]] = next;
            }
            std::cout << "--- ENDBLOCK ---" << std::endl;
            L -= 65;
         }
         std::cout << std::endl << "------------- DEFINE QUANTIZATION TABLE END --------------" << std::endl;
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

static int parse_entropy_coded_data_fhe(jpeg *z)
{
   reset(z);
   if (z->scan_n == 1) {
      std::cout << "Parsing non-interleaved data..." << std::endl;
      int i,j;
      short data[64];
      int n = z->order[0];
      // non-interleaved data, we just need to process one block at a time,
      // in trivial scanline order
      // number of blocks to do just depends on how many actual "pixels" this
      // component has, independent of interleaved MCU blocking and such
      int w = (z->img_comp[n].x+7) >> 3;
      int h = (z->img_comp[n].y+7) >> 3;
      for (j=0; j < h; ++j) {
         for (i=0; i < w; ++i) {
            if (!decode_block_fhe(z, data, z->huff_dc+z->img_comp[n].hd, z->huff_ac+z->img_comp[n].ha, n)) return 0;
            idct_block(z->img_comp[n].data+z->img_comp[n].w2*j*8+i*8, z->img_comp[n].w2, data, z->dequant[z->img_comp[n].tq]);
            // every data block is an MCU, so countdown the restart interval
            if (--z->todo <= 0) {
               if (z->code_bits < 24) grow_buffer_unsafe(z);
               // if it's NOT a restart, then just bail, so we get corrupt data
               // rather than no data
               if (!RESTART(z->marker)) return 1;
               reset(z);
            }
         }
      }
   } else { // interleaved!
      std::cout << "Parsing interleaved data..." << std::endl;
      int i,j,k,x,y;
      short data[64];
      for (j=0; j < z->img_mcu_y; ++j) {
         for (i=0; i < z->img_mcu_x; ++i) {
            // scan an interleaved mcu... process scan_n components in order
            for (k=0; k < z->scan_n; ++k) {
               int n = z->order[k];
               // scan out an mcu's worth of this component; that's just determined
               // by the basic H and V specified for the component
               for (y=0; y < z->img_comp[n].v; ++y) {
                  for (x=0; x < z->img_comp[n].h; ++x) {
                     int x2 = (i*z->img_comp[n].h + x)*8;
                     int y2 = (j*z->img_comp[n].v + y)*8;
                     if (!decode_block_fhe(z, data, z->huff_dc+z->img_comp[n].hd, z->huff_ac+z->img_comp[n].ha, n)) return 0;
                     idct_block(z->img_comp[n].data+z->img_comp[n].w2*y2+x2, z->img_comp[n].w2, data, z->dequant[z->img_comp[n].tq]);
                  }
               }
            }
            // after all interleaved components, that's an interleaved MCU,
            // so now count down the restart interval
            if (--z->todo <= 0) {
               if (z->code_bits < 24) grow_buffer_unsafe(z);
               // if it's NOT a restart, then just bail, so we get corrupt data
               // rather than no data
               if (!RESTART(z->marker)) return 1;
               reset(z);
            }
         }
      }
   }
   return 1;
}


// decode one 64-entry block--
static int decode_block_fhe(jpeg *j, short data[64], huffman *hdc, huffman *hac, int b)
{
   int diff,dc,k;
   int t = decode_fhe(j, hdc);
   if (t < 0) {
       std::cout << "bad huffman code - Corrupt JPEG" << std::endl;
       exit(1);
   }
   // 0 all the ac values now so we can do it 32-bits at a time
   memset(data,0,64*sizeof(data[0]));

   diff = t ? extend_receive(j, t) : 0;
   dc = j->img_comp[b].dc_pred + diff;
   j->img_comp[b].dc_pred = dc;
   data[0] = (short) dc;

   // decode AC components, see JPEG spec
   k = 1;
   do {
      int r,s;
      int rs = decode_fhe(j, hac);
      if (rs < 0) return e("bad huffman code","Corrupt JPEG");
      s = rs & 15;
      r = rs >> 4;
      if (s == 0) {
         if (rs != 0xf0) break; // end block
         k += 16;
      } else {
         k += r;
         std::cout << std::endl;
         // decode into unzigzag'd location
         data[dezigzag[k++]] = (short) extend_receive(j,s);
      }
   } while (k < 64);
   std::cout << std::endl << "---------- END BLOCK ----------" << std::endl;
   return 1;
}

static int decode_fhe(jpeg *j, huffman *h)
{
   unsigned int temp;
   int c,k;
    std::cout << std::hex << j->code_buffer << " " ;
   if (j->code_bits < 16) grow_buffer_unsafe(j);

   // look at the top FAST_BITS and determine what symbol ID it is,
   // if the code is <= FAST_BITS
   c = (j->code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS)-1);
   k = h->fast[c];
   if (k < 255) {
      int s = h->size[k];
      if (s > j->code_bits)
         return -1;
      j->code_buffer <<= s;
      j->code_bits -= s;
      return h->values[k];
   }

   // naive test is to shift the code_buffer down so k bits are
   // valid, then test against maxcode. To speed this up, we've
   // preshifted maxcode left so that it has (16-k) 0s at the
   // end; in other words, regardless of the number of bits, it
   // wants to be compared against something shifted to have 16;
   // that way we don't need to shift inside the loop.
   temp = j->code_buffer >> 16;
   for (k=FAST_BITS+1 ; ; ++k)
      if (temp < h->maxcode[k])
         break;
   if (k == 17) {
      // error! code not found
      j->code_bits -= 16;
      return -1;
   }

   if (k > j->code_bits)
      return -1;

   // convert the huffman code to the symbol id
   c = ((j->code_buffer >> (32 - k)) & bmask[k]) + h->delta[k];
   assert((((j->code_buffer) >> (32 - h->size[c])) & bmask[h->size[c]]) == h->code[c]);

   // convert the id to a symbol
   j->code_bits -= k;
   j->code_buffer <<= k;
   return h->values[c];
}