/**
 * LoveJS CLI - 命令行工具
 * 
 * 用法:
 *   lovejs run <file.js>      - 运行 JS 文件
 *   lovejs run <file.qbc>     - 运行编译后的字节码文件
 *   lovejs build <file.js>    - 编译 JS 文件到 ./dist/<name>.qbc
 *   lovejs embed <file.qbc>   - 将字节码嵌入到可执行文件，生成独立程序
 */

#include <sokol_app.h>
#include "core/GameLoop.hpp"
#include "core/JSEngine.hpp"
#include "core/Embed.hpp"
#include <iostream>

namespace fs = std::filesystem;

//=============================================================================
// 命令处理
//=============================================================================

static void printUsage(const char* progName) {
    std::cout << "LoveJS - JavaScript Game Engine\n\n"
              << "Usage:\n"
              << "  " << progName << " run <file.js|qbc>   Run JS or bytecode file\n"
              << "  " << progName << " build <file.js>     Compile JS to ./dist/<name>.qbc\n"
              << "  " << progName << " embed <file.qbc>    Embed bytecode into standalone executable\n"
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
    
    auto result = JSEngine::compile(code, inputPath.string());
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
    
    sapp_desc desc;
    if (inputPath.extension() == ".qbc") {
        desc = GameLoop::setupBytecode(inputPath.string().c_str());
    } else {
        desc = GameLoop::setup(inputPath.string().c_str());
    }
    
    sapp_run(&desc);
    return 0;
}

static int cmdRunBundled() {
    // 首先检查是否有嵌入的字节码
    std::vector<uint8_t> embedded = Embed::readEmbeddedBytecode();
    if (!embedded.empty()) {
        sapp_desc desc = GameLoop::setupFromMemory(embedded.data(), embedded.size());
        sapp_run(&desc);
        return 0;
    }
    
    fs::path exePath = Embed::getExecutablePath();
    fs::path exeDir = exePath.parent_path();
    std::string exeName = exePath.stem().string();
    
    // 检查可执行文件目录
    fs::path qbcPath = exeDir / (exeName + ".qbc");
    if (fs::exists(qbcPath)) {
        return cmdRun(qbcPath);
    }
    
    // 检查当前目录
    qbcPath = fs::path(exeName + ".qbc");
    if (fs::exists(qbcPath)) {
        return cmdRun(qbcPath);
    }
    
    return -1; // 未找到
}

//=============================================================================
// 主函数
//=============================================================================

int main(int argc, char* argv[]) {
    // 无参数：尝试运行嵌入或同名 .qbc 文件
    if (argc == 1) {
        int result = cmdRunBundled();
        if (result >= 0) return result;
        
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
