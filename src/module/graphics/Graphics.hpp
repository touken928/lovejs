#pragma once
#include "../../core/SDLManager.hpp"
#include "Renderer.hpp"
#include "Texture.hpp"
#include "Color.hpp"
#include <memory>
#include <unordered_map>
#include <string>
#include <iostream>

/**
 * Graphics - 图形模块核心类
 * 负责管理渲染器和纹理资源，提供统一的图形接口
 */
class Graphics {
public:
    // 获取渲染器实例
    static Renderer& getRenderer() {
        ensureInitialized();
        return *renderer_;
    }
    
    // 纹理管理
    static std::shared_ptr<Texture> loadTexture(const std::string& path) {
        ensureInitialized();
        
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
    
    static void clearAllTextures() {
        textures_.clear();
    }
    
    // 窗口管理
    static void setWindow(const std::string& title, int width, int height) {
        ensureInitialized();
        renderer_->createWindow(title, width, height);
    }
    
    static Size getWindowSize() {
        ensureInitialized();
        return renderer_->getWindowSize();
    }
    
    // 渲染控制
    static void clear(const Color& color = Color::BLACK) {
        ensureInitialized();
        renderer_->clear(color);
    }
    
    static void present() {
        ensureInitialized();
        renderer_->present();
    }
    
    // 颜色设置
    static void setColor(const Color& color) {
        ensureInitialized();
        renderer_->setColor(color);
    }
    
    // 基本绘制
    static void drawPoint(double x, double y) {
        ensureInitialized();
        renderer_->drawPoint(x, y);
    }
    
    static void drawLine(double x1, double y1, double x2, double y2) {
        ensureInitialized();
        renderer_->drawLine(x1, y1, x2, y2);
    }
    
    static void drawRectangle(double x, double y, double width, double height, bool filled = false) {
        ensureInitialized();
        renderer_->drawRectangle(Rect(x, y, width, height), filled);
    }
    
    static void drawCircle(double x, double y, double radius, bool filled = false) {
        ensureInitialized();
        renderer_->drawCircle(x, y, radius, filled);
    }
    
    // 纹理绘制
    static void drawTexture(const std::string& textureId, double x, double y, 
                           double rotation = 0.0, double scaleX = 1.0, double scaleY = 1.0) {
        ensureInitialized();
        
        auto it = textures_.find(textureId);
        if (it != textures_.end()) {
            renderer_->drawTexture(it->second.get(), x, y, rotation, scaleX, scaleY);
        }
    }
    
    // 变换管理
    static void pushMatrix() {
        ensureInitialized();
        renderer_->pushMatrix();
    }
    
    static void popMatrix() {
        ensureInitialized();
        renderer_->popMatrix();
    }
    
    static void translate(double x, double y) {
        ensureInitialized();
        renderer_->translate(x, y);
    }
    
    static void rotate(double angle) {
        ensureInitialized();
        renderer_->rotate(angle);
    }
    
    static void scale(double x, double y) {
        ensureInitialized();
        renderer_->scale(x, y);
    }
    
    // 文本渲染
    static void print(const std::string& text, double x, double y) {
        ensureInitialized();
        renderer_->drawText(text, x, y);
    }

private:
    static std::unique_ptr<Renderer> renderer_;
    static std::unordered_map<std::string, std::shared_ptr<Texture>> textures_;
    static bool initialized_;
    
    // 确保图形系统已初始化
    static void ensureInitialized() {
        if (initialized_) return;
        
        // 确保SDL已初始化
        if (!SDLManager::instance().isInitialized()) {
            throw std::runtime_error("SDL未初始化，无法使用图形功能");
        }
        
        renderer_ = std::make_unique<Renderer>();
        initialized_ = true;
    }
};

// 静态成员定义
inline std::unique_ptr<Renderer> Graphics::renderer_ = nullptr;
inline std::unordered_map<std::string, std::shared_ptr<Texture>> Graphics::textures_;
inline bool Graphics::initialized_ = false;