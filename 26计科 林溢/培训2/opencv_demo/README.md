# OpenCV Demo

使用 OpenCV 读取并显示一张图片的最小示例。项目源码和配置中不写死任何盘符或绝对路径。

## 目录结构

```text
opencv_demo/
├── CMakeLists.txt
├── main.cpp
└── test.jpg
```

## 依赖

- CMake 3.10+
- 支持 C++17 的编译器（程序使用 `std::filesystem` 定位图片）
- OpenCV 4.x / 5.x（需要 core、imgcodecs、highgui 模块）

## 构建

`CMakeLists.txt` 通过 `find_package(OpenCV REQUIRED)` 查找 OpenCV，不指定固定安装路径。

如果 OpenCV 已安装在 CMake 能搜索到的位置：

```bash
cmake -S . -B build
cmake --build build
```

否则在配置阶段告诉 CMake OpenCV 的 CMake 配置目录（即包含 `OpenCVConfig.cmake` 的目录，通常位于 OpenCV 安装目录的 `build` 或 `lib/cmake/opencv4` 下）：

```bash
cmake -S . -B build -DOpenCV_DIR="<OpenCV 的 CMake 配置目录>"
cmake --build build
```

也可以把该目录加入 `CMAKE_PREFIX_PATH` 环境变量，效果相同。路径由构建者通过命令行提供，不写进项目文件。

构建完成后，CMake 会把 `test.jpg` 自动复制到可执行文件旁边。

## 运行

```bash
# Visual Studio 等多配置生成器：可执行文件通常在 build\Release 下
.\build\Release\main_app.exe

# MinGW Makefiles 等单配置生成器：可执行文件通常在 build 下
.\build\main_app.exe
```

程序会到可执行文件所在目录查找 `test.jpg`，因此从哪个工作目录启动都可以，不需要把图片路径写死在代码里。

也可以指定图片路径（相对路径按当前工作目录解析）：

```bash
.\build\Release\main_app.exe .\test.jpg
```

Linux / macOS 下对应 `./build/Release/main_app` 或 `./build/main_app`。

Windows 下如果运行时提示找不到 `opencv_worldXXX.dll`，请把 OpenCV 的 `bin` 目录加入 `PATH`，或把对应 DLL 复制到可执行文件所在目录。

## 说明

- 程序未指定参数时，按可执行文件的实际位置解析 `test.jpg`；指定参数时使用传入的路径。
- 如果手动用 `g++` 编译，请把 `test.jpg` 放在可执行文件旁边，或通过参数指定图片路径。
- 图片读取失败时向标准错误输出原因和用法，并返回非零退出码。
- 使用 MSVC 构建时会自动添加 `/utf-8`，保证源码中的中文提示正常显示。
- `build/` 是 CMake 生成的构建目录，可以随时删除后重新生成。
