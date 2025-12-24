#include <sokol_app.h>
#include "core/GameLoop.hpp"

int main(int argc, char* argv[]) {
    const char* jsFile = (argc > 1) ? argv[1] : "main.js";
    
    sapp_desc desc = GameLoop::setup(jsFile);
    sapp_run(&desc);
    
    return 0;
}
