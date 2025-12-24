#pragma once
#include "JSEngine.hpp"
#include "../render/SokolRenderer.hpp"
#include "../module/graphics/Graphics.hpp"
#include "../module/init.hpp"
#include <sokol_app.h>
#include <sokol_log.h>
#include <quickjs.h>
#include <chrono>
#include <string>
#include <iostream>
#include <fstream>

/**
 * GameLoop - 游戏主循环
 * 使用 Sokol 作为底层渲染和窗口系统
 * Header-only 实现
 */
class GameLoop {
public:
    // 设置并返回窗口配置
    static sapp_desc setup(const char* jsFile) {
        // 静态渲染器实例
        static render::SokolRenderer renderer;
        renderer_ = &renderer;
        
        // 检查 JS 文件
        std::ifstream file(jsFile);
        if (!file.good()) {
            std::cerr << "Error: file not found: " << jsFile << std::endl;
            exit(-1);
        }
        file.close();
        
        // 创建默认窗口
        renderer_->createWindow("LoveJS", 800, 600);
        
        // 设置渲染器
        Graphics::setRenderer(renderer_);
        
        // 注册所有模块
        initAllModules();
        
        // 加载 JS 模块
        if (!loadMainModule(JSEngine::instance(), jsFile)) {
            std::cerr << "Error: failed to load " << jsFile << std::endl;
            exit(-1);
        }
        
        // 预先调用 load 回调，让JS设置窗口
        callLoadCallback();
        
        // 返回窗口配置
        sapp_desc desc = {};
        desc.init_cb = nullptr;  // 由外部设置
        desc.frame_cb = nullptr;
        desc.cleanup_cb = nullptr;
        desc.event_cb = nullptr;
        desc.width = renderer_->getWidth();
        desc.height = renderer_->getHeight();
        desc.window_title = renderer_->getTitle().c_str();
        desc.logger.func = slog_func;
        desc.fullscreen = false;
        desc.enable_clipboard = true;
        desc.enable_dragndrop = false;
        
        return desc;
    }
    
    // sokol 回调
    static void init() {
        if (!renderer_) {
            std::cerr << "Renderer not set" << std::endl;
            return;
        }
        
        renderer_->setupPipeline();
        lastTime_ = std::chrono::high_resolution_clock::now();
        
        // 只在未调用过load时才调用
        if (!loadCalled_) {
            callCallback("load");
            loadCalled_ = true;
        }
    }
    
    static void frame() {
        if (!renderer_) return;
        
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto deltaTime = std::chrono::duration<double>(currentTime - lastTime_).count();
        lastTime_ = currentTime;
        
        callCallbackWithDouble("update", deltaTime);
        
        renderer_->beginFrame();
        callCallback("draw");
        renderer_->endFrame();
    }
    
    static void cleanup() {
    }
    
    static void handleEvent(const sapp_event* event) {
        if (!ctx_) return;
        
        switch (event->type) {
            case SAPP_EVENTTYPE_KEY_DOWN:
                if (!event->key_repeat) {
                    callCallbackWithString("keypressed", getKeyName(event->key_code).c_str());
                }
                break;
                
            case SAPP_EVENTTYPE_KEY_UP:
                callCallbackWithString("keyreleased", getKeyName(event->key_code).c_str());
                break;
                
            case SAPP_EVENTTYPE_MOUSE_DOWN:
                callCallbackWithMouseButton("mousepressed", 
                    (int)event->mouse_x, (int)event->mouse_y, (int)event->mouse_button + 1);
                break;
                
            case SAPP_EVENTTYPE_MOUSE_UP:
                callCallbackWithMouseButton("mousereleased",
                    (int)event->mouse_x, (int)event->mouse_y, (int)event->mouse_button + 1);
                break;
                
            case SAPP_EVENTTYPE_MOUSE_SCROLL:
                callCallbackWithWheel("wheelmoved", event->scroll_x, event->scroll_y);
                break;
                
            default:
                break;
        }
    }

private:
    inline static JSEngine* engine_ = nullptr;
    inline static JSContext* ctx_ = nullptr;
    inline static render::SokolRenderer* renderer_ = nullptr;
    inline static std::chrono::high_resolution_clock::time_point lastTime_;
    inline static bool loadCalled_ = false;
    
