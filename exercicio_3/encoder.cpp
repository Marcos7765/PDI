#include<stdint.h>
#include<iostream>
#include<lyra/lyra.hpp>
#include<opencv2/opencv.hpp>
#include<utils.hpp>

std::string window_name = "encoder";
constexpr const char* help_msg = "Applies <bitmask> and it's complement to both <base_image> and <hidden_image>, then sums them together into <output_prefix>.png.";
constexpr const char* default_output_path = "encoded_image";

template <typename T>
cv::Mat apply_bitmask(cv::Mat base_image, T bitmask){
    cv::Mat res;
    res.create(base_image.rows, base_image.cols, base_image.type());
    res.setTo(bitmask);
    cv::bitwise_and(base_image, res, res);
    return res;
}

void apply_bitshift(cv::Mat& base_image, uint8_t shift_amount){
    base_image.forEach<cv::Point3_<uint8_t>>([&](cv::Point3_<uint8_t>& pixel, const int* position){
        pixel.x >>= shift_amount;
        pixel.y >>= shift_amount;
        pixel.z >>= shift_amount;
    });
}

//error_codes:
//error_code < 7 -> invalid character at error_code
//8 -> invalid size for str_input
//9 -> all ok
uint8_t string_to_bitmask(std::string str_input, uint8_t* bitmask_output){
    uint32_t counter = str_input.size();
    uint8_t bitmask = 0;
    if (counter > 8 || counter < 8){
        return 8;
    }
    counter--;
    for (char it : str_input){
        switch (it){
        case '0':
            break;
        case '1':
            bitmask = bitmask | (1 << counter);
            break;
        default:
            return 7-counter;
            break;
        }
        counter--;
    }
    printf("\n");
    *bitmask_output = bitmask;
    return 9;
}

//here i noticed i could've just made it fail at string_to_bitmask instead of this
void handled_string_to_bitmask(std::string str_input, uint8_t* bitmask_output){
    uint8_t err_code = string_to_bitmask(str_input, bitmask_output);
    if (err_code < 8){
        printf("Error. invalid character in <bitmask> at position %u: %c. %s\n", err_code, str_input[err_code], str_input.c_str());
        exit(1);
    }
    if (err_code == 8){
        printf("Error. invalid size for <bitmask> (%lu).\n", str_input.size());
        exit(1);
    }
}

int main(int argc, char** argv) {
    std::string base_image_path;
    std::string hidden_image_path;
    bool help_flag = false;
    bool save_flag = false;
    unsigned int bit_shift = 5;
    std::string output_prefix = default_output_path;
    std::string bitmask_untreated = "00000111";
    
    auto cli = lyra::cli() | lyra::help(help_flag).description(help_msg)
        | lyra::arg(base_image_path, "base_image_path")
            ("the path of the first input image")
            .required()
        | lyra::arg(hidden_image_path, "hidden_image_path")
            ("the path of the first input image")
            .required()
        | (lyra::group()
            | lyra::opt(save_flag)
                ["-s"]["--save"]
                ("saves generated image as [output_prefix]_<bitmask>_filtered.png")
                .required()
            | lyra::arg(output_prefix, "output_prefix")
                (std::string("specifies the prefix output_prefix when saving. if not provided, defaults to ")
                    +default_output_path
                )
            )
        | lyra::arg(bitmask_untreated, "bitmask")
            ("the bitmask to apply, defaults to " + bitmask_untreated)
        | lyra::arg(bit_shift, "bitshift")
            ("a right-bitshift to apply to <hidden_image>, defaults to " + std::to_string(bit_shift));


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

    uint8_t bitmask;
    handled_string_to_bitmask(bitmask_untreated, &bitmask);
    uint8_t inverse_bitmask = ~bitmask;
    
    cv::Mat base_image = default_imread(base_image_path, cv::IMREAD_COLOR);
    cv::Mat hidden_image = default_imread(hidden_image_path, cv::IMREAD_COLOR);
    
    
    apply_bitshift(hidden_image, bit_shift);

    cv::Mat result_image = apply_bitmask(base_image, inverse_bitmask) + apply_bitmask(hidden_image, bitmask);

    default_imshow(window_name+"_" + bitmask_untreated + "_encoded", result_image);
    
    cv::namedWindow(window_name+"_" + bitmask_untreated + "_encoded", cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO | cv::WINDOW_GUI_NORMAL);
    cv::imshow(window_name+"_" + bitmask_untreated + "_encoded", result_image);
    
    if (save_flag){
        cv::imwrite(output_prefix + "_" + bitmask_untreated + "_encoded" + ".png", result_image);
    }

    cv::waitKey();
    return 0;
}