#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>
#include <lyra/lyra.hpp>
#include <string>

void swapQuadrants(cv::Mat& image) {
  cv::Mat tmp, A, B, C, D;

  image = image(cv::Rect(0, 0, image.cols & -2, image.rows & -2));

  int centerX = image.cols / 2;
  int centerY = image.rows / 2;

  A = image(cv::Rect(0, 0, centerX, centerY));
  B = image(cv::Rect(centerX, 0, centerX, centerY));
  C = image(cv::Rect(0, centerY, centerX, centerY));
  D = image(cv::Rect(centerX, centerY, centerX, centerY));

  A.copyTo(tmp);
  D.copyTo(A);
  tmp.copyTo(D);

  C.copyTo(tmp);
  B.copyTo(C);
  tmp.copyTo(B);
}



auto butterworth_generator(float cut, float low_freq, float high_freq, int centerX, int centerY, int order = 1){
    return [=](float &point, const int * pos) -> void {
        float D = (pos[0] - centerY)*(pos[0] - centerY) + (pos[1] - centerX)*(pos[1] - centerX);
        if (D == 0){ //just to prevent nan from a division by zero
            point = low_freq;
        } else{
            point = low_freq + (high_freq - low_freq)/(1.+std::pow(cut/D, 2*order));
        }
    };
}

void homomorphic_filter(const cv::Mat &image_ref, cv::Mat &filter_out, float low_freq, float high_freq, float cut_rate, int order){
    cv::Mat_<float> filter2D(image_ref.rows, image_ref.cols);
    int centerX = filter2D.cols/2;
    int centerY = filter2D.rows/2;
    int dist_max = std::pow(
        std::min(filter2D.rows - centerY, filter2D.cols - centerX), 2);

    float cut_frequency = cut_rate <= 1. ? dist_max*cut_rate : cut_rate;
    auto filter_func = butterworth_generator(cut_frequency, low_freq, high_freq, centerX, centerY, order);

    printf("Effective cutoff frequency: %f\n", cut_frequency);

    filter2D.forEach(filter_func);

    cv::Mat planes[] = {cv::Mat_<float>(filter2D), cv::Mat::zeros(filter2D.size(), CV_32F)};
    cv::merge(planes, 2, filter_out); //limp
}

constexpr const char* help_msg = "Applies a configurable butterworth highpass filter to the input image";
constexpr const char* default_output_path = "filtered.png";

int main(int argc, char** argv) {
  cv::Mat image, padded, complexImage;
  std::vector<cv::Mat> planos;

    bool help_flag = false;
    bool save_flag = false;
    std::string input_file;
    std::string output_prefix = default_output_path;
    float low_freq = 0.0;
    float high_freq = 1.;
    float cut_freq = 4.;
    int butter_order = 1;
    


    auto cli = lyra::cli() | lyra::help(help_flag).description(help_msg)
        | lyra::arg(input_file, "input_file_path")
            (std::string("the image input file path"))
            .required()
        | lyra::arg(low_freq, "minimum_gain")
            (std::string("minimum gain of the filter, defaults to ") + std::to_string(low_freq) + ". float")
            .optional()
        | lyra::arg(high_freq, "maximum_gain")
            (std::string("maximum gain of the filter, defaults to ") + std::to_string(high_freq) + ". float")
            .optional()
        | lyra::arg(cut_freq, "cut_frequency")
            (std::string("filter relative cut frequency. if cut_frequency is greater than 1, it is used as an absolute value. defaults to ")
                + std::to_string(cut_freq) + ". float")
            .optional()
        | lyra::arg(butter_order, "butterworth_order")
            (std::string("butterworth filter parameter for order, defaults to ") + std::to_string(butter_order))
            .optional()
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

  //image = imread(input_file, cv::IMREAD_GRAYSCALE);
  image = cv::imread(input_file, cv::IMREAD_COLOR);
  if (image.empty()) {
    std::cout << "Erro abrindo imagem" << argv[1] << std::endl;
    return EXIT_FAILURE;
  }
  cv::imshow("imagem base", image);
  cv::cvtColor(image, image, cv::COLOR_BGR2HSV);
  std::vector<cv::Mat> base_image_hsv_channels;
  cv::split(image, base_image_hsv_channels);

  image = base_image_hsv_channels[2].clone();
  cv::imshow("V original", image);

  // expande a imagem de entrada para o melhor tamanho no qual a DFT pode ser
  // executada, preenchendo com zeros a lateral inferior direita.
  int dft_M = cv::getOptimalDFTSize(image.rows);
  int dft_N = cv::getOptimalDFTSize(image.cols); 
  cv::copyMakeBorder(image, padded, 0, dft_M - image.rows, 0, dft_N - image.cols, cv::BORDER_CONSTANT, cv::Scalar::all(0));

  // prepara a matriz complexa para ser preenchida
  // primeiro a parte real, contendo a imagem de entrada
  planos.push_back(cv::Mat_<float>(padded)); 
  // depois a parte imaginaria com valores nulos
  planos.push_back(cv::Mat::zeros(padded.size(), CV_32F));

  // combina os planos em uma unica estrutura de dados complexa
  cv::merge(planos, complexImage);  

  // calcula a DFT
  cv::dft(complexImage, complexImage); 
  swapQuadrants(complexImage);

  cv::Mat filter;
  homomorphic_filter(complexImage, filter, low_freq, high_freq, cut_freq, butter_order);

  std::vector<cv::Mat> filtro_img; 
  cv::split(filter, filtro_img);

  cv::imshow("filtro", filtro_img[0]);

  cv::mulSpectrums(complexImage, filter, complexImage, 0);

  // calcula a DFT inversa
  swapQuadrants(complexImage);
  cv::idft(complexImage, complexImage);

  cv::split(complexImage, planos);

  // recorta a imagem filtrada para o tamanho original
  // selecionando a regiao de interesse (roi)
  cv::Rect roi(0, 0, image.cols, image.rows);
  cv::Mat result = planos[0](roi);

  cv::normalize(result, result, 0, 255, cv::NORM_MINMAX, CV_8UC1);

  cv::imshow("V pos filtro", result);

  base_image_hsv_channels[2] = result;
  
  cv::merge(base_image_hsv_channels, result);

  cv::Mat final_img;
  cv::cvtColor(result, final_img, cv::COLOR_HSV2BGR);

  cv::imshow("imagem final", final_img);
  if (save_flag){
    cv::imwrite(output_prefix, final_img);
  }

  cv::waitKey();
  return EXIT_SUCCESS;
}