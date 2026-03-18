#pragma once
#include <slowjs/Plugin.hpp>
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
class GraphicsPlugin final : public slowjs::IEnginePlugin {
public:
    const char* name() const override { return "graphics"; }
    void install(slowjs::JSEngine& engine, slowjs::JSModule& root) override {
        // Bind renderer from typed host storage (set by AppLoop) into Graphics facade
        if (auto* renderer = engine.host<render::IRenderer>()) {
            Graphics::bindRenderer(renderer);
        }

        auto& g = root.module("graphics");

        // Window
        g.func("setWindow", [](const std::string& t, int w, int h) {
            Graphics::instance().setWindow(t, w, h);
        });
        g.func("getWindowSize", []() {
            auto [w, h] = Graphics::instance().windowSize();
            return std::vector<double>{(double)w, (double)h};
        });

        // Render
        g.func("clear", [](double r, double g, double b, double a) {
            Graphics::instance().clear((float)r, (float)g, (float)b, (float)a);
        });
        g.func("present", []() { Graphics::instance().present(); });
        g.func("setColor", [](double r, double g, double b, double a) {
            Graphics::instance().setColor((float)r, (float)g, (float)b, (float)a);
        });

        // Shapes
        g.func("point", [](double x, double y) { Graphics::instance().point(x, y); });
        g.func("line", [](double x1, double y1, double x2, double y2) {
            Graphics::instance().line(x1, y1, x2, y2);
        });
        g.func("rectangle", [](double x, double y, double w, double h, bool filled) {
            Graphics::instance().rectangle(x, y, w, h, filled);
        });
        g.func("circle", [](double x, double y, double radius, bool filled) {
            Graphics::instance().circle(x, y, radius, filled);
        });

        // Textures
        g.func("loadTexture", [](const std::string& path) -> std::string {
            // Lazy-loaded by drawTexture; return id for compatibility
            return path;
        });
        g.func("drawTexture", [](const std::string& id, double x, double y, double rot, double sx, double sy) {
            Graphics::instance().drawTexture(id, x, y, rot, sx, sy);
        });

        // Transforms
        g.func("push", []() { Graphics::instance().push(); });
        g.func("pop", []() { Graphics::instance().pop(); });
        g.func("translate", [](double x, double y) { Graphics::instance().translate(x, y); });
        g.func("rotate", [](double a) { Graphics::instance().rotate(a); });
        g.func("scale", [](double x, double y) { Graphics::instance().scale(x, y); });

        // Text
        g.func("print", [](const std::string& text, double x, double y) {
            Graphics::instance().print(text, x, y);
        });

        // Constants
        g.value("WHITE", std::vector<double>{1, 1, 1, 1});
        g.value("BLACK", std::vector<double>{0, 0, 0, 1});
        g.value("RED", std::vector<double>{1, 0, 0, 1});
        g.value("GREEN", std::vector<double>{0, 1, 0, 1});
        g.value("BLUE", std::vector<double>{0, 0, 1, 1});
        g.value("YELLOW", std::vector<double>{1, 1, 0, 1});
        g.value("CYAN", std::vector<double>{0, 1, 1, 1});
        g.value("MAGENTA", std::vector<double>{1, 0, 1, 1});
    }
};
