#pragma once

#include <js_plugin.h>

class ConsolePlugin final : public qjs::IEnginePlugin {
public:
    const char* name() const override;
    void install(qjs::JSEngine& engine, qjs::JSModule& root) override;
};
