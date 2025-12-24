#pragma once
#include "JSEngine.hpp"
#include "../render/SokolRenderer.hpp"
#include "../module/graphics/Graphics.hpp"
#include "../module/init.hpp"
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <sokol_log.h>
#include <chrono>
#include <string>
#include <iostream>
#include <fstream>

/**
 * GameLoop - 游戏主循环管理器
 * 
 * 职责：
 * - 管理 Sokol 应用生命周期
 * - 协调 JS 引擎和渲染器
 * - 处理游戏循环和事件分发
 * 
 * 使用方式：
 *   sapp_desc desc = GameLoop::setup("main.js");
 *   sapp_run(&desc);
 */
class GameLoop {
public:
    /**
     * 设置游戏循环并返回 Sokol 应用配置
     * @param jsFile JS 入口文件路径
     * @return Sokol 应用描述符
     */
    static sapp_desc setup(const char* jsFile) {
        initializeRenderer();
        initializeModules();
        loadJSFile(jsFile);
        
        return createAppDesc();
    }

private:
    //=========================================================================
    // 初始化阶段
    //=========================================================================
    
    /**
     * 初始化渲染器
     */
    static void initializeRenderer() {
        static render::SokolRenderer renderer;
        renderer_ = &renderer;
        
        // 创建默认窗口
        renderer_->createWindow("LoveJS", 800, 600);
        Graphics::setRenderer(renderer_);
    }
    
    /**
     * 初始化所有 JS 模块
     */
    static void initializeModules() {
        JSEngine::initialize();
        initAllModules();
    }
    
    /**
     * 加载 JS 入口文件
     */
    static void loadJSFile(const char* jsFile) {
        // 检查文件是否存在
        std::ifstream file(jsFile);
        if (!file.good()) {
            std::cerr << "Error: file not found: " << jsFile << std::endl;
            exit(-1);
        }
        file.close();
        
        // 加载 JS 模块
        if (!loadMainModule(jsFile)) {
            std::cerr << "Error: failed to load " << jsFile << std::endl;
            exit(-1);
        }
        
        // 预先调用 load 回调，让 JS 设置窗口参数
        callLoadCallback();
    }
    
    /**
     * 创建 Sokol 应用描述符
     */
    static sapp_desc createAppDesc() {
        // 保存窗口标题到静态变量，避免指针失效
        static std::string windowTitle = renderer_->getTitle();
        
        sapp_desc desc = {};
        desc.init_cb = init_cb;
        desc.frame_cb = frame_cb;
        desc.cleanup_cb = cleanup_cb;
        desc.event_cb = event_cb;
        desc.width = renderer_->getWidth();
        desc.height = renderer_->getHeight();
        desc.window_title = windowTitle.c_str();
        desc.logger.func = slog_func;
        desc.fullscreen = false;
        desc.enable_clipboard = true;
        desc.enable_dragndrop = false;
        
        return desc;
    }
    
    //=========================================================================
    // Sokol 回调函数
    //=========================================================================
    
    /**
     * 初始化回调 - 设置图形管线
     */
    static void init_cb() {
        // 初始化 Sokol 图形
        sg_desc desc = {};
        desc.environment = sglue_environment();
        desc.logger.func = slog_func;
        sg_setup(&desc);
        
        // 初始化渲染器管线
        if (!renderer_) {
            std::cerr << "Error: Renderer not initialized" << std::endl;
            return;
        }
        renderer_->setupPipeline();
        
        // 初始化时间
        lastTime_ = std::chrono::high_resolution_clock::now();
        
        // 调用 JS load 回调（如果之前未调用）
        if (!loadCalled_) {
            callCallback("load");
            loadCalled_ = true;
        }
    }
    
    /**
     * 帧回调 - 更新和渲染
     */
    static void frame_cb() {
        if (!renderer_) return;
        
        // 计算帧时间
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto deltaTime = std::chrono::duration<double>(currentTime - lastTime_).count();
        lastTime_ = currentTime;
        
        // 更新逻辑
        callCallback("update", deltaTime);
        
        // 渲染
        renderer_->beginFrame();
        callCallback("draw");
        renderer_->endFrame();
    }
    
    /**
     * 清理回调 - 释放资源
     */
    static void cleanup_cb() {
        // 先清理图形资源（Sokol）
        sg_shutdown();
        
        // 再显式清理 JS 引擎
        JSEngine::cleanup();
        
        // 清空静态指针
        renderer_ = nullptr;
        loadCalled_ = false;
    }
    
