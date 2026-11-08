#include "armor.hpp"

#include <algorithm>
#include <cmath>

namespace train3
{

// 移植自 sp_vision_25_hero/tasks/auto_aim/armor.cpp: Lightbar::Lightbar
Lightbar::Lightbar(const cv::RotatedRect & rotated_rect, std::size_t id)
: id(id), rotated_rect(rotated_rect)
{
  std::vector<cv::Point2f> corners(4);
  rotated_rect.points(&corners[0]);
  // 4 个角点按 y 排序，前两个是上边，后两个是下边
  std::sort(corners.begin(), corners.end(), [](const cv::Point2f & a, const cv::Point2f & b) {
    return a.y < b.y;
  });

  center = rotated_rect.center;
  top = (corners[0] + corners[1]) / 2;
  bottom = (corners[2] + corners[3]) / 2;
  top2bottom = bottom - top;

  points.emplace_back(top);
  points.emplace_back(bottom);

  width = cv::norm(corners[0] - corners[1]);
  angle = std::atan2(top2bottom.y, top2bottom.x);
  angle_error = std::abs(angle - CV_PI / 2);
  length = cv::norm(top2bottom);
  ratio = width > 1e-6 ? length / width : 0.0;
}

// 移植自 sp_vision_25_hero/tasks/auto_aim/armor.cpp: Armor::Armor(left, right)
Armor::Armor(const Lightbar & left, const Lightbar & right) : left(left), right(right)
{
  color = left.color;
  center = (left.center + right.center) / 2;

  // 装甲板的四个角点 = 两根灯条的上下端点
  points.emplace_back(left.top);
  points.emplace_back(right.top);
  points.emplace_back(right.bottom);
  points.emplace_back(left.bottom);

  auto left2right = right.center - left.center;
  auto width = cv::norm(left2right);
  auto max_lightbar_length = std::max(left.length, right.length);
  auto min_lightbar_length = std::min(left.length, right.length);
  ratio = max_lightbar_length > 1e-6 ? width / max_lightbar_length : 0.0;
  side_ratio = min_lightbar_length > 1e-6 ? max_lightbar_length / min_lightbar_length : 99.0;

  auto roll = std::atan2(left2right.y, left2right.x);
  auto left_rectangular_error = std::abs(left.angle - roll - CV_PI / 2);
  auto right_rectangular_error = std::abs(right.angle - roll - CV_PI / 2);
  rectangular_error = std::max(left_rectangular_error, right_rectangular_error);
}

}  // namespace train3