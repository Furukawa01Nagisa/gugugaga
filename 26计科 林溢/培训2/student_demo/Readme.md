# Student 类信息管理与静态库打包 Demo

本项目是一个 C++ 面向对象编程的基础实践任务。主要实现了 `Student` 类用于存储和处理学生信息，并演示了如何通过 CMake 将类的实现代码（`student.cpp`）打包成静态库（Static Library），最后在主程序（`main.cpp`）中链接该静态库并完成测试。

## 📂 目录结构

```text
student_demo/
├── CMakeLists.txt      # CMake 构建脚本（定义静态库与可执行程序）
├── includes/
│   └── student.hpp     # Student 类声明（头文件）
├── tools/
│   └── student.cpp     # Student 类成员函数实现（被编译为静态库）
└── src/
    └── main.cpp        # main 函数，测试类的功能