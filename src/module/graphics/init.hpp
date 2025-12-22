#pragma once
#include "../../core/JSEngine.hpp"
#include "JSBindings.hpp"

/**
 * Graphics模块初始化
 * 负责将图形功能注册到JavaScript引擎
 */
inline void initGraphicsModule() {
    auto& engine = JSEngine::instance();
    auto& graphics = engine.global().module("graphics");
    
    // 窗口管理
    graphics.func("setWindow", GraphicsJS::setWindow);
    graphics.func("getWindowSize", GraphicsJS::getWindowSize);
    
    // 渲染控制
    graphics.func("clear", GraphicsJS::clear);
    graphics.func("present", GraphicsJS::present);
    
    // 颜色和绘制
    graphics.func("setColor", GraphicsJS::setColor);
    graphics.func("point", GraphicsJS::point);
    graphics.func("line", GraphicsJS::line);
    graphics.func("rectangle", GraphicsJS::rectangle);
    graphics.func("circle", GraphicsJS::circle);
    
    // 纹理
    graphics.func("loadTexture", GraphicsJS::loadTexture);
    graphics.func("drawTexture", GraphicsJS::drawTexture);
    
    // 变换
    graphics.func("push", GraphicsJS::push);
    graphics.func("pop", GraphicsJS::pop);
    graphics.func("translate", GraphicsJS::translate);
    graphics.func("rotate", GraphicsJS::rotate);
    graphics.func("scale", GraphicsJS::scale);
    
    // 文本渲染
    graphics.func("print", GraphicsJS::print);
    
    // 预定义颜色常量
    graphics.value("WHITE", std::vector<double>{1.0, 1.0, 1.0, 1.0});
    graphics.value("BLACK", std::vector<double>{0.0, 0.0, 0.0, 1.0});
    graphics.value("RED", std::vector<double>{1.0, 0.0, 0.0, 1.0});
    graphics.value("GREEN", std::vector<double>{0.0, 1.0, 0.0, 1.0});
    graphics.value("BLUE", std::vector<double>{0.0, 0.0, 1.0, 1.0});
    graphics.value("YELLOW", std::vector<double>{1.0, 1.0, 0.0, 1.0});
    graphics.value("CYAN", std::vector<double>{0.0, 1.0, 1.0, 1.0});
    graphics.value("MAGENTA", std::vector<double>{1.0, 0.0, 1.0, 1.0});
}