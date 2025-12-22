#pragma once
#include <SDL2/SDL.h>
#include <memory>
#include <stack>
#include <cmath>
#include <iostream>
#include "Color.hpp"
#include "Texture.hpp"
#include "Math.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class Renderer {
public:
    Renderer() : window_(nullptr), renderer_(nullptr), currentColor_(Color::WHITE) {}
    
    ~Renderer() {
        destroyWindow();
    }
    
    // 窗口管理
    bool createWindow(const std::string& title, int width, int height) {
        if (window_) {
            destroyWindow();
        }
        
        window_ = SDL_CreateWindow(
            title.c_str(),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            width, height,
            SDL_WINDOW_SHOWN
        );
        
        if (!window_) {
            std::cerr << "创建窗口失败: " << SDL_GetError() << std::endl;
            return false;
        }
        
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer_) {
            std::cerr << "创建渲染器失败: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window_);
            window_ = nullptr;
            return false;
        }
        
        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
        return true;
    }
    
    void destroyWindow() {
        if (renderer_) {
            SDL_DestroyRenderer(renderer_);
            renderer_ = nullptr;
        }
        if (window_) {
            SDL_DestroyWindow(window_);
            window_ = nullptr;
        }
    }
    
    bool isWindowCreated() const { return window_ != nullptr; }
    
    // 渲染控制
    void clear(const Color& color = Color::BLACK) {
        if (!renderer_) return;
        
        SDL_Color c = color.toSDL();
        SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
        SDL_RenderClear(renderer_);
    }
    
    void present() {
        if (!renderer_) return;
        SDL_RenderPresent(renderer_);
    }
    
    // 基本绘制
    void setColor(const Color& color) {
        currentColor_ = color;
        if (renderer_) {
            SDL_Color c = color.toSDL();
            SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
        }
    }
    
    void drawPoint(double x, double y) {
        if (!renderer_) return;
        setColor(currentColor_);
        SDL_RenderDrawPoint(renderer_, static_cast<int>(x), static_cast<int>(y));
    }
    
    void drawLine(double x1, double y1, double x2, double y2) {
        if (!renderer_) return;
        setColor(currentColor_);
        SDL_RenderDrawLine(renderer_, 
            static_cast<int>(x1), static_cast<int>(y1),
            static_cast<int>(x2), static_cast<int>(y2));
    }
    
    void drawRectangle(const Rect& rect, bool filled = false) {
        if (!renderer_) return;
        setColor(currentColor_);
        
        SDL_Rect sdlRect = {
            static_cast<int>(rect.x),
            static_cast<int>(rect.y),
            static_cast<int>(rect.width),
            static_cast<int>(rect.height)
        };
        
        if (filled) {
            SDL_RenderFillRect(renderer_, &sdlRect);
        } else {
            SDL_RenderDrawRect(renderer_, &sdlRect);
        }
    }
    
    void drawCircle(double x, double y, double radius, bool filled = false) {
        if (!renderer_) return;
        setColor(currentColor_);
        
        int cx = static_cast<int>(x);
        int cy = static_cast<int>(y);
        int r = static_cast<int>(radius);
        
        if (filled) {
            // 填充圆形 - 使用扫描线算法
            for (int dy = -r; dy <= r; dy++) {
                int dx = static_cast<int>(std::sqrt(r * r - dy * dy));
                SDL_RenderDrawLine(renderer_, cx - dx, cy + dy, cx + dx, cy + dy);
            }
        } else {
            // 空心圆形 - 使用Bresenham算法
            int dx = 0, dy = r;
            int d = 3 - 2 * r;
            
            drawCirclePoints(cx, cy, dx, dy);
            
            while (dy >= dx) {
                dx++;
                if (d > 0) {
                    dy--;
                    d = d + 4 * (dx - dy) + 10;
                } else {
                    d = d + 4 * dx + 6;
                }
                drawCirclePoints(cx, cy, dx, dy);
            }
        }
    }
    
    // 纹理绘制
    void drawTexture(Texture* texture, double x, double y, double rotation = 0.0, 
                    double scaleX = 1.0, double scaleY = 1.0) {
        if (!renderer_ || !texture) return;
        
        SDL_Rect dst = {
            static_cast<int>(x),
            static_cast<int>(y),
            static_cast<int>(texture->getWidth() * scaleX),
            static_cast<int>(texture->getHeight() * scaleY)
        };
        
        SDL_Point center = {
            static_cast<int>(texture->getWidth() * scaleX / 2),
            static_cast<int>(texture->getHeight() * scaleY / 2)
        };
        
        SDL_RenderCopyEx(renderer_, texture->getSDLTexture(), nullptr, &dst, 
                         rotation * 180.0 / M_PI, &center, SDL_FLIP_NONE);
    }
    
    void drawTexture(Texture* texture, const Rect& src, const Rect& dst, double rotation = 0.0) {
        if (!renderer_ || !texture) return;
        
        SDL_Rect srcRect = {
            static_cast<int>(src.x), static_cast<int>(src.y),
            static_cast<int>(src.width), static_cast<int>(src.height)
        };
        
        SDL_Rect dstRect = {
            static_cast<int>(dst.x), static_cast<int>(dst.y),
            static_cast<int>(dst.width), static_cast<int>(dst.height)
        };
        
        SDL_Point center = {
            static_cast<int>(dst.width / 2),
            static_cast<int>(dst.height / 2)
        };
        
        SDL_RenderCopyEx(renderer_, texture->getSDLTexture(), &srcRect, &dstRect,
                         rotation * 180.0 / M_PI, &center, SDL_FLIP_NONE);
    }
    
    // 变换管理
    void pushMatrix() {
        transformStack_.push(currentTransform_);
    }
    
    void popMatrix() {
        if (!transformStack_.empty()) {
            currentTransform_ = transformStack_.top();
            transformStack_.pop();
            applyTransform();
        }
    }
    
    void translate(double x, double y) {
        currentTransform_.translateX += x;
        currentTransform_.translateY += y;
        applyTransform();
    }
    
    void rotate(double angle) {
        currentTransform_.rotation += angle;
        applyTransform();
    }
    
    void scale(double x, double y) {
        currentTransform_.scaleX *= x;
        currentTransform_.scaleY *= y;
        applyTransform();
    }
    
    SDL_Renderer* getSDLRenderer() const { return renderer_; }
    
    Size getWindowSize() const {
        if (!window_) return Size(0, 0);
        
        int w, h;
        SDL_GetWindowSize(window_, &w, &h);
        return Size(w, h);
    }
    
    // 文本渲染（简单的像素字体）
    void drawText(const std::string& text, double x, double y) {
        if (!renderer_) return;
        
        int cursorX = static_cast<int>(x);
        int cursorY = static_cast<int>(y);
        
        for (char c : text) {
            drawChar(c, cursorX, cursorY);
            cursorX += 8; // 字符宽度 + 间距
        }
    }
    
