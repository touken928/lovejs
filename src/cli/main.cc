/**
 * QianJS CLI — lightweight JavaScript runtime (QuickJS / qjs).
 *
 * Usage:
 *   qianjs run <file.js>      - Run a JS file (ES module)
 *   qianjs run <file.qbc>     - Run compiled bytecode
 *   qianjs build <file.js>    - Compile JS to ./dist/<name>.qbc
 *   qianjs embed <file.qbc>   - Append bytecode to a copy of this executable
 */

#include "native/default_plugins.h"
#include "runtime/embed.h"
#include "runtime/headless_runtime.h"

#include <js_engine.h>
#include <iostream>

namespace fs = std::filesystem;

static void printUsage(const char* progName) {
    std::cout << "QianJS — JavaScript runtime\n\n"
              << "Usage:\n"
              << "  " << progName << " run <file.js|qbc>   Run JS or bytecode\n"
              << "  " << progName << " build <file.js>     Compile JS to ./dist/<name>.qbc\n"
              << "  " << progName << " embed <file.qbc>    Embed bytecode into a standalone executable\n"
              << "  " << progName << " help                Show this help\n"
              << std::endl;
}

static int cmdBuild(const fs::path& inputPath) {
    if (!fs::exists(inputPath)) {
        std::cerr << "Error: File not found: " << inputPath << std::endl;
        return 1;
    }

    std::string code = Embed::readTextFile(inputPath);
    if (code.empty()) {
        std::cerr << "Error: Cannot read file: " << inputPath << std::endl;
        return 1;
    }

    qjs::JSEngine engine;
    engine.initialize();
    defaultPlugins().installAll(engine, engine.root());
    qjs::JSEngine::CompileResult result = engine.compileModuleFromSource(code, inputPath.string());
    engine.cleanup();
    if (!result.success) {
        std::cerr << "Compile error: " << result.error << std::endl;
        return 1;
    }

    fs::path outputPath = fs::path("dist") / inputPath.stem();
    outputPath.replace_extension(".qbc");

    if (!Embed::writeBinaryFile(outputPath, result.bytecode)) {
        std::cerr << "Error: Cannot write file: " << outputPath << std::endl;
        return 1;
    }

    std::cout << "Compiled: " << inputPath.string() << " -> " << outputPath.string()
              << " (" << result.bytecode.size() << " bytes)" << std::endl;
    return 0;
}

static int cmdEmbed(const fs::path& qbcPath) {
    if (!fs::exists(qbcPath)) {
        std::cerr << "Error: File not found: " << qbcPath << std::endl;
        return 1;
    }

    if (qbcPath.extension() != ".qbc") {
        std::cerr << "Error: Expected .qbc file, got: " << qbcPath << std::endl;
        return 1;
    }

    std::vector<uint8_t> bytecode = Embed::readBinaryFile(qbcPath);
    if (bytecode.empty()) {
        std::cerr << "Error: Cannot read bytecode file: " << qbcPath << std::endl;
        return 1;
    }

    fs::path outputPath = qbcPath.parent_path() / qbcPath.stem();
#ifdef _WIN32
    outputPath.replace_extension(".exe");
#endif

    if (!Embed::createEmbeddedExecutable(bytecode, outputPath)) {
        std::cerr << "Error: Cannot create embedded executable: " << outputPath << std::endl;
        return 1;
    }

    auto outputSize = fs::file_size(outputPath);
    std::cout << "Embedded: " << qbcPath.string() << " -> " << outputPath.string()
              << " (" << outputSize << " bytes)" << std::endl;
    return 0;
}

static int cmdRun(const fs::path& inputPath) {
    if (!fs::exists(inputPath)) {
        std::cerr << "Error: File not found: " << inputPath << std::endl;
        return 1;
    }
    return qianjs::runHeadlessScript(inputPath);
}

static int cmdRunBundled() {
    int result = qianjs::runHeadlessEmbedded();
    if (result >= 0)
        return result;

    fs::path exePath = Embed::getExecutablePath();
    fs::path exeDir = exePath.parent_path();
    std::string exeName = exePath.stem().string();

    fs::path qbcPath = exeDir / (exeName + ".qbc");
    if (fs::exists(qbcPath))
        return cmdRun(qbcPath);

    qbcPath = fs::path(exeName + ".qbc");
    if (fs::exists(qbcPath))
        return cmdRun(qbcPath);

    return -1;
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        int result = cmdRunBundled();
        if (result >= 0)
            return result;

        printUsage(argv[0]);
        return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "help" || cmd == "--help" || cmd == "-h") {
        printUsage(argv[0]);
        return 0;
    }

    if (cmd == "build") {
        if (argc < 3) {
            std::cerr << "Error: Missing input file\n"
                      << "Usage: " << argv[0] << " build <file.js>" << std::endl;
            return 1;
        }
        return cmdBuild(argv[2]);
    }

    if (cmd == "embed") {
        if (argc < 3) {
            std::cerr << "Error: Missing input file\n"
                      << "Usage: " << argv[0] << " embed <file.qbc>" << std::endl;
            return 1;
        }
        return cmdEmbed(argv[2]);
    }

    if (cmd == "run") {
        if (argc < 3) {
            std::cerr << "Error: Missing input file\n"
                      << "Usage: " << argv[0] << " run <file.js|file.qbc>" << std::endl;
            return 1;
        }
        return cmdRun(argv[2]);
    }

    std::cerr << "Error: Unknown command: " << cmd << std::endl;
    printUsage(argv[0]);
    return 1;
}
