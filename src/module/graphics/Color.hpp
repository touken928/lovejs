#pragma once
#include <SDL2/SDL.h>
#include <algorithm>

class Color {
public:
    double r, g, b, a;
    
    Color(double r = 1.0, double g = 1.0, double b = 1.0, double a = 1.0)
        : r(std::clamp(r, 0.0, 1.0))
        , g(std::clamp(g, 0.0, 1.0))
        , b(std::clamp(b, 0.0, 1.0))
        , a(std::clamp(a, 0.0, 1.0)) {}
    
    static Color fromRGB(int r, int g, int b, int a = 255) {
        return Color(r / 255.0, g / 255.0, b / 255.0, a / 255.0);
    }
    
    SDL_Color toSDL() const {
        return {
            static_cast<Uint8>(r * 255),
            static_cast<Uint8>(g * 255),
            static_cast<Uint8>(b * 255),
            static_cast<Uint8>(a * 255)
        };
    }
    
    static const Color WHITE;
    static const Color BLACK;
    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;
    static const Color YELLOW;
    static const Color MAGENTA;
    static const Color CYAN;
    static const Color TRANSPARENT;
};

// 预定义颜色实现
inline const Color Color::WHITE = Color(1.0, 1.0, 1.0, 1.0);
inline const Color Color::BLACK = Color(0.0, 0.0, 0.0, 1.0);
inline const Color Color::RED = Color(1.0, 0.0, 0.0, 1.0);
inline const Color Color::GREEN = Color(0.0, 1.0, 0.0, 1.0);
inline const Color Color::BLUE = Color(0.0, 0.0, 1.0, 1.0);
inline const Color Color::YELLOW = Color(1.0, 1.0, 0.0, 1.0);
inline const Color Color::MAGENTA = Color(1.0, 0.0, 1.0, 1.0);
inline const Color Color::CYAN = Color(0.0, 1.0, 1.0, 1.0);
inline const Color Color::TRANSPARENT = Color(0.0, 0.0, 0.0, 0.0);