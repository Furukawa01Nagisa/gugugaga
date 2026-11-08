# detect_demo —— 装甲板灯条配对检测演示

对一段 RoboMaster 比赛/测试视频逐帧做**传统视觉装甲板检测**：HSV 提红色 → 轮廓 → 灯条筛选 → 左右两两配对 → 贪心去重 → 画出装甲板四边形，并输出标注视频与统计指标。

不训练任何模型，纯 OpenCV 几何规则，**约 190 行**，适合作为理解「灯条 → 装甲板」经典 pipeline 的入门样例。

```
视频帧 ──▶ HSV 双区间阈值 ──▶ 轮廓 ──▶ 灯条筛选 ──▶ 两两配对打分 ──▶ 贪心去重 ──▶ 绘制/统计
```

---

## 1. 目录结构

```
detect_demo/
├── main.cpp          # 全部逻辑（约 190 行）
├── CMakeLists.txt    # CMake 构建脚本
├── test_clip.mp4     # 输入视频
├── output_demo.mp4   # 运行后生成的标注视频
├── README.md
└── build/            # CMake 生成的构建目录（可删除重建）
    ├── Release/detect_demo.exe
    └── ...
```

## 2. 环境要求

| 项目 | 要求 |
| --- | --- |
| 操作系统 | Windows（示例基于 MSVC；Linux 下改链接方式亦可） |
| OpenCV | **5.x**（源码 `#include <opencv2/geometry/2d.hpp>`，为 OpenCV 5 的头文件；本仓库已在 **5.0.0** 上编译运行通过） |
| 编译器 | 支持 C++17；本仓库用 Visual Studio 18 2026 生成器编译，链接 vc16 版 OpenCV（二进制兼容） |
| 构建工具 | CMake ≥ 3.16 |
| 编解码 | 需能播放/写入 mp4（`VideoCapture` 读、`mp4v` 写），要求系统有对应解码器 |
| 其他 | 需要**桌面环境**，因为用到了 `imshow` 弹窗；按 `ESC` 可提前退出 |

若本机是 OpenCV 4.x，可尝试删掉 `main.cpp` 顶部的 `#include <opencv2/geometry/2d.hpp>`（当前代码并未使用其中的符号，`opencv2/opencv.hpp` 已足够）。

OpenCV 依赖由 `CMakeLists.txt` 中的 `find_package(OpenCV REQUIRED)` 提供，并优先读取环境变量 `OpenCV_DIR`：

```cmake
if(DEFINED ENV{OpenCV_DIR})
    set(OpenCV_DIR $ENV{OpenCV_DIR})
endif()
```

## 3. 编译

在 `detect_demo` 目录下执行：

```powershell
# 1) 指向本机 OpenCV 的 cmake 配置目录（按自己的安装路径修改）
$env:OpenCV_DIR = "D:\Code\OpenCV\opencv\build\x64\vc16\lib"

# 2) 配置 + 生成工程（Visual Studio 生成器）
cmake -S . -B build -A x64

# 3) 编译 Release 版本
cmake --build build --config Release
```

生成的可执行文件位于：

```
detect_demo\build\Release\detect_demo.exe
```

> 也支持在 VS Code / Visual Studio 中直接打开 `build\detect_demo.slnx` 后编译运行。

## 4. 运行

⚠️ **程序里视频路径是写死的相对路径 `test_clip.mp4`**，所以**工作目录必须是视频所在目录**。

```powershell
cd detect_demo
.\build\Release\detect_demo.exe
```

运行后：

1. 弹出 `Detect` 窗口实时播放带标注的画面；
2. 同时把标注结果写入当前工作目录的 `output_demo.mp4`（帧率、分辨率与源视频一致，编码 `mp4v`）；
3. 播放结束（或按 **ESC** 提前退出）后在控制台打印统计（示例输出，具体数值取决于视频）：

```
总帧数: 320 | 平均灯条: 2.13 | 平均装甲板: 0.91 | 检出率: 86.5%
视频已保存至 output_demo.mp4
```

> 若从 `build\Release` 目录直接双击 exe，当前目录里没有 `test_clip.mp4`，就会得到
> `无法打开视频，请确认 test_clip.mp4 是否在当前目录！`（退出码 `-1`）。

## 5. 处理流程

### 5.1 每帧的检测步骤

| 步骤 | 代码位置 | 说明 |
| --- | --- | --- |
| ① 图像预处理 | `main()` 循环内 | `cvtColor` 转 HSV，`inRange` 取 `[0,10] ∪ [170,180]`（S≥100，V≥70），`bitwise_or` 合并，再做 `3×3` 闭运算——与 [`hsv_demo`](../hsv_demo/README.md) 完全一致 |
| ② 找轮廓 | `findContours(..., RETR_EXTERNAL, CHAIN_APPROX_SIMPLE)` | 只取最外层轮廓，每个连通域视为候选灯条 |
| ③ 拟合灯条 | `cv::minAreaRect` + `makeLightbar()` | 最小外接旋转矩形 → 计算长、宽、倾角，并做形状筛选（见 5.2） |
| ④ 按 x 排序 | `sort(bars, ... center.x)` | 保证候选总是"左灯条在前、右灯条在后" |
| ⑤ 两两配对 | 双重循环 + `tryMatch()` | 所有 `(i, j)` 组合都尝试配对，得到候选装甲板（见 5.3） |
| ⑥ 打分排序 | `sort(cands, ... score 降序)` | 好配对的先被选中 |
| ⑦ 贪心去重 | `vector<bool> used` | 按分数从高到低取，**一根灯条只允许属于一块装甲板**，避免同一对灯条被重复框选 |
| ⑧ 绘制 + 写出 | `line` / `circle` / `writer.write` | 黄色细框=灯条，绿色粗框=装甲板，红色圆点=角点 |
| ⑨ 统计 | 循环结束 | 累计帧数、灯条总数、装甲板总数、有目标的帧数 |

