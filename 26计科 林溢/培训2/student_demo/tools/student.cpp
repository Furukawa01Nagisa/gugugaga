#include "student.hpp"

#include <iostream>
#include <stdexcept>

namespace {

constexpr double kMinScore = 0.0;
constexpr double kMaxScore = 100.0;

// 构造函数和 setScore() 共用这一套校验，保证成绩范围规则一致。
double validateScore(double score) {
    if (score < kMinScore || score > kMaxScore) {
        throw std::out_of_range("score must be between 0 and 100");
    }
    return score;
}

}  // namespace

Student::Student(const std::string& n, const std::string& i, double s)
    : name(n), id(i), score(validateScore(s)) {}

void Student::print() const {
    std::cout << "Name: " << name << std::endl;
    std::cout << "ID: " << id << std::endl;
    std::cout << "Score: " << score << std::endl;
}

void Student::setScore(double newScore) {
    score = validateScore(newScore);
}
