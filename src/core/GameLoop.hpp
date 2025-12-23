#pragma once
#include "JSEngine.hpp"
#include <quickjs.h>
#include <SDL3/SDL.h>
#include <chrono>
#include <iostream>
#include <string>

class GameLoop {
public:
    // 加载JS模块并获取回调函数
    static bool loadMainModule(JSEngine& engine, const std::string& jsFile = "main.js") {
        engine_ = &engine;
        ctx_ = engine.getContext();
        
        // 使用ES6模块方式加载JS文件，并将导出绑定到全局
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
    
    // 运行游戏循环
    static void run() {
        if (!engine_ || !ctx_) {
            std::cerr << "Engine not initialized" << std::endl;
            return;
        }
        
        running_ = true;
        auto lastTime = std::chrono::high_resolution_clock::now();
        
        // 调用load回调
        callCallback("load");
        
        while (running_) {
            // 计算delta time
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto deltaTime = std::chrono::duration<double>(currentTime - lastTime).count();
            lastTime = currentTime;
            
            // 处理SDL事件
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                handleEvent(event);
                
                if (event.type == SDL_EVENT_QUIT) {
                    running_ = false;
                }
            }
            
            // 调用update回调
            callCallbackWithDouble("update", deltaTime);
            
            // 调用draw回调
            callCallback("draw");
            
            // 控制帧率
            SDL_Delay(16); // ~60 FPS
        }
    }
    
    static void stop() {
        running_ = false;
    }

private:
    static JSEngine* engine_;
    static JSContext* ctx_;
    static bool running_;
    
    static void handleEvent(const SDL_Event& event) {
        switch (event.type) {
            case SDL_EVENT_KEY_DOWN:
                if (!event.key.repeat) {
                    callCallbackWithString("keypressed", getKeyName(event.key.key));
                }
                break;
                
            case SDL_EVENT_KEY_UP:
                callCallbackWithString("keyreleased", getKeyName(event.key.key));
                break;
                
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                callCallbackWithMouseButton("mousepressed", 
                    event.button.x, event.button.y, event.button.button);
                break;
                
            case SDL_EVENT_MOUSE_BUTTON_UP:
                callCallbackWithMouseButton("mousereleased",
                    event.button.x, event.button.y, event.button.button);
                break;
                
            case SDL_EVENT_MOUSE_WHEEL:
                callCallbackWithWheel("wheelmoved", event.wheel.x, event.wheel.y);
                break;
        }
    }
    
    static std::string getKeyName(SDL_Keycode keycode) {
        const char* name = SDL_GetKeyName(keycode);
        std::string result = name ? std::string(name) : "unknown";
        // 转换为小写
        for (char& c : result) {
            c = std::tolower(c);
        }
        return result;
    }
    
    // 调用JS全局函数
    static void callJSFunction(const char* name, int argc, JSValue* argv) {
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
    
    // 无参数回调
    static void callCallback(const std::string& name) {
        callJSFunction(name.c_str(), 0, nullptr);
    }
    
    // 带double参数的回调
    static void callCallbackWithDouble(const std::string& name, double value) {
        JSValue argv[1] = { JS_NewFloat64(ctx_, value) };
        callJSFunction(name.c_str(), 1, argv);
        JS_FreeValue(ctx_, argv[0]);
    }
    
    // 带string参数的回调
    static void callCallbackWithString(const std::string& name, const std::string& value) {
        JSValue argv[1] = { JS_NewString(ctx_, value.c_str()) };
        callJSFunction(name.c_str(), 1, argv);
        JS_FreeValue(ctx_, argv[0]);
    }
    
    // 鼠标按钮回调
    static void callCallbackWithMouseButton(const std::string& name, int x, int y, int button) {
        JSValue argv[3] = { 
            JS_NewInt32(ctx_, x), 
            JS_NewInt32(ctx_, y), 
            JS_NewInt32(ctx_, button) 
        };
        callJSFunction(name.c_str(), 3, argv);
        JS_FreeValue(ctx_, argv[0]);
        JS_FreeValue(ctx_, argv[1]);
        JS_FreeValue(ctx_, argv[2]);
    }
    
    // 滚轮回调
    static void callCallbackWithWheel(const std::string& name, int x, int y) {
        JSValue argv[2] = { 
            JS_NewInt32(ctx_, x), 
            JS_NewInt32(ctx_, y) 
        };
        callJSFunction(name.c_str(), 2, argv);
        JS_FreeValue(ctx_, argv[0]);
        JS_FreeValue(ctx_, argv[1]);
    }
};

// 静态成员定义
inline JSEngine* GameLoop::engine_ = nullptr;
inline JSContext* GameLoop::ctx_ = nullptr;
inline bool GameLoop::running_ = false;