// 培训三 · 任务一
//   1. 读取装甲板图片，转 HSV；
//   2. 分离 H、S、V 三个通道，分别弹出三个窗口展示；
//   3. 写阈值，用 inRange 提取红色装甲二值掩膜；
//   4. 把掩膜保存为 red_mask.png。
//
// 用法：
//   task1_hsv --image assets/armor.jpg --out outputs          （只保存图片）
//   task1_hsv --image assets/armor.jpg --out outputs --show   （同时弹出窗口）

#include <opencv2/opencv.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "detector.hpp"

namespace fs = std::filesystem;

namespace
{

void usage()
{
  std::cout << "usage: task1_hsv --image <armor image> [--out <dir>] [--show]\n"
               "                [--h1 0 10] [--h2 170 180] [--s 100] [--v 70]\n";
}

// 保存图片：OpenCV 的 imwrite 不支持中文路径，这里用 imencode + ofstream
bool write_image(const fs::path & path, const cv::Mat & img)
{
  auto ext = path.extension().string();
  if (ext.empty()) ext = ".png";
  std::vector<uchar> buf;
  if (!cv::imencode(ext, img, buf)) return false;
  std::ofstream ofs(path, std::ios::binary);
  ofs.write(reinterpret_cast<const char *>(buf.data()), static_cast<std::streamsize>(buf.size()));
  return ofs.good();
}

// 读图片：同样绕开中文路径问题
cv::Mat read_image(const fs::path & path)
{
  std::ifstream ifs(path, std::ios::binary);
  std::vector<uchar> buf((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  if (buf.empty()) return {};
  return cv::imdecode(buf, cv::IMREAD_COLOR);
}

// 给图加上标题，方便截图/展示（单通道自动转成三通道）
cv::Mat titled(const cv::Mat & img, const std::string & title)
{
  cv::Mat color;
  if (img.channels() == 1)
    cv::cvtColor(img, color, cv::COLOR_GRAY2BGR);
  else
    color = img.clone();

  cv::Mat canvas(color.rows + 34, color.cols, CV_8UC3, cv::Scalar(32, 32, 32));
  color.copyTo(canvas(cv::Rect(0, 34, color.cols, color.rows)));
  cv::putText(canvas, title, {10, 24}, cv::FONT_HERSHEY_SIMPLEX, 0.7, {255, 255, 255}, 2);
  return canvas;
}

// 把若干窗口按 3 列平铺显示：上排 H/S/V，下排掩膜与叠加图
void show_windows(
  const cv::Size & src_size, const std::vector<std::pair<std::string, cv::Mat>> & wins)
{
  const int w = 430;
  const int h = std::max(1, cvRound(static_cast<double>(w) * src_size.height / src_size.width));
  const int gap = 16;
  for (std::size_t i = 0; i < wins.size(); ++i) {
    // 先把图缩到统一大小，再用 AUTOSIZE 显示：窗口大小就等于图片大小，且不带工具条
    cv::Mat small;
    cv::resize(wins[i].second, small, {w, h}, 0, 0, cv::INTER_AREA);
    cv::namedWindow(wins[i].first, cv::WINDOW_AUTOSIZE | cv::WINDOW_GUI_NORMAL);
    cv::imshow(wins[i].first, small);
    int col = static_cast<int>(i % 3), row = static_cast<int>(i / 3);
    cv::moveWindow(wins[i].first, 20 + col * (w + gap), 40 + row * (h + 44));
  }
}

}  // namespace

int main(int argc, char ** argv)
{
  std::string image_path = "assets/armor.jpg";
  std::string out_dir = "outputs";
  bool show = false;
  train3::DetectConfig cfg;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    auto next = [&](int offset) -> std::string {
      return (i + offset < argc) ? std::string(argv[i + offset]) : std::string();
    };
    if (arg == "--image" && i + 1 < argc) {
      image_path = next(1);
      ++i;
    } else if (arg == "--out" && i + 1 < argc) {
      out_dir = next(1);
      ++i;
    } else if (arg == "--show") {
      show = true;
    } else if (arg == "--h1" && i + 2 < argc) {
      cfg.h_low_min = std::stoi(next(1));
      cfg.h_low_max = std::stoi(next(2));
      i += 2;
    } else if (arg == "--h2" && i + 2 < argc) {
      cfg.h_high_min = std::stoi(next(1));
      cfg.h_high_max = std::stoi(next(2));
      i += 2;
    } else if (arg == "--s" && i + 1 < argc) {
      cfg.s_min = std::stoi(next(1));
      ++i;
    } else if (arg == "--v" && i + 1 < argc) {
      cfg.v_min = std::stoi(next(1));
      ++i;
    } else if (arg == "--help" || arg == "-h") {
      usage();
      return 0;
    }
  }

  fs::create_directories(out_dir);

  cv::Mat bgr_img = read_image(image_path);
  if (bgr_img.empty()) {
    std::cerr << "[task1] cannot read image: " << image_path << "\n";
    return 1;
  }
  std::cout << "[task1] image " << image_path << " " << bgr_img.cols << "x" << bgr_img.rows << "\n";

  // 1. BGR -> HSV
  cv::Mat hsv_img;
  cv::cvtColor(bgr_img, hsv_img, cv::COLOR_BGR2HSV);

  // 2. 分离 H、S、V 三个通道
  std::vector<cv::Mat> channels;
  cv::split(hsv_img, channels);  // 0:H 1:S 2:V
  const cv::Mat & h_channel = channels[0];
  const cv::Mat & s_channel = channels[1];
  const cv::Mat & v_channel = channels[2];

  // 2.1 三个通道分别保存成图片（等价于三个窗口里的内容）
  write_image(fs::path(out_dir) / "H_channel.png", titled(h_channel, "H channel (hue)"));
  write_image(fs::path(out_dir) / "S_channel.png", titled(s_channel, "S channel (saturation)"));
  write_image(fs::path(out_dir) / "V_channel.png", titled(v_channel, "V channel (brightness)"));

  // 3. 写阈值，inRange 提取红色装甲二值掩膜
  cv::Mat mask_low, mask_high, red_mask;
  cv::inRange(
    hsv_img, cv::Scalar(cfg.h_low_min, cfg.s_min, cfg.v_min), cv::Scalar(cfg.h_low_max, 255, 255),
    mask_low);
  cv::inRange(
    hsv_img, cv::Scalar(cfg.h_high_min, cfg.s_min, cfg.v_min),
    cv::Scalar(cfg.h_high_max, 255, 255), mask_high);
  cv::bitwise_or(mask_low, mask_high, red_mask);  // 红色跨越 0 度，两段色相合并

  if (cfg.morph_kernel > 0) {
    auto kernel = cv::getStructuringElement(
      cv::MORPH_RECT, cv::Size(cfg.morph_kernel, cfg.morph_kernel));
    cv::morphologyEx(red_mask, red_mask, cv::MORPH_CLOSE, kernel);
  }

  std::cout << "[task1] red threshold: H[" << cfg.h_low_min << "," << cfg.h_low_max << "] U ["
            << cfg.h_high_min << "," << cfg.h_high_max << "]  S>=" << cfg.s_min
            << "  V>=" << cfg.v_min << "\n";

  // 4. 保存掩膜
  const auto mask_path = fs::path(out_dir) / "red_mask.png";
  if (!write_image(mask_path, red_mask)) {
    std::cerr << "[task1] failed to save " << mask_path.string() << "\n";
    return 1;
  }
  std::cout << "[task1] saved " << mask_path.string() << "\n";

  // 额外：掩膜叠加到原图，直观看到提取结果
  cv::Mat overlay = bgr_img.clone();
  overlay.setTo(cv::Scalar(0, 255, 255), red_mask);
  cv::Mat blended;
  cv::addWeighted(bgr_img, 0.55, overlay, 0.45, 0, blended);
  write_image(fs::path(out_dir) / "red_mask_overlay.png", blended);

  // 汇总图：上排 原图/H/S，下排 V/掩膜/叠加，方便写文档和传 GitHub
  cv::Mat row1, row2, montage;
  cv::hconcat(
    std::vector<cv::Mat>{
      titled(bgr_img, "original"), titled(h_channel, "H channel"), titled(s_channel, "S channel")},
    row1);
  cv::hconcat(
    std::vector<cv::Mat>{
      titled(v_channel, "V channel"), titled(red_mask, "red mask (inRange)"),
      titled(blended, "mask overlay")},
    row2);
  cv::vconcat(row1, row2, montage);
  write_image(fs::path(out_dir) / "task1_result.png", montage);
  std::cout << "[task1] saved " << (fs::path(out_dir) / "task1_result.png").string() << "\n";

  // 5. 弹出窗口：H/S/V 三个通道 + 掩膜 + 叠加
  if (show) {
    show_windows(
      bgr_img.size(),
      {{"H channel", h_channel},
       {"S channel", s_channel},
       {"V channel", v_channel},
       {"red mask", red_mask},
       {"mask overlay", blended}});
    std::cout << "[task1] press any key in the image windows to exit...\n";
    cv::waitKey(0);
  }

  return 0;
}