#pragma once
#include "Types.hpp"
#include <string>
#include <memory>

namespace render {

/**
 * IRenderer - 渲染器抽象接口
 * 定义与具体图形 API 无关的渲染操作
 */
class IRenderer {
public:
    virtual ~IRenderer() = default;
    
    // 窗口管理
    virtual bool createWindow(const std::string& title, int width, int height) = 0;
    virtual void destroyWindow() = 0;
    virtual bool isWindowCreated() const = 0;
    virtual Size getWindowSize() const = 0;
    
    // 渲染控制
    virtual void clear(const Color& color) = 0;
    virtual void present() = 0;
    virtual void setColor(const Color& color) = 0;
    
    // 基本图形
    virtual void drawPoint(float x, float y) = 0;
    virtual void drawLine(float x1, float y1, float x2, float y2) = 0;
    virtual void drawRect(const Rect& rect, bool filled) = 0;
    virtual void drawCircle(float x, float y, float radius, bool filled) = 0;
    
    // 纹理
    virtual TextureHandle loadTexture(const std::string& path) = 0;
    virtual void unloadTexture(TextureHandle handle) = 0;
    virtual Size getTextureSize(TextureHandle handle) const = 0;
    virtual void drawTexture(TextureHandle handle, float x, float y, 
                            float rotation = 0, float scaleX = 1, float scaleY = 1) = 0;
    
    // 变换
    virtual void pushMatrix() = 0;
    virtual void popMatrix() = 0;
    virtual void translate(float x, float y) = 0;
    virtual void rotate(float angle) = 0;
    virtual void scale(float x, float y) = 0;
};

} // namespace render
