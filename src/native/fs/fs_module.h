#pragma once

#include <js_engine.h>
#include <js_plugin.h>

#include "fs_async.h"

#include <cstdint>
#include <unordered_map>

namespace qianjs::fs_async {

struct PendingPromise {
    qjs::JSEngine::PromiseHandle ph;
    bool isWrite = false;
};

inline std::unordered_map<std::uint64_t, PendingPromise>& pendingPromises() {
    static std::unordered_map<std::uint64_t, PendingPromise> m;
    return m;
}

inline void pumpPromises(qjs::JSEngine& engine) {
    auto results = pumpResults();
    if (results.empty()) return;

    auto& pending = pendingPromises();

    for (auto& cr : results) {
        auto it = pending.find(cr.id);
        if (it == pending.end()) continue;

        const bool isWrite = it->second.isWrite;
        auto ph = it->second.ph;
        pending.erase(it);

        if (isWrite) {
            if (cr.result.has_value()) {
                engine.rejectPromise(ph, cr.result.value());
            } else {
                engine.resolvePromiseVoid(ph);
            }
        } else {
            if (cr.result.has_value()) {
                engine.resolvePromise(ph, cr.result.value());
            } else {
                engine.rejectPromise(ph, "Failed to read file");
            }
        }
        engine.freePromise(ph);
    }
}

} // namespace qianjs::fs_async

class FsPlugin final : public qjs::IEnginePlugin {
public:
    const char* name() const override { return "fs"; }

    void install(qjs::JSEngine& engine, qjs::JSModule& root) override {
        qjs::JSEngine* eng = &engine;
        auto& m = root.module("fs");

        m.func("readFile", [eng](const std::string& path) -> qjs::RawJSValue {
            auto ph = eng->createPromise();
            auto id = qianjs::fs_async::readFileAsync(path);
            qianjs::fs_async::pendingPromises()[id] = {ph, false};
            return eng->promiseValue(ph);
        });

        m.func("writeFile", [eng](const std::string& path, const std::string& data) -> qjs::RawJSValue {
            auto ph = eng->createPromise();
            auto id = qianjs::fs_async::writeFileAsync(path, data);
            qianjs::fs_async::pendingPromises()[id] = {ph, true};
            return eng->promiseValue(ph);
        });
    }
};
