cmake_minimum_required(VERSION 3.10)
project(OpenCVDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# OpenCV 的位置由使用者在配置阶段提供（-DOpenCV_DIR=... 或 CMAKE_PREFIX_PATH），
# 这里不写死任何绝对路径。
find_package(OpenCV REQUIRED)

message(STATUS "OpenCV 库: ${OpenCV_LIBS}")
message(STATUS "OpenCV 头文件: ${OpenCV_INCLUDE_DIRS}")

add_executable(main_app main.cpp)
target_include_directories(main_app PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(main_app PRIVATE ${OpenCV_LIBS})

# 把 test.jpg 复制到可执行文件旁边。程序运行时按可执行文件的实际位置查找图片，
# 因此从任意工作目录启动都能找到它，同时不用在源码里写死绝对路径。
add_custom_command(TARGET main_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/test.jpg"
            "$<TARGET_FILE_DIR:main_app>/test.jpg"
    COMMENT "Copying test.jpg next to the executable")

# 源码中的中文提示按 UTF-8 编译，避免 MSVC 在本地代码页下出现乱码。
if(MSVC)
    target_compile_options(main_app PRIVATE /utf-8)
endif()
