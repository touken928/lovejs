#pragma once
#include "graphics/init.hpp"
#include "fs/init.hpp"

inline slowjs::PluginRegistry defaultPlugins() {
    slowjs::PluginRegistry r;
    r.emplace<GraphicsPlugin>();
    r.emplace<FsPlugin>();
    return r;
}
