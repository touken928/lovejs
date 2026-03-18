#pragma once
#include "../../render/IRenderer.hpp"
#include "Font.hpp"
#include <string>
#include <utility>
#include <unordered_map>
#include <stdexcept>

/**
 * Graphics - 图形模块核心
 * 使用 Sokol 渲染器
 */
class Graphics {
public:
    static Graphics& instance() { static Graphics g; return g; }

    // Renderer injection (only core/app loop should call this)
    static void bindRenderer(render::IRenderer* renderer) {
        instance().renderer_ = renderer;
    }

    bool isBound() const { return renderer_ != nullptr; }

    // Window/query facade
    bool setWindow(const std::string& title, int width, int height) {
        return renderer().createWindow(title, width, height);
    }
    std::pair<int, int> windowSize() const {
        auto s = renderer().getWindowSize();
        return {s.width, s.height};
    }

    // Drawing facade (used by modules/plugins)
    void clear(float r, float g, float b, float a) { renderer().clear({r, g, b, a}); }
    void present() { renderer().present(); }
    void setColor(float r, float g, float b, float a) { renderer().setColor({r, g, b, a}); }
    void point(double x, double y) { renderer().drawPoint((float)x, (float)y); }
    void line(double x1, double y1, double x2, double y2) { renderer().drawLine((float)x1, (float)y1, (float)x2, (float)y2); }
    void rectangle(double x, double y, double w, double h, bool filled) { renderer().drawRect({(float)x, (float)y, (float)w, (float)h}, filled); }
    void circle(double x, double y, double radius, bool filled) { renderer().drawCircle((float)x, (float)y, (float)radius, filled); }
    void push() { renderer().pushMatrix(); }
    void pop() { renderer().popMatrix(); }
    void translate(double x, double y) { renderer().translate((float)x, (float)y); }
    void rotate(double a) { renderer().rotate((float)a); }
    void scale(double x, double y) { renderer().scale((float)x, (float)y); }
    void drawTexture(const std::string& id, double x, double y, double rot, double sx, double sy) {
        auto h = getTexture(id);
        if (h) renderer().drawTexture(h, (float)x, (float)y, (float)rot, (float)sx, (float)sy);
    }
    void print(const std::string& text, double x, double y) { drawText(text, (int)x, (int)y); }
    
    void drawText(const std::string& text, int x, int y) {
        for (char c : text) {
            if (auto* bmp = Font::getBitmap(c)) {
                for (int row = 0; row < Font::CHAR_HEIGHT; row++) {
                    for (int col = 0; col < Font::CHAR_WIDTH; col++) {
                        if (bmp[row] & (1 << col))
                            renderer().drawPoint((float)(x + col), (float)(y + row));
                    }
                }
            }
            x += Font::CHAR_WIDTH + Font::CHAR_SPACING;
        }
    }

    void clearTextureCache() {
        if (!renderer_) { textures_.clear(); return; }
        for (auto& [_, handle] : textures_) {
            if (handle) renderer().unloadTexture(handle);
        }
        textures_.clear();
    }

private:
    render::IRenderer& renderer() const {
        if (!renderer_) {
            throw std::runtime_error("Graphics renderer not bound. Call Graphics::bindRenderer() first.");
        }
        return *renderer_;
    }

    render::TextureHandle getTexture(const std::string& path) {
        auto it = textures_.find(path);
        if (it != textures_.end()) return it->second;
        auto h = renderer().loadTexture(path);
        if (h) textures_[path] = h;
        return h;
    }

    render::IRenderer* renderer_ = nullptr;
    std::unordered_map<std::string, render::TextureHandle> textures_;
};

