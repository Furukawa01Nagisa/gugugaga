// 培训三 · 任务二
//   识别视频里的装甲板灯条与四个角点：
//     - 用最大外接矩形（minAreaRect 的轴对齐外框）框住灯条；
//     - 在图像上显示装甲板的四个角点；
//     - 连接四个角点，画出装甲板外框；
//     - 结果写成 mp4，作为培训三的示例视频。
//
// 用法：
//   task2_video --video assets/train3.mp4 --out outputs/train3_demo.mp4
//                [--dump-every 60] [--no-connect-corners] [--hud] [--mask-inset] [--show]

#include <opencv2/opencv.hpp>

#include <algorithm>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "detector.hpp"

namespace fs = std::filesystem;

namespace
{

const cv::Scalar kYellow(0, 255, 255);
const cv::Scalar kGreen(0, 255, 0);
const cv::Scalar kWhite(255, 255, 255);
const cv::Scalar kGray(160, 160, 160);

// 四角点先按 {左灯条上、左灯条下、右灯条上、右灯条下} 收集，
// 再按这个顺序重排成 左上 -> 右上 -> 右下 -> 左下，连起来才不会交叉
const std::vector<std::size_t> kQuadOrder = {0, 2, 3, 1};

void usage()
{
  std::cout << "usage: task2_video --video <input.mp4> [--out <output.mp4>] [--dump-every N]\n"
               "                   [--no-connect-corners] [--hud] [--stats] [--mask-inset]\n"
               "                   [--rotated-outline] [--show] [--gray-thresh 150]\n";
}

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

cv::Point ip(const cv::Point2f & p) { return {cvRound(p.x), cvRound(p.y)}; }

// 半透明信息面板
void draw_panel(cv::Mat & img, const std::vector<std::string> & lines, cv::Point org)
{
  int line_h = 26;
  int pad = 10;
  int width = 0;
  for (const auto & line : lines) {
    int baseline = 0;
    auto size = cv::getTextSize(line, cv::FONT_HERSHEY_SIMPLEX, 0.6, 1, &baseline);
    width = std::max(width, size.width);
  }
  cv::Rect box(org.x, org.y, width + 2 * pad, static_cast<int>(lines.size()) * line_h + 2 * pad - 6);
  box &= cv::Rect(0, 0, img.cols, img.rows);
  if (box.width <= 0 || box.height <= 0) return;

  cv::Mat roi = img(box);
  cv::Mat dark = roi * 0.25;  // 压暗，保证文字清晰
  dark.copyTo(roi);
  cv::rectangle(img, box, cv::Scalar(70, 70, 70), 1);

  for (std::size_t i = 0; i < lines.size(); ++i) {
    cv::putText(
      img, lines[i], {box.x + pad, box.y + pad + static_cast<int>(i) * line_h + 12},
      cv::FONT_HERSHEY_SIMPLEX, 0.6, i == 0 ? kYellow : kWhite, 1, cv::LINE_AA);
  }
}

// 灯条：最大外接矩形 + 上下端点
void draw_lightbar(cv::Mat & img, const train3::Lightbar & lightbar, bool rotated_rect)
{
  // minAreaRect 本身是“最大外接旋转矩形”，取它的轴对齐外框就是图上看到的黄框
  cv::Rect box = lightbar.rotated_rect.boundingRect() & cv::Rect(0, 0, img.cols, img.rows);
  if (box.width > 0 && box.height > 0)
    cv::rectangle(img, box, kYellow, 2, cv::LINE_AA);

  if (rotated_rect) {
    cv::Point2f pts[4];
    lightbar.rotated_rect.points(pts);
    for (int i = 0; i < 4; ++i)
      cv::line(img, ip(pts[i]), ip(pts[(i + 1) % 4]), kYellow, 1, cv::LINE_AA);
  }

  // 灯条上下端点（也是装甲板四个角点的来源）
  cv::circle(img, ip(lightbar.top), 3, kYellow, -1, cv::LINE_AA);
  cv::circle(img, ip(lightbar.bottom), 3, kYellow, -1, cv::LINE_AA);
}

// 灯条“最往外凸”的上下两个角点。
// minAreaRect 的四个角点里，先按 y 分成上沿两点、下沿两点，再在每一对里取 x 最小（左灯条）
// 或 x 最大（右灯条）的那个 —— 就是视觉上灯条最外侧的尖角，而不是灯条端边的中点。
struct CornerPair
{
  cv::Point2f top, bottom;
};

CornerPair outer_corners(const train3::Lightbar & lightbar, bool take_max_x)
{
  cv::Point2f p[4];
  lightbar.rotated_rect.points(p);
  std::vector<cv::Point2f> pts(p, p + 4);
  std::sort(pts.begin(), pts.end(), [](const cv::Point2f & a, const cv::Point2f & b) {
    return a.y < b.y;
  });
  auto pick = [take_max_x](const cv::Point2f & a, const cv::Point2f & b) {
    return (take_max_x ? a.x > b.x : a.x < b.x) ? a : b;
  };
  return {pick(pts[0], pts[1]), pick(pts[2], pts[3])};
}

// 装甲板：四个最外侧角点 + 连线
void draw_armor(
  cv::Mat & img, const train3::Armor & armor, bool connect_corners = true,
  bool show_stats = false)
{
  // 左灯条取最左侧的两个尖角，右灯条取最右侧的两个尖角
  const CornerPair l = outer_corners(armor.left, false);
  const CornerPair r = outer_corners(armor.right, true);
  std::vector<cv::Point> quad = {ip(l.top), ip(l.bottom), ip(r.top), ip(r.bottom)};
  // 顺序整理成 左上 -> 右上 -> 右下 -> 左下，连起来才是一个不交叉的四边形
  std::vector<cv::Point> ordered(kQuadOrder.size());
  for (std::size_t i = 0; i < kQuadOrder.size(); ++i) ordered[i] = quad[kQuadOrder[i]];

  // 连接四个角点
  if (connect_corners)
    cv::polylines(img, std::vector<std::vector<cv::Point>>{ordered}, true, kGreen, 1, cv::LINE_AA);

  // 十字标出四个角点
  const char * labels[4] = {"P1", "P2", "P3", "P4"};
  for (int i = 0; i < 4; ++i) {
    const auto & p = ordered[i];
    cv::line(img, {p.x - 8, p.y}, {p.x + 8, p.y}, kGreen, 1, cv::LINE_AA);
    cv::line(img, {p.x, p.y - 8}, {p.x, p.y + 8}, kGreen, 1, cv::LINE_AA);
    cv::circle(img, p, 4, kGreen, -1, cv::LINE_AA);
    cv::circle(img, p, 8, kGreen, 1, cv::LINE_AA);

    // P1/P2 在灯条上方，P3/P4 在灯条下方，避免压住黄色外接矩形
    bool up = (i < 2);
    cv::putText(
      img, labels[i], {p.x + 11, up ? p.y - 12 : p.y + 22}, cv::FONT_HERSHEY_SIMPLEX, 0.45, kGreen,
      1, cv::LINE_AA);
  }

  // 中心点
  cv::drawMarker(img, ip(armor.center), kGreen, cv::MARKER_CROSS, 14, 1);

  // 几何参数（ratio / side / rect），默认不显示，--stats 才画
  if (!show_stats) return;
  char info[128];
  std::snprintf(
    info, sizeof(info), "ratio %.2f  side %.2f  rect %.1f deg",
    armor.ratio, armor.side_ratio, armor.rectangular_error * 57.3);
  cv::Point text_org{ordered[0].x, std::min(ordered[0].y, ordered[1].y) - 12};
  // 描边用“8 邻域偏移的细字”实现，不要用 thickness=3 的粗描边：
  // OpenCV5 里先画 thickness>=2 的 putText 再叠一遍细字，文字末尾会多蹦出一个字形（已用
  // build/_t.cpp 复现），偏移描边的画法干净。
  for (int dx = -1; dx <= 1; ++dx)
    for (int dy = -1; dy <= 1; ++dy)
      if (dx != 0 || dy != 0)
        cv::putText(
          img, info, text_org + cv::Point(dx, dy), cv::FONT_HERSHEY_SIMPLEX, 0.55,
          cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
  cv::putText(img, info, text_org, cv::FONT_HERSHEY_SIMPLEX, 0.55, kGreen, 1, cv::LINE_AA);
}

// 右下角掩膜小窗
void draw_mask_inset(cv::Mat & img, const cv::Mat & mask)
{
  const double scale = 0.28;
  cv::Mat small;
  cv::resize(mask, small, {}, scale, scale, cv::INTER_NEAREST);
  cv::Mat small_bgr;
  cv::cvtColor(small, small_bgr, cv::COLOR_GRAY2BGR);

  cv::Mat canvas(small_bgr.rows + 30, small_bgr.cols, CV_8UC3, cv::Scalar(28, 28, 28));
  small_bgr.copyTo(canvas(cv::Rect(0, 30, small_bgr.cols, small_bgr.rows)));
  cv::putText(
    canvas, "binary mask (HSV red inRange)", {8, 21}, cv::FONT_HERSHEY_SIMPLEX, 0.45, kWhite, 1,
    cv::LINE_AA);

  int x = img.cols - canvas.cols - 16;
  int y = img.rows - canvas.rows - 16;
  if (x < 0 || y < 0) return;
  canvas.copyTo(img(cv::Rect(x, y, canvas.cols, canvas.rows)));
  cv::rectangle(img, cv::Rect(x, y, canvas.cols, canvas.rows), cv::Scalar(90, 90, 90), 1);
}

}  // namespace

int main(int argc, char ** argv)
{
  std::string video_path = "assets/train3.mp4";
  std::string out_path = "outputs/train3_demo.mp4";
  int dump_every = 0;
  bool mask_inset = false;
  bool show_hud = false;
  bool show_stats = false;
  bool show = false;
  bool connect_corners = true;
  bool rotated_outline = false;
  train3::DetectConfig cfg;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    auto next = [&](int offset) -> std::string {
      return (i + offset < argc) ? std::string(argv[i + offset]) : std::string();
    };
    if (arg == "--video" && i + 1 < argc) {
      video_path = next(1);
      ++i;
    } else if (arg == "--out" && i + 1 < argc) {
      out_path = next(1);
      ++i;
    } else if (arg == "--dump-every" && i + 1 < argc) {
      dump_every = std::stoi(next(1));
      ++i;
    } else if (arg == "--mask-inset") {
      mask_inset = true;
    } else if (arg == "--hud") {
      show_hud = true;
    } else if (arg == "--stats") {
      show_stats = true;
    } else if (arg == "--no-mask-inset") {
      mask_inset = false;
    } else if (arg == "--show") {
      show = true;
    } else if (arg == "--no-connect-corners") {
      connect_corners = false;
    } else if (arg == "--rotated-outline") {
      rotated_outline = true;
    } else if (arg == "--gray-thresh" && i + 1 < argc) {
      cfg.mask_mode = train3::MaskMode::gray;
      cfg.gray_thresh = std::stoi(next(1));
      ++i;
    } else if (arg == "--help" || arg == "-h") {
      usage();
      return 0;
    }
  }

