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
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
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
// 嵌入字节码支持
//=============================================================================

// 嵌入数据的尾部结构 (16 bytes):
// [8 bytes: magic "LOVEJSBC"] [4 bytes: bytecode size] [4 bytes: reserved]
static constexpr char EMBED_MAGIC[8] = {'L', 'O', 'V', 'E', 'J', 'S', 'B', 'C'};
static constexpr size_t EMBED_FOOTER_SIZE = 16;

struct EmbedFooter {
    char magic[8];
    uint32_t bytecodeSize;
    uint32_t reserved;
};

static std::vector<uint8_t> readEmbeddedBytecode() {
    fs::path exePath = getExecutablePath();
    std::ifstream f(exePath, std::ios::binary | std::ios::ate);
    if (!f) return {};
    
    auto fileSize = static_cast<size_t>(f.tellg());
    if (fileSize < EMBED_FOOTER_SIZE) return {};
    
    // 读取尾部
    f.seekg(-static_cast<std::streamoff>(EMBED_FOOTER_SIZE), std::ios::end);
    EmbedFooter footer;
    f.read(reinterpret_cast<char*>(&footer), sizeof(footer));
    
    // 检查 magic
    if (std::memcmp(footer.magic, EMBED_MAGIC, 8) != 0) return {};
    
    // 验证大小
    if (footer.bytecodeSize == 0 || footer.bytecodeSize > fileSize - EMBED_FOOTER_SIZE) return {};
    
    // 读取字节码
    f.seekg(-static_cast<std::streamoff>(EMBED_FOOTER_SIZE + footer.bytecodeSize), std::ios::end);
    std::vector<uint8_t> bytecode(footer.bytecodeSize);
    f.read(reinterpret_cast<char*>(bytecode.data()), footer.bytecodeSize);
    
    return bytecode;
}

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

static int cmdEmbed(const fs::path& qbcPath) {
    if (!fs::exists(qbcPath)) {
        std::cerr << "Error: File not found: " << qbcPath << std::endl;
        return 1;
    }
    
    if (qbcPath.extension() != ".qbc") {
        std::cerr << "Error: Expected .qbc file, got: " << qbcPath << std::endl;
        return 1;
    }
    
    // 读取字节码
    std::vector<uint8_t> bytecode = readBinaryFile(qbcPath);
    if (bytecode.empty()) {
        std::cerr << "Error: Cannot read bytecode file: " << qbcPath << std::endl;
        return 1;
    }
    
    // 读取当前可执行文件
    fs::path exePath = getExecutablePath();
    std::vector<uint8_t> exeData = readBinaryFile(exePath);
    if (exeData.empty()) {
        std::cerr << "Error: Cannot read executable: " << exePath << std::endl;
        return 1;
    }
    
    // 检查当前可执行文件是否已经嵌入了字节码，如果是则去掉
    if (exeData.size() >= EMBED_FOOTER_SIZE) {
        EmbedFooter* existingFooter = reinterpret_cast<EmbedFooter*>(
            exeData.data() + exeData.size() - EMBED_FOOTER_SIZE);
        if (std::memcmp(existingFooter->magic, EMBED_MAGIC, 8) == 0) {
            // 去掉已嵌入的数据
            size_t originalSize = exeData.size() - EMBED_FOOTER_SIZE - existingFooter->bytecodeSize;
            exeData.resize(originalSize);
        }
    }
    
    // 输出路径：qbc 同目录，文件名为 qbc 的 stem
    fs::path outputPath = qbcPath.parent_path() / qbcPath.stem();
#ifdef _WIN32
    outputPath.replace_extension(".exe");
#endif
    
    // 构建新的可执行文件：原始exe + 字节码 + footer
    std::vector<uint8_t> outputData;
    outputData.reserve(exeData.size() + bytecode.size() + EMBED_FOOTER_SIZE);
    outputData.insert(outputData.end(), exeData.begin(), exeData.end());
    outputData.insert(outputData.end(), bytecode.begin(), bytecode.end());
    
    // 添加 footer
    EmbedFooter footer;
    std::memcpy(footer.magic, EMBED_MAGIC, 8);
    footer.bytecodeSize = static_cast<uint32_t>(bytecode.size());
    footer.reserved = 0;
    
    const uint8_t* footerBytes = reinterpret_cast<const uint8_t*>(&footer);
    outputData.insert(outputData.end(), footerBytes, footerBytes + sizeof(footer));
    
    // 写入文件
    if (!writeBinaryFile(outputPath, outputData)) {
        std::cerr << "Error: Cannot write output file: " << outputPath << std::endl;
        return 1;
    }
    
    // 设置可执行权限 (Unix)
#ifndef _WIN32
    fs::permissions(outputPath, fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
                    fs::perm_options::add);
#endif
    
    std::cout << "Embedded: " << qbcPath.string() << " -> " << outputPath.string()
              << " (" << outputData.size() << " bytes)" << std::endl;
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
    std::vector<uint8_t> embedded = readEmbeddedBytecode();
    if (!embedded.empty()) {
        sapp_desc desc = GameLoop::setupFromMemory(embedded.data(), embedded.size());
        sapp_run(&desc);
        return 0;
    }
    
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
    
    // embed 命令
    if (cmd == "embed") {
        if (argc < 3) {
            std::cerr << "Error: Missing input file\n"
                      << "Usage: " << argv[0] << " embed <file.qbc>" << std::endl;
            return 1;
        }
        return cmdEmbed(argv[2]);
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
