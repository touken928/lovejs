#pragma once
#include "IRenderer.hpp"
#include <SDL3/SDL.h>
#include <stack>
#include <unordered_map>
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace render {

/**
 * SDLRenderer - SDL3 渲染器实现
 * 负责 SDL 初始化/销毁和所有渲染操作
 */
class SDLRenderer : public IRenderer {
public:
    SDLRenderer() {
        // 初始化 SDL
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
            return;
        }
        
        sdlInitialized_ = true;
    }
    
    ~SDLRenderer() override {
        destroyWindow();
        if (sdlInitialized_) {
            SDL_Quit();
        }
    }
    
    bool isInitialized() const { return sdlInitialized_; }
    
    // 窗口管理
    bool createWindow(const std::string& title, int width, int height) override {
        if (!sdlInitialized_) return false;
        if (window_) destroyWindow();
        
        window_ = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_RESIZABLE);
        if (!window_) {
            std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
            return false;
        }
        
        renderer_ = SDL_CreateRenderer(window_, nullptr);
        if (!renderer_) {
            std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window_);
            window_ = nullptr;
            return false;
        }
        
        SDL_SetRenderVSync(renderer_, 1);  // 启用垂直同步
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
        SDL_RenderPoint(renderer_, x, y);
    }
    
    void drawLine(float x1, float y1, float x2, float y2) override {
        if (!renderer_) return;
        applyColor();
        SDL_RenderLine(renderer_, x1, y1, x2, y2);
    }
    
    void drawRect(const Rect& rect, bool filled) override {
        if (!renderer_) return;
        applyColor();
        SDL_FRect r = {rect.x, rect.y, rect.width, rect.height};
        if (filled) SDL_RenderFillRect(renderer_, &r);
        else SDL_RenderRect(renderer_, &r);
    }
    
    void drawCircle(float x, float y, float radius, bool filled) override {
        if (!renderer_) return;
        applyColor();
        
        int cx = static_cast<int>(x), cy = static_cast<int>(y), r = static_cast<int>(radius);
        
        if (filled) {
            for (int dy = -r; dy <= r; dy++) {
                int dx = static_cast<int>(std::sqrt(r * r - dy * dy));
                SDL_RenderLine(renderer_, cx - dx, cy + dy, cx + dx, cy + dy);
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
        
        // 只支持 BMP 格式（SDL3 内置支持）
        SDL_Surface* surface = SDL_LoadBMP(path.c_str());
        if (!surface) {
            std::cerr << "Failed to load BMP image: " << path << " - " << SDL_GetError() << std::endl;
            return nullptr;
        }
        
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer_, surface);
        int w = surface->w, h = surface->h;
        SDL_DestroySurface(surface);
        
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
        SDL_FRect dst = {x, y, tex.width * scaleX, tex.height * scaleY};
        SDL_FPoint center = {dst.w / 2, dst.h / 2};
        SDL_RenderTextureRotated(renderer_, tex.sdlTexture, nullptr, &dst,
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
        SDL_RenderPoint(renderer_, cx + x, cy + y);
        SDL_RenderPoint(renderer_, cx - x, cy + y);
        SDL_RenderPoint(renderer_, cx + x, cy - y);
        SDL_RenderPoint(renderer_, cx - x, cy - y);
        SDL_RenderPoint(renderer_, cx + y, cy + x);
        SDL_RenderPoint(renderer_, cx - y, cy + x);
        SDL_RenderPoint(renderer_, cx + y, cy - x);
        SDL_RenderPoint(renderer_, cx - y, cy - x);
    }
};

} // namespace render
