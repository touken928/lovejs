/**
 * LoveJS CLI - 命令行工具
 * 
 * 用法:
 *   lovejs                    - 运行同名 .qbc 文件 (lovejs.qbc)
 *   lovejs run <file.js>      - 运行 JS 文件
 *   lovejs run <file.qbc>     - 运行编译后的字节码文件
 *   lovejs build <file.js>    - 编译 JS 文件到 ./dist/<name>.qbc
 */

#include <sokol_app.h>
#include "core/GameLoop.hpp"
#include "core/JSEngine.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace fs = std::filesystem;

//=============================================================================
// 文件工具函数
//=============================================================================

static fs::path getExecutablePath() {
#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return fs::path(path);
#elif defined(__APPLE__)
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        return fs::canonical(path);
    }
    return fs::path();
#else
    return fs::canonical("/proc/self/exe");
#endif
}

static std::string readTextFile(const fs::path& path) {
    std::ifstream f(path);
    if (!f) return "";
    return std::string(std::istreambuf_iterator<char>(f), {});
}

static std::vector<uint8_t> readBinaryFile(const fs::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    
    auto size = f.tellg();
    f.seekg(0);
    
    std::vector<uint8_t> data(size);
    f.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

static bool writeBinaryFile(const fs::path& path, const std::vector<uint8_t>& data) {
    // 确保目录存在
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path());
    }
    
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(data.data()), data.size());
    return f.good();
}

//=============================================================================
// 命令处理
//=============================================================================

static void printUsage(const char* progName) {
    std::cout << "LoveJS - JavaScript Game Engine\n\n"
              << "Usage:\n"
              << "  " << progName << "                     Run bundled bytecode (<name>.qbc)\n"
              << "  " << progName << " run <file.js|qbc>   Run JS or bytecode file\n"
              << "  " << progName << " build <file.js>     Compile JS to ./dist/<name>.qbc\n"
              << "  " << progName << " help                Show this help\n"
              << std::endl;
}

static int cmdBuild(const fs::path& inputPath) {
    if (!fs::exists(inputPath)) {
        std::cerr << "Error: File not found: " << inputPath << std::endl;
        return 1;
    }
    
    // 读取源文件
    std::string code = readTextFile(inputPath);
    if (code.empty()) {
        std::cerr << "Error: Cannot read file: " << inputPath << std::endl;
        return 1;
    }
    
    // 编译
    auto result = JSEngine::compile(code, inputPath.string());
    if (!result.success) {
        std::cerr << "Compile error: " << result.error << std::endl;
        return 1;
    }
    
    // 输出路径
    fs::path outputPath = fs::path("dist") / inputPath.stem();
    outputPath.replace_extension(".qbc");
    
    // 写入文件
    if (!writeBinaryFile(outputPath, result.bytecode)) {
        std::cerr << "Error: Cannot write file: " << outputPath << std::endl;
        return 1;
    }
    
    std::cout << "Compiled: " << inputPath.string() << " -> " << outputPath.string()
              << " (" << result.bytecode.size() << " bytes)" << std::endl;
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
    fs::path exePath = getExecutablePath();
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
    // 无参数：尝试运行同名 .qbc 文件
    if (argc == 1) {
        int result = cmdRunBundled();
        if (result >= 0) return result;
        
        printUsage(argv[0]);
        return 1;
    }
    
    std::string cmd = argv[1];
    
    // help 命令
    if (cmd == "help" || cmd == "--help" || cmd == "-h") {
        printUsage(argv[0]);
        return 0;
    }
    
    // build 命令
    if (cmd == "build") {
        if (argc < 3) {
            std::cerr << "Error: Missing input file\n"
                      << "Usage: " << argv[0] << " build <file.js>" << std::endl;
            return 1;
        }
        return cmdBuild(argv[2]);
    }
    
    // run 命令
    if (cmd == "run") {
        if (argc < 3) {
            std::cerr << "Error: Missing input file\n"
                      << "Usage: " << argv[0] << " run <file.js|file.qbc>" << std::endl;
            return 1;
        }
        return cmdRun(argv[2]);
    }
    
    // 未知命令
    std::cerr << "Error: Unknown command: " << cmd << std::endl;
    printUsage(argv[0]);
    return 1;
}
