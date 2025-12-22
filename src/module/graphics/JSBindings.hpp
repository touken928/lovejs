#pragma once
#include "Graphics.hpp"
#include <vector>
#include <iostream>

/**
 * JSBindings - JavaScript绑定函数
 * 负责将Graphics类的功能暴露给JavaScript
 */
namespace GraphicsJS {
    
    // 窗口管理
    inline void setWindow(const std::string& title, int width, int height) {
        Graphics::setWindow(title, width, height);
    }
    
    inline std::vector<double> getWindowSize() {
        Size size = Graphics::getWindowSize();
        return {size.width, size.height};
    }
    
    // 渲染控制
    inline void clear(double r, double g, double b, double a) {
        Graphics::clear(Color(r, g, b, a));
    }
    
    inline void present() {
        Graphics::present();
    }
    
    // 颜色设置
    inline void setColor(double r, double g, double b, double a) {
        Graphics::setColor(Color(r, g, b, a));
    }
    
    // 基本绘制
    inline void point(double x, double y) {
        Graphics::drawPoint(x, y);
    }
    
    inline void line(double x1, double y1, double x2, double y2) {
        Graphics::drawLine(x1, y1, x2, y2);
    }
    
    inline void rectangle(double x, double y, double width, double height, bool filled = false) {
        Graphics::drawRectangle(x, y, width, height, filled);
    }
    
    inline void circle(double x, double y, double radius, bool filled = false) {
        Graphics::drawCircle(x, y, radius, filled);
    }
    
    // 纹理管理
    inline std::string loadTexture(const std::string& path) {
        auto texture = Graphics::loadTexture(path);
        return texture ? path : "";
    }
    
    inline void drawTexture(const std::string& textureId, double x, double y, 
                           double rotation = 0.0, double scaleX = 1.0, double scaleY = 1.0) {
        Graphics::drawTexture(textureId, x, y, rotation, scaleX, scaleY);
    }
    
    // 变换管理
    inline void push() {
        Graphics::pushMatrix();
    }
    
    inline void pop() {
        Graphics::popMatrix();
    }
    
    inline void translate(double x, double y) {
        Graphics::translate(x, y);
    }
    
    inline void rotate(double angle) {
        Graphics::rotate(angle);
    }
    
    inline void scale(double x, double y) {
        Graphics::scale(x, y);
    }
    
}