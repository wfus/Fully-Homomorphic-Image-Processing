#include "seal/seal.h"
#include "fhe_decode.h"
#include "fhe_image.h"

using namespace seal;

#define APPROX(a, b, epsilon) ((a - b) <= epsilon && (b - a) <= epsilon)


int main(int argc, const char** argv) {

    int n_number_coeffs = N_NUMBER_COEFFS;
    int n_fractional_coeffs = N_FRACTIONAL_COEFFS;
    int n_poly_base = POLY_BASE;
    int plain_modulus = 1 << 14;
    int coeff_modulus = COEFF_MODULUS;

    EncryptionParameters params;
    char poly_mod[16];
    snprintf(poly_mod, 16, "1x^%i + 1", coeff_modulus);
    params.set_poly_modulus(poly_mod);
    params.set_coeff_modulus(coeff_modulus_128(coeff_modulus));
    params.set_plain_modulus(plain_modulus);
    SEALContext context(params);
    print_parameters(context);

    KeyGenerator keygen(context);
    auto public_key = keygen.public_key();
    auto secret_key = keygen.secret_key();
    
    // Encrytor and decryptor setup
    Encryptor encryptor(context, public_key);
    Evaluator evaluator(context);
    Decryptor decryptor(context, secret_key);

    FractionalEncoder encoder(context.plain_modulus(), context.poly_modulus(), n_number_coeffs, n_fractional_coeffs, n_poly_base);


    std::vector<double> test_numbers = {1, 2, 3, 4, 5, 6, 7};
    for (int i = 0; i < test_numbers.size(); i++) {
        Ciphertext c, res;
        Plaintext p;
        encryptor.encrypt(encoder.encode(test_numbers[i]), c);
        homomorphic_sine(c, res, evaluator, encoder, encryptor);
        std::cout << "Noise Budget Left: " << decryptor.invariant_noise_budget(res) << std::endl;
        decryptor.decrypt(res, p);
        std::cout << "Res: " << encoder.decode(p) << " Actual Sin: " << sin(test_numbers[i]) << std::endl;
    }

    
    return 0;
}