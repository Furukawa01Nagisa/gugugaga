#ifndef TRAIN3__DETECTOR_HPP
#define TRAIN3__DETECTOR_HPP

// 传统视觉装甲板检测，移植自 sp_vision_25_hero/tasks/auto_aim/detector.cpp
// 与原版的差异：
//   1. 二值化可以使用两套方案（见 DetectConfig::mask_mode）：
//      - HSV 红色阈值（inRange，培训一的思路，默认，适合本培训的暗光视频）
//      - 灰度阈值（与原仓库 detector.cpp 一致，thresh=150）
//   2. 去掉了 YOLO / 数字分类器 / PCA 角点修正（需要 onnx 模型与标定参数），
//      因此不再输出装甲板编号（name），只输出灯条与装甲板的几何信息。

#include <opencv2/opencv.hpp>

#include <string>
#include <vector>

#include "armor.hpp"

namespace train3
{

enum class MaskMode
{
  hsv_red,   // HSV 红色阈值，inRange，两段色相
  gray       // 灰度阈值，与原仓库一致
};

struct DetectConfig
{
  MaskMode mask_mode = MaskMode::hsv_red;

  // ---- HSV 红色阈值（OpenCV 中 H:0-180, S:0-255, V:0-255）----
  int h_low_min = 0, h_low_max = 10;     // 红色低色相区间
  int h_high_min = 170, h_high_max = 180; // 红色高色相区间（色相环回绕）
  int s_min = 100;
  int v_min = 70;
  int morph_kernel = 3;                   // 开闭运算核大小，0 表示不做

  // ---- 灰度阈值（原仓库 configs/demo.yaml）----
  int gray_thresh = 150;

  // ---- 灯条 / 装甲板几何约束（原仓库 configs/demo.yaml）----
  double max_angle_error_deg = 45.0;
  double min_lightbar_ratio = 1.5;
  double max_lightbar_ratio = 20.0;
  double min_lightbar_length = 8.0;

  double min_armor_ratio = 1.0;
  double max_armor_ratio = 5.0;
  double max_side_ratio = 1.5;
  double max_rectangular_error_deg = 25.0;
};

class Detector
{
public:
  explicit Detector(const DetectConfig & config = DetectConfig());

  // 生成二值掩膜
  cv::Mat mask(const cv::Mat & bgr_img) const;

  // 检测灯条与装甲板（candidates 为通过灯条几何约束的所有灯条）
  void detect(
    const cv::Mat & bgr_img, std::vector<Lightbar> & lightbars,
    std::vector<Armor> & armors) const;

  const DetectConfig & config() const { return config_; }

private:
  DetectConfig config_;
  double max_angle_error_;         // rad
  double max_rectangular_error_;   // rad

  bool check_geometry(const Lightbar & lightbar) const;
  bool check_geometry(const Armor & armor) const;
  Color get_color(const cv::Mat & bgr_img, const std::vector<cv::Point> & contour) const;
};

}  // namespace train3

#endif  // TRAIN3__DETECTOR_HPP