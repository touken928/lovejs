#pragma once
#include "IRenderer.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stack>
#include <unordered_map>
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace render {

/**
 * SDLRenderer - SDL2 渲染器实现
 * 负责 SDL 初始化/销毁和所有渲染操作
 */
class SDLRenderer : public IRenderer {
public:
    SDLRenderer() {
        // 初始化 SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL初始化失败: " << SDL_GetError() << std::endl;
            return;
        }
        
        int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
        if (!(IMG_Init(imgFlags) & imgFlags)) {
            std::cerr << "SDL_image初始化失败: " << IMG_GetError() << std::endl;
            SDL_Quit();
            return;
        }
        
        sdlInitialized_ = true;
    }
    
    ~SDLRenderer() override {
        destroyWindow();
        if (sdlInitialized_) {
            IMG_Quit();
            SDL_Quit();
        }
    }
    
    bool isInitialized() const { return sdlInitialized_; }
    
    // 窗口管理
    bool createWindow(const std::string& title, int width, int height) override {
        if (!sdlInitialized_) return false;
        if (window_) destroyWindow();
        
        window_ = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   width, height, SDL_WINDOW_SHOWN);
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
    
    void destroyWindow() override {
        for (auto& [handle, texture] : textures_) {
            SDL_DestroyTexture(texture.sdlTexture);
        }
        textures_.clear();
        
        if (renderer_) { SDL_DestroyRenderer(renderer_); renderer_ = nullptr; }
        if (window_) { SDL_DestroyWindow(window_); window_ = nullptr; }
    }
    
    bool isWindowCreated() const override { return window_ != nullptr; }
    
    Size getWindowSize() const override {
        if (!window_) return {};
        int w, h;
        SDL_GetWindowSize(window_, &w, &h);
        return {w, h};
    }
    
    // 渲染控制
    void clear(const Color& color) override {
        if (!renderer_) return;
        SDL_SetRenderDrawColor(renderer_, color.r8(), color.g8(), color.b8(), color.a8());
        SDL_RenderClear(renderer_);
    }
    
    void present() override {
        if (renderer_) SDL_RenderPresent(renderer_);
    }
    
    void setColor(const Color& color) override {
        currentColor_ = color;
        if (renderer_) {
            SDL_SetRenderDrawColor(renderer_, color.r8(), color.g8(), color.b8(), color.a8());
        }
    }
    
    // 基本图形
    void drawPoint(float x, float y) override {
        if (!renderer_) return;
        applyColor();
        SDL_RenderDrawPoint(renderer_, static_cast<int>(x), static_cast<int>(y));
    }
    
    void drawLine(float x1, float y1, float x2, float y2) override {
        if (!renderer_) return;
        applyColor();
        SDL_RenderDrawLine(renderer_, static_cast<int>(x1), static_cast<int>(y1),
                          static_cast<int>(x2), static_cast<int>(y2));
    }
    
    void drawRect(const Rect& rect, bool filled) override {
        if (!renderer_) return;
        applyColor();
        SDL_Rect r = {static_cast<int>(rect.x), static_cast<int>(rect.y),
                      static_cast<int>(rect.width), static_cast<int>(rect.height)};
        if (filled) SDL_RenderFillRect(renderer_, &r);
        else SDL_RenderDrawRect(renderer_, &r);
    }
    
    void drawCircle(float x, float y, float radius, bool filled) override {
        if (!renderer_) return;
        applyColor();
        
        int cx = static_cast<int>(x), cy = static_cast<int>(y), r = static_cast<int>(radius);
        
        if (filled) {
            for (int dy = -r; dy <= r; dy++) {
                int dx = static_cast<int>(std::sqrt(r * r - dy * dy));
                SDL_RenderDrawLine(renderer_, cx - dx, cy + dy, cx + dx, cy + dy);
            }
        } else {
            int dx = 0, dy = r, d = 3 - 2 * r;
            drawCirclePoints(cx, cy, dx, dy);
            while (dy >= dx) {
                dx++;
                d = d > 0 ? d + 4 * (dx - dy--) + 10 : d + 4 * dx + 6;
                drawCirclePoints(cx, cy, dx, dy);
            }
        }
    }
    
    // 纹理
    TextureHandle loadTexture(const std::string& path) override {
        if (!renderer_) return nullptr;
        
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (!surface) surface = SDL_LoadBMP(path.c_str());
        if (!surface) {
            std::cerr << "无法加载图像: " << path << std::endl;
            return nullptr;
        }
        
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer_, surface);
        int w = surface->w, h = surface->h;
        SDL_FreeSurface(surface);
        
        if (!tex) return nullptr;
        
        TextureHandle handle = reinterpret_cast<TextureHandle>(nextTextureId_++);
        textures_[handle] = {tex, w, h};
        return handle;
    }
    
    void unloadTexture(TextureHandle handle) override {
        auto it = textures_.find(handle);
        if (it != textures_.end()) {
            SDL_DestroyTexture(it->second.sdlTexture);
            textures_.erase(it);
        }
    }
    
    Size getTextureSize(TextureHandle handle) const override {
        auto it = textures_.find(handle);
        return it != textures_.end() ? Size{it->second.width, it->second.height} : Size{};
    }
    
    void drawTexture(TextureHandle handle, float x, float y,
                    float rotation, float scaleX, float scaleY) override {
        if (!renderer_) return;
        auto it = textures_.find(handle);
        if (it == textures_.end()) return;
        
        auto& tex = it->second;
        SDL_Rect dst = {static_cast<int>(x), static_cast<int>(y),
                       static_cast<int>(tex.width * scaleX), static_cast<int>(tex.height * scaleY)};
        SDL_Point center = {dst.w / 2, dst.h / 2};
        SDL_RenderCopyEx(renderer_, tex.sdlTexture, nullptr, &dst,
                        rotation * 180.0 / M_PI, &center, SDL_FLIP_NONE);
    }
    
    // 变换
    void pushMatrix() override { transformStack_.push(transform_); }
    
    void popMatrix() override {
        if (!transformStack_.empty()) {
            transform_ = transformStack_.top();
            transformStack_.pop();
        }
    }
    
    void translate(float x, float y) override { transform_.tx += x; transform_.ty += y; }
    void rotate(float angle) override { transform_.rotation += angle; }
    void scale(float x, float y) override { transform_.sx *= x; transform_.sy *= y; }

private:
    bool sdlInitialized_ = false;
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    Color currentColor_ = Color::WHITE;
    
    struct Transform { float tx = 0, ty = 0, rotation = 0, sx = 1, sy = 1; };
    Transform transform_;
    std::stack<Transform> transformStack_;
    
    struct TextureData { SDL_Texture* sdlTexture; int width, height; };
    std::unordered_map<TextureHandle, TextureData> textures_;
    uintptr_t nextTextureId_ = 1;
    
    void applyColor() {
        SDL_SetRenderDrawColor(renderer_, currentColor_.r8(), currentColor_.g8(),
                              currentColor_.b8(), currentColor_.a8());
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
};

} // namespace render
