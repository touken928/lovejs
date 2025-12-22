#include <iostream>
#include <fstream>
#include <string>
#include "core/SDLManager.hpp"
#include "core/JSEngine.hpp"
#include "core/GameLoop.hpp"
#include "module/init.hpp"

int main(int argc, char* argv[]) {
    // 获取要运行的JS文件，默认为main.js
    std::string jsFile = (argc > 1) ? argv[1] : "main.js";
    
    // 初始化SDL
    if (!SDLManager::instance().initialize()) {
        return -1;
    }

    // 初始化所有模块
    initAllModules();
    
    // 检查JS文件是否存在
    std::ifstream file(jsFile);
    if (!file.good()) {
        std::cerr << "错误: 找不到文件 " << jsFile << std::endl;
        return -1;
    }
    file.close();
    
    // 加载JS模块
    if (!GameLoop::loadMainModule(JSEngine::instance(), jsFile)) {
        std::cerr << "无法加载 " << jsFile << std::endl;
        return -1;
    }
    
    // 启动游戏循环
    GameLoop::run();
    
    return 0;
}