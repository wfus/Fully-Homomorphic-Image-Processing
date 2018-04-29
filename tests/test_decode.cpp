#include "seal/seal.h"
#include "fhe_decode.h"

using namespace seal;

#define APPROX(a, b, epsilon) ((a - b) < epsilon && (b - a) < epsilon)


int main(int argc, const char** argv) {
    assert(APPROX(1, 2, 1));
    assert(APPROX(0.9, 1, 0.05));
    return 0;
}