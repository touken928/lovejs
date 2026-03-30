#pragma once

#include <js_engine.h>

#include "native/default_plugins.h"
#include "runtime/event_loop/event_loop.h"
#include "runtime/embed.h"
#include "runtime/runtime_context.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <utility>
#include <vector>

namespace qianjs {

/** Run the event loop and microtasks until native I/O and JS jobs are idle (or timeout). */
inline void drainAsyncWork(qjs::JSEngine& engine) {
    using clock = std::chrono::steady_clock;
    const auto deadline = clock::now() + std::chrono::seconds(120);
    int idleRounds = 0;

    while (clock::now() < deadline) {
        qianjs::event_loop::tick();
        qianjs::event_loop::run_deferred(engine);
        engine.pumpMicrotasks();

        const bool noNativeWait = qianjs::event_loop::pending_operations() == 0;
        const bool noJsJobs = !engine.isJobPending();

        if (noNativeWait && noJsJobs) {
            if (++idleRounds >= 3)
                return;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        idleRounds = 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cerr << "Warning: timed out while waiting for pending native I/O to finish\n";
}

/** Run a `.js` module or `.qbc` file from disk; installs default plugins and drains async work before exit. */
inline int runScriptFile(const std::filesystem::path& inputPath, std::vector<std::string> argv = {}) {
    qjs::JSEngine engine;
    engine.initialize();
    RuntimeContext runtime;
    if (argv.empty())
        runtime.argv.push_back(inputPath.string());
    else
        runtime.argv = std::move(argv);
    runtime.env = captureEnvironment();
    engine.setHost<RuntimeContext>(&runtime);
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
    return runtime.exit_code;
}

/** Run bytecode embedded in this executable (`Embed::readEmbeddedBytecode`); returns -1 if none. */
inline int runEmbeddedBytecode(std::vector<std::string> argv = {}) {
    std::vector<uint8_t> embedded = Embed::readEmbeddedBytecode();
    if (embedded.empty())
        return -1;

    qjs::JSEngine engine;
    engine.initialize();
    RuntimeContext runtime;
    runtime.argv = std::move(argv);
    runtime.env = captureEnvironment();
    engine.setHost<RuntimeContext>(&runtime);
    defaultPlugins().installAll(engine, engine.root());

    if (!engine.runBytecode(embedded.data(), embedded.size())) {
        engine.cleanup();
        return 1;
    }

    drainAsyncWork(engine);
    engine.cleanup();
    return runtime.exit_code;
}

} // namespace qianjs
