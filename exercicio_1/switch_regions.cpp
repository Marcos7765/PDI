#include<stdint.h>
#include<iostream>
#include<lyra/lyra.hpp>
#include<opencv2/opencv.hpp>

std::string window_name = "switch_regions";
bool pressing_left = false;
bool slide_enable = false;
cv::Mat redraw_image;
cv::Vec2i last_pos = {0,0};
cv::Vec2i velocity = {0,0};

void redraw_window(std::string win_name, cv::Vec2i div_point, void* image_ptr_opaque){
    cv::Mat* image_ptr = (cv::Mat*) image_ptr_opaque;

    if (div_point[0] == 0 || div_point[1] == 0){
        printf("Cancelling redraw 'cause OpenCV doesn't like degenerate rectangles.\n");
        return;
    }
    

    auto left_div_rem = image_ptr->cols - div_point[0];
    auto up_div_rem = image_ptr->rows - div_point[1];
    
    //this was the first try, it would get the cross on the opposite side
    //cv::Mat top_left(*image_ptr, cv::Rect(0,0, div_point[0], div_point[1]));
    //cv::Mat bottom_left(*image_ptr, cv::Rect(0,div_point[1], div_point[0], up_div_rem));
    //cv::Mat top_right(*image_ptr, cv::Rect(div_point[0], 0, left_div_rem, div_point[1]));
    //cv::Mat bottom_right(*image_ptr, cv::Rect(div_point[0], div_point[1], left_div_rem, up_div_rem));

    //bottom_right.copyTo(
    //    redraw_image(cv::Rect(0, 0, left_div_rem, up_div_rem))
    //);
    //bottom_left.copyTo(
    //    redraw_image(cv::Rect(left_div_rem,0, div_point[0], up_div_rem))
    //);
    //top_right.copyTo(
    //    redraw_image(cv::Rect(0, up_div_rem, left_div_rem, div_point[1]))
    //);
    //top_left.copyTo(
    //    redraw_image(cv::Rect(left_div_rem,up_div_rem, div_point[0], div_point[1]))
    //);

    cv::Mat top_left(*image_ptr, cv::Rect(0,0, left_div_rem, up_div_rem));
    cv::Mat bottom_left(*image_ptr, cv::Rect(0,up_div_rem, left_div_rem, div_point[1]));
    cv::Mat top_right(*image_ptr, cv::Rect(left_div_rem, 0, div_point[0], up_div_rem));
    cv::Mat bottom_right(*image_ptr, cv::Rect(left_div_rem, up_div_rem, div_point[0], div_point[1]));

    bottom_right.copyTo(
        redraw_image(cv::Rect(0, 0, div_point[0], div_point[1]))
    );
    bottom_left.copyTo(
        redraw_image(cv::Rect(div_point[0],0, left_div_rem, div_point[1]))
    );
    top_right.copyTo(
        redraw_image(cv::Rect(0, div_point[1], div_point[0], up_div_rem))
    );
    top_left.copyTo(
        redraw_image(cv::Rect(div_point[0],div_point[1], left_div_rem, up_div_rem))
    );

    cv::imshow(win_name, redraw_image);
}

void mouse_callback(int event, int x, int y, int flags, void* userdata){
    cv::Vec2i div_point = {x, y};
    switch (event){
    case cv::EVENT_LBUTTONDOWN:
        pressing_left = true;
        redraw_window(window_name, div_point, userdata);
        last_pos[0] = x; last_pos[1] = y;
        velocity[0] = 0; velocity[1] = 0;
        break;
    case cv::EVENT_LBUTTONUP:
        pressing_left = false;
        break;
    case cv::EVENT_MBUTTONDOWN:
        slide_enable = !slide_enable;
        break;
    case cv::EVENT_MOUSEMOVE:
        if (pressing_left){
            redraw_window(window_name, div_point, userdata);
            if (slide_enable){
                velocity[0] = x - last_pos[0]; velocity[1] = y - last_pos[1];
            }
            last_pos[0] = x; last_pos[1] = y;
        }
        break;
    case cv::EVENT_RBUTTONDOWN:
        cv::imshow(window_name, *((cv::Mat*) userdata));
        velocity[0] = 0; velocity[1] = 0;
        last_pos[0] = 0; last_pos[1] = 0;
        break;
    default:
        break;
    }
}

void animation_loop(cv::Mat *image){
    if (slide_enable && !pressing_left){
        last_pos[0] = (last_pos[0]+velocity[0]);
        last_pos[1] = (last_pos[1]+velocity[1]);
        last_pos[0] = last_pos[0] < 0 ? image->cols + last_pos[0] : last_pos[0]%image->cols;
        last_pos[1] = last_pos[1] < 0 ? image->rows + last_pos[1] : last_pos[1]%image->rows;

        redraw_window(window_name, last_pos, (void*) image);
    }
}

constexpr const char* help_msg = "Loads the image at `input_image_path` into display and allow selection of a division point that will divide and rearrange the image. Left-click to select a point, hold it to keep moving the point. Right-click resets. Middle (scroll-wheel) mouse button will enable/disable sliding mode, which estimates a linear velocity when you hold the image and keeps it after you release.";

int main(int argc, char** argv) {
    std::string image_path;
    bool help_flag = false;
    
    auto cli = lyra::help(help_flag).description(help_msg)
        | lyra::arg(image_path, "input_image_path")
            ("the path of the input image")
            .required();

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

    image = cv::imread(image_path);
    if (image.empty()){printf("Failed to read image in %s.\n", image_path.c_str()); exit(1);}
    redraw_image = image.clone();


    cv::namedWindow(window_name, cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO | cv::WINDOW_GUI_NORMAL);
    cv::setMouseCallback(window_name, mouse_callback, &image);
    cv::imshow(window_name, image);
    auto k = cv::waitKey(10);
    while (k == -1){
        animation_loop(&image);
        k = cv::waitKey(10);
    }
    
    return 0;
}