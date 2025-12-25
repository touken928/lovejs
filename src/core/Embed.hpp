#pragma once

#include <filesystem>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace fs = std::filesystem;

/**
 * Embed - 字节码嵌入支持
 * 
 * 提供将字节码嵌入到可执行文件的功能，以及从可执行文件中读取嵌入的字节码。
 * 
 * 嵌入格式：
 *   [原始可执行文件] [字节码数据] [16字节 Footer]
 *   
 * Footer 结构 (16 bytes):
 *   [8 bytes: magic "LOVEJSBC"] [4 bytes: bytecode size] [4 bytes: reserved]
 */
class Embed {
public:
    // 嵌入数据的 magic number
    static constexpr char MAGIC[8] = {'L', 'O', 'V', 'E', 'J', 'S', 'B', 'C'};
    static constexpr size_t FOOTER_SIZE = 16;
    
    struct Footer {
        char magic[8];
        uint32_t bytecodeSize;
        uint32_t reserved;
    };
    
    /**
     * 获取当前可执行文件路径
     */
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
    
    /**
     * 读取二进制文件
     */
    static std::vector<uint8_t> readBinaryFile(const fs::path& path) {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f) return {};
        
        auto size = f.tellg();
        f.seekg(0);
        
        std::vector<uint8_t> data(size);
        f.read(reinterpret_cast<char*>(data.data()), size);
        return data;
    }
    
    /**
     * 写入二进制文件
     */
    static bool writeBinaryFile(const fs::path& path, const std::vector<uint8_t>& data) {
        if (path.has_parent_path()) {
            fs::create_directories(path.parent_path());
        }
        
        std::ofstream f(path, std::ios::binary);
        if (!f) return false;
        f.write(reinterpret_cast<const char*>(data.data()), data.size());
        return f.good();
    }
    
    /**
     * 读取文本文件
     */
    static std::string readTextFile(const fs::path& path) {
        std::ifstream f(path);
        if (!f) return "";
        return std::string(std::istreambuf_iterator<char>(f), {});
    }
    
    /**
     * 检查可执行文件是否包含嵌入的字节码
     */
    static bool hasEmbeddedBytecode() {
        return !readEmbeddedBytecode().empty();
    }
    
    /**
     * 从当前可执行文件读取嵌入的字节码
     * @return 字节码数据，如果没有嵌入则返回空 vector
     */
    static std::vector<uint8_t> readEmbeddedBytecode() {
        fs::path exePath = getExecutablePath();
        std::ifstream f(exePath, std::ios::binary | std::ios::ate);
        if (!f) return {};
        
        auto fileSize = static_cast<size_t>(f.tellg());
        if (fileSize < FOOTER_SIZE) return {};
        
        // 读取尾部
        f.seekg(-static_cast<std::streamoff>(FOOTER_SIZE), std::ios::end);
        Footer footer;
        f.read(reinterpret_cast<char*>(&footer), sizeof(footer));
        
        // 检查 magic
        if (std::memcmp(footer.magic, MAGIC, 8) != 0) return {};
        
        // 验证大小
        if (footer.bytecodeSize == 0 || footer.bytecodeSize > fileSize - FOOTER_SIZE) return {};
        
        // 读取字节码
        f.seekg(-static_cast<std::streamoff>(FOOTER_SIZE + footer.bytecodeSize), std::ios::end);
        std::vector<uint8_t> bytecode(footer.bytecodeSize);
        f.read(reinterpret_cast<char*>(bytecode.data()), footer.bytecodeSize);
        
        return bytecode;
    }
    
    /**
     * 获取去除嵌入数据后的原始可执行文件数据
     * @return 原始可执行文件数据
     */
    static std::vector<uint8_t> getCleanExecutable() {
        fs::path exePath = getExecutablePath();
        std::vector<uint8_t> exeData = readBinaryFile(exePath);
        if (exeData.empty()) return {};
        
        // 检查是否已经嵌入了字节码
        if (exeData.size() >= FOOTER_SIZE) {
            Footer* existingFooter = reinterpret_cast<Footer*>(
                exeData.data() + exeData.size() - FOOTER_SIZE);
            if (std::memcmp(existingFooter->magic, MAGIC, 8) == 0) {
                // 去掉已嵌入的数据
                size_t originalSize = exeData.size() - FOOTER_SIZE - existingFooter->bytecodeSize;
                exeData.resize(originalSize);
            }
        }
        
        return exeData;
    }
    
    /**
     * 创建嵌入字节码的可执行文件
     * @param bytecode 要嵌入的字节码
     * @param outputPath 输出路径
     * @return 是否成功
     */
    static bool createEmbeddedExecutable(const std::vector<uint8_t>& bytecode, const fs::path& outputPath) {
        // 获取干净的可执行文件
        std::vector<uint8_t> exeData = getCleanExecutable();
        if (exeData.empty()) return false;
        
        // 构建新的可执行文件：原始exe + 字节码 + footer
        std::vector<uint8_t> outputData;
        outputData.reserve(exeData.size() + bytecode.size() + FOOTER_SIZE);
        outputData.insert(outputData.end(), exeData.begin(), exeData.end());
        outputData.insert(outputData.end(), bytecode.begin(), bytecode.end());
        
        // 添加 footer
        Footer footer;
        std::memcpy(footer.magic, MAGIC, 8);
        footer.bytecodeSize = static_cast<uint32_t>(bytecode.size());
        footer.reserved = 0;
        
        const uint8_t* footerBytes = reinterpret_cast<const uint8_t*>(&footer);
        outputData.insert(outputData.end(), footerBytes, footerBytes + sizeof(footer));
        
        // 写入文件
        if (!writeBinaryFile(outputPath, outputData)) return false;
        
        // 设置可执行权限 (Unix)
#ifndef _WIN32
        fs::permissions(outputPath, fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
                        fs::perm_options::add);
#endif
        
        return true;
    }
};
