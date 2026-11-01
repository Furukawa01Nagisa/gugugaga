#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
    cv::Mat img = cv::imread("test.jpg");

    if (img.empty()) {
        std::cout << "错误：无法读取图片，请检查图片是否存在！" << std::endl;
        return -1;
    }

    cv::imshow("My OpenCV Window", img);
    std::cout << "按任意键关闭图片窗口..." << std::endl;

    cv::waitKey(0); 
    return 0;
}