#include "seal/seal.h"
#include "fhe_image.h"
#include "jpge.h"
#include "stb_image.c"
#include "jo_jpeg.h"


using namespace seal;

auto start = std::chrono::steady_clock::now();
auto diff = std::chrono::steady_clock::now() - start; 
const bool VERBOSE = true;

std::vector<double> read_image(std::string fname);

const unsigned char s_ZigZag[] = { 0,1,5,6,14,15,27,28,2,4,7,13,16,26,29,42,3,8,12,17,25,30,41,43,9,11,18,24,31,40,44,53,10,19,23,32,39,45,52,54,20,22,33,38,46,51,55,60,21,34,37,47,50,56,59,61,35,36,48,49,57,58,62,63 };

const unsigned char std_dc_luminance_nrcodes[] = {0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0};
const unsigned char std_dc_luminance_values[] = {0,1,2,3,4,5,6,7,8,9,10,11};
const unsigned char std_ac_luminance_nrcodes[] = {0,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d};
const unsigned char std_ac_luminance_values[] = {
    0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,
    0x23,0x42,0xb1,0xc1,0x15,0x52,0xd1,0xf0,0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,0x17,0x18,0x19,0x1a,0x25,0x26,0x27,0x28,
    0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,
    0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
    0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,
    0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,
    0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa
};
const unsigned char std_dc_chrominance_nrcodes[] = {0,0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0};
const unsigned char std_dc_chrominance_values[] = {0,1,2,3,4,5,6,7,8,9,10,11};
const unsigned char std_ac_chrominance_nrcodes[] = {0,0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77};
const unsigned char std_ac_chrominance_values[] = {
    0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,
    0xa1,0xb1,0xc1,0x09,0x23,0x33,0x52,0xf0,0x15,0x62,0x72,0xd1,0x0a,0x16,0x24,0x34,0xe1,0x25,0xf1,0x17,0x18,0x19,0x1a,0x26,
    0x27,0x28,0x29,0x2a,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,
    0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x82,0x83,0x84,0x85,0x86,0x87,
    0x88,0x89,0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,
    0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,
    0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa
};
// Huffman tables
const unsigned short YDC_HT[256][2] = { {0,2},{2,3},{3,3},{4,3},{5,3},{6,3},{14,4},{30,5},{62,6},{126,7},{254,8},{510,9}};
const unsigned short UVDC_HT[256][2] = { {0,2},{1,2},{2,2},{6,3},{14,4},{30,5},{62,6},{126,7},{254,8},{510,9},{1022,10},{2046,11}};
const unsigned short YAC_HT[256][2] = { 
    {10,4},{0,2},{1,2},{4,3},{11,4},{26,5},{120,7},{248,8},{1014,10},{65410,16},{65411,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {12,4},{27,5},{121,7},{502,9},{2038,11},{65412,16},{65413,16},{65414,16},{65415,16},{65416,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {28,5},{249,8},{1015,10},{4084,12},{65417,16},{65418,16},{65419,16},{65420,16},{65421,16},{65422,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {58,6},{503,9},{4085,12},{65423,16},{65424,16},{65425,16},{65426,16},{65427,16},{65428,16},{65429,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {59,6},{1016,10},{65430,16},{65431,16},{65432,16},{65433,16},{65434,16},{65435,16},{65436,16},{65437,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {122,7},{2039,11},{65438,16},{65439,16},{65440,16},{65441,16},{65442,16},{65443,16},{65444,16},{65445,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {123,7},{4086,12},{65446,16},{65447,16},{65448,16},{65449,16},{65450,16},{65451,16},{65452,16},{65453,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {250,8},{4087,12},{65454,16},{65455,16},{65456,16},{65457,16},{65458,16},{65459,16},{65460,16},{65461,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {504,9},{32704,15},{65462,16},{65463,16},{65464,16},{65465,16},{65466,16},{65467,16},{65468,16},{65469,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {505,9},{65470,16},{65471,16},{65472,16},{65473,16},{65474,16},{65475,16},{65476,16},{65477,16},{65478,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {506,9},{65479,16},{65480,16},{65481,16},{65482,16},{65483,16},{65484,16},{65485,16},{65486,16},{65487,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {1017,10},{65488,16},{65489,16},{65490,16},{65491,16},{65492,16},{65493,16},{65494,16},{65495,16},{65496,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {1018,10},{65497,16},{65498,16},{65499,16},{65500,16},{65501,16},{65502,16},{65503,16},{65504,16},{65505,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {2040,11},{65506,16},{65507,16},{65508,16},{65509,16},{65510,16},{65511,16},{65512,16},{65513,16},{65514,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {65515,16},{65516,16},{65517,16},{65518,16},{65519,16},{65520,16},{65521,16},{65522,16},{65523,16},{65524,16},{0,0},{0,0},{0,0},{0,0},{0,0},
    {2041,11},{65525,16},{65526,16},{65527,16},{65528,16},{65529,16},{65530,16},{65531,16},{65532,16},{65533,16},{65534,16},{0,0},{0,0},{0,0},{0,0},{0,0}
};
const unsigned short UVAC_HT[256][2] = { 
    {0,2},{1,2},{4,3},{10,4},{24,5},{25,5},{56,6},{120,7},{500,9},{1014,10},{4084,12},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {11,4},{57,6},{246,8},{501,9},{2038,11},{4085,12},{65416,16},{65417,16},{65418,16},{65419,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {26,5},{247,8},{1015,10},{4086,12},{32706,15},{65420,16},{65421,16},{65422,16},{65423,16},{65424,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {27,5},{248,8},{1016,10},{4087,12},{65425,16},{65426,16},{65427,16},{65428,16},{65429,16},{65430,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {58,6},{502,9},{65431,16},{65432,16},{65433,16},{65434,16},{65435,16},{65436,16},{65437,16},{65438,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {59,6},{1017,10},{65439,16},{65440,16},{65441,16},{65442,16},{65443,16},{65444,16},{65445,16},{65446,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {121,7},{2039,11},{65447,16},{65448,16},{65449,16},{65450,16},{65451,16},{65452,16},{65453,16},{65454,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {122,7},{2040,11},{65455,16},{65456,16},{65457,16},{65458,16},{65459,16},{65460,16},{65461,16},{65462,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {249,8},{65463,16},{65464,16},{65465,16},{65466,16},{65467,16},{65468,16},{65469,16},{65470,16},{65471,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {503,9},{65472,16},{65473,16},{65474,16},{65475,16},{65476,16},{65477,16},{65478,16},{65479,16},{65480,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {504,9},{65481,16},{65482,16},{65483,16},{65484,16},{65485,16},{65486,16},{65487,16},{65488,16},{65489,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {505,9},{65490,16},{65491,16},{65492,16},{65493,16},{65494,16},{65495,16},{65496,16},{65497,16},{65498,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {506,9},{65499,16},{65500,16},{65501,16},{65502,16},{65503,16},{65504,16},{65505,16},{65506,16},{65507,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {2041,11},{65508,16},{65509,16},{65510,16},{65511,16},{65512,16},{65513,16},{65514,16},{65515,16},{65516,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {16352,14},{65517,16},{65518,16},{65519,16},{65520,16},{65521,16},{65522,16},{65523,16},{65524,16},{65525,16},{0,0},{0,0},{0,0},{0,0},{0,0},
    {1018,10},{32707,15},{65526,16},{65527,16},{65528,16},{65529,16},{65530,16},{65531,16},{65532,16},{65533,16},{65534,16},{0,0},{0,0},{0,0},{0,0},{0,0}
};
const int YQT[] = {16,11,10,16,24,40,51,61,12,12,14,19,26,58,60,55,14,13,16,24,40,57,69,56,14,17,22,29,51,87,80,62,18,22,37,56,68,109,103,77,24,35,55,64,81,104,113,92,49,64,78,87,103,121,120,101,72,92,95,98,112,100,103,99};
const int UVQT[] = {17,18,24,47,99,99,99,99,18,21,26,66,99,99,99,99,24,26,56,99,99,99,99,99,47,66,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99};
const float aasf[] = { 1.0f * 2.828427125f, 1.387039845f * 2.828427125f, 1.306562965f * 2.828427125f, 1.175875602f * 2.828427125f, 1.0f * 2.828427125f, 0.785694958f * 2.828427125f, 0.541196100f * 2.828427125f, 0.275899379f * 2.828427125f };


void writeBits(FILE *fp, int &bitBuf, int &bitCnt, const unsigned short *bs) {
	bitCnt += bs[1];
	bitBuf |= bs[0] << (24 - bitCnt);
	while(bitCnt >= 8) {
		unsigned char c = (bitBuf >> 16) & 255;
		putc(c, fp);
		if(c == 255) {
			putc(0, fp);
		}
		bitBuf <<= 8;
		bitCnt -= 8;
	}
}

void calcBits(int val, unsigned short bits[2]) {
	int tmp1 = val < 0 ? -val : val;
	val = val < 0 ? val-1 : val;
	bits[1] = 1;
	while(tmp1 >>= 1) {
		++bits[1];
	}
	bits[0] = val & ((1<<bits[1])-1);
}

int processBlock(FILE *fp, int &bitBuf, int &bitCnt, int* DU, float *fdtbl, int DC, const unsigned short HTDC[256][2], const unsigned short HTAC[256][2]){
    const unsigned short EOB[2] = { HTAC[0x00][0], HTAC[0x00][1] };
	const unsigned short M16zeroes[2] = { HTAC[0xF0][0], HTAC[0xF0][1] };
    // Encode DC
	int diff = DU[0] - DC; 
	if (diff == 0) {
		writeBits(fp, bitBuf, bitCnt, HTDC[0]);
	} else {
		unsigned short bits[2];
		calcBits(diff, bits);
		writeBits(fp, bitBuf, bitCnt, HTDC[bits[1]]);
		writeBits(fp, bitBuf, bitCnt, bits);
	}
	// Encode ACs
	int end0pos = 63;
	for(; (end0pos>0)&&(DU[end0pos]==0); --end0pos) {
	}
	// end0pos = first element in reverse order !=0
	if(end0pos == 0) {
		writeBits(fp, bitBuf, bitCnt, EOB);
		return DU[0];
	}
	for(int i = 1; i <= end0pos; ++i) {
		int startpos = i;
		for (; DU[i]==0 && i<=end0pos; ++i) {
		}
		int nrzeroes = i-startpos;
		if ( nrzeroes >= 16 ) {
			int lng = nrzeroes>>4;
			for (int nrmarker=1; nrmarker <= lng; ++nrmarker)
				writeBits(fp, bitBuf, bitCnt, M16zeroes);
			nrzeroes &= 15;
		}
		unsigned short bits[2];
		calcBits(DU[i], bits);
		writeBits(fp, bitBuf, bitCnt, HTAC[(nrzeroes<<4)+bits[1]]);
		writeBits(fp, bitBuf, bitCnt, bits);
	}
	if(end0pos != 63) {
		writeBits(fp, bitBuf, bitCnt, EOB);
	}
    return DU[0];
}


int main(int argc, char** argv) {
    bool sending = true;
    if (argc >= 2) {
        sending = false;
    }
    if (sending) {

        const char* test_filename = "../image/boazbarak.jpg";
        const int requested_composition = 3;
        int width = 0, height = 0, actual_composition = 0;
        uint8_t *image_data = stbi_load(test_filename, &width, &height, &actual_composition, requested_composition);
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
        // We are recieving the results and then doing the actual compression
        // Note that it is very difficult to do compression with purely FHE, 
        // it is possible to do decompression though.
        const char* infile = "../image/zoop.txt";
        const char* outfile = "../image/new_boazzzzzz.jpg";

        int QUALITY = 0;

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
        params.set_coeff_modulus(coeff_modulus_128(2048));
        params.set_plain_modulus(1 << 14);
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

        // Base + Number of coefficients used for encoding past the decimal point (both pos and neg)
        // Example: if poly_base = 11, and N_FRACTIONAL_COEFFS=3, then we will have 
        // a1 * 11^-1 + a2 * 11^-2 + a3 * 11^-3

        FractionalEncoder encoder(context.plain_modulus(), context.poly_modulus(), N_NUMBER_COEFFS, N_FRACTIONAL_COEFFS, POLY_BASE);

        int block_pix = BLOCK_SIZE * BLOCK_SIZE;
        int num_blocks = ((WIDTH + BLOCK_SIZE - 1) / BLOCK_SIZE) * ((HEIGHT + BLOCK_SIZE - 1) / BLOCK_SIZE);

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
        static const unsigned char head0[] = { 0xFF,0xD8,0xFF,0xE0,0,0x10,'J','F','I','F',0,1,1,0,0,1,0,1,0,0,0xFF,0xDB,0,0x84,0 };
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
        start = std::chrono::steady_clock::now(); 
        for (int i = 0; i < num_blocks; i++) {
            int block_zz[3][block_pix];
            for (int k = 0; k < 3; k++) {
                std::cout << i << "\t" << k << std::endl;
                for (int j = 0; j < block_pix; j++) {
                    Ciphertext c;
                    c.load(myfile);
                    Plaintext p;
                    decryptor.decrypt(c, p);
                    v = encoder.decode(p);
                    // std::cout << v << ", ";
                    block_zz[k][s_ZigZag[j]] = (int)(v < 0 ? ceilf(v - 0.5f) : floorf(v + 0.5f));
                }
            }
            DCY = processBlock(fp, bitBuf, bitCnt, block_zz[0], fdtbl_Y, DCY, YDC_HT, YAC_HT);
            DCU = processBlock(fp, bitBuf, bitCnt, block_zz[1], fdtbl_UV, DCU, UVDC_HT, UVAC_HT);
            DCV = processBlock(fp, bitBuf, bitCnt, block_zz[2], fdtbl_UV, DCV, UVDC_HT, UVAC_HT);
        }
        // Write ending sequence
        static const unsigned short fillBits[] = {0x7F, 7};
	    writeBits(fp, bitBuf, bitCnt, fillBits);
        putc(0xFF, fp);
        putc(0xD9, fp);
        myfile.close();
        fclose(fp);

    }

    return 0;
}
