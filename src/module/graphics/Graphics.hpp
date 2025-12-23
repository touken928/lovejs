#pragma once
#include "../../render/Render.hpp"
#include "Font.hpp"
#include <memory>
#include <unordered_map>

/**
 * Graphics - 图形模块核心
 */
class Graphics {
public:
    static Graphics& instance() { static Graphics g; return g; }
    
    render::IRenderer& renderer() {
        if (!renderer_) {
            renderer_ = std::make_unique<render::SDLRenderer>();
            if (!renderer_->isInitialized())
                throw std::runtime_error("Renderer init failed");
        }
        return *renderer_;
    }
    
    // 纹理缓存
    render::TextureHandle getTexture(const std::string& path) {
        auto it = textures_.find(path);
        if (it != textures_.end()) return it->second;
        auto h = renderer().loadTexture(path);
        if (h) textures_[path] = h;
        return h;
    }
    
    // 文本绘制
    void drawText(const std::string& text, int x, int y) {
        for (char c : text) {
            if (auto* bmp = Font::getBitmap(c)) {
                for (int row = 0; row < Font::CHAR_HEIGHT; row++) {
                    for (int col = 0; col < Font::CHAR_WIDTH; col++) {
                        if (bmp[row] & (1 << (Font::CHAR_WIDTH - 1 - col)))
                            renderer().drawPoint(x + col, y + row);
                    }
                }
            }
            x += Font::CHAR_WIDTH + Font::CHAR_SPACING;
        }
    }

private:
    std::unique_ptr<render::SDLRenderer> renderer_;
    std::unordered_map<std::string, render::TextureHandle> textures_;
};
