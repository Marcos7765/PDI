#include<stdint.h>
#include<iostream>
#include<lyra/lyra.hpp>
#include<opencv2/opencv.hpp>
#include <string>

constexpr const char* help_msg = "Showcase of truncation effects with YAML's human-readable serialization. The program generates a image in grayscale with f(x,y) = 127*sin(2*<sine_period>*x*PI/<sine_period>), serializes it to yaml, serializes a normalized version to png, reads back the yaml matrix, and then compare both the original float version with the yaml float version as well as the png and a normalized yaml version. All images on 8-bit grayscale.";

int main(int argc, char** argv) {
    bool help_flag = false;
    bool print_diff_csv = false;
    std::string img_path = "senoide_";
    std::string yml_path = "senoide_";
    u_int32_t SIDE = 256;
    u_int32_t PERIODOS = 4;

    auto cli = lyra::help(help_flag).description(help_msg)
        | lyra::opt(print_diff_csv)
            ["-p"]["--csv"]["--printCSV"]
            ("enables formatted csv output of the differences into the programs stdout")
        | lyra::arg(SIDE, "image_side_size")
            ("the image's side size in pixels, defaults to "+std::to_string(SIDE))
        | lyra::arg(PERIODOS, "sine_period")
            ("the sine wave's period, defaults to "+std::to_string(SIDE))
        | lyra::arg(img_path, "output_image_prefix")
            ("the prefix of the output PNG image path, defaults to "+img_path)
        | lyra::arg(yml_path, "output_yaml_prefix")
            ("the prefix of the output YAML file path, defaults to "+yml_path);

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

    yml_path +=std::to_string(SIDE)+"_"+std::to_string(PERIODOS)+".yaml";
    img_path +=std::to_string(SIDE)+"_"+std::to_string(PERIODOS)+".png";
    
    cv::Mat image_orig = cv::Mat::zeros(SIDE, SIDE, CV_32FC1);
    cv::FileStorage fs(yml_path, cv::FileStorage::WRITE);

    for (int i = 0; i < SIDE; i++) {
        for (int j = 0; j < SIDE; j++) {
            image_orig.at<float>(i, j) = 127 * sin(2 * PERIODOS * j * M_PI / SIDE) + 128; //reordered to avoid integer division truncation
        }
    }

    fs << "mat" << image_orig;
    fs.release();

    cv::Mat image_png;
    cv::normalize(image_orig, image_png, 0, 255, cv::NORM_MINMAX);
    image_png.convertTo(image_png, CV_8U);
    cv::imwrite(img_path, image_png);
    cv::imshow("image from png", image_png);

    fs.open(yml_path, cv::FileStorage::READ);

    cv::Mat image_yaml;
    cv::Mat image_yaml_normalized;
    fs["mat"] >> image_yaml;
    
    cv::normalize(image_yaml, image_yaml_normalized, 0, 255, cv::NORM_MINMAX);
    image_yaml_normalized.convertTo(image_yaml_normalized, CV_8U);
    cv::imshow("image from yaml, normalized", image_yaml_normalized);
    //right now: image_yaml holds the float32 image read from the yaml file,
    //  image_yaml_normalized holds its normalized uint8 version,
    //  image_orig holds the original float32 image,
    //  image_png holds the normalized uint8 version of image_orig (which is the same as the png)

    cv::Mat float_diff_img; //to check yaml's serialization diff
    image_orig -= image_yaml;
    cv::normalize(image_orig, float_diff_img, 0, 255, cv::NORM_MINMAX);
    float_diff_img.convertTo(float_diff_img, CV_8U);
    cv::imshow("image_orig - image_yaml, normalized", float_diff_img);
    
    cv::Mat norm_diff; //to check if the difference, if any, matters after conversion to integer
    image_png.convertTo(norm_diff, CV_16S); 
    norm_diff -= image_yaml_normalized;
    cv::normalize(norm_diff, norm_diff, 0, 255, cv::NORM_MINMAX);
    norm_diff.convertTo(norm_diff, CV_8U);
    
    cv::imshow("image_png - image_yaml_normalized, normalized", norm_diff);
    cv::waitKey();
    if (print_diff_csv){
        printf("FLOAT_DIFF, INTEGER_DIFF, CROSS_DIFF\n");
        for (int j = 0; j < SIDE; j++) {
            printf("%f, %d, %f\n", image_orig.at<float>(0, j), norm_diff.at<uint8_t>(0, j), image_yaml.at<float>(0, j)-image_png.at<uint8_t>(0,j));
        }
    }
    return 0;
}