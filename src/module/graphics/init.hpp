#pragma once
#include "../../core/JSEngine.hpp"
#include "Graphics.hpp"

/**
 * Graphics 模块 JS 接口注册
 * 
 * 窗口管理:
 *   setWindow(title: string, width: number, height: number): void
 *   getWindowSize(): [number, number]
 * 
 * 渲染控制:
 *   clear(r: number, g: number, b: number, a: number): void
 *   present(): void
 *   setColor(r: number, g: number, b: number, a: number): void
 * 
 * 基本图形:
 *   point(x: number, y: number): void
 *   line(x1: number, y1: number, x2: number, y2: number): void
 *   rectangle(x: number, y: number, width: number, height: number, filled: boolean): void
 *   circle(x: number, y: number, radius: number, filled: boolean): void
 * 
 * 纹理:
 *   loadTexture(path: string): string
 *   drawTexture(id: string, x: number, y: number, rotation?: number, scaleX?: number, scaleY?: number): void
 * 
 * 变换:
 *   push(): void
 *   pop(): void
 *   translate(x: number, y: number): void
 *   rotate(angle: number): void
 *   scale(x: number, y: number): void
 * 
 * 文本:
 *   print(text: string, x: number, y: number): void
 * 
 * 颜色常量:
 *   WHITE, BLACK, RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA: [number, number, number, number]
 */
inline void initGraphicsModule() {
    auto& g = JSEngine::global().module("graphics");
    
    // 窗口
    g.func("setWindow", [](const std::string& t, int w, int h) {
        Graphics::instance().renderer().createWindow(t, w, h);
    });
    g.func("getWindowSize", []() {
        auto s = Graphics::instance().renderer().getWindowSize();
        return std::vector<double>{(double)s.width, (double)s.height};
    });
    
    // 渲染
    g.func("clear", [](double r, double g, double b, double a) {
        Graphics::instance().renderer().clear({(float)r, (float)g, (float)b, (float)a});
    });
    g.func("present", []() { Graphics::instance().renderer().present(); });
    g.func("setColor", [](double r, double g, double b, double a) {
        Graphics::instance().renderer().setColor({(float)r, (float)g, (float)b, (float)a});
    });
    
    // 图形
    g.func("point", [](double x, double y) {
        Graphics::instance().renderer().drawPoint(x, y);
    });
    g.func("line", [](double x1, double y1, double x2, double y2) {
        Graphics::instance().renderer().drawLine(x1, y1, x2, y2);
    });
    g.func("rectangle", [](double x, double y, double w, double h, bool filled) {
        Graphics::instance().renderer().drawRect({(float)x, (float)y, (float)w, (float)h}, filled);
    });
    g.func("circle", [](double x, double y, double radius, bool filled) {
        Graphics::instance().renderer().drawCircle(x, y, radius, filled);
    });
    
    // 纹理
    g.func("loadTexture", [](const std::string& path) -> std::string {
        return Graphics::instance().getTexture(path) ? path : "";
    });
    g.func("drawTexture", [](const std::string& id, double x, double y, double rot, double sx, double sy) {
        if (auto h = Graphics::instance().getTexture(id))
            Graphics::instance().renderer().drawTexture(h, x, y, rot, sx, sy);
    });
    
    // 变换
    g.func("push", []() { Graphics::instance().renderer().pushMatrix(); });
    g.func("pop", []() { Graphics::instance().renderer().popMatrix(); });
    g.func("translate", [](double x, double y) { Graphics::instance().renderer().translate(x, y); });
    g.func("rotate", [](double a) { Graphics::instance().renderer().rotate(a); });
    g.func("scale", [](double x, double y) { Graphics::instance().renderer().scale(x, y); });
    
    // 文本
    g.func("print", [](const std::string& text, double x, double y) {
        Graphics::instance().drawText(text, (int)x, (int)y);
    });
    
    // 颜色常量
    g.value("WHITE", std::vector<double>{1, 1, 1, 1});
    g.value("BLACK", std::vector<double>{0, 0, 0, 1});
    g.value("RED", std::vector<double>{1, 0, 0, 1});
    g.value("GREEN", std::vector<double>{0, 1, 0, 1});
    g.value("BLUE", std::vector<double>{0, 0, 1, 1});
    g.value("YELLOW", std::vector<double>{1, 1, 0, 1});
    g.value("CYAN", std::vector<double>{0, 1, 1, 1});
    g.value("MAGENTA", std::vector<double>{1, 0, 1, 1});
}
