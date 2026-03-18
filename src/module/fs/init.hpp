#pragma once
#include <slowjs/Plugin.hpp>
#include "FsAsync.hpp"

#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace lovejs::fs_async {

// Map JobId -> PromiseHandle, accessed only on the main thread during enqueue
// and pump, so no mutex is needed beyond the results mutex already in FsAsync.
inline std::unordered_map<std::uint64_t, slowjs::JSEngine::PromiseHandle>& pendingMap() {
    static std::unordered_map<std::uint64_t, slowjs::JSEngine::PromiseHandle> m;
    return m;
}

// Track which jobs are writes (resolve with void) vs reads (resolve with string).
inline std::unordered_map<std::uint64_t, bool>& writeMap() {
    static std::unordered_map<std::uint64_t, bool> m;
    return m;
}

inline void pumpPromises(slowjs::JSEngine& engine) {
    auto results = pumpResults();
    if (results.empty()) return;

    auto& pm = pendingMap();
    auto& wm = writeMap();

    for (auto& cr : results) {
        auto it = pm.find(cr.id);
        if (it == pm.end()) continue;

        auto ph = it->second;
        bool isWrite = false;
        auto wit = wm.find(cr.id);
        if (wit != wm.end()) {
            isWrite = wit->second;
            wm.erase(wit);
        }
        pm.erase(it);

        if (cr.result.ok) {
            if (isWrite)
                engine.resolvePromiseVoid(ph);
            else
                engine.resolvePromise(ph, cr.result.data);
        } else {
            engine.rejectPromise(ph, cr.result.data);
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
            lovejs::fs_async::pendingMap()[id] = ph;
            lovejs::fs_async::writeMap()[id] = false;
            return eng->promiseValue(ph);
        });

        m.func("writeFile", [eng](const std::string& path, const std::string& data) -> slowjs::RawJSValue {
            auto ph = eng->createPromise();
            auto id = lovejs::fs_async::writeFileAsync(path, data);
            lovejs::fs_async::pendingMap()[id] = ph;
            lovejs::fs_async::writeMap()[id] = true;
            return eng->promiseValue(ph);
        });
    }
};
