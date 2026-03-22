#pragma once
#include <slowjs/Plugin.hpp>
#include "FsAsync.hpp"

#include <cstdint>
#include <unordered_map>

namespace lovejs::fs_async {

struct PendingPromise {
    slowjs::JSEngine::PromiseHandle ph;
    bool isWrite = false;
};

// async op id -> promise + read/write 区分（主线程；与 FsAsync::pumpResults 对应）
inline std::unordered_map<std::uint64_t, PendingPromise>& pendingPromises() {
    static std::unordered_map<std::uint64_t, PendingPromise> m;
    return m;
}

inline void pumpPromises(slowjs::JSEngine& engine) {
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

} // namespace lovejs::fs_async

class FsPlugin final : public slowjs::IEnginePlugin {
public:
    const char* name() const override { return "fs"; }

    void install(slowjs::JSEngine& engine, slowjs::JSModule& root) override {
        slowjs::JSEngine* eng = &engine;
        auto& m = root.module("fs");

        m.func("readFile", [eng](const std::string& path) -> slowjs::RawJSValue {
            auto ph = eng->createPromise();
            auto id = lovejs::fs_async::readFileAsync(path);
            lovejs::fs_async::pendingPromises()[id] = {ph, false};
            return eng->promiseValue(ph);
        });

        m.func("writeFile", [eng](const std::string& path, const std::string& data) -> slowjs::RawJSValue {
            auto ph = eng->createPromise();
            auto id = lovejs::fs_async::writeFileAsync(path, data);
            lovejs::fs_async::pendingPromises()[id] = {ph, true};
            return eng->promiseValue(ph);
        });
    }
};
