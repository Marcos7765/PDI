#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include<iostream>
#include<lyra/lyra.hpp>
#include<string>

void swapQuadrants(cv::Mat& image) {
    cv::Mat tmp, A, B, C, D;

    // se a imagem tiver tamanho impar, recorta a regiao para o maior
    // tamanho par possivel (-2 = 1111...1110)
    image = image(cv::Rect(0, 0, image.cols & -2, image.rows & -2));
    
    int centerX = image.cols / 2;
    int centerY = image.rows / 2;
    
    // rearranja os quadrantes da transformada de Fourier de forma que
    // a origem fique no centro da imagem
    // A B   ->  D C
    // C D       B A
    A = image(cv::Rect(0, 0, centerX, centerY));
    B = image(cv::Rect(centerX, 0, centerX, centerY));
    C = image(cv::Rect(0, centerY, centerX, centerY));
    D = image(cv::Rect(centerX, centerY, centerX, centerY));
    
    // swap quadrants (Top-Left with Bottom-Right)
    A.copyTo(tmp);
    D.copyTo(A);
    tmp.copyTo(D);
    
    // swap quadrant (Top-Right with Bottom-Left)
    C.copyTo(tmp);
    B.copyTo(C);
    tmp.copyTo(B);
}

constexpr const char* help_msg = "Calculates the 2D DFT of the input image";
constexpr const char* default_output_path = "dft_frequency.png";

int main(int argc, char** argv) {
    
    bool help_flag = false;
    bool save_flag = false;
    std::string input_file;
    std::string output_prefix = default_output_path;

    auto cli = lyra::cli() | lyra::help(help_flag).description(help_msg)
        | lyra::arg(input_file, "input_file_path")
            (std::string("the image/filestorage compatible input file path"))
            .required()
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
    
    cv::Mat image, padded, complexImage;
    std::vector<cv::Mat> planos;

    if (input_file.ends_with("ml") || input_file.ends_with("json")){
        cv::FileStorage fs(input_file, cv::FileStorage::READ);
        fs["mat"] >> image;
    } else{
        image = imread(input_file, cv::IMREAD_GRAYSCALE);
    }
    if (image.empty()) {
        std::cout << "Erro abrindo imagem" << argv[1] << std::endl;
        return EXIT_FAILURE;
    }

    // expande a imagem de entrada para o melhor tamanho no qual a DFT pode ser
    // executada, preenchendo com zeros a lateral inferior direita.
    int dft_M = cv::getOptimalDFTSize(image.rows);
    int dft_N = cv::getOptimalDFTSize(image.cols);
    cv::copyMakeBorder(image, padded, 0, dft_M - image.rows, 0,
                        dft_N - image.cols, cv::BORDER_CONSTANT,
                        cv::Scalar::all(0));

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

    // planos[0] : Re(DFT(image)
    // planos[1] : Im(DFT(image)
    cv::split(complexImage, planos);

    // calcula o espectro de magnitude e de fase (em radianos)
    cv::Mat magn, fase;
    cv::cartToPolar(planos[0], planos[1], magn, fase, false);
    cv::normalize(fase, fase, 0, 1, cv::NORM_MINMAX);

    // caso deseje apenas o espectro de magnitude da DFT, use:
    cv::magnitude(planos[0], planos[1], magn);

    std::cout << magn.at<float>(128,128) << "\n";
    std::cout << magn.at<float>(128,120) << "\n";
    std::cout << magn.at<float>(128,119) << "\n";
    std::cout << magn.at<float>(128,136) << "\n";

    // some uma constante para evitar log(0)
    // log(1 + sqrt(Re(DFT(image))^2 + Im(DFT(image))^2))
    magn += cv::Scalar::all(1);

    // calcula o logaritmo da magnitude para exibir
    // com compressao de faixa dinamica
    cv::log(magn, magn);
    cv::normalize(magn, magn, 0, 255, cv::NORM_MINMAX);
    magn.convertTo(magn, CV_8U);

    // exibe as imagens processadas
    cv::imshow("Imagem", image);
    cv::imshow("Espectro de magnitude", magn);
    cv::imshow("Espectro de fase", fase);

    if (save_flag){
        cv::imwrite(output_prefix, magn);
    }

    cv::waitKey();
    return EXIT_SUCCESS;
}