    /**
     * 事件回调 - 处理输入事件
     */
    static void event_cb(const sapp_event* event) {
        switch (event->type) {
            case SAPP_EVENTTYPE_KEY_DOWN:
                if (!event->key_repeat) {
                    callCallback("keypressed", getKeyName(event->key_code));
                }
                break;
                
            case SAPP_EVENTTYPE_KEY_UP:
                callCallback("keyreleased", getKeyName(event->key_code));
                break;
                
            case SAPP_EVENTTYPE_MOUSE_DOWN:
                callCallback("mousepressed", 
                    (int)event->mouse_x, (int)event->mouse_y, (int)event->mouse_button + 1);
                break;
                
            case SAPP_EVENTTYPE_MOUSE_UP:
                callCallback("mousereleased",
                    (int)event->mouse_x, (int)event->mouse_y, (int)event->mouse_button + 1);
                break;
                
            case SAPP_EVENTTYPE_MOUSE_SCROLL:
                callCallback("wheelmoved", event->scroll_x, event->scroll_y);
                break;
                
            default:
                break;
        }
    }
    
    //=========================================================================
    // JS 模块加载
    //=========================================================================
    
    /**
     * 加载主 JS 模块并注册全局回调
     */
    static bool loadMainModule(const std::string& jsFile) {
        // 生成模块加载代码
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
        
        if (!JSEngine::runFile("_loader.js", code)) {
            std::cerr << "Failed to load module: " << jsFile << std::endl;
            return false;
        }
        
        return true;
    }
    
    /**
     * 预先调用 load 回调
     */
    static void callLoadCallback() {
        if (!loadCalled_) {
            callCallback("load");
            loadCalled_ = true;
        }
    }
    
    //=========================================================================
    // JS 回调调用
    //=========================================================================
    
    /**
     * 调用 JS 回调（统一接口，支持任意参数）
     */
    template<typename... Args>
    static void callCallback(const char* name, Args... args) {
        JSEngine::callGlobal(name, args...);
    }
    
    //=========================================================================
    // 键盘映射
    //=========================================================================
    
    /**
     * 将 Sokol 键码转换为字符串名称
     */
    static std::string getKeyName(sapp_keycode keycode) {
        switch (keycode) {
            // 特殊键
            case SAPP_KEYCODE_SPACE: return "space";
            case SAPP_KEYCODE_ESCAPE: return "escape";
            case SAPP_KEYCODE_ENTER: return "return";
            case SAPP_KEYCODE_TAB: return "tab";
            case SAPP_KEYCODE_BACKSPACE: return "backspace";
            case SAPP_KEYCODE_INSERT: return "insert";
            case SAPP_KEYCODE_DELETE: return "delete";
            
            // 方向键
            case SAPP_KEYCODE_RIGHT: return "right";
            case SAPP_KEYCODE_LEFT: return "left";
            case SAPP_KEYCODE_DOWN: return "down";
            case SAPP_KEYCODE_UP: return "up";
            
            // 翻页键
            case SAPP_KEYCODE_PAGE_UP: return "pageup";
            case SAPP_KEYCODE_PAGE_DOWN: return "pagedown";
            case SAPP_KEYCODE_HOME: return "home";
            case SAPP_KEYCODE_END: return "end";
            
            // 功能键
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
            
            // 数字键
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
            
            // 字母键
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
            
            // 符号键
            case SAPP_KEYCODE_APOSTROPHE: return "'";
            case SAPP_KEYCODE_COMMA: return ",";
            case SAPP_KEYCODE_MINUS: return "-";
            case SAPP_KEYCODE_PERIOD: return ".";
            case SAPP_KEYCODE_SLASH: return "/";
            case SAPP_KEYCODE_SEMICOLON: return ";";
            case SAPP_KEYCODE_EQUAL: return "=";
            case SAPP_KEYCODE_LEFT_BRACKET: return "[";
            case SAPP_KEYCODE_BACKSLASH: return "\\";
            case SAPP_KEYCODE_RIGHT_BRACKET: return "]";
            
            default: return "unknown";
        }
    }
    
    //=========================================================================
    // 静态成员变量
    //=========================================================================
    
    inline static render::SokolRenderer* renderer_ = nullptr;
    inline static std::chrono::high_resolution_clock::time_point lastTime_;
    inline static bool loadCalled_ = false;
};
