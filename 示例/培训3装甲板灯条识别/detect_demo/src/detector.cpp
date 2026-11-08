#include "detector.hpp"

#include <algorithm>
#include <cmath>

// OpenCV5 把 minAreaRect 一类二维几何函数挪到了 opencv2/geometry 下，
// OpenCV4 及以前在 opencv2/imgproc.hpp 里，这里兼容两种版本。
#if __has_include(<opencv2/geometry/2d.hpp>)
#include <opencv2/geometry/2d.hpp>
#endif

namespace train3
{

Detector::Detector(const DetectConfig & config) : config_(config)
{
  max_angle_error_ = config_.max_angle_error_deg / 57.3;              // degree -> rad
  max_rectangular_error_ = config_.max_rectangular_error_deg / 57.3;  // degree -> rad
}

// 二值掩膜：HSV 红色 inRange 或 灰度阈值
cv::Mat Detector::mask(const cv::Mat & bgr_img) const
{
  cv::Mat binary;
  if (config_.mask_mode == MaskMode::gray) {
    cv::Mat gray_img;
    cv::cvtColor(bgr_img, gray_img, cv::COLOR_BGR2GRAY);
    cv::threshold(gray_img, binary, config_.gray_thresh, 255, cv::THRESH_BINARY);
    return binary;
  }

  cv::Mat hsv_img;
  cv::cvtColor(bgr_img, hsv_img, cv::COLOR_BGR2HSV);

  // 红色在色相环上跨越 0 度，所以要取两段再合并
  cv::Mat mask_low, mask_high;
  cv::inRange(
    hsv_img, cv::Scalar(config_.h_low_min, config_.s_min, config_.v_min),
    cv::Scalar(config_.h_low_max, 255, 255), mask_low);
  cv::inRange(
    hsv_img, cv::Scalar(config_.h_high_min, config_.s_min, config_.v_min),
    cv::Scalar(config_.h_high_max, 255, 255), mask_high);
  cv::bitwise_or(mask_low, mask_high, binary);

  // 轻微闭运算，填补灯条中间被反光/过曝打断的空洞
  if (config_.morph_kernel > 0) {
    auto kernel = cv::getStructuringElement(
      cv::MORPH_RECT, cv::Size(config_.morph_kernel, config_.morph_kernel));
    cv::morphologyEx(binary, binary, cv::MORPH_CLOSE, kernel);
  }
  return binary;
}

// 主流程，对应 sp_vision_25_hero/tasks/auto_aim/detector.cpp: Detector::detect
void Detector::detect(
  const cv::Mat & bgr_img, std::vector<Lightbar> & lightbars, std::vector<Armor> & armors) const
{
  lightbars.clear();
  armors.clear();

  auto binary_img = mask(bgr_img);

  // 取外轮廓
  std::vector<std::vector<cv::Point>> contours;
  cv::findContours(binary_img, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

  // 轮廓 -> 最大外接矩形 -> 灯条
  std::size_t lightbar_id = 0;
  for (const auto & contour : contours) {
    auto rotated_rect = cv::minAreaRect(contour);
    Lightbar lightbar(rotated_rect, lightbar_id);

    if (!check_geometry(lightbar)) continue;

    lightbar.color = get_color(bgr_img, contour);
    lightbars.emplace_back(lightbar);
    lightbar_id += 1;
  }

  // 灯条从左到右排序，方便左右配对
  std::sort(lightbars.begin(), lightbars.end(), [](const Lightbar & a, const Lightbar & b) {
    return a.center.x < b.center.x;
  });
  for (std::size_t i = 0; i < lightbars.size(); ++i) lightbars[i].id = i;

  // 两两配对成装甲板
  for (auto left = lightbars.begin(); left != lightbars.end(); ++left) {
    for (auto right = std::next(left); right != lightbars.end(); ++right) {
      if (left->color != right->color) continue;

      Armor armor(*left, *right);
      if (!check_geometry(armor)) continue;
      armors.emplace_back(armor);
    }
  }

  // 同一个灯条被两块装甲板共用时，保留 patch 更小（更可能是正对视角）的那块
  std::vector<bool> duplicated(armors.size(), false);
  for (std::size_t i = 0; i < armors.size(); ++i) {
    for (std::size_t j = i + 1; j < armors.size(); ++j) {
      if (duplicated[i] && duplicated[j]) continue;
      const auto & a1 = armors[i];
      const auto & a2 = armors[j];
      if (
        a1.left.id != a2.left.id && a1.left.id != a2.right.id && a1.right.id != a2.left.id &&
        a1.right.id != a2.right.id)
        continue;

      auto area = [](const Armor & a) {
        auto w = cv::norm(a.right.center - a.left.center);
        return w * std::max(a.left.length, a.right.length);
      };
      if (a1.left.id == a2.left.id || a1.right.id == a2.right.id) {
        if (area(a1) < area(a2))
          duplicated[j] = true;
        else
          duplicated[i] = true;
      } else {
        // 两块板首尾共用灯条，保留灯条更长（更可信）的那块
        if (std::max(a1.left.length, a1.right.length) < std::max(a2.left.length, a2.right.length))
          duplicated[i] = true;
        else
          duplicated[j] = true;
      }
    }
  }

  std::vector<Armor> kept;
  for (std::size_t i = 0; i < armors.size(); ++i)
    if (!duplicated[i]) kept.emplace_back(armors[i]);
  armors.swap(kept);
}

// 移植自 sp_vision_25_hero/tasks/auto_aim/detector.cpp: check_geometry(Lightbar)
bool Detector::check_geometry(const Lightbar & lightbar) const
{
  auto angle_ok = lightbar.angle_error < max_angle_error_;
  auto ratio_ok =
    lightbar.ratio > config_.min_lightbar_ratio && lightbar.ratio < config_.max_lightbar_ratio;
  auto length_ok = lightbar.length > config_.min_lightbar_length;
  return angle_ok && ratio_ok && length_ok;
}

// 移植自 sp_vision_25_hero/tasks/auto_aim/detector.cpp: check_geometry(Armor)
bool Detector::check_geometry(const Armor & armor) const
{
  auto ratio_ok = armor.ratio > config_.min_armor_ratio && armor.ratio < config_.max_armor_ratio;
  auto side_ratio_ok = armor.side_ratio < config_.max_side_ratio;
  auto rectangular_error_ok = armor.rectangular_error < max_rectangular_error_;
  return ratio_ok && side_ratio_ok && rectangular_error_ok;
}

// 移植自 sp_vision_25_hero/tasks/auto_aim/detector.cpp: get_color
// 统计轮廓上像素的 B、R 通道之和判断红蓝
Color Detector::get_color(const cv::Mat & bgr_img, const std::vector<cv::Point> & contour) const
{
  long red_sum = 0, blue_sum = 0;
  for (const auto & point : contour) {
    if (point.x < 0 || point.y < 0 || point.x >= bgr_img.cols || point.y >= bgr_img.rows) continue;
    const auto & bgr = bgr_img.at<cv::Vec3b>(point);
    blue_sum += bgr[0];
    red_sum += bgr[2];
  }
  return blue_sum > red_sum ? Color::blue : Color::red;
}

}  // namespace train3