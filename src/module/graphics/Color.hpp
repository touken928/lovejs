#pragma once
#include <SDL2/SDL.h>
#include <algorithm>

/**
 * Color - 颜色类
 * 支持浮点数和整数颜色值，提供常用颜色常量
 */
class Color {
public:
    double r, g, b, a;
    
    // 构造函数
    Color() : r(0), g(0), b(0), a(1) {}
    Color(double r, double g, double b, double a = 1.0) 
        : r(r), g(g), b(b), a(a) {}
    Color(int r, int g, int b, int a = 255) 
        : r(r / 255.0), g(g / 255.0), b(b / 255.0), a(a / 255.0) {}
    
    // 转换为SDL颜色
    SDL_Color toSDL() const {
        return {
            static_cast<Uint8>(std::clamp(r * 255.0, 0.0, 255.0)),
            static_cast<Uint8>(std::clamp(g * 255.0, 0.0, 255.0)),
            static_cast<Uint8>(std::clamp(b * 255.0, 0.0, 255.0)),
            static_cast<Uint8>(std::clamp(a * 255.0, 0.0, 255.0))
        };
    }
    
    // 常用颜色常量
    static const Color WHITE;
    static const Color BLACK;
    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;
    static const Color YELLOW;
    static const Color CYAN;
    static const Color MAGENTA;
    static const Color TRANSPARENT;
};

// 常量定义
inline const Color Color::WHITE(1.0, 1.0, 1.0, 1.0);
inline const Color Color::BLACK(0.0, 0.0, 0.0, 1.0);
inline const Color Color::RED(1.0, 0.0, 0.0, 1.0);
inline const Color Color::GREEN(0.0, 1.0, 0.0, 1.0);
inline const Color Color::BLUE(0.0, 0.0, 1.0, 1.0);
inline const Color Color::YELLOW(1.0, 1.0, 0.0, 1.0);
inline const Color Color::CYAN(0.0, 1.0, 1.0, 1.0);
inline const Color Color::MAGENTA(1.0, 0.0, 1.0, 1.0);
inline const Color Color::TRANSPARENT(0.0, 0.0, 0.0, 0.0);