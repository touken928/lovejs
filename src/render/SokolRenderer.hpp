#pragma once
#include "IRenderer.hpp"
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <vector>
#include <string>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace render {

/**
 * SokolRenderer - Sokol 渲染器实现
 * 使用 sokol_gfx 进行跨平台渲染
 * Header-only 实现
 */
class SokolRenderer : public IRenderer {
public:
    struct Vertex {
        float x, y;
        float r, g, b, a;
    };
    
    SokolRenderer() {
        clearColor_ = Color::BLACK;
        currentColor_ = Color::WHITE;
        primitiveType_ = SG_PRIMITIVETYPE_TRIANGLES;
    }
    
    ~SokolRenderer() override {
        destroyWindow();
    }
    
    // 窗口管理
    bool createWindow(const std::string& title, int width, int height) override {
        title_ = title;
        width_ = width;
        height_ = height;
        return true;
    }
    
    void destroyWindow() override {
        if (shd_.id != SG_INVALID_ID) {
            sg_destroy_shader(shd_);
            shd_.id = SG_INVALID_ID;
        }
    }
    
    bool isWindowCreated() const override {
        return true;
    }
    
    Size getWindowSize() const override {
        return {width_, height_};
    }
    
    // 渲染控制
    void clear(const Color& color) override {
        clearColor_ = color;
    }
    
    void present() override {
        flushVertices();
    }
    
    void setColor(const Color& color) override {
        currentColor_ = color;
    }
    
    // 基本图形
    void drawPoint(float x, float y) override {
        drawRect({x, y, 2, 2}, true);
    }
    
