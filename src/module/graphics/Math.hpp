#pragma once
#include <cmath>

namespace Math {
    constexpr double PI = 3.14159265358979323846;
    
    inline double radians(double degrees) {
        return degrees * PI / 180.0;
    }
    
    inline double degrees(double radians) {
        return radians * 180.0 / PI;
    }
    
    inline double clamp(double value, double min, double max) {
        return value < min ? min : (value > max ? max : value);
    }
}

struct Point {
    double x, y;
    Point(double x = 0, double y = 0) : x(x), y(y) {}
};

struct Size {
    double width, height;
    Size(double w = 0, double h = 0) : width(w), height(h) {}
};

struct Rect {
    double x, y, width, height;
    Rect(double x = 0, double y = 0, double w = 0, double h = 0) 
        : x(x), y(y), width(w), height(h) {}
};