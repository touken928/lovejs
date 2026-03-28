#pragma once

#include <js_engine.h>
#include <js_plugin.h>

#include <iostream>
#include <string>

class ConsolePlugin final : public qjs::IEnginePlugin {
public:
    const char* name() const override { return "console"; }

    void install(qjs::JSEngine&, qjs::JSModule& root) override {
        auto& c = root.module("console");
        c.func("log", [](const std::string& msg) { std::cout << msg << std::endl; });
    }
};
