#pragma once

#include <js_engine.h>
#include <js_plugin.h>

#include "net_uv.h"

class NetPlugin final : public qjs::IEnginePlugin {
public:
    const char* name() const override { return "net"; }

    void install(qjs::JSEngine& engine, qjs::JSModule& root) override {
        qjs::JSEngine* eng = &engine;
        auto& m = root.module("net");

        // Node-like: TCP client. connect(port, host) — host may be "" for 127.0.0.1.
        // connectLocal(port) — shorthand for localhost only.
        m.func("connect", [eng](int port, const std::string& host) -> qjs::RawJSValue {
            return netConnectAsync(*eng, port, host);
        });
        m.func("connectLocal", [eng](int port) -> qjs::RawJSValue { return netConnectAsync(*eng, port, {}); });

        m.func("write", [eng](int64_t sock, const std::string& data) -> qjs::RawJSValue {
            return netWriteAsync(*eng, sock, data);
        });

        m.func("read", [eng](int64_t sock) -> qjs::RawJSValue { return netReadAsync(*eng, sock); });

        m.func("close", [](int64_t sock) { netClose(sock); });
    }
};
