cmake_minimum_required(VERSION 3.10)
project(student_demo)
set(CMAKE_CXX_STANDARD 11)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/includes)
add_library(student_lib STATIC tools/student.cpp)
add_executable(main_app src/main.cpp)
target_link_libraries(main_app student_lib)