    void drawLine(float x1, float y1, float x2, float y2) override {
        if (primitiveType_ != SG_PRIMITIVETYPE_LINES && !vertices_.empty()) {
            flushVertices();
        }
        
        vertices_.push_back({x1, y1, currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
        vertices_.push_back({x2, y2, currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
        primitiveType_ = SG_PRIMITIVETYPE_LINES;
    }
    
    void drawRect(const Rect& rect, bool filled) override {
        float x = rect.x, y = rect.y, w = rect.width, h = rect.height;
        
        if (filled) {
            if (primitiveType_ != SG_PRIMITIVETYPE_TRIANGLES && !vertices_.empty()) {
                flushVertices();
            }
            
            vertices_.push_back({x, y, currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
            vertices_.push_back({x + w, y, currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
            vertices_.push_back({x + w, y + h, currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
            
            vertices_.push_back({x, y, currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
            vertices_.push_back({x + w, y + h, currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
            vertices_.push_back({x, y + h, currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
            primitiveType_ = SG_PRIMITIVETYPE_TRIANGLES;
        } else {
            if (primitiveType_ != SG_PRIMITIVETYPE_LINES && !vertices_.empty()) {
                flushVertices();
            }
            
            vertices_.push_back({x, y, currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
            vertices_.push_back({x + w, y, currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
            
            vertices_.push_back({x + w, y, currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
            vertices_.push_back({x + w, y + h, currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
            
            vertices_.push_back({x + w, y + h, currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
            vertices_.push_back({x, y + h, currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
            
            vertices_.push_back({x, y + h, currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
            vertices_.push_back({x, y, currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
            
            primitiveType_ = SG_PRIMITIVETYPE_LINES;
        }
    }
    
    void drawCircle(float cx, float cy, float radius, bool filled) override {
        const int segments = 32;
        const float angleStep = 2.0f * M_PI / segments;
        
        if (filled) {
            if (primitiveType_ != SG_PRIMITIVETYPE_TRIANGLES && !vertices_.empty()) {
                flushVertices();
            }
            
            for (int i = 0; i < segments; i++) {
                float a1 = i * angleStep;
                float a2 = (i + 1) * angleStep;
                
                vertices_.push_back({cx, cy, currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
                vertices_.push_back({cx + radius * cosf(a1), cy + radius * sinf(a1), 
                                   currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
                vertices_.push_back({cx + radius * cosf(a2), cy + radius * sinf(a2),
                                   currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
            }
            primitiveType_ = SG_PRIMITIVETYPE_TRIANGLES;
        } else {
            if (primitiveType_ != SG_PRIMITIVETYPE_LINES && !vertices_.empty()) {
                flushVertices();
            }
            
            for (int i = 0; i < segments; i++) {
                float a1 = i * angleStep;
                float a2 = (i + 1) * angleStep;
                
                vertices_.push_back({cx + radius * cosf(a1), cy + radius * sinf(a1),
                                   currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
                vertices_.push_back({cx + radius * cosf(a2), cy + radius * sinf(a2),
                                   currentColor_.r, currentColor_.g, currentColor_.b, currentColor_.a});
            }
            primitiveType_ = SG_PRIMITIVETYPE_LINES;
        }
    }
    
    // 纹理（未实现）
    TextureHandle loadTexture(const std::string& path) override {
        return nullptr;
    }
    
    void unloadTexture(TextureHandle handle) override {
    }
    
    Size getTextureSize(TextureHandle handle) const override {
        return {};
    }
    
    void drawTexture(TextureHandle handle, float x, float y, 
                    float rotation = 0, float scaleX = 1, float scaleY = 1) override {
    }
    
    // 变换（未实现）
    void pushMatrix() override {
    }
    
    void popMatrix() override {
    }
    
    void translate(float x, float y) override {
    }
    
    void rotate(float angle) override {
    }
    
    void scale(float x, float y) override {
    }
    
    // Sokol 特定方法
    void setupPipeline() {
        sg_shader_desc shd_desc = {};
        
        #if defined(_WIN32)
        // HLSL shader for D3D11
        shd_desc.vertex_func.source = 
            "cbuffer uniforms : register(b0) {\n"
            "    float2 resolution;\n"
            "};\n"
            "struct vs_in {\n"
            "    float2 position: POSITION;\n"
            "    float4 color: COLOR0;\n"
            "};\n"
            "struct vs_out {\n"
            "    float4 color: COLOR0;\n"
            "    float4 position: SV_Position;\n"
            "};\n"
            "vs_out main(vs_in inp) {\n"
            "    vs_out outp;\n"
            "    float2 pos = inp.position / resolution * 2.0 - 1.0;\n"
            "    pos.y = -pos.y;\n"
            "    outp.position = float4(pos, 0.0, 1.0);\n"
            "    outp.color = inp.color;\n"
            "    return outp;\n"
            "}\n";
        shd_desc.vertex_func.entry = "main";
        
        shd_desc.fragment_func.source = 
            "float4 main(float4 color: COLOR0): SV_Target0 {\n"
            "    return color;\n"
            "}\n";
        shd_desc.fragment_func.entry = "main";
        
        // D3D11 需要在shader描述中指定语义名称
        shd_desc.attrs[0].hlsl_sem_name = "POSITION";
        shd_desc.attrs[0].hlsl_sem_index = 0;
        shd_desc.attrs[1].hlsl_sem_name = "COLOR";
        shd_desc.attrs[1].hlsl_sem_index = 0;
        
        #elif defined(__APPLE__)
        // Metal shader for macOS/iOS
        shd_desc.vertex_func.source = 
            "#include <metal_stdlib>\n"
            "using namespace metal;\n"
            "struct vs_in {\n"
            "    float2 position [[attribute(0)]];\n"
            "    float4 color [[attribute(1)]];\n"
            "};\n"
            "struct vs_out {\n"
            "    float4 position [[position]];\n"
            "    float4 color;\n"
            "};\n"
            "struct uniforms {\n"
            "    float2 resolution;\n"
            "};\n"
            "vertex vs_out _main(vs_in in [[stage_in]], constant uniforms& u [[buffer(0)]]) {\n"
            "    vs_out out;\n"
            "    float2 pos = in.position / u.resolution * 2.0 - 1.0;\n"
            "    pos.y = -pos.y;\n"
            "    out.position = float4(pos, 0.0, 1.0);\n"
            "    out.color = in.color;\n"
            "    return out;\n"
            "}\n";
        shd_desc.vertex_func.entry = "_main";
            
        shd_desc.fragment_func.source = 
            "#include <metal_stdlib>\n"
            "using namespace metal;\n"
            "struct fs_in {\n"
            "    float4 color;\n"
            "};\n"
            "fragment float4 _main(fs_in in [[stage_in]]) {\n"
            "    return in.color;\n"
            "}\n";
        shd_desc.fragment_func.entry = "_main";
        
        #else
        // GLSL shader for OpenGL
        shd_desc.vertex_func.source = 
            "#version 330\n"
            "uniform vec2 resolution;\n"
            "layout(location=0) in vec2 position;\n"
            "layout(location=1) in vec4 color0;\n"
            "out vec4 color;\n"
            "void main() {\n"
            "    vec2 pos = position / resolution * 2.0 - 1.0;\n"
            "    pos.y = -pos.y;\n"
            "    gl_Position = vec4(pos, 0.0, 1.0);\n"
            "    color = color0;\n"
            "}\n";
        shd_desc.vertex_func.entry = "main";
        
        shd_desc.fragment_func.source = 
            "#version 330\n"
            "in vec4 color;\n"
            "out vec4 frag_color;\n"
            "void main() {\n"
            "    frag_color = color;\n"
            "}\n";
        shd_desc.fragment_func.entry = "main";
        #endif
        
        // 声明uniform block
        shd_desc.uniform_blocks[0].stage = SG_SHADERSTAGE_VERTEX;
        shd_desc.uniform_blocks[0].size = 8;
        shd_desc.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_NATIVE;
        
        shd_ = sg_make_shader(shd_desc);
    }
    
    void beginFrame() {
        // 使用实际framebuffer尺寸来保持正确的长宽比
        width_ = sapp_width();
        height_ = sapp_height();
        
        sg_pass_action pass_action = {};
        pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
        pass_action.colors[0].clear_value = {clearColor_.r, clearColor_.g, clearColor_.b, clearColor_.a};
        sg_begin_pass({.action = pass_action, .swapchain = sglue_swapchain()});
        
        vertices_.clear();
        primitiveType_ = SG_PRIMITIVETYPE_TRIANGLES;
    }
    
    void endFrame() {
        flushVertices();
        sg_end_pass();
        sg_commit();
    }
    
    const std::string& getTitle() const { return title_; }
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

private:
    std::string title_;
    int width_;
    int height_;
    Color clearColor_;
    Color currentColor_;
    
    std::vector<Vertex> vertices_;
    sg_primitive_type primitiveType_;
    sg_shader shd_;
    
    void flushVertices() {
        if (vertices_.empty()) return;
        
        sg_buffer_desc buf_desc = {};
        buf_desc.data.ptr = vertices_.data();
        buf_desc.data.size = vertices_.size() * sizeof(Vertex);
        
        sg_buffer vbuf = sg_make_buffer(buf_desc);
        
        sg_bindings bind = {};
        bind.vertex_buffers[0] = vbuf;
        
        sg_pipeline_desc pip_desc = {};
        pip_desc.shader = shd_;
        pip_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
        pip_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT4;
        pip_desc.primitive_type = primitiveType_;
        pip_desc.colors[0].blend.enabled = true;
        pip_desc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
        pip_desc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        sg_pipeline pip = sg_make_pipeline(pip_desc);
        
        sg_apply_pipeline(pip);
        sg_apply_bindings(&bind);
        
        float resolution[2] = {(float)width_, (float)height_};
        sg_range uniform_data = {resolution, sizeof(resolution)};
        sg_apply_uniforms(0, &uniform_data);
        
        sg_draw(0, vertices_.size(), 1);
        
        sg_destroy_pipeline(pip);
        sg_destroy_buffer(vbuf);
        vertices_.clear();
    }
};

} // namespace render
