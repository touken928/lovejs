#pragma once

#include "console/console_plugin.h"
#include "fs/fs_module.h"

inline qjs::PluginRegistry defaultPlugins() {
    qjs::PluginRegistry r;
    r.emplace<ConsolePlugin>();
    r.emplace<FsPlugin>();
    return r;
}
