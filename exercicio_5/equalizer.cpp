#include<stdint.h>
#include<iostream>
#include<lyra/lyra.hpp>
#include<opencv2/opencv.hpp>
#include<utils.hpp>
#include"histogram.hpp"

std::string window_name = "histogram";
constexpr const char* help_msg = "Generates";
constexpr const char* default_output_path = "histogram";

int main(int argc, char** argv) {
    bool help_flag = false;
    bool save_flag = false;
    std::string output_prefix = default_output_path;
    
    auto cli = lyra::cli() | lyra::help(help_flag).description(help_msg)
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

    cv::Mat image;
    int width, height;
    int camera;
    cv::VideoCapture cap;
    std::vector<cv::Mat> planes;
    cv::Mat histR, histG, histB;
    int nbins = 64;
    float range[] = {0, 255};
    const float *histrange = { range };
    bool uniform = true;
    bool acummulate = false;
    int key;

    //camera = cameraEnumerator();
    //cap.open(camera);
    cap.open("udp://@0.0.0.0:8081");

    if(!cap.isOpened()){
        std::cout << "cameras indisponiveis";
        return -1;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);  
    width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);

    std::cout << "largura = " << width << std::endl;
    std::cout << "altura  = " << height << std::endl;

    int histw = nbins, histh = nbins/2;
    cv::Mat histImgR(histh, histw, CV_8UC3, cv::Scalar(0,0,0));
    cv::Mat histImgG(histh, histw, CV_8UC3, cv::Scalar(0,0,0));
    cv::Mat histImgB(histh, histw, CV_8UC3, cv::Scalar(0,0,0));

    cv::Size video_size(cap.get(cv::CAP_PROP_FRAME_WIDTH),cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    
    cv::VideoWriter video_writer(output_prefix+"_video.mp4", -1, cap.get(cv::CAP_PROP_FPS), video_size);

    if (!video_writer.isOpened()){
        std::cout << "deu pau na gravação\n";
    }
    
    while(cap.isOpened()){
        cap >> image;
        cv::split (image, planes);
        cv::calcHist(&planes[0], 1, 0, cv::Mat(), histB, 1, &nbins, &histrange, uniform, acummulate);
        cv::calcHist(&planes[1], 1, 0, cv::Mat(), histG, 1, &nbins, &histrange, uniform, acummulate);
        cv::calcHist(&planes[2], 1, 0, cv::Mat(), histR, 1, &nbins, &histrange, uniform, acummulate);

        cv::normalize(histR, histR, 0, histImgR.rows, cv::NORM_MINMAX, -1, cv::Mat());
        cv::normalize(histG, histG, 0, histImgG.rows, cv::NORM_MINMAX, -1, cv::Mat());
        cv::normalize(histB, histB, 0, histImgB.rows, cv::NORM_MINMAX, -1, cv::Mat());

        histImgR.setTo(cv::Scalar(0));
        histImgG.setTo(cv::Scalar(0));
        histImgB.setTo(cv::Scalar(0));

        for(int i=0; i<nbins; i++){
            cv::line(histImgR,
            cv::Point(i, histh),
            cv::Point(i, histh-cvRound(histR.at<float>(i))),
            cv::Scalar(0, 0, 255), 1, 8, 0);
            cv::line(histImgG,
            cv::Point(i, histh),
            cv::Point(i, histh-cvRound(histG.at<float>(i))),
            cv::Scalar(0, 255, 0), 1, 8, 0);
            cv::line(histImgB,
            cv::Point(i, histh),
            cv::Point(i, histh-cvRound(histB.at<float>(i))),
            cv::Scalar(255, 0, 0), 1, 8, 0);
        }
        histImgR.copyTo(image(cv::Rect(0, 0       ,nbins, histh)));
        histImgG.copyTo(image(cv::Rect(0, histh   ,nbins, histh)));
        histImgB.copyTo(image(cv::Rect(0, 2*histh ,nbins, histh)));
        
        video_writer << image;
        cv::imshow("image", image);
        key = cv::waitKey(30);
        if (key == 27) {break;}
    }
    return 0;
}