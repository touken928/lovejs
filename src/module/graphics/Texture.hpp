#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <memory>
#include <string>
#include <iostream>

// 前向声明
class Renderer;

/**
 * Texture - 纹理类
 * 封装SDL2纹理，提供加载和管理功能
 */
class Texture {
public:
    Texture(SDL_Texture* texture, int width, int height)
        : texture_(texture), width_(width), height_(height) {}
    
    ~Texture() {
        if (texture_) {
            SDL_DestroyTexture(texture_);
        }
    }
    
    // 禁止拷贝
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    
    // 允许移动
    Texture(Texture&& other) noexcept 
        : texture_(other.texture_), width_(other.width_), height_(other.height_) {
        other.texture_ = nullptr;
    }
    
    Texture& operator=(Texture&& other) noexcept {
        if (this != &other) {
            if (texture_) {
                SDL_DestroyTexture(texture_);
            }
            texture_ = other.texture_;
            width_ = other.width_;
            height_ = other.height_;
            other.texture_ = nullptr;
        }
        return *this;
    }
    
    // 访问器
    SDL_Texture* getSDLTexture() const { return texture_; }
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    
    // 静态加载函数（实现在Renderer.hpp中，因为需要Renderer定义）
    static std::unique_ptr<Texture> loadFromFile(const std::string& path, Renderer* renderer);

private:
    SDL_Texture* texture_;
    int width_, height_;
};