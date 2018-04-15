#include "fhe_resize.h"

int main(int argc, char** argv) {
    
    std::string fname("image/coolboaz.jpg");
    resize_image_opencv(fname.c_str(), 100, 200);

    cv::Mat image;
    image = cv::imread("image/test.jpg", cv::IMREAD_COLOR);
    #ifdef linux
        cv::imwrite("image/lol.png", image);
    #endif
    return 0;
}



