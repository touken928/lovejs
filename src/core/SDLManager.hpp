#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <string>

class SDLManager {
public:
    static SDLManager& instance() {
        static SDLManager inst;
        return inst;
    }
    
    bool initialize() {
        if (initialized_) return true;
        
        // 初始化SDL2
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL初始化失败: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // 初始化SDL2_image
        int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
        if (!(IMG_Init(imgFlags) & imgFlags)) {
            std::cerr << "SDL_image初始化失败: " << IMG_GetError() << std::endl;
            SDL_Quit();
            return false;
        }
        
        initialized_ = true;
        return true;
    }
    
    void shutdown() {
        if (!initialized_) return;
        
        IMG_Quit();
        SDL_Quit();
        initialized_ = false;
    }
    
    bool isInitialized() const {
        return initialized_;
    }
    
    ~SDLManager() {
        shutdown();
    }

private:
    SDLManager() = default;
    bool initialized_ = false;
    
    // 禁止拷贝
    SDLManager(const SDLManager&) = delete;
    SDLManager& operator=(const SDLManager&) = delete;
};