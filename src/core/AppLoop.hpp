#pragma once
#include <slowjs/JSEngine.hpp>
#include "../render/SokolRenderer.hpp"
#include "../module/init.hpp"
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <sokol_log.h>
#include <chrono>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

namespace lovejs {
struct GameApp {
    render::SokolRenderer renderer;
    slowjs::JSEngine engine;
    std::vector<uint8_t> bytecodeData;
    bool loadCalled = false;
    bool bytecodeMode = false;
    std::chrono::high_resolution_clock::time_point lastTime;

    GameApp() = default;
};
} // namespace lovejs

/**
 * AppLoop - 游戏主循环管理器
 *
 * 职责：
 * - 管理 Sokol 应用生命周期
 * - 协调 JS 引擎和渲染器
 * - 处理游戏循环和事件分发
 *
 * 使用方式：
 *   sapp_desc desc = AppLoop::setup("main.js");
 *   sapp_run(&desc);
 *
 *   // 或者加载字节码
 *   sapp_desc desc = AppLoop::setupBytecode("main.qbc");
 *   sapp_run(&desc);
 */
class AppLoop {
public:
    static sapp_desc setup(const char* jsFile) {
        auto& a = app();
        a.bytecodeMode = false;
        initializeRenderer(a);
        initializeModules(a);
        loadJSFile(a, jsFile);

        return createAppDesc();
    }

    static sapp_desc setupBytecode(const char* qbcFile) {
        auto& a = app();
        a.bytecodeMode = true;
        initializeRenderer(a);
        initializeModules(a);
        loadBytecodeFile(a, qbcFile);

        return createAppDesc();
    }

    static sapp_desc setupFromMemory(const uint8_t* data, size_t size) {
        auto& a = app();
        a.bytecodeMode = true;
        initializeRenderer(a);
        initializeModules(a);
        loadBytecodeFromMemory(a, data, size);

        return createAppDesc();
    }

private:
    static lovejs::GameApp& app() {
        static lovejs::GameApp a;
        return a;
    }

    static void initializeRenderer(lovejs::GameApp& app) {
        app.renderer.createWindow("LoveJS", 800, 600);
        // Expose renderer pointer to plugins via slowjs typed host storage
        app.engine.setHost<render::IRenderer>(&app.renderer);
    }

    static void initializeModules(lovejs::GameApp& app) {
        app.engine.initialize();
        auto plugins = defaultPlugins();
        plugins.installAll(app.engine, app.engine.root());
    }

    static void loadJSFile(lovejs::GameApp& app, const char* jsFile) {
        std::ifstream file(jsFile);
        if (!file.good()) {
            std::cerr << "Error: file not found: " << jsFile << std::endl;
            exit(-1);
        }
        file.close();

        if (!loadMainModule(app, jsFile)) {
            std::cerr << "Error: failed to load " << jsFile << std::endl;
            exit(-1);
        }

        callLoadCallback(app);
    }

