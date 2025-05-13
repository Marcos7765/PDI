#include<stdint.h>
#include<iostream>
#include<lyra/lyra.hpp>
#include<opencv2/opencv.hpp>
#include<utils.hpp>

std::string window_name = "histogram";
constexpr const char* help_msg = "Embeds RGB histograms and a configurable motion detector based on the red histogram immediate difference";
constexpr const char* default_output_path = "histogram_video.mkv";

int main(int argc, char** argv) {
    bool help_flag = false;
    bool save_flag = false;
    std::string input_file;
    std::string output_path = default_output_path;
    double diff_threshold = 0.065;
    double histogram_image_scale = 5.;
    
    auto cli = lyra::cli() | lyra::help(help_flag).description(help_msg)
        | lyra::arg(input_file, "input_file_path")
            (std::string("the video file path/uri to be open"))
            .required()
        | (lyra::group()
            | lyra::opt(save_flag)
                ["-s"]["--save"]
                ("saves extracted image as [output_prefix]_<bitmask>_filtered.png")
                .required()
            | lyra::arg(output_path, "output_path")
                (std::string("specifies the output_path when saving, so it also affects codecs. if not provided, defaults to ")
                    +default_output_path
                )
            )
        | lyra::arg(diff_threshold, "diff_threshold")
            (std::string("how much difference between histogram is needed to trigger the motion detector, defaults to")
                +std::to_string(diff_threshold)
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

    cap.open(input_file);

    if(!cap.isOpened()){
        std::cout << "Error with capture device\n";
        return -1;
    }
  
    width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);

    std::cout << "largura = " << width << std::endl;
    std::cout << "altura  = " << height << std::endl;

    int histw = nbins*histogram_image_scale, histh = histogram_image_scale*nbins/2;
    cv::Mat histImgR(histh, histw, CV_8UC3, cv::Scalar(0,0,0));
    cv::Mat histImgG(histh, histw, CV_8UC3, cv::Scalar(0,0,0));
    cv::Mat histImgB(histh, histw, CV_8UC3, cv::Scalar(0,0,0));
    

    cv::VideoWriter video_writer;
    if(save_flag){
        cv::Size video_size(width,height);
        video_writer.open(output_path, cv::VideoWriter::fourcc('M', 'P', '4', 'V'), cap.get(cv::CAP_PROP_FPS),
            video_size);

        if (!video_writer.isOpened()){
            std::cout << "deu pau na gravação\n";
            exit(1);
        }
    }
    cap >> image;
    cv::split (image, planes);
    cv::calcHist(&planes[0], 1, 0, cv::Mat(), histB, 1, &nbins, &histrange, uniform, acummulate);
    cv::calcHist(&planes[1], 1, 0, cv::Mat(), histG, 1, &nbins, &histrange, uniform, acummulate);
    cv::calcHist(&planes[2], 1, 0, cv::Mat(), histR, 1, &nbins, &histrange, uniform, acummulate);
    cv::Mat old_hist_r = histR.clone();
    
    while(cap.isOpened()){
        if (!cap.read(image)){break;} //when the cap has no frame to return (eg end of the video) it returns false,
        //otherwise image wouldn't be valid and opencv would raise an exception
        cv::split (image, planes);
        cv::calcHist(&planes[0], 1, 0, cv::Mat(), histB, 1, &nbins, &histrange, uniform, acummulate);
        cv::calcHist(&planes[1], 1, 0, cv::Mat(), histG, 1, &nbins, &histrange, uniform, acummulate);
        cv::calcHist(&planes[2], 1, 0, cv::Mat(), histR, 1, &nbins, &histrange, uniform, acummulate);

        cv::Mat diff_img;
        cv::absdiff(old_hist_r, histR, diff_img);
        double diff_sum = cv::sum(diff_img).val[0]*3/(image.rows*image.cols);
            cv::putText(image, "Diff: "+std::to_string(diff_sum), {image.cols*5/8, image.rows*3/16}, cv::FONT_HERSHEY_PLAIN, 2., cv::Scalar(0, 0, 0), 12);
            cv::putText(image, "Diff: "+std::to_string(diff_sum), {image.cols*5/8, image.rows*3/16}, cv::FONT_HERSHEY_PLAIN, 2., cv::Scalar(255, 255, 255), 3);
        if (diff_sum > diff_threshold){
            cv::putText(image, "Movimento!", {image.cols*5/8, image.rows*1/8}, cv::FONT_HERSHEY_PLAIN, 4., cv::Scalar(0, 0, 0), 12);
            cv::putText(image, "Movimento!", {image.cols*5/8, image.rows*1/8}, cv::FONT_HERSHEY_PLAIN, 4., cv::Scalar(255, 255, 255), 3);
        }

        old_hist_r = histR.clone();

        cv::normalize(histR, histR, 0, histImgR.rows, cv::NORM_MINMAX, -1, cv::Mat());
        cv::normalize(histG, histG, 0, histImgG.rows, cv::NORM_MINMAX, -1, cv::Mat());
        cv::normalize(histB, histB, 0, histImgB.rows, cv::NORM_MINMAX, -1, cv::Mat());

        histImgR.setTo(cv::Scalar(0));
        histImgG.setTo(cv::Scalar(0));
        histImgB.setTo(cv::Scalar(0));

        for(int i=0; i<nbins; i++){
            cv::rectangle(histImgR,
                cv::Point(i*histogram_image_scale, histh-cvRound(histR.at<float>(i))),
                cv::Point((i+1)*histogram_image_scale, histh),
                cv::Scalar(0, 0, 255), cv::FILLED);
            cv::rectangle(histImgG,
                cv::Point(i*histogram_image_scale, histh-cvRound(histG.at<float>(i))),
                cv::Point((i+1)*histogram_image_scale, histh),
                cv::Scalar(0, 255, 0), cv::FILLED);
            cv::rectangle(histImgB,
                cv::Point(i*histogram_image_scale, histh-cvRound(histB.at<float>(i))),
                cv::Point((i+1)*histogram_image_scale, histh),
                cv::Scalar(255, 0, 0), cv::FILLED);
        }
        histImgR.copyTo(image(cv::Rect(0, 0       ,histw, histh)));
        histImgG.copyTo(image(cv::Rect(0, histh   ,histw, histh)));
        histImgB.copyTo(image(cv::Rect(0, 2*histh ,histw, histh)));

        
        
        if(save_flag){video_writer << image;}
        //video_writer << image;
        //else{
            cv::imshow("image", image);
            key = cv::waitKey(30);
            if (key == 27) {break;}
        //}
    }
    return 0;
}