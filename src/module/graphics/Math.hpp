#pragma once
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * 数学工具类和结构体
 */

// 2D点
struct Point {
    double x, y;
    
    Point() : x(0), y(0) {}
    Point(double x, double y) : x(x), y(y) {}
    
    Point operator+(const Point& other) const {
        return Point(x + other.x, y + other.y);
    }
    
    Point operator-(const Point& other) const {
        return Point(x - other.x, y - other.y);
    }
    
    Point operator*(double scale) const {
        return Point(x * scale, y * scale);
    }
    
    double distance(const Point& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        return std::sqrt(dx * dx + dy * dy);
    }
};

// 2D尺寸
struct Size {
    double width, height;
    
    Size() : width(0), height(0) {}
    Size(double width, double height) : width(width), height(height) {}
};

// 2D矩形
struct Rect {
    double x, y, width, height;
    
    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(double x, double y, double width, double height) 
        : x(x), y(y), width(width), height(height) {}
    
    bool contains(const Point& point) const {
        return point.x >= x && point.x <= x + width &&
               point.y >= y && point.y <= y + height;
    }
    
    bool intersects(const Rect& other) const {
        return !(x + width < other.x || other.x + other.width < x ||
                 y + height < other.y || other.y + other.height < y);
    }
    
    Point center() const {
        return Point(x + width / 2, y + height / 2);
    }
};

// 数学工具函数
namespace MathUtils {
    inline double toRadians(double degrees) {
        return degrees * M_PI / 180.0;
    }
    
    inline double toDegrees(double radians) {
        return radians * 180.0 / M_PI;
    }
    
    inline double clamp(double value, double min, double max) {
        return value < min ? min : (value > max ? max : value);
    }
    
    inline double lerp(double a, double b, double t) {
        return a + (b - a) * t;
    }
}