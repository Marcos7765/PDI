#include<stdint.h>
#include<iostream>
#include<lyra/lyra.hpp>
#include<opencv2/opencv.hpp>

cv::Vec2i rectangle_vertex[2] = {{0,0},{0,0}};
cv::Vec2i points[2] = {{0,0},{0,0}};
uint8_t selected_points_counter = 0;
std::string window_name = "regions";
bool has_draw = false;
bool first_two_done = false;

inline constexpr uint64_t calc_sqr_dist(int x1, int y1, int x2, int y2){
    return (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1);
}

cv::Vec3b calc_negative_color(cv::Vec3b input_color){
    cv::Vec3b res;
    res[0] = 255 - input_color[0];
    res[1] = 255 - input_color[1];
    res[2] = 255 - input_color[2];
    return res;
}

void negativeRect(cv::Mat* image, int x1, int y1, int x2, int y2){
    int low_x, high_x, low_y, high_y;
    if (x1 > x2){low_x = x2; high_x = x1;} else {low_x = x1; high_x = x2;}
    if (y1 > y2){low_y = y2; high_y = y1;} else {low_y = y1; high_y = y2;}
    for (int j = low_y; j <= high_y; j++){
        for (int i = low_x; i <= high_x; i++){
            image->at<cv::Vec3b>(j, i) = calc_negative_color(image->at<cv::Vec3b>(j, i));
        }
    }
}

void redraw_window(std::string win_name, cv::Vec2i new_points[2], cv::Vec2i point_buffer[2],
    uint8_t selected_points_counter, bool* has_draw_ptr, void* image_ptr_opaque){
    cv::Mat* image_ptr = (cv::Mat*) image_ptr_opaque;
    
    if (*has_draw_ptr){
        negativeRect(image_ptr, point_buffer[0][0], point_buffer[0][1], point_buffer[1][0],
            point_buffer[1][1]);
    }

    if (selected_points_counter == 2){
        negativeRect(image_ptr, new_points[0][0], new_points[0][1], new_points[1][0], new_points[1][1]);
        *has_draw_ptr = true;
        point_buffer[0] = new_points[0];
        point_buffer[1] = new_points[1];
        cv::imshow(win_name, *image_ptr);
    } else{
        if (*has_draw_ptr){cv::imshow(win_name, *image_ptr);}
        *has_draw_ptr = false;
    }
}

uint32_t closest_point_index(int x, int y, cv::Vec2i* point_list, uint8_t list_size){
    uint32_t res_index = 0;
    cv::Vec2i curr_point = point_list[0];
    uint64_t min_square_distance = calc_sqr_dist(x,y, curr_point[0], curr_point[1]);
    uint64_t candidate;
    for (auto i = 1; i < list_size; i++){
        curr_point = point_list[i];
        candidate = calc_sqr_dist(x,y, curr_point[0], curr_point[1]);
        if (candidate < min_square_distance){
            min_square_distance = candidate;
            res_index = i;
        }
    }
    return res_index;
}

void mouse_callback(int event, int x, int y, int flags, void* userdata){
    uint32_t index = 0;
    switch (event)
    {
    case cv::EVENT_LBUTTONDOWN:
        
        if (selected_points_counter == 2){
            index = closest_point_index(x, y, points, 2);
        } else {
            index = selected_points_counter;
        }
        points[index][0] = x;
        points[index][1] = y;
        selected_points_counter = selected_points_counter < 2 ? selected_points_counter+1 : 2;
        redraw_window(window_name, points, rectangle_vertex, selected_points_counter, &has_draw, userdata);
        break;
    case cv::EVENT_RBUTTONDOWN:
        index = selected_points_counter;
        selected_points_counter = selected_points_counter == 0 ? 0 : selected_points_counter-1;
        redraw_window(window_name, points, rectangle_vertex, selected_points_counter, &has_draw, userdata);
        break;
    default:
        break;
    }
}

constexpr const char* help_msg = "Loads the image at `input_image_path` into display and allow selection of two oposing vertex of a rectangle that will paint itself by the image's negative within its area. Left-click to select points, right-click to deselect one.";

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

    image = cv::imread(image_path, cv::IMREAD_COLOR);
    if (image.empty()){printf("Failed to read image in %s.\n", image_path.c_str()); exit(1);}


    cv::namedWindow(window_name, cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO | cv::WINDOW_GUI_NORMAL);
    cv::setMouseCallback(window_name, mouse_callback, &image);
    cv::imshow(window_name, image);
    cv::waitKey();
    return 0;
}