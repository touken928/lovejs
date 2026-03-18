#pragma once
#include "graphics/init.hpp"

inline slowjs::PluginRegistry defaultPlugins() {
    slowjs::PluginRegistry r;
    r.emplace<GraphicsPlugin>();
    return r;
}
