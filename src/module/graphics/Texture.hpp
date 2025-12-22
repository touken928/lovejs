#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <memory>
#include <iostream>

class Renderer;

class Texture {
public:
    Texture(SDL_Texture* texture, int width, int height)
        : texture_(texture), width_(width), height_(height) {}
    
    ~Texture() {
        if (texture_) {
            SDL_DestroyTexture(texture_);
        }
    }
    
    // 禁止拷贝，允许移动
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    
    Texture(Texture&& other) noexcept
        : texture_(other.texture_), width_(other.width_), height_(other.height_) {
        other.texture_ = nullptr;
        other.width_ = 0;
        other.height_ = 0;
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
            other.width_ = 0;
            other.height_ = 0;
        }
        return *this;
    }
    
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    SDL_Texture* getSDLTexture() const { return texture_; }
    
    static std::unique_ptr<Texture> loadFromFile(const std::string& path, Renderer* renderer);
    
private:
    SDL_Texture* texture_;
    int width_;
    int height_;
};