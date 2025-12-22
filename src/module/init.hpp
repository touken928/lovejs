#pragma once
#include "../core/JSEngine.hpp"
#include "graphics/init.hpp"
#include <iostream>

// 通用工具函数
namespace ModuleUtils {
    inline void print(const std::string& msg) {
        std::cout << msg << std::endl;
    }
}

// 模块自动注册函数
inline void initAllModules() {
    // 注册通用工具函数
    auto& engine = JSEngine::instance();
    engine.global().func("print", ModuleUtils::print);
    
    // 注册所有模块
    initGraphicsModule();
    
    // 未来可以在这里添加更多模块
    // initAudioModule();
    // initInputModule();
    // initPhysicsModule();
}