  cv::VideoCapture cap(video_path);
  if (!cap.isOpened()) {
    std::cerr << "[task2] cannot open video: " << video_path << "\n";
    return 1;
  }

  const int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
  const int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
  const double fps = cap.get(cv::CAP_PROP_FPS) > 1.0 ? cap.get(cv::CAP_PROP_FPS) : 30.0;
  const int total = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
  std::cout << "[task2] input " << video_path << " " << width << "x" << height << " @" << fps
            << "fps, frames " << total << "\n";

  auto out_parent = fs::path(out_path).parent_path();
  if (!out_parent.empty()) fs::create_directories(out_parent);

  cv::VideoWriter writer(
    out_path, cv::VideoWriter::fourcc('m', 'p', '4', 'v'), fps, cv::Size(width, height));
  if (!writer.isOpened()) {
    std::cerr << "[task2] cannot open VideoWriter: " << out_path << "\n";
    return 1;
  }

  train3::Detector detector(cfg);
  cv::Mat frame;
  int frame_index = 0;
  int frames_with_armor = 0;
  long long bars_total = 0, armors_total = 0;
  const auto t_start = cv::getTickCount();

  while (cap.read(frame)) {
    if (frame.empty()) break;

    std::vector<train3::Lightbar> lightbars;
    std::vector<train3::Armor> armors;
    detector.detect(frame, lightbars, armors);
    auto mask = detector.mask(frame);

    cv::Mat canvas = frame.clone();

    // 只画属于装甲板的灯条（candidates 里的孤立灯条会带来噪点）
    std::vector<bool> used(lightbars.size(), false);
    for (const auto & armor : armors) {
      if (armor.left.id < used.size()) used[armor.left.id] = true;
      if (armor.right.id < used.size()) used[armor.right.id] = true;
    }
    for (std::size_t i = 0; i < lightbars.size(); ++i)
      if (used[i]) draw_lightbar(canvas, lightbars[i], rotated_outline);

    for (const auto & armor : armors) draw_armor(canvas, armor, connect_corners, show_stats);

    // 左上角信息（默认不显示，--hud 打开）
    int used_count = 0;
    for (bool u : used) used_count += u ? 1 : 0;
    if (show_hud) {
      char line3[96], line4[96];
      std::snprintf(line3, sizeof(line3), "frame: %d / %d", frame_index + 1, total);
      std::snprintf(
        line4, sizeof(line4), "lightbars: %d (cand %zu)    armors: %zu", used_count,
        lightbars.size(), armors.size());
      draw_panel(
        canvas,
        {"train3 - armor lightbar & corner detection",
         "detector: sp_vision_25_hero (traditional)",
         line3, line4},
        {16, 16});
    }
    (void)used_count;

    if (mask_inset) draw_mask_inset(canvas, mask);

    writer.write(canvas);

    if (dump_every > 0 && frame_index % dump_every == 0) {
      fs::create_directories("outputs/frames");
      char name[64];
      std::snprintf(name, sizeof(name), "outputs/frames/frame_%04d.png", frame_index);
      write_image(name, canvas);
    }

    if (show) {
      cv::imshow("detection", canvas);
      if (cv::waitKey(1) == 27) break;
    }

    bars_total += static_cast<long long>(lightbars.size());
    armors_total += static_cast<long long>(armors.size());
    if (!armors.empty()) frames_with_armor += 1;

    if (frame_index % 60 == 0)
      std::cout << "  frame " << frame_index << " bars " << lightbars.size() << " armors "
                << armors.size() << "\n";
    ++frame_index;
  }

  writer.release();
  cap.release();

  const double seconds = (cv::getTickCount() - t_start) / cv::getTickFrequency();
  std::cout << "[task2] done: " << frame_index << " frames in " << seconds << " s ("
            << (seconds > 0 ? frame_index / seconds : 0) << " fps)\n";
  if (frame_index > 0) {
    std::cout << "[task2] avg lightbars " << static_cast<double>(bars_total) / frame_index
              << ", avg armors " << static_cast<double>(armors_total) / frame_index
              << ", frames with armor " << frames_with_armor << "\n";
  }
  std::cout << "[task2] saved " << out_path << "\n";
  return 0;
}