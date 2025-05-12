#include<stdint.h>
#include<iostream>
#include<lyra/lyra.hpp>
#include<opencv2/opencv.hpp>
#include<utils.hpp>

std::string window_name = "labelling";
constexpr const char* help_msg = "Labelling algorithm example";
constexpr const char* default_output_path = "labelling";

template <typename T, uint8_t bytesize = 8>
constexpr inline T gen_ones_mask(uint8_t length, uint8_t leftshift){
    return (( ~((T) 0) >> (sizeof(T)*bytesize - length)) << leftshift);
}

bool compare_colors(cv::Vec3b color_a, cv::Vec3b color_b){
    bool res = true;
    for (auto i = 0; i < 3; i++){
        res &= color_a[i] == color_b[i];
    }
    return res;
}

//aint linear but surely is injective for input_value < 2^25 -1
cv::Vec3b map_color(uint32_t input_value){
    cv::Vec3b result = {0,0,0};
    result[0] = (input_value & gen_ones_mask<uint32_t>(8, 16)) >> 16;
    result[1] = (input_value & gen_ones_mask<uint32_t>(8, 8)) >> 8;
    result[2] = input_value & gen_ones_mask<uint32_t>(8, 0);
    return result;
}

uint32_t unmap_color(cv::Vec3b input_color){
    uint32_t res = 0;
    res |= (input_color[0] << 16);
    res |= (input_color[0] << 8);
    res |= input_color[2];
    return res;
}

int main(int argc, char** argv) {
    std::string image_path;
    bool help_flag = false;
    bool save_flag = false;
    std::string output_prefix = default_output_path;
    
    auto cli = lyra::cli() | lyra::help(help_flag).description(help_msg)
        | lyra::arg(image_path, "input_image_path")
            ("the path of the input image")
            .required()
        | (lyra::group()
            | lyra::opt(save_flag)
                ["-s"]["--save"]
                ("saves extracted image as [output_prefix]_<bitmask>_filtered.png")
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

    cv::Mat image = default_imread(image_path, cv::IMREAD_COLOR);
    default_imshow(window_name+"_original", image);

    uint32_t count = 0;
    cv::Vec3b background_color = map_color(count); //map_color(0) maps to 0,0,0
    cv::Vec3b target_color = map_color(count);
    cv::Vec3b object_color = {255,255,255};

    //removing objects touching the edges
    for (auto j = 0; j < image.cols; j++){
        cv::Point2l pos_start = {j, 0};
        cv::Point2l pos_end = {j, image.rows-1};
        if (compare_colors(image.at<typeof(target_color)>(0, j),object_color)){
            cv::floodFill(image, pos_start, background_color);
        }
        if (compare_colors(image.at<typeof(target_color)>(image.rows-1, j),object_color)){
            cv::floodFill(image, pos_end, background_color);
        }
    }
    for (auto i = 1; i < image.rows-1; i++){
        cv::Point2l pos_start = {0, i};
        cv::Point2l pos_end = {image.cols-1, i};
        if (compare_colors(image.at<typeof(target_color)>(i, 0),object_color)){
            cv::floodFill(image, pos_start, background_color);
        }
        if (compare_colors(image.at<typeof(target_color)>(i, image.cols-1),object_color)){
            cv::floodFill(image, pos_end, background_color);
        }
    }

    //labelling all objects
    for (auto j = 0; j < image.cols; j++){
        for (auto i = 0; i < image.rows; i++){
            cv::Point2l pos = {j, i};
            if (compare_colors(image.at<typeof(target_color)>(i, j),object_color)){
                count++;
                cv::Vec3b target_color = map_color(count);
                cv::floodFill(image, pos, target_color);
            }
        }
    }

    cv::floodFill(image, {0,0}, object_color);

    uint32_t bubble_count = 0;
    uint32_t last_object = 0;
    bool* has_bubble = new bool[count](); //initializes all values to false
    for (auto j = 0; j < image.cols; j++){
        for (auto i = 0; i < image.rows; i++){
            cv::Point2l pos = {j, i};
            if (compare_colors(image.at<typeof(target_color)>(i, j), background_color)){
                if (!has_bubble[last_object-1]){
                    bubble_count++;
                    has_bubble[last_object-1] = true;
                }
                cv::floodFill(image, pos, map_color(last_object));
            } else{
                if (!compare_colors(image.at<typeof(target_color)>(i, j), object_color)){
                    last_object = unmap_color(image.at<typeof(target_color)>(i, j));
                }
            }
        }
    }

    printf("Total labels: %u\n", count);
    printf("Bubble count: %u\n", bubble_count);
    
    default_imshow(window_name+"_labeled", image);
    if (save_flag){
        cv::imwrite(output_prefix + "_labeled.png", image);
    }

    cv::waitKey();
    return 0;
}