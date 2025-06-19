#include<stdint.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <random>
#include <lyra/lyra.hpp>

#define CELL_STRIDE 3
#define POINT_RADIUS 3


std::mt19937 prng_gen;
std::uniform_int_distribution<int> uni_dist(0, CELL_STRIDE);
std::normal_distribution<float> normal_dist(0., 1.);

void paint_point(cv::Mat img_inout, int x, int y, int px_radius, cv::Vec3b color){
    cv::circle(img_inout, cv::Point(y, x), px_radius, color, cv::FILLED, cv::LINE_AA);
}

typedef struct paint_point_args {
    int x;
    int y;
    int px_radius;
    cv::Vec3b color;
} point_args;

point_args gaussian_sample_point(cv::Mat img_in, int x_start, int y_start, float radius_std, int radius_mean,
    int side_size){

    int height = img_in.rows;
    int width = img_in.cols;
    
    std::uniform_int_distribution<int>::param_type offset_range(1, side_size);
    //wrap-around pra celulas que ultrapassam a imagem
    int effective_size_x = std::min(height - x_start, side_size);
    int effective_size_y = std::min(width - y_start, side_size);
    
    int offset_x = uni_dist(prng_gen, offset_range) -1;
    int offset_y = uni_dist(prng_gen, offset_range) -1;
    int pos_x = offset_x + x_start;
    int pos_y = offset_y + y_start;
    
    int color_pos_x = offset_x%effective_size_x + x_start;
    int color_pos_y = offset_y%effective_size_y + y_start;

    std::normal_distribution<float>::param_type radius_params(radius_mean, radius_std);
    int point_radius = std::max<float>(std::round(normal_dist(prng_gen, radius_params)), 0);
    return {pos_x, pos_y, point_radius, img_in.at<cv::Vec3b>(color_pos_x, color_pos_y)};
}

void canny_to_points(cv::Mat reference_img, double threshold_start, double threshold_size, int cell_size,
    int radius_mean, float radius_std, cv::Mat &target_img){
    
    cv::Mat edge_img;
    cv::Canny(reference_img, edge_img, threshold_start, threshold_start+threshold_size);
    
    int width = reference_img.cols;
    int height = reference_img.rows;

    int x_points = height/cell_size;// + (height%cell_size > 0);
    int y_points = width/cell_size;// + (width%cell_size > 0);

    std::vector<point_args> point_list;

    for (uint i = 0; i < x_points; i++) {
        int eff_size_x = std::min<int>(height - i*cell_size, cell_size);
        for (uint j = 0; j < y_points; j++) {
            int eff_size_y = std::min<int>(width - j*cell_size, cell_size);
            cv::Rect cell_rect(j*cell_size, i*cell_size, eff_size_x, eff_size_y);
            cv::Mat cell_ref = edge_img(cell_rect);
            std::vector<cv::Point2i> edge_points;
            cv::findNonZero(cell_ref, edge_points);
            
            if (edge_points.empty()){continue;}
            
            int sample_i = uni_dist(prng_gen, std::uniform_int_distribution<int>::param_type(0, edge_points.size()-1));
            cv::Point2i chosen_point = edge_points[sample_i];
        
            auto p_params = gaussian_sample_point(reference_img, i*cell_size, j*cell_size, radius_std, radius_mean,
                1);
            point_list.push_back(p_params);
        }
    }

    std::shuffle(point_list.begin(), point_list.end(), prng_gen);
    for (auto p_params : point_list){
        paint_point(target_img, p_params.x, p_params.y, p_params.px_radius, p_params.color);
    }
}

void canny_sweep(cv::Mat reference_img, double threshold_start, double threshold_size, int total_canny, int cell_start,
    int cell_incr, int radius_start, int radius_incr, float radius_stddev_rel, cv::Mat &target_img){
    for (int i = 0; i < total_canny; i++){
        int rad_mean = std::max(radius_start+i*radius_incr, 1);
        int cell_size = std::max(cell_start+i*cell_incr, 1);
        canny_to_points(reference_img, threshold_start+i*threshold_size, threshold_size, cell_start+i*cell_incr,
            rad_mean, rad_mean*radius_stddev_rel, target_img);
    }
}

constexpr const char* help_msg = "Applies a pointillism-like effect to a copy of the input image";
constexpr const char* default_output_path = "pontilhismo.png";

