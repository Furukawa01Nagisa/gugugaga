#ifndef TRAIN3__ARMOR_HPP
#define TRAIN3__ARMOR_HPP

// 本文件的几何定义与构造逻辑移植自 sp_vision_25_hero 仓库：
//   tasks/auto_aim/armor.hpp
//   tasks/auto_aim/armor.cpp  （传统构造函数 Lightbar::Lightbar / Armor::Armor）
// 只保留培训三需要的字段，去掉了 Eigen/弹道/跟踪等无关部分。

#include <opencv2/opencv.hpp>

#include <cstddef>
#include <vector>

namespace train3
{

enum Color
{
  red,
  blue
};

// 灯条：由 minAreaRect（最大外接旋转矩形）构造
struct Lightbar
{
  std::size_t id;
  Color color;

  cv::Point2f center, top, bottom, top2bottom;
  std::vector<cv::Point2f> points;  // {top, bottom}

  double angle;        // top -> bottom 的方向角
  double angle_error;  // 与竖直方向 (pi/2) 的偏差
  double length;       // 灯条长度
  double width;        // 灯条宽度
  double ratio;        // length / width

  cv::RotatedRect rotated_rect;  // 最大外接旋转矩形

  Lightbar() : id(0), color(red), angle(0), angle_error(0), length(0), width(0), ratio(0) {}
  Lightbar(const cv::RotatedRect & rotated_rect, std::size_t id);
};

// 装甲板：由左右两根灯条配对构造
struct Armor
{
  Color color;
  Lightbar left, right;

  cv::Point2f center;
  std::vector<cv::Point2f> points;  // {left.top, right.top, right.bottom, left.bottom}

  double ratio;              // 两灯条中心连线 / 较长灯条长度
  double side_ratio;         // 长灯条 / 短灯条
  double rectangular_error;  // 灯条与中心连线夹角偏离 90 度的最大值（弧度）

  Armor(const Lightbar & left, const Lightbar & right);
};

}  // namespace train3

#endif  // TRAIN3__ARMOR_HPP