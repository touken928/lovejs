#pragma once
#include <cstdint>

namespace render {

struct Color {
    float r, g, b, a;
    
    Color() : r(0), g(0), b(0), a(1) {}
    Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
    
    uint8_t r8() const { return static_cast<uint8_t>(r * 255); }
    uint8_t g8() const { return static_cast<uint8_t>(g * 255); }
    uint8_t b8() const { return static_cast<uint8_t>(b * 255); }
    uint8_t a8() const { return static_cast<uint8_t>(a * 255); }
    
    static const Color WHITE;
    static const Color BLACK;
    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;
};

inline const Color Color::WHITE{1, 1, 1, 1};
inline const Color Color::BLACK{0, 0, 0, 1};
inline const Color Color::RED{1, 0, 0, 1};
inline const Color Color::GREEN{0, 1, 0, 1};
inline const Color Color::BLUE{0, 0, 1, 1};

struct Rect {
    float x, y, width, height;
    
    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {}
};

struct Size {
    int width, height;
    
    Size() : width(0), height(0) {}
    Size(int w, int h) : width(w), height(h) {}
};

using TextureHandle = void*;

} // namespace render
