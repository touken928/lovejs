#pragma once

#include <js_engine.h>

#include "native/default_plugins.h"
#include "runtime/embed.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <vector>

namespace qianjs {

inline void drainAsyncWork(qjs::JSEngine& engine) {
    using clock = std::chrono::steady_clock;
    const auto deadline = clock::now() + std::chrono::seconds(120);
    int idleRounds = 0;

    while (clock::now() < deadline) {
        fs_async::pumpPromises(engine);
        engine.pumpMicrotasks();

        const bool noFsWait = fs_async::pendingPromises().empty();
        const bool noJsJobs = !engine.isJobPending();

        if (noFsWait && noJsJobs) {
            if (++idleRounds >= 3)
                return;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        idleRounds = 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cerr << "Warning: timed out while waiting for async work to finish\n";
}

inline int runHeadlessScript(const std::filesystem::path& inputPath) {
    qjs::JSEngine engine;
    engine.initialize();
    defaultPlugins().installAll(engine, engine.root());

    bool ok = false;
    if (inputPath.extension() == ".qbc") {
        std::vector<uint8_t> bytecode = Embed::readBinaryFile(inputPath);
        if (bytecode.empty()) {
            std::cerr << "Error: Cannot read bytecode: " << inputPath << std::endl;
            return 1;
        }
        ok = engine.runBytecode(bytecode.data(), bytecode.size());
    } else {
        ok = engine.runFile(inputPath.string());
    }

    if (!ok) {
        engine.cleanup();
        return 1;
    }

    drainAsyncWork(engine);
    engine.cleanup();
    return 0;
}

inline int runHeadlessEmbedded() {
    std::vector<uint8_t> embedded = Embed::readEmbeddedBytecode();
    if (embedded.empty())
        return -1;

    qjs::JSEngine engine;
    engine.initialize();
    defaultPlugins().installAll(engine, engine.root());

    if (!engine.runBytecode(embedded.data(), embedded.size())) {
        engine.cleanup();
        return 1;
    }

    drainAsyncWork(engine);
    engine.cleanup();
    return 0;
}

} // namespace qianjs
