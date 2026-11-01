#include"student.hpp"

int main() {
    Student s("Alice", 1, 85.5);
    std::cout << "Student Information:" << std::endl;
    s.print();
    std::cout << "Updated Information:" << std::endl;
    s.Score(90.0);
    std::cout << "After updating score:" << std::endl;
    s.print();
    return 0;
}
