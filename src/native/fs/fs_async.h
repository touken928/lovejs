#pragma once

#include "runtime/async/async_runtime.h"

#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace qianjs::fs_async {

// 与 FsModule 中 isWrite 配合：
// - readFile：有值 = 成功（含空文件），无值 = 读失败
// - writeFile：无值 = 成功，有值 = 失败原因
using FsData = std::optional<std::string>;

inline qianjs::async::AsyncOps<FsData>& fsAsyncOps() {
    static qianjs::async::AsyncOps<FsData> ops;
    return ops;
}

inline FsData readFileSync(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return std::nullopt;
    f.seekg(0, std::ios::end);
    std::streamsize size = f.tellg();
    f.seekg(0, std::ios::beg);
    std::string out;
    if (size > 0) {
        out.resize(static_cast<std::size_t>(size));
        if (!f.read(&out[0], size)) return std::nullopt;
    }
    return out;
}

inline FsData writeFileSync(const std::string& path, const std::string& data) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return std::string("Cannot open file for writing: " + path);
    if (!data.empty())
        f.write(data.data(), static_cast<std::streamsize>(data.size()));
    if (!f) return std::string("Failed to write file: " + path);
    return std::nullopt;
}

inline std::uint64_t readFileAsync(const std::string& path) {
    return fsAsyncOps().enqueue([path]() { return readFileSync(path); });
}

inline std::uint64_t writeFileAsync(const std::string& path, const std::string& data) {
    return fsAsyncOps().enqueue([path, data]() { return writeFileSync(path, data); });
}

struct CompletedResult {
    std::uint64_t id;
    FsData result;
};

inline std::vector<CompletedResult> pumpResults() {
    std::vector<CompletedResult> out;
    for (auto& [id, r] : fsAsyncOps().drainReady()) {
        out.push_back(CompletedResult{id, std::move(r)});
    }
    return out;
}

} // namespace qianjs::fs_async
