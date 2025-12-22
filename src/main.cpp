#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <fstream>
#include <string>
#include "core/JSEngine.hpp"
#include "core/GameLoop.hpp"
#include "module/graphics/init.hpp"

int main(int argc, char* argv[]) {
    // 获取要运行的JS文件，默认为main.js
    std::string jsFile = (argc > 1) ? argv[1] : "main.js";
    
    // 初始化SDL2
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL初始化失败: " << SDL_GetError() << std::endl;
        return -1;
    }

    // 初始化SDL2_image
    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "SDL_image初始化失败: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    // 初始化模块
    initGraphicsModule();
    
    // 检查JS文件是否存在
    std::ifstream file(jsFile);
    if (!file.good()) {
        std::cerr << "错误: 找不到文件 " << jsFile << std::endl;
        IMG_Quit();
        SDL_Quit();
        return -1;
    }
    file.close();
    
    // 加载JS模块
    if (!GameLoop::loadMainModule(JSEngine::instance(), jsFile)) {
        std::cerr << "无法加载 " << jsFile << std::endl;
        IMG_Quit();
        SDL_Quit();
        return -1;
    }
    
    // 启动游戏循环
    GameLoop::run();
    
    IMG_Quit();
    SDL_Quit();
    return 0;
}