int main(int argc, char** argv) {
    std::vector<int> yrange;
    std::vector<int> xrange;

    cv::Mat image, points;

    std::string input_file;
    std::string output_prefix;
    bool help_flag = false;
    bool save_flag = false;
    bool seed_flag = false;

    float rad_var = 0.6;
    int rad_mean = POINT_RADIUS;
    int cell_stride = CELL_STRIDE;
    int width, height;
    unsigned long seed_value = 7;
    //int x, y;


    auto cli = lyra::cli() | lyra::help(help_flag).description(help_msg)
        | lyra::arg(input_file, "input_file_path")
            (std::string("the image input file path"))
            .required()
        | lyra::arg(cell_stride, "cell_side_size")
            (std::string("side size of every cell of the regular grid. defaults to ") + std::to_string(cell_stride) + ".")
            .optional()
        | lyra::arg(rad_mean, "dot_radius_mean")
            (std::string("mean dot radius for gaussian sampling. defaults to ") + std::to_string(rad_mean) + ". Integer")
            .optional()
        | lyra::arg(rad_var, "dot_radius_std_deviation")
            (std::string("dot radius standard deviation for gaussian sampling. defaults to ")
                + std::to_string(rad_var) + ". float")
            .optional()
        | (lyra::group()
            | lyra::opt(seed_flag)
                ["--seed"]
                ("assures deterministic behaviour by seeding the PRNG generator.")
                .required()
            | lyra::arg(seed_value, "seed_val")
                ("the number to seed the PRNG. defaults to " +std::to_string(seed_value) +".")
                .required()
            )
        | (lyra::group()
            | lyra::opt(save_flag)
                ["-s"]["--save"]
                ("saves extracted image as <output_path>")
                .required()
            | lyra::arg(output_prefix, "output_path")
                (std::string("specifies the output_path when saving. if not provided, defaults to ")
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

    image = cv::imread(input_file, cv::IMREAD_COLOR);
    if (image.empty()) {
    std::cout << "Erro abrindo imagem" << argv[1] << std::endl;
    return EXIT_FAILURE;
    }

    image = cv::imread(argv[1], cv::IMREAD_COLOR);

    if (!seed_flag){
        seed_value = std::random_device()();
        printf("The seed used was %lu\n", seed_value);
    }
    prng_gen.seed(seed_value);

    if (image.empty()) {
        std::cout << "Could not open or find the image" << std::endl;
        return -1;
    }

    //to the incomplete side to be random aswell, -1 -> x and y inversion, 0 -> x inversion, 1 -> y inversion, 2 -> nothing
    int flip_mode = uni_dist(prng_gen, std::uniform_int_distribution<int>::param_type(-1, 2));

    if (flip_mode != 2){
        cv::flip(image, image, flip_mode);
    }

    width = image.cols;
    height = image.rows;

    int x_points = height/cell_stride + (height%cell_stride > 0);
    int y_points = width/cell_stride + (width%cell_stride > 0);
    xrange.resize(x_points);
    yrange.resize(y_points);

    for (uint i = 0; i < x_points; i++) {
        xrange[i] = i*cell_stride;
    }

    for (uint i = 0; i < y_points; i++) {
        yrange[i] = i*cell_stride;
    }

    points = cv::Mat(x_points*cell_stride, y_points*cell_stride, CV_8UC3, cv::Vec3b(255,255,255));

    std::shuffle(xrange.begin(), xrange.end(), prng_gen);

    for (auto x_start : xrange) {
        std::shuffle(yrange.begin(), yrange.end(), prng_gen);
        for (auto y_start : yrange) {
            auto point = gaussian_sample_point(image, x_start, y_start, rad_var, rad_mean, cell_stride);
            paint_point(points, point.x, point.y, point.px_radius, point.color);
        }
    }


    //it is hardcoded from a fine tune with the baez image, but i don't feel like passing all those values through the
    //cli is a good idea (nor i managed to find good relation with the input values)
    
    //fine-tuned
    //canny_sweep(image, 25., 25., 4, 5, -1, 4, -1, 0.2, points);
    
    //"rationalized" from fine-tune
    int canny_total = 4;
    int cell_incr = -(cell_stride/(canny_total+1));
    int radius_start = (rad_mean*canny_total/(canny_total+2));
    int radius_incr = -std::max((radius_start/(canny_total+1)), 1);
    //canny_sweep(image, 25., 25., canny_total, cell_stride, cell_incr, radius_start, radius_incr, 0.2, points);
    
    //alternative with fixed cell_sizes, too much overlap
    //canny_sweep(image, 25., 25., 4, 5, -1, radius_start, radius_incr, 0.2, points); //mt overlap

    if (flip_mode != 2){
        cv::flip(points, points, flip_mode);
    }
    
    if (save_flag){
        cv::imwrite(output_prefix, points(cv::Rect(0, 0, width, height)));
    } else{
        cv::imshow("Points", points(cv::Rect(0, 0, width, height)));
    }
    cv::waitKey();
    return 0;
}