    static bool loadMainModule(JSEngine& engine, const std::string& jsFile) {
        engine_ = &engine;
        ctx_ = engine.getContext();
        
        std::string code = R"(
            import * as _main from ')" + jsFile + R"(';
            globalThis.load = _main.load;
            globalThis.update = _main.update;
            globalThis.draw = _main.draw;
            globalThis.keypressed = _main.keypressed;
            globalThis.keyreleased = _main.keyreleased;
            globalThis.mousepressed = _main.mousepressed;
            globalThis.mousereleased = _main.mousereleased;
            globalThis.wheelmoved = _main.wheelmoved;
        )";
        
        if (!engine.runFile("_loader.js", code)) {
            std::cerr << "Failed to load module: " << jsFile << std::endl;
            return false;
        }
        
        return true;
    }
    
    static void callLoadCallback() {
        if (!loadCalled_) {
            callCallback("load");
            loadCalled_ = true;
        }
    }
    
    static std::string getKeyName(sapp_keycode keycode) {
        switch (keycode) {
            case SAPP_KEYCODE_SPACE: return "space";
            case SAPP_KEYCODE_APOSTROPHE: return "'";
            case SAPP_KEYCODE_COMMA: return ",";
            case SAPP_KEYCODE_MINUS: return "-";
            case SAPP_KEYCODE_PERIOD: return ".";
            case SAPP_KEYCODE_SLASH: return "/";
            case SAPP_KEYCODE_0: return "0";
            case SAPP_KEYCODE_1: return "1";
            case SAPP_KEYCODE_2: return "2";
            case SAPP_KEYCODE_3: return "3";
            case SAPP_KEYCODE_4: return "4";
            case SAPP_KEYCODE_5: return "5";
            case SAPP_KEYCODE_6: return "6";
            case SAPP_KEYCODE_7: return "7";
            case SAPP_KEYCODE_8: return "8";
            case SAPP_KEYCODE_9: return "9";
            case SAPP_KEYCODE_SEMICOLON: return ";";
            case SAPP_KEYCODE_EQUAL: return "=";
            case SAPP_KEYCODE_A: return "a";
            case SAPP_KEYCODE_B: return "b";
            case SAPP_KEYCODE_C: return "c";
            case SAPP_KEYCODE_D: return "d";
            case SAPP_KEYCODE_E: return "e";
            case SAPP_KEYCODE_F: return "f";
            case SAPP_KEYCODE_G: return "g";
            case SAPP_KEYCODE_H: return "h";
            case SAPP_KEYCODE_I: return "i";
            case SAPP_KEYCODE_J: return "j";
            case SAPP_KEYCODE_K: return "k";
            case SAPP_KEYCODE_L: return "l";
            case SAPP_KEYCODE_M: return "m";
            case SAPP_KEYCODE_N: return "n";
            case SAPP_KEYCODE_O: return "o";
            case SAPP_KEYCODE_P: return "p";
            case SAPP_KEYCODE_Q: return "q";
            case SAPP_KEYCODE_R: return "r";
            case SAPP_KEYCODE_S: return "s";
            case SAPP_KEYCODE_T: return "t";
            case SAPP_KEYCODE_U: return "u";
            case SAPP_KEYCODE_V: return "v";
            case SAPP_KEYCODE_W: return "w";
            case SAPP_KEYCODE_X: return "x";
            case SAPP_KEYCODE_Y: return "y";
            case SAPP_KEYCODE_Z: return "z";
            case SAPP_KEYCODE_LEFT_BRACKET: return "[";
            case SAPP_KEYCODE_BACKSLASH: return "\\";
            case SAPP_KEYCODE_RIGHT_BRACKET: return "]";
            case SAPP_KEYCODE_ESCAPE: return "escape";
            case SAPP_KEYCODE_ENTER: return "return";
            case SAPP_KEYCODE_TAB: return "tab";
            case SAPP_KEYCODE_BACKSPACE: return "backspace";
            case SAPP_KEYCODE_INSERT: return "insert";
            case SAPP_KEYCODE_DELETE: return "delete";
            case SAPP_KEYCODE_RIGHT: return "arrowright";
            case SAPP_KEYCODE_LEFT: return "arrowleft";
            case SAPP_KEYCODE_DOWN: return "arrowdown";
            case SAPP_KEYCODE_UP: return "arrowup";
            case SAPP_KEYCODE_PAGE_UP: return "pageup";
            case SAPP_KEYCODE_PAGE_DOWN: return "pagedown";
            case SAPP_KEYCODE_HOME: return "home";
            case SAPP_KEYCODE_END: return "end";
            case SAPP_KEYCODE_F1: return "f1";
            case SAPP_KEYCODE_F2: return "f2";
            case SAPP_KEYCODE_F3: return "f3";
            case SAPP_KEYCODE_F4: return "f4";
            case SAPP_KEYCODE_F5: return "f5";
            case SAPP_KEYCODE_F6: return "f6";
            case SAPP_KEYCODE_F7: return "f7";
            case SAPP_KEYCODE_F8: return "f8";
            case SAPP_KEYCODE_F9: return "f9";
            case SAPP_KEYCODE_F10: return "f10";
            case SAPP_KEYCODE_F11: return "f11";
            case SAPP_KEYCODE_F12: return "f12";
            default: return "unknown";
        }
    }
    
    static void callJSFunction(const char* name, int argc, JSValue* argv) {
        if (!ctx_) return;
        
        JSValue global = JS_GetGlobalObject(ctx_);
        JSValue func = JS_GetPropertyStr(ctx_, global, name);
        
        if (JS_IsFunction(ctx_, func)) {
            JSValue result = JS_Call(ctx_, func, global, argc, argv);
            if (JS_IsException(result)) {
                JSValue ex = JS_GetException(ctx_);
                const char* str = JS_ToCString(ctx_, ex);
                if (str) {
                    std::cerr << "Callback error [" << name << "]: " << str << std::endl;
                    JS_FreeCString(ctx_, str);
                }
                JS_FreeValue(ctx_, ex);
            }
            JS_FreeValue(ctx_, result);
        }
        
        JS_FreeValue(ctx_, func);
        JS_FreeValue(ctx_, global);
    }
    
    static void callCallback(const char* name) {
        callJSFunction(name, 0, nullptr);
    }
    
    static void callCallbackWithDouble(const char* name, double value) {
        JSValue argv[1] = { JS_NewFloat64(ctx_, value) };
        callJSFunction(name, 1, argv);
        JS_FreeValue(ctx_, argv[0]);
    }
    
    static void callCallbackWithString(const char* name, const char* value) {
        JSValue argv[1] = { JS_NewString(ctx_, value) };
        callJSFunction(name, 1, argv);
        JS_FreeValue(ctx_, argv[0]);
    }
    
    static void callCallbackWithMouseButton(const char* name, int x, int y, int button) {
        JSValue argv[3] = { 
            JS_NewInt32(ctx_, x), 
            JS_NewInt32(ctx_, y), 
            JS_NewInt32(ctx_, button) 
        };
        callJSFunction(name, 3, argv);
        JS_FreeValue(ctx_, argv[0]);
        JS_FreeValue(ctx_, argv[1]);
        JS_FreeValue(ctx_, argv[2]);
    }
    
    static void callCallbackWithWheel(const char* name, float x, float y) {
        JSValue argv[2] = { 
            JS_NewFloat64(ctx_, x), 
            JS_NewFloat64(ctx_, y) 
        };
        callJSFunction(name, 2, argv);
        JS_FreeValue(ctx_, argv[0]);
        JS_FreeValue(ctx_, argv[1]);
    }
};