### 5.2 灯条（Lightbar）判定条件

`makeLightbar()` 中，以下条件全部满足才保留（另外 `main()` 里会跳过顶点数 `< 4` 的退化轮廓）：

| 条件 | 阈值 | 作用 |
| --- | --- | --- |
| 最长边 | `length ≥ 8 px` | 排除噪点小碎块 |
| 长宽比 | `1.5 ≤ length/width ≤ 20` | 灯条是细长条；太方（数字/色块）或太扁（噪点线）都丢弃 |
| 倾角 | `tilt ≤ 45°` | `tilt = 90° − abs(angle)`，angle 归一化到 `(−90, 90]`；过滤"躺平"或竖得离谱的干扰物 |

### 5.3 配对（tryMatch）约束

设左右灯条 `L`、`R`，中心距 `gap`，平均长度 `avgLen`：

| 约束 | 阈值 | 含义 |
| --- | --- | --- |
| `gapRatio = gap / avgLen` | `1.0 ~ 5.0` | 两灯条间距与灯条长度的比例，太近/太远都不是一块装甲板 |
| `lenRatio = max/min`（长度） | `≤ 1.5` | 左右灯条应长度相近（透视/遮挡下允许一定差异） |
| `tiltDiff = abs(L.tilt − R.tilt)` | `≤ 25°` | 两灯条倾斜方向应一致 |
| `perpErr` | `≤ 25°` | 连线方向应与灯条走向**近似垂直**：`perpErr = ½ · (abs(90° − angleDiff(gapAngle, L.angle)) + abs(90° − angleDiff(gapAngle, R.angle)))` |

### 5.4 置信度打分

```
score = 100 − perpErr − tiltDiff − |lenRatio − 1| × 10 − |gapRatio − 3| × 2
```

各项都是"偏离理想值"的惩罚：`perpErr`、`tiltDiff` 为角度误差（度），长度比以 1.0 为理想、间距比以 3.0 为理想。分数越高说明越像标准装甲板。

### 5.5 角点顺序

`outerCorners()` 先按 y 排序取上下两组，再按"左侧取更小的 x、右侧取更大的 x"取**外侧两点**，最后按 `{0,2,3,1}` 重排为：

```
corners[0] = 左上   corners[1] = 右上
corners[2] = 右下   corners[3] = 左下
```

顺序固定，可直接对接后续 PnP 解算或 ROI 裁剪。

## 6. 参数总表（调参入口）

| 类别 | 参数 | 当前值 | 位置 |
| --- | --- | --- | --- |
| 颜色 | H 低段 / 高段 | `0–10` / `170–180` | `main()` 两个 `inRange` |
| 颜色 | S 下限 / V 下限 | `100` / `70` | 同上 |
| 形态学 | 闭运算核 | `3×3` | `getStructuringElement` |
| 灯条 | 最小长度 | `8 px` | `makeLightbar()` |
| 灯条 | 长宽比范围 | `1.5 ~ 20` | `makeLightbar()` |
| 灯条 | 最大倾角 | `45°` | `makeLightbar()` |
| 配对 | 间距比范围 | `1.0 ~ 5.0` | `tryMatch()` |
| 配对 | 长度比上限 | `1.5` | `tryMatch()` |
| 配对 | 倾角差上限 | `25°` | `tryMatch()` |
| 配对 | 垂直误差上限 | `25°` | `tryMatch()` |
| 输出 | 视频编码 | `mp4v` | `VideoWriter::fourcc` |

## 7. 调参建议

- **漏检（灯条都没提出来）**：先按 `hsv_demo` 的思路单独看掩膜，多半是 `V 下限` 太高或光照突变；
- **误检（背景被当灯条）**：收紧 `S/V`，或把 `长宽比` 下限提到 `2.0`、最大倾角降到 `30°`；
- **一块装甲板被框多次**：检查 `gapRatio` 上限是否过宽（当前 5.0 偏宽松）；
- **同一灯条被两块装甲板共用**：贪心去重本身已防住，若仍出现说明灯条被拆成了多个小轮廓，加大闭运算核；
- **灯条一闪一闪**：传统方法无时序滤波，可对 `armors` 加简单的帧间关联或 IoU 跟踪平滑。

## 8. 常见问题 / 已知限制

| 现象 | 说明 |
| --- | --- |
| `无法打开视频…` | 工作目录不对；先 `cd detect_demo` 再运行 |
| 窗口无画面 / 程序秒退 | 无桌面会话（SSH、CI）时 `imshow` 不可用，可注释掉 `imshow`/`waitKey` |
| `output_demo.mp4` 为空或打不开 | 部分环境缺少 `mp4v` 编码器；代码**未检查** `writer.isOpened()`，可改成 `avc1` 或先判空 |
| 控制台出现 `inf` / `nan` | 视频读到 **0 帧**时统计公式会除零（源视频损坏或解码失败），属边界情况 |
| 只支持红色 | 蓝方灯条需改 `inRange` 的 H 区间 |
| 不区分装甲板类型 | 未做数字/图案识别，也没有大小装甲板分类，只输出几何外框 |
| 复杂度 O(n²) | 每帧对所有灯条两两配对；灯条数通常 < 20，实际开销可忽略 |
| 无跟踪/无滤波 | 逐帧独立检测，抖动与闪断直接反映在结果里 |

---

**相关**：颜色阈值的单独调试请看 [`hsv_demo`](../hsv_demo/README.md)。
