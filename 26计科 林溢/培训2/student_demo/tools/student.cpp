#include"student.hpp"
Student::Student(std::string n,int i,double s):name(n),id(i),score(s){}
void Student::print()const{
    std::cout<<"Name: "<<name<<std::endl;
    std::cout<<"ID: "<<id<<std::endl;
    std::cout<<"Score: "<<score<<std::endl;
}

void Student::Score(double newScore){
    score=newScore;
    std::cout<<"New Score: "<<score<<std::endl;
}

