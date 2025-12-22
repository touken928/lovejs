#pragma once
#include "../../core/JSEngine.hpp"
#include "Renderer.hpp"
#include "Texture.hpp"
#include "Color.hpp"
#include "Math.hpp"
#include <memory>
#include <unordered_map>
#include <string>
#include <iostream>

class Graphics {
public:
    static Renderer& getRenderer() {
        if (!renderer_) {
            renderer_ = std::make_unique<Renderer>();
        }
        return *renderer_;
    }
    
    static std::shared_ptr<Texture> loadTexture(const std::string& path) {
        auto it = textures_.find(path);
        if (it != textures_.end()) {
            return it->second;
        }
        
        auto texture = Texture::loadFromFile(path, renderer_.get());
        if (texture) {
            auto shared = std::shared_ptr<Texture>(texture.release());
            textures_[path] = shared;
            return shared;
        }
        
        return nullptr;
    }
    
    static void unloadTexture(const std::string& path) {
        textures_.erase(path);
    }
    
    static void clearTextures() {
        textures_.clear();
    }

private:
    static std::unique_ptr<Renderer> renderer_;
    static std::unordered_map<std::string, std::shared_ptr<Texture>> textures_;
    
    // JS绑定函数
    static void jsSetWindow(const std::string& title, int width, int height) {
        if (renderer_) {
            renderer_->createWindow(title, width, height);
        }
    }
    
    static void jsClear(double r, double g, double b, double a) {
        if (renderer_) {
            renderer_->clear(Color(r, g, b, a));
        }
    }
    
    static void jsPresent() {
        if (renderer_) {
            renderer_->present();
        }
    }
    
    static void jsSetColor(double r, double g, double b, double a) {
        if (renderer_) {
            renderer_->setColor(Color(r, g, b, a));
        }
    }
    
    static void jsDrawPoint(double x, double y) {
        if (renderer_) {
            renderer_->drawPoint(x, y);
        }
    }
    
    static void jsDrawLine(double x1, double y1, double x2, double y2) {
        if (renderer_) {
            renderer_->drawLine(x1, y1, x2, y2);
        }
    }
    
    static void jsDrawRectangle(double x, double y, double width, double height, bool filled) {
        if (renderer_) {
            renderer_->drawRectangle(Rect(x, y, width, height), filled);
        }
    }
    
    static void jsDrawCircle(double x, double y, double radius, bool filled) {
        if (renderer_) {
            renderer_->drawCircle(x, y, radius, filled);
        }
    }
    
    static std::string jsLoadTexture(const std::string& path) {
        auto texture = loadTexture(path);
        return texture ? path : "";
    }
    
    static void jsDrawTexture(const std::string& textureId, double x, double y, 
                             double rotation, double scaleX, double scaleY) {
        if (!renderer_) return;
        
        auto it = textures_.find(textureId);
        if (it != textures_.end()) {
            renderer_->drawTexture(it->second.get(), x, y, rotation, scaleX, scaleY);
        }
    }
    
    static void jsPushMatrix() {
        if (renderer_) {
            renderer_->pushMatrix();
        }
    }
    
    static void jsPopMatrix() {
        if (renderer_) {
            renderer_->popMatrix();
        }
    }
    
    static void jsTranslate(double x, double y) {
        if (renderer_) {
            renderer_->translate(x, y);
        }
    }
    
    static void jsRotate(double angle) {
        if (renderer_) {
            renderer_->rotate(angle);
        }
    }
    
    static void jsScale(double x, double y) {
        if (renderer_) {
            renderer_->scale(x, y);
        }
    }
    
    static std::vector<double> jsGetWindowSize() {
        if (renderer_) {
            Size size = renderer_->getWindowSize();
            return {size.width, size.height};
        }
        return {0, 0};
    }
    
    static void jsConsoleLog(const std::string& msg) {
        std::cout << "[JS] " << msg << std::endl;
    }
    
    friend void initGraphicsModule();
};

// 静态成员定义
inline std::unique_ptr<Renderer> Graphics::renderer_ = nullptr;
inline std::unordered_map<std::string, std::shared_ptr<Texture>> Graphics::textures_;