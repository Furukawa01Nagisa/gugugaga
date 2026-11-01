#pragma once
#include<string>
#include<iostream>

class Student{
    private:        
        std::string name;
        int id;
        double score;

    public:  
        Student(std::string n,int i,double s);
        void print()const;
        void Score(double newScore);

};