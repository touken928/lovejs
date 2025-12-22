#pragma once
#include "../../core/JSEngine.hpp"
#include "Graphics.hpp"

// 图形模块初始化函数
inline void initGraphicsModule() {
    // 初始化渲染器
    Graphics::getRenderer();
    
    auto& engine = JSEngine::instance();
    auto& graphics = engine.global().module("graphics");
    
    // 窗口管理
    graphics.func("setWindow", Graphics::jsSetWindow);
    graphics.func("getWindowSize", Graphics::jsGetWindowSize);
    
    // 渲染控制
    graphics.func("clear", Graphics::jsClear);
    graphics.func("present", Graphics::jsPresent);
    
    // 颜色和绘制
    graphics.func("setColor", Graphics::jsSetColor);
    graphics.func("point", Graphics::jsDrawPoint);
    graphics.func("line", Graphics::jsDrawLine);
    graphics.func("rectangle", Graphics::jsDrawRectangle);
    graphics.func("circle", Graphics::jsDrawCircle);
    
    // 纹理
    graphics.func("loadTexture", Graphics::jsLoadTexture);
    graphics.func("drawTexture", Graphics::jsDrawTexture);
    
    // 变换
    graphics.func("push", Graphics::jsPushMatrix);
    graphics.func("pop", Graphics::jsPopMatrix);
    graphics.func("translate", Graphics::jsTranslate);
    graphics.func("rotate", Graphics::jsRotate);
    graphics.func("scale", Graphics::jsScale);
    
    // 预定义颜色常量
    graphics.value("WHITE", std::vector<double>{1.0, 1.0, 1.0, 1.0});
    graphics.value("BLACK", std::vector<double>{0.0, 0.0, 0.0, 1.0});
    graphics.value("RED", std::vector<double>{1.0, 0.0, 0.0, 1.0});
    graphics.value("GREEN", std::vector<double>{0.0, 1.0, 0.0, 1.0});
    graphics.value("BLUE", std::vector<double>{0.0, 0.0, 1.0, 1.0});
    graphics.value("YELLOW", std::vector<double>{1.0, 1.0, 0.0, 1.0});
    
    // 添加console.log支持
    engine.global().func("console_log", Graphics::jsConsoleLog);
}