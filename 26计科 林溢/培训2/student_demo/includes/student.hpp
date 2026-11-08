#pragma once

#include <string>

class Student {
public:
    Student(const std::string& name, const std::string& id, double score);

    void print() const;
    void setScore(double newScore);

private:
    std::string name;
    std::string id;
    double score;
};
