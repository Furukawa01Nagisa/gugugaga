#include <filesystem>
#include <iostream>
#include <string>

#include <opencv2/opencv.hpp>

namespace {

// 根据可执行文件的实际位置定位 test.jpg，避免依赖启动时的工作目录。
std::filesystem::path defaultImagePath(const char* argv0) {
    if (argv0 == nullptr || *argv0 == '\0') {
        return std::filesystem::path("test.jpg");
    }

    std::error_code ec;
    const std::filesystem::path exePath = std::filesystem::absolute(argv0, ec);
    if (ec) {
        return std::filesystem::path("test.jpg");
    }
    return exePath.parent_path() / "test.jpg";
}

}  // namespace

int main(int argc, char** argv) {
    // 未指定参数时，使用可执行文件旁边的 test.jpg；也可以传入任意图片路径。
    const std::filesystem::path imagePath =
        (argc > 1) ? std::filesystem::path(argv[1]) : defaultImagePath(argv[0]);

    cv::Mat img = cv::imread(imagePath.string());
    if (img.empty()) {
        std::cerr << "错误：无法读取图片 \"" << imagePath.string()
                  << "\"，请检查路径是否正确！" << std::endl;
        std::cerr << "用法: " << argv[0] << " [图片路径]" << std::endl;
        return -1;
    }

    cv::imshow("My OpenCV Window", img);
    std::cout << "按任意键关闭图片窗口..." << std::endl;

    cv::waitKey(0);
    return 0;
}
