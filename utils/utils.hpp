#include<opencv2/opencv.hpp>
#include<stdint.h>

void default_imshow(std::string window_name, cv::Mat image, 
    int flags = cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO | cv::WINDOW_GUI_NORMAL){
    cv::namedWindow(window_name, flags);
    cv::imshow(window_name, image);
}

cv::Mat default_imread(std::string filepath, int flags = cv::IMREAD_COLOR){
    cv::Mat res = cv::imread(filepath, flags);
    if (filepath.empty()){printf("Failed to read image in %s.\n", filepath.c_str()); exit(1);}
    return res;
}