private:
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    Color currentColor_;
    
    struct Transform {
        double translateX = 0, translateY = 0;
        double rotation = 0;
        double scaleX = 1, scaleY = 1;
    };
    std::stack<Transform> transformStack_;
    Transform currentTransform_;
    
    void applyTransform() {
        // SDL2没有内置的变换矩阵，这里只是记录状态
    }
    
    void drawCirclePoints(int cx, int cy, int x, int y) {
        SDL_RenderDrawPoint(renderer_, cx + x, cy + y);
        SDL_RenderDrawPoint(renderer_, cx - x, cy + y);
        SDL_RenderDrawPoint(renderer_, cx + x, cy - y);
        SDL_RenderDrawPoint(renderer_, cx - x, cy - y);
        SDL_RenderDrawPoint(renderer_, cx + y, cy + x);
        SDL_RenderDrawPoint(renderer_, cx - y, cy + x);
        SDL_RenderDrawPoint(renderer_, cx + y, cy - x);
        SDL_RenderDrawPoint(renderer_, cx - y, cy - x);
    }
    
    // 简单的像素字体绘制
    void drawChar(char c, int x, int y) {
        setColor(currentColor_);
        
        // 简单的7x8像素字体数据
        const int charWidth = 7;
        const int charHeight = 8;
        
        // 获取字符的位图数据
        const uint8_t* bitmap = getCharBitmap(c);
        if (!bitmap) return;
        
        // 绘制字符
        for (int row = 0; row < charHeight; row++) {
            uint8_t rowData = bitmap[row];
            for (int col = 0; col < charWidth; col++) {
                if (rowData & (1 << (charWidth - 1 - col))) {
                    SDL_RenderDrawPoint(renderer_, x + col, y + row);
                }
            }
        }
    }
    
    // 获取字符的位图数据
    const uint8_t* getCharBitmap(char c) {
        // 简单的ASCII字符位图 (7x8像素)
        static const uint8_t font_data[][8] = {
            // 空格 (32)
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
            // ! (33)
            {0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x08, 0x00},
            // " (34)
            {0x14, 0x14, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00},
            // # (35)
            {0x14, 0x14, 0x3E, 0x14, 0x3E, 0x14, 0x14, 0x00},
            // $ (36)
            {0x08, 0x1E, 0x20, 0x1C, 0x02, 0x3C, 0x08, 0x00},
            // % (37)
            {0x30, 0x32, 0x04, 0x08, 0x10, 0x26, 0x06, 0x00},
            // & (38)
            {0x10, 0x28, 0x28, 0x10, 0x2A, 0x24, 0x1A, 0x00},
            // ' (39)
            {0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00},
            // ( (40)
            {0x04, 0x08, 0x10, 0x10, 0x10, 0x08, 0x04, 0x00},
            // ) (41)
            {0x10, 0x08, 0x04, 0x04, 0x04, 0x08, 0x10, 0x00},
            // * (42)
            {0x00, 0x08, 0x2A, 0x1C, 0x2A, 0x08, 0x00, 0x00},
            // + (43)
            {0x00, 0x08, 0x08, 0x3E, 0x08, 0x08, 0x00, 0x00},
            // , (44)
            {0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x10, 0x00},
            // - (45)
            {0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x00},
            // . (46)
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x00},
            // / (47)
            {0x02, 0x02, 0x04, 0x08, 0x10, 0x20, 0x20, 0x00},
            // 0-9 (48-57)
            {0x1C, 0x22, 0x26, 0x2A, 0x32, 0x22, 0x1C, 0x00}, // 0
            {0x08, 0x18, 0x08, 0x08, 0x08, 0x08, 0x1C, 0x00}, // 1
            {0x1C, 0x22, 0x02, 0x0C, 0x10, 0x20, 0x3E, 0x00}, // 2
            {0x1C, 0x22, 0x02, 0x0C, 0x02, 0x22, 0x1C, 0x00}, // 3
            {0x04, 0x0C, 0x14, 0x24, 0x3E, 0x04, 0x04, 0x00}, // 4
            {0x3E, 0x20, 0x3C, 0x02, 0x02, 0x22, 0x1C, 0x00}, // 5
            {0x0E, 0x10, 0x20, 0x3C, 0x22, 0x22, 0x1C, 0x00}, // 6
            {0x3E, 0x02, 0x04, 0x08, 0x10, 0x10, 0x10, 0x00}, // 7
            {0x1C, 0x22, 0x22, 0x1C, 0x22, 0x22, 0x1C, 0x00}, // 8
            {0x1C, 0x22, 0x22, 0x1E, 0x02, 0x04, 0x38, 0x00}, // 9
            // : (58)
            {0x00, 0x08, 0x08, 0x00, 0x08, 0x08, 0x00, 0x00},
            // ; (59)
            {0x00, 0x08, 0x08, 0x00, 0x08, 0x08, 0x10, 0x00},
            // < (60)
            {0x04, 0x08, 0x10, 0x20, 0x10, 0x08, 0x04, 0x00},
            // = (61)
            {0x00, 0x00, 0x3E, 0x00, 0x3E, 0x00, 0x00, 0x00},
            // > (62)
            {0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10, 0x00},
            // ? (63)
            {0x1C, 0x22, 0x02, 0x04, 0x08, 0x00, 0x08, 0x00},
            // @ (64)
            {0x1C, 0x22, 0x2A, 0x2E, 0x28, 0x20, 0x1E, 0x00},
            // A-Z (65-90)
            {0x08, 0x14, 0x22, 0x22, 0x3E, 0x22, 0x22, 0x00}, // A
            {0x3C, 0x22, 0x22, 0x3C, 0x22, 0x22, 0x3C, 0x00}, // B
            {0x1C, 0x22, 0x20, 0x20, 0x20, 0x22, 0x1C, 0x00}, // C
            {0x3C, 0x22, 0x22, 0x22, 0x22, 0x22, 0x3C, 0x00}, // D
            {0x3E, 0x20, 0x20, 0x3C, 0x20, 0x20, 0x3E, 0x00}, // E
            {0x3E, 0x20, 0x20, 0x3C, 0x20, 0x20, 0x20, 0x00}, // F
            {0x1C, 0x22, 0x20, 0x2E, 0x22, 0x22, 0x1C, 0x00}, // G
            {0x22, 0x22, 0x22, 0x3E, 0x22, 0x22, 0x22, 0x00}, // H
            {0x1C, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1C, 0x00}, // I
            {0x0E, 0x04, 0x04, 0x04, 0x04, 0x24, 0x18, 0x00}, // J
            {0x22, 0x24, 0x28, 0x30, 0x28, 0x24, 0x22, 0x00}, // K
            {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3E, 0x00}, // L
            {0x22, 0x36, 0x2A, 0x2A, 0x22, 0x22, 0x22, 0x00}, // M
            {0x22, 0x32, 0x2A, 0x26, 0x22, 0x22, 0x22, 0x00}, // N
            {0x1C, 0x22, 0x22, 0x22, 0x22, 0x22, 0x1C, 0x00}, // O
            {0x3C, 0x22, 0x22, 0x3C, 0x20, 0x20, 0x20, 0x00}, // P
            {0x1C, 0x22, 0x22, 0x22, 0x2A, 0x24, 0x1A, 0x00}, // Q
            {0x3C, 0x22, 0x22, 0x3C, 0x28, 0x24, 0x22, 0x00}, // R
            {0x1C, 0x22, 0x20, 0x1C, 0x02, 0x22, 0x1C, 0x00}, // S
            {0x3E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00}, // T
            {0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x1C, 0x00}, // U
            {0x22, 0x22, 0x22, 0x22, 0x14, 0x14, 0x08, 0x00}, // V
            {0x22, 0x22, 0x22, 0x2A, 0x2A, 0x36, 0x22, 0x00}, // W
            {0x22, 0x22, 0x14, 0x08, 0x14, 0x22, 0x22, 0x00}, // X
            {0x22, 0x22, 0x14, 0x08, 0x08, 0x08, 0x08, 0x00}, // Y
            {0x3E, 0x02, 0x04, 0x08, 0x10, 0x20, 0x3E, 0x00}, // Z
        };
        
        if (c >= 32 && c <= 90) {
            return font_data[c - 32];
        }
        
        // 对于小写字母，使用大写字母的位图
        if (c >= 97 && c <= 122) {
            return font_data[c - 97 + 33]; // 'a' -> 'A'
        }
        
        return font_data[0]; // 默认返回空格
    }
};

// Texture实现（需要Renderer定义后才能实现）
inline std::unique_ptr<Texture> Texture::loadFromFile(const std::string& path, Renderer* renderer) {
    if (!renderer || !renderer->getSDLRenderer()) {
        std::cerr << "无效的渲染器" << std::endl;
        return nullptr;
    }
    
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        surface = SDL_LoadBMP(path.c_str());
        if (!surface) {
            std::cerr << "无法加载图像: " << path << " - " << SDL_GetError() << std::endl;
            return nullptr;
        }
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer->getSDLRenderer(), surface);
    int width = surface->w;
    int height = surface->h;
    SDL_FreeSurface(surface);
    
    if (!texture) {
        std::cerr << "无法创建纹理: " << SDL_GetError() << std::endl;
        return nullptr;
    }
    
    return std::make_unique<Texture>(texture, width, height);
}