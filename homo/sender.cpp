#include "seal/seal.h"
#include "fhe_image.h"
#include "jpge.h"
#include "stb_image.c"


using namespace seal;

auto start = std::chrono::steady_clock::now();
const bool VERBOSE = true;

std::vector<double> read_image(std::string fname);

int main()
{
    const char* test_filename = "../image/kung.jpg";
    const int requested_composition = 3;
    int width = 0, height = 0, actual_composition = 0;
    uint8_t *image_data = stbi_load(test_filename, &width, &height, &actual_composition, requested_composition);
    std::cout << width << " x " << height << std::endl;
    print_image(image_data,  width, height);
    return 0;
}
