# Student Demo

一个演示 C++ 类声明与实现分离的小项目：`Student` 类保存姓名、学号和成绩，支持打印信息和修改成绩。

## 目录结构

```text
student_demo/
├── CMakeLists.txt
├── includes/
│   └── student.hpp      # 类声明（仅接口）
├── src/
│   └── main.cpp         # 程序入口
└── tools/
    └── student.cpp      # 类实现
```

## 接口说明

| 成员 | 说明 |
| --- | --- |
| `Student(const std::string& name, const std::string& id, double score)` | 构造对象。`id` 用字符串保存；`score` 必须在 0–100 之间，否则抛出 `std::out_of_range`。 |
| `void print() const` | 输出姓名、学号、成绩。 |
| `void setScore(double newScore)` | 修改成绩。与构造函数共用同一套校验：0–100 之外的值抛出 `std::out_of_range`，原成绩保持不变。 |

### 设计要点

- `student.hpp` 只包含 `<string>` 和接口声明，不包含 `<iostream>`。
- `<iostream>` 只出现在真正使用 `std::cout` / `std::cerr` 的 `student.cpp` 与 `main.cpp` 中，`main.cpp` 显式包含它。
- 构造函数和 `setScore()` 共用 `student.cpp` 内部的同一套成绩校验逻辑，避免两处规则不一致。
- 学号声明为 `std::string`，可以保存带前导零或字母的学号。
- `CMakeLists.txt` 用 `${CMAKE_CURRENT_SOURCE_DIR}` 定位头文件目录，源码和配置中没有写死绝对路径。

## 构建

在 `student_demo` 目录下执行（需要支持 C++11 的编译器和 CMake 3.10+）：

```bash
cmake -S . -B build
cmake --build build
```

Windows 下使用 MinGW 或 Visual Studio 均可。也可以不经过 CMake，直接编译：

```bash
g++ -std=c++11 -I includes tools/student.cpp src/main.cpp -o student_demo
```

## 运行

```bash
# Linux / macOS
./build/main_app

# Windows + Visual Studio 等多配置生成器
.\build\Debug\main_app.exe

# Windows + MinGW Makefiles 等单配置生成器
.\build\main_app.exe
```

示例输出：

```text
Student Information:
Name: Alice
ID: 2025001
Score: 85.5
Updated Information:
Name: Alice
ID: 2025001
Score: 90
```

## 成绩校验示例

```cpp
try {
    s.setScore(120.0);   // 超出 0–100
} catch (const std::out_of_range& e) {
    std::cerr << e.what() << '\n';
}
```

`build/` 是 CMake 生成的构建目录，可以随时删除后重新生成。
