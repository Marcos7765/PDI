#include<stdint.h>
#include<iostream>
#include<lyra/lyra.hpp>
#include<opencv2/opencv.hpp>
#include<utils.hpp>
#include"camera.hpp"

std::string window_name = "depthoffield";
constexpr const char* help_msg = "Embeds RGB histograms and a configurable motion detector based on the red histogram immediate difference";
constexpr const char* default_output_path = "depthoffield_video_";

cv::Mat average_filter(uint8_t width, uint8_t height){
    return cv::Mat(width, height, CV_32F, 1./(width*height));
}

//generates 11114 or 111111118 only
cv::Mat laplacian_filter(bool eight_neighbours=false, float gain=1.){
    cv::Mat res(3, 3, CV_32F, -gain);
    res.at<float>(1,1) = gain*(eight_neighbours ? 8 : 4);
    if (!eight_neighbours){
        for (auto j = 0; j < 3; j+=2){
            for (auto i = 0; i < 3; i+=2){
                res.at<float>(j,i) = 0.;
            }
        }
    }
    return res;
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
    

    cv::VideoWriter video_writers[2];
    if(save_flag){
        cv::Size video_size(width,height);
        for (auto i = 0; i < 2; i++){
            printf("Press any key to starta\n");
            video_writers[i].open(output_prefix+"_"+std::to_string(i)+".mkv", cv::VideoWriter::fourcc('F', 'M', 'P', '4'), cap.get(cv::CAP_PROP_FPS),
            video_size);

            if (!video_writers[i].isOpened()){
                std::cout << "Error trying to record.\n";
                exit(1);
            }
        }
    }
    

    cv::Mat filter = laplacian_filter();
    cv::Mat gray_lapl;
    cv::Mat gray_record;
    
    if (!cap.read(image)){return 1;}
    cv::Mat result = image.clone();

    cv::cvtColor(image, gray_lapl, cv::COLOR_BGR2GRAY);
    cv::filter2D(gray_lapl, gray_lapl, -1, filter, cv::Point(-1, -1), cv::BORDER_REPLICATE);
    gray_record = gray_lapl.clone();//just to start it with a value
    cv::Mat output_mask(gray_record.rows, gray_record.cols, CV_8UC1, cv::Scalar(0));
    cv::Mat result_mask;
    cv::cvtColor(output_mask, result_mask, cv::COLOR_GRAY2BGR);

    if(save_flag){
        video_writers[1] << result_mask;
        video_writers[0] << result;
    }

    cv::imshow(window_name+"output_mask", output_mask);
    cv::imshow(window_name+"_filter", result);
    cv::imshow(window_name+"_base", image);
    
    printf("Press any key to start\n");
    int key = cv::waitKey(0);

    while(cap.isOpened()){
        if (!cap.read(image)){break;} //when the cap has no frame to return (eg end of the video) it returns false,
        //otherwise image wouldn't be valid and opencv would raise an exception
        cv::cvtColor(image, gray_lapl, cv::COLOR_BGR2GRAY);
        cv::filter2D(gray_lapl, gray_lapl, -1, filter, cv::Point(-1, -1), cv::BORDER_REPLICATE);
        cv::compare(gray_lapl, gray_record, output_mask, cv::CMP_GE);
        cv::copyTo(gray_lapl, gray_record, output_mask);
        cv::copyTo(image, result, output_mask);
        cv::imshow(window_name+"output_mask", output_mask);
        cv::imshow(window_name+"_filter", result);
        cv::imshow(window_name+"_base", image);
        cv::cvtColor(output_mask, result_mask, cv::COLOR_GRAY2BGR);
        
        if(save_flag){
            video_writers[0] << result;
            video_writers[1] << result_mask;
        }
        //video_writer << image;
        //else{
            int key = cv::waitKey(30);
            if (key == 27) {break;}
        //}
    }
    return 0;
}