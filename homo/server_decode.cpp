#include "seal/seal.h"
#include "fhe_decode.h"
#include "fhe_image.h"
#include "jpge.h"
#include <math.h>


using namespace seal;
using namespace cv;

auto start = std::chrono::steady_clock::now();
auto diff = std::chrono::steady_clock::now() - start;




int main(int argc, char** argv) {
    
    // Read encryption parameters from file
    int WIDTH = 0, HEIGHT = 0, CHANNELS=0;
    std::ifstream paramfile;
    paramfile.open("../keys/params.txt");
    paramfile >> WIDTH;
    paramfile >> HEIGHT;
    paramfile >> CHANNELS;
    std::cout << WIDTH << " " << HEIGHT << " Channels: " << CHANNELS << std::endl;
    assert(CHANNELS != 0);
    assert(CHANNELS == 3);
    assert(WIDTH != 0);
    assert(HEIGHT != 0);
    paramfile.close();


    // Encryption Parameters
    EncryptionParameters params;
    params.set_poly_modulus("1x^8192 + 1");
    params.set_coeff_modulus(coeff_modulus_128(COEFF_MODULUS));
    params.set_plain_modulus(PLAIN_MODULUS);
    SEALContext context(params);
    print_parameters(context);


    // Generate keys
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

    // Encrytor and decryptor setup
    Encryptor encryptor(context, public_key);
    Evaluator evaluator(context);

    // FOR DEBUGGING ONLY!
    Decryptor decryptor(context, secret_key);
    FractionalEncoder encoder(context.plain_modulus(), context.poly_modulus(), N_NUMBER_COEFFS, N_FRACTIONAL_COEFFS, POLY_BASE);

    std::ofstream outfile;
    outfile.open("../image/zoop.txt"); 
    std::ifstream myfile;
    myfile.open("../image/nothingpersonnel.txt");
    Ciphertext c;
    
    myfile.close();
    outfile.close();  

    int precision = 8;
    double cosine[106];
    int coefficients[4096];
    for (int i=0; i < 106; i++) {
            cosine[i] = std::cos(i*M_PI/16);
    }
    for(int i=0;i<8;i++){
        for(int j=0;j<8;j++){
            for(int u=0;u<8;u++){
                for(int v=0;v<8;v++){
                    int index = i*512 + j*64 + u*8 + v;
                    double tmp = 1024 * cosine[(2*i+1)*u] * cosine[(2*j+1)*v];
                    if (u==0 && v==0)
                        tmp /= 8;
                    else if (u>0 && v>0)
                        tmp /= 4;
                    else
                        tmp *= 0.1767766952966368811;
                    coefficients[index] = tmp;
                }
            }
        }
    }

    return 0;
}


// https://github.com/amirduran/jpeg-encoder-decoder/blob/master/jpegdecode.cpp
void IDCT (int block[8][8], int transformedBlock[8][8]) {
    int sum=0;
    int counter=0;

    for(int i=0;i<8;i++){
        for(int j=0;j<8;j++){
            sum=0;
            for(int u=0;u<8;u++){
                for(int v=0;v<8;v++){
                    sum = sum + block[u][v] * coefficients[ counter++ ];
                }
            }

            // All coefficients are multiplied by 1024 since they are int
            sum = sum >> 10;

            // Only 8 and 12-bit is supported by DCT per JPEG standard
            if (precision == 8)
                transformedBlock[i][j]=sum+128;
            else if (precision == 12)
                transformedBlock[i][j]=sum+2048;
        }
    }
}
