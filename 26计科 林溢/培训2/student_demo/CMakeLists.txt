cmake_minimum_required(VERSION 3.10)
project(student_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(student_lib STATIC tools/student.cpp)
target_include_directories(student_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/includes)

add_executable(main_app src/main.cpp)
target_link_libraries(main_app PRIVATE student_lib)
