#include <opencv2/opencv.hpp>
#include <iostream>

// 支持中文路径读取
cv::Mat imreadUnicode(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return cv::Mat();
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<uchar> buf(len);
    fread(buf.data(), 1, len, f);
    fclose(f);
    return cv::imdecode(buf, cv::IMREAD_COLOR);
}

int main() {
    std::string imgPath = "armor.jpg";
    cv::Mat img = imreadUnicode(imgPath);
    
    if (img.empty()) {
        std::cerr << "无法读取图片，请确认 " << imgPath << " 是否在当前目录！\n";
        return -1;
    }

    cv::Mat hsv, mask1, mask2, mask;
    cv::cvtColor(img, hsv, cv::COLOR_BGR2HSV);

    // 红色在 HSV 色环中跨越 0 度，需要分两段提取
    cv::inRange(hsv, cv::Scalar(0, 100, 70), cv::Scalar(10, 255, 255), mask1);
    cv::inRange(hsv, cv::Scalar(170, 100, 70), cv::Scalar(180, 255, 255), mask2);
    cv::bitwise_or(mask1, mask2, mask);

    // 闭运算填补灯条内部空洞
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);

    cv::imwrite("red_mask.png", mask);
    std::cout << "掩膜已保存为 red_mask.png（请检查灯条是否发白、背景是否发黑）\n";

    cv::imshow("Original", img);
    cv::imshow("Red Mask", mask);
    cv::waitKey(0);
    return 0;
}