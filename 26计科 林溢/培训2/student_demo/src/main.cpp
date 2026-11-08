#include "student.hpp"

#include <iostream>

int main() {
    Student s("Alice", "2025001", 85.5);

    std::cout << "Student Information:" << std::endl;
    s.print();

    std::cout << "Updated Information:" << std::endl;
    s.setScore(90.0);
    s.print();

    return 0;
}
