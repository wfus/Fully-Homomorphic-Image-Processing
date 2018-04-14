#include "fhe_resize.h"

int main(int argc, char** argv) {
    
    std::string fname("image/coolboaz.jpg");
    resize_image_opencv(fname.c_str(), 100, 200);

    return 0;
}



