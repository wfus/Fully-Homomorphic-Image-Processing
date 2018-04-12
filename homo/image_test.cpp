#include "fhe_resize.h"

int main(int argc, char** argv) {
    // show_image("image/coolboaz.jpg");
    std::vector<uint8_t> red {12, 15, 222, 112, 115, 112,12, 15, 222, 112, 115, 112 ,12, 15, 222, 112, 115, 112};
    std::vector<uint8_t> green {12, 15, 222, 112, 115, 112,12, 15, 222, 112, 115, 112 ,12, 15, 222, 112, 115, 112};
    std::vector<uint8_t> blue {12, 15, 222, 112, 115, 112,12, 15, 222, 112, 115, 112 ,12, 15, 222, 112, 115, 112};
    show_image_rgb(6, 3, red, green, blue);
    return 0;
}



