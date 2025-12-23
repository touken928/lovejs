#include <iostream>
#include <fstream>
#include <string>
#include "core/JSEngine.hpp"
#include "core/GameLoop.hpp"
#include "module/init.hpp"

int main(int argc, char* argv[]) {
    std::string jsFile = (argc > 1) ? argv[1] : "main.js";
    
    // 注册所有模块到 JS 引擎（SDL 在首次调用 graphics 时懒加载）
    initAllModules();
    
    // 检查 JS 文件
    std::ifstream file(jsFile);
    if (!file.good()) {
        std::cerr << "Error: file not found: " << jsFile << std::endl;
        return -1;
    }
    file.close();
    
    // 加载并运行
    if (!GameLoop::loadMainModule(JSEngine::instance(), jsFile)) {
        std::cerr << "Error: failed to load " << jsFile << std::endl;
        return -1;
    }
    
    GameLoop::run();
    return 0;
}
