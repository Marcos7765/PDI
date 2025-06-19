#include<stdint.h>
#include<iostream>
#include<lyra/lyra.hpp>
#include<opencv2/opencv.hpp>

std::string window_name = "bitplanes";

template <typename T>
cv::Mat apply_bitmask(cv::Mat base_image, T bytemask){
    cv::Mat res;
    res.create(base_image.rows, base_image.cols, base_image.type());
    res.setTo(bytemask);
    cv::bitwise_and(base_image, res, res);
    cv::threshold(res, res, 0, 255, cv::THRESH_BINARY);
    return res;
}

constexpr const char* help_msg = "Loads up the image in <input_image_path> in 8-bit grayscale and generate each one of its bitplanes. ";
constexpr const char* default_output_path = "bitplane";

int main(int argc, char** argv) {
    std::string image_path;
    bool help_flag = false;
    bool colored_flag = false;
    bool save_flag = false;
    std::string output_prefix = default_output_path;
    
    auto cli = lyra::help(help_flag).description(help_msg)
        | lyra::arg(image_path, "input_image_path")
            ("the path of the input image")
            .required()
        | lyra::opt(colored_flag)
            ["-c"]["--colored"]
            ("loads and generate bitplanes of the image in RGB (doesn't separate channels)")
        | (lyra::group()
            | lyra::opt(save_flag)
                ["-s"]["--save"]
                ("saves generated bitplanes as [output_prefix]_<bit_significance>.png")
                .required()
            | lyra::arg(output_prefix, "output_prefix")
                (std::string("specifies the prefix output_prefix when saving. if not provided, defaults to ")
                    +default_output_path
                )
        );


    auto res = cli.parse({argc, argv});
    if (!res){
        printf("Error. %s\n", res.message().c_str());
        std::cout << cli << "\n";
        exit(1);
    }
    if (help_flag){
        std::cout << cli << "\n";
        exit(0);
    }
    
    cv::Mat image;
    cv::Vec3b val;

    image = cv::imread(image_path, colored_flag ? cv::IMREAD_COLOR : cv::IMREAD_GRAYSCALE);
    if (image.empty()){printf("Failed to read image in %s.\n", image_path.c_str()); exit(1);}

    cv::namedWindow(window_name, cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO | cv::WINDOW_GUI_NORMAL);
    cv::imshow(window_name, image);
    
    for (auto i = 0; i < 8; i++){
        cv::Mat bitplane = apply_bitmask(image, 0b00000001 << i);
        cv::threshold(bitplane, bitplane, 0, 255, cv::THRESH_BINARY);
        cv::imshow(window_name+"_" + std::to_string(i), bitplane);
        if (save_flag){
            cv::imwrite(output_prefix + "_" + std::to_string(i) + ".png", bitplane);
        }
    }

    cv::waitKey();
    return 0;
}