    static void loadBytecodeFile(lovejs::GameApp& app, const char* qbcFile) {
        std::ifstream file(qbcFile, std::ios::binary | std::ios::ate);
        if (!file.good()) {
            std::cerr << "Error: file not found: " << qbcFile << std::endl;
            exit(-1);
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        app.bytecodeData.resize((size_t)size);
        if (!file.read(reinterpret_cast<char*>(app.bytecodeData.data()), size)) {
            std::cerr << "Error: failed to read " << qbcFile << std::endl;
            exit(-1);
        }
        file.close();

        if (!loadBytecodeModule(app)) {
            std::cerr << "Error: failed to load bytecode " << qbcFile << std::endl;
            exit(-1);
        }

        callLoadCallback(app);
    }

    static void loadBytecodeFromMemory(lovejs::GameApp& app, const uint8_t* data, size_t size) {
        app.bytecodeData.assign(data, data + size);
        if (!loadBytecodeModule(app)) {
            std::cerr << "Error: failed to load embedded bytecode" << std::endl;
            exit(-1);
        }
        callLoadCallback(app);
    }

    static sapp_desc createAppDesc() {
        auto& a = app();
        static std::string windowTitle;
        windowTitle = a.renderer.getTitle();

        sapp_desc desc = {};
        desc.init_cb = init_cb;
        desc.frame_cb = frame_cb;
        desc.cleanup_cb = cleanup_cb;
        desc.event_cb = event_cb;
        desc.user_data = &a;
        desc.width = a.renderer.getWidth();
        desc.height = a.renderer.getHeight();
        desc.window_title = windowTitle.c_str();
        desc.logger.func = slog_func;
        desc.fullscreen = false;
        desc.enable_clipboard = true;
        desc.enable_dragndrop = false;

        return desc;
    }

    static void init_cb() {
        auto* app = static_cast<lovejs::GameApp*>(sapp_userdata());
        sg_desc desc = {};
        desc.environment = sglue_environment();
        desc.logger.func = slog_func;
        sg_setup(&desc);

        if (!app) {
            std::cerr << "Error: Renderer not initialized" << std::endl;
            return;
        }
        app->renderer.setupPipeline();

        app->lastTime = std::chrono::high_resolution_clock::now();

        if (!app->loadCalled) {
            callCallback(*app, "load");
            app->loadCalled = true;
        }
    }

    static void frame_cb() {
        auto* app = static_cast<lovejs::GameApp*>(sapp_userdata());
        if (!app) return;

        auto currentTime = std::chrono::high_resolution_clock::now();
        auto deltaTime = std::chrono::duration<double>(currentTime - app->lastTime).count();
        app->lastTime = currentTime;

        callCallback(*app, "update", deltaTime);

        app->renderer.beginFrame();
        callCallback(*app, "draw");
        app->renderer.endFrame();
    }

    static void cleanup_cb() {
        auto* app = static_cast<lovejs::GameApp*>(sapp_userdata());
        Graphics::instance().clearTextureCache();

        if (app) {
            app->renderer.destroyWindow();
        }

        sg_shutdown();

        if (app) {
            app->engine.cleanup();
            app->loadCalled = false;
        }
    }

    static void event_cb(const sapp_event* event) {
        auto* app = static_cast<lovejs::GameApp*>(sapp_userdata());
        if (!app) return;
        switch (event->type) {
            case SAPP_EVENTTYPE_KEY_DOWN:
                if (!event->key_repeat) {
                    callCallback(*app, "keypressed", getKeyName(event->key_code));
                }
                break;

            case SAPP_EVENTTYPE_KEY_UP:
                callCallback(*app, "keyreleased", getKeyName(event->key_code));
                break;

            case SAPP_EVENTTYPE_MOUSE_DOWN:
                callCallback(*app, "mousepressed",
                    (int)event->mouse_x, (int)event->mouse_y, (int)event->mouse_button + 1);
                break;

            case SAPP_EVENTTYPE_MOUSE_UP:
                callCallback(*app, "mousereleased",
                    (int)event->mouse_x, (int)event->mouse_y, (int)event->mouse_button + 1);
                break;

            case SAPP_EVENTTYPE_MOUSE_SCROLL:
                callCallback(*app, "wheelmoved", event->scroll_x, event->scroll_y);
                break;

            default:
                break;
        }
    }

    static bool loadMainModule(lovejs::GameApp& app, const std::string& jsFile) {
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

        if (!app.engine.runModuleCode("_loader.js", code)) {
            std::cerr << "Failed to load module: " << jsFile << std::endl;
            return false;
        }

        return true;
    }

    static bool loadBytecodeModule(lovejs::GameApp& app) {
        if (!app.engine.runBytecode(app.bytecodeData.data(), app.bytecodeData.size())) {
            std::cerr << "Failed to load bytecode module" << std::endl;
            return false;
        }
        return true;
    }

    static void callLoadCallback(lovejs::GameApp& app) {
        if (!app.loadCalled) {
            callCallback(app, "load");
            app.loadCalled = true;
        }
    }

    template<typename... Args>
    static void callCallback(lovejs::GameApp& app, const char* name, Args... args) {
        app.engine.callGlobal(name, args...);
    }

    static std::string getKeyName(sapp_keycode keycode) {
        switch (keycode) {
            case SAPP_KEYCODE_SPACE: return "space";
            case SAPP_KEYCODE_ESCAPE: return "escape";
            case SAPP_KEYCODE_ENTER: return "return";
            case SAPP_KEYCODE_TAB: return "tab";
            case SAPP_KEYCODE_BACKSPACE: return "backspace";
            case SAPP_KEYCODE_INSERT: return "insert";
            case SAPP_KEYCODE_DELETE: return "delete";

            case SAPP_KEYCODE_RIGHT: return "right";
            case SAPP_KEYCODE_LEFT: return "left";
            case SAPP_KEYCODE_DOWN: return "down";
            case SAPP_KEYCODE_UP: return "up";

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
};

