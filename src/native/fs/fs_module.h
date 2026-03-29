#pragma once

#include <js_engine.h>
#include <js_plugin.h>

#include "fs_uv.h"

class FsPlugin final : public qjs::IEnginePlugin {
public:
    const char* name() const override { return "fs"; }

    void install(qjs::JSEngine& engine, qjs::JSModule& root) override {
        qjs::JSEngine* eng = &engine;
        auto& m = root.module("fs");

        m.func("readFile", [eng](const std::string& path) -> qjs::RawJSValue {
            return fsReadFileAsync(*eng, path);
        });

        m.func("writeFile", [eng](const std::string& path, const std::string& data) -> qjs::RawJSValue {
            return fsWriteFileAsync(*eng, path, data);
        });
    }
};
