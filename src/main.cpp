#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <sokol_log.h>
#include "core/GameLoop.hpp"

void init_cb() {
    sg_desc desc = {};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);
    
    GameLoop::init();
}

void frame_cb() {
    GameLoop::frame();
}

void cleanup_cb() {
    GameLoop::cleanup();
    sg_shutdown();
}

void event_cb(const sapp_event* event) {
    GameLoop::handleEvent(event);
}

sapp_desc sokol_main(int argc, char* argv[]) {
    const char* jsFile = (argc > 1) ? argv[1] : "main.js";
    
    sapp_desc desc = GameLoop::setup(jsFile);
    desc.init_cb = init_cb;
    desc.frame_cb = frame_cb;
    desc.cleanup_cb = cleanup_cb;
    desc.event_cb = event_cb;
    
    return desc;
}
