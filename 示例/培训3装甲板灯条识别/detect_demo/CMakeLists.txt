cmake_minimum_required(VERSION 3.10)

project(detect_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

add_executable(detect_demo
        main.cpp
        src/armor.cpp
        src/detector.cpp)

target_include_directories(detect_demo PRIVATE ${OpenCV_INCLUDE_DIRS} src)
target_link_libraries(detect_demo PRIVATE ${OpenCV_LIBS})

# 测试视频拷到构建目录，避免“在 build/ 里运行找不到文件”
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/train3.mp4 DESTINATION ${CMAKE_CURRENT_BINARY_DIR})