cmake_minimum_required(VERSION 3.16)
project(hsv_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 优先从环境变量查找 OpenCV_DIR
if(DEFINED ENV{OpenCV_DIR})
    set(OpenCV_DIR $ENV{OpenCV_DIR})
endif()

find_package(OpenCV REQUIRED)
message(STATUS "Found OpenCV ${OpenCV_VERSION}")

add_executable(hsv_demo main.cpp)
target_include_directories(hsv_demo PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(hsv_demo PRIVATE ${OpenCV_LIBS})