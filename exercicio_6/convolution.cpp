#include<stdint.h>
#include<iostream>
#include<lyra/lyra.hpp>
#include<opencv2/opencv.hpp>
#include<utils.hpp>
#include"camera.hpp"

std::string window_name = "convolution";
constexpr const char* help_msg = "Embeds RGB histograms and a configurable motion detector based on the red histogram immediate difference";
constexpr const char* default_output_path = "convolution_video_";

cv::Mat average_filter(uint8_t width, uint8_t height){
    return cv::Mat(width, height, CV_32F, 1./(width*height));
}

int main(int argc, char** argv) {
    bool help_flag = false;
    bool save_flag = false;
    std::string input_file;
    std::string output_prefix = default_output_path;
    uint16_t mean_filter_size = 3;
    
    auto cli = lyra::cli() | lyra::help(help_flag).description(help_msg)
        | lyra::arg(input_file, "input_file_path")
            (std::string("the video file path/uri to be open"))
            .required()
        | (lyra::group()
            | lyra::opt(save_flag)
                ["-s"]["--save"]
                ("saves extracted image as [output_prefix]_<bitmask>_filtered.png")
                .required()
            | lyra::arg(output_prefix, "output_prefix")
                (std::string("specifies the output_prefix when saving. if not provided, defaults to ")
                    +default_output_path
                )
            )
        | lyra::arg(mean_filter_size, "mean_filter_size")
            (std::string("how much difference between histogram is needed to trigger the motion detector, defaults to")
                +std::to_string(mean_filter_size)
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
    int width, height;
    cv::VideoCapture cap;

    cap.open(input_file);

    if(!cap.isOpened()){
        std::cout << "cameras indisponiveis\n";
        return -1;
    }
  
    width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    

    cv::VideoWriter video_writers[3];
    if(save_flag){
        cv::Size video_size(width,height);
        for (auto i = 0; i < 3; i++){
            video_writers[i].open(output_prefix+std::to_string(i)+".mkv", cv::VideoWriter::fourcc('M', 'P', '4', 'V'), cap.get(cv::CAP_PROP_FPS),
                video_size);

            if (!video_writers[i].isOpened()){
                std::cout << "Error trying to record.\n";
                exit(1);
            }
        }
    }

    cv::Mat filters[3] = {average_filter(3,3), average_filter(11,11), average_filter(21,21)};
    cv::Mat results[3];
    bool start = false;

    while(cap.isOpened()){
        if (!cap.read(image)){break;} //when the cap has no frame to return (eg end of the video) it returns false,
        //otherwise image wouldn't be valid and opencv would raise an exception
        for (auto i = 0; i < 3; i++){
            cv::filter2D(image, results[i], -1, filters[i], cv::Point(-1, -1), cv::BORDER_REPLICATE);
            cv::imshow(window_name+"_filter_"+std::to_string(i), results[i]);
            if(save_flag){
                video_writers[i] << results[i];
            }
        }

        
        //if(save_flag){video_writer << image;}
        //video_writer << image;
        //else{
        cv::imshow("image", image);
            if (!start){
                printf("Press any key to start\n");
                int key = cv::waitKey(0);
                start = true;
            }
            int key = cv::waitKey(30);
            if (key == 27) {break;}
        //}
    }
    return 0;
}