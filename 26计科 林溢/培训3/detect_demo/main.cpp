#include <opencv2/opencv.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/geometry/2d.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace cv;
using namespace std;

struct Lightbar {
    RotatedRect rect;
    float length = 0, width = 0, angle = 0, tilt = 0;
    Point2f pts[4];
};

struct Armor {
    Point2f corners[4];
    float score = 0;
};

// 角度归一化
float normAngle(float a) {
    while (a > 90)   a -= 180;
    while (a <= -90) a += 180;
    return a;
}

// 两角度的最小差
float angleDiff180(float a, float b) {
    float d = fmod(fabs(a - b), 180.0f);
    if (d > 90) d = 180 - d;
    return d;
}

// 处理单个轮廓
bool makeLightbar(const RotatedRect& rr, Lightbar& lb) {
    rr.points(lb.pts);
    float maxLen = 0; Point2f dir;
    for (int i = 0; i < 4; ++i) {
        Point2f d = lb.pts[(i + 1) % 4] - lb.pts[i];
        float len = norm(d);
        if (len > maxLen) { maxLen = len; dir = d; }
    }
    lb.rect = rr;
    lb.length = max(rr.size.width, rr.size.height);
    lb.width = min(rr.size.width, rr.size.height);
    lb.angle = normAngle(atan2(dir.y, dir.x) * 180.0f / CV_PI);
    lb.tilt = 90.0f - fabs(lb.angle);

    if (lb.length < 8.0f) return false;
    float aspect = lb.length / max(lb.width, 1e-3f);
    if (aspect < 1.5f || aspect > 20.0f) return false;
    if (lb.tilt > 45.0f) return false;
    return true;
}

// 获取外侧角点
void outerCorners(const Lightbar& lb, bool isLeft, Point2f& top, Point2f& bottom) {
    vector<Point2f> v(lb.pts, lb.pts + 4);
    sort(v.begin(), v.end(), [](const Point2f& a, const Point2f& b) { return a.y < b.y; });
    if (isLeft) {
        top = (v[0].x < v[1].x) ? v[0] : v[1];
        bottom = (v[2].x < v[3].x) ? v[2] : v[3];
    } else {
        top = (v[0].x > v[1].x) ? v[0] : v[1];
        bottom = (v[2].x > v[3].x) ? v[2] : v[3];
    }
}

// 尝试配对装甲板
bool tryMatch(const Lightbar& L, const Lightbar& R, Armor& out) {
    float avgLen = (L.length + R.length) * 0.5f;
    Point2f d = R.rect.center - L.rect.center;
    float gap = norm(d);

    float gapRatio = gap / max(avgLen, 1e-3f);
    if (gapRatio < 1.0f || gapRatio > 5.0f) return false;

    float lenRatio = max(L.length, R.length) / max(min(L.length, R.length), 1e-3f);
    if (lenRatio > 1.5f) return false;

    float tiltDiff = fabs(L.tilt - R.tilt);
    if (tiltDiff > 25.0f) return false;

    float gapAngle = atan2(d.y, d.x) * 180.0f / CV_PI;
    float perpL = fabs(90.0f - angleDiff180(gapAngle, L.angle));
    float perpR = fabs(90.0f - angleDiff180(gapAngle, R.angle));
    float perpErr = (perpL + perpR) * 0.5f;
    if (perpErr > 25.0f) return false;

    Point2f lt, lb, rt, rb;
    outerCorners(L, true, lt, lb);
    outerCorners(R, false, rt, rb);
    Point2f collected[4] = { lt, lb, rt, rb };
    const int order[4] = { 0, 2, 3, 1 }; // 左上，右上，右下，左下
    for (int i = 0; i < 4; ++i) out.corners[i] = collected[order[i]];

    out.score = 100.0f - perpErr - tiltDiff - fabs(lenRatio - 1.0f) * 10.0f - fabs(gapRatio - 3.0f) * 2.0f;
    return true;
}

int main() {
    string input = "test_clip.mp4";
    VideoCapture cap(input);
    if (!cap.isOpened()) {
        cerr << "无法打开视频，请确认 " << input << " 是否在当前目录！\n";
        return -1;
    }

    int W = (int)cap.get(CAP_PROP_FRAME_WIDTH);
    int H = (int)cap.get(CAP_PROP_FRAME_HEIGHT);
    double fps = cap.get(CAP_PROP_FPS);
    if (fps <= 0) fps = 30;

    VideoWriter writer("output_demo.mp4", VideoWriter::fourcc('m', 'p', '4', 'v'), fps, Size(W, H));
    
    int frames = 0, framesWithArmor = 0;
    long totalBars = 0, totalArmors = 0;
    Mat frame;

    while (cap.read(frame)) {
        Mat hsv, m1, m2, mask;
        cvtColor(frame, hsv, COLOR_BGR2HSV);
        inRange(hsv, Scalar(0, 100, 70), Scalar(10, 255, 255), m1);
        inRange(hsv, Scalar(170, 100, 70), Scalar(180, 255, 255), m2);
        bitwise_or(m1, m2, mask);
        morphologyEx(mask, mask, MORPH_CLOSE, getStructuringElement(MORPH_RECT, Size(3, 3)));

        vector<vector<Point>> contours;
        findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        vector<Lightbar> bars;
        for (const auto& c : contours) {
            if (c.size() < 4) continue;
            Lightbar lb;
            if (makeLightbar(cv::minAreaRect(c), lb)) bars.push_back(lb);
        }

        sort(bars.begin(), bars.end(), [](const Lightbar& a, const Lightbar& b) {
            return a.rect.center.x < b.rect.center.x;
        });

        struct Cand { int i, j; Armor a; };
        vector<Cand> cands;
        for (size_t i = 0; i < bars.size(); ++i) {
            for (size_t j = i + 1; j < bars.size(); ++j) {
                Armor a;
                if (tryMatch(bars[i], bars[j], a)) cands.push_back({ (int)i, (int)j, a });
            }
        }
        sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) { return a.a.score > b.a.score; });

        vector<bool> used(bars.size(), false);
        vector<Armor> armors;
        for (const auto& c : cands) {
            if (used[c.i] || used[c.j]) continue;
            used[c.i] = used[c.j] = true;
            armors.push_back(c.a);
        }

        // 绘制
        for (const auto& lb : bars) {
            for (int i = 0; i < 4; ++i) line(frame, lb.pts[i], lb.pts[(i + 1) % 4], Scalar(0, 255, 255), 1);
        }
        for (const auto& a : armors) {
            for (int i = 0; i < 4; ++i) {
                line(frame, a.corners[i], a.corners[(i + 1) % 4], Scalar(0, 255, 0), 2);
                circle(frame, a.corners[i], 4, Scalar(0, 0, 255), -1);
            }
        }

        writer.write(frame);
        imshow("Detect", frame);
        if (waitKey(1) == 27) break; // 按 ESC 退出

        frames++;
        if (!armors.empty()) framesWithArmor++;
        totalBars += bars.size();
        totalArmors += armors.size();
    }

    cout << "总帧数: " << frames 
         << " | 平均灯条: " << (double)totalBars / frames 
         << " | 平均装甲板: " << (double)totalArmors / frames 
         << " | 检出率: " << 100.0 * framesWithArmor / frames << "%\n";
    cout << "视频已保存至 output_demo.mp4\n";
    return 0;
}