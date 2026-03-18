#pragma once

#include "../../async/TaskSystem.hpp"

#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>

namespace lovejs::fs_async {

struct Result {
    bool ok = false;
    std::string data;
};

inline std::mutex& resultsMutex() {
    static std::mutex m;
    return m;
}

inline std::unordered_map<std::uint64_t, Result>& resultsMap() {
    static std::unordered_map<std::uint64_t, Result> m;
    return m;
}

inline std::uint64_t readFileAsync(const std::string& path) {
    auto& ts = lovejs::async::TaskSystem::instance();
    lovejs::async::JobId id = ts.enqueue([path](lovejs::async::JobId jobId) {
        Result r;
        std::ifstream f(path, std::ios::binary);
        if (!f) {
            r.ok = false;
            r.data = "Cannot open file: " + path;
        } else {
            f.seekg(0, std::ios::end);
            std::streamsize size = f.tellg();
            f.seekg(0, std::ios::beg);
            if (size > 0) {
                r.data.resize(static_cast<std::size_t>(size));
                if (!f.read(&r.data[0], size)) {
                    r.ok = false;
                    r.data = "Failed to read file: " + path;
                } else {
                    r.ok = true;
                }
            } else {
                r.ok = true;
                r.data.clear();
            }
        }
        {
            std::lock_guard<std::mutex> lock(resultsMutex());
            resultsMap()[jobId.value] = std::move(r);
        }
    });
    return id.value;
}

inline std::uint64_t writeFileAsync(const std::string& path, const std::string& data) {
    auto& ts = lovejs::async::TaskSystem::instance();
    lovejs::async::JobId id = ts.enqueue([path, data](lovejs::async::JobId jobId) {
        Result r;
        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        if (!f) {
            r.ok = false;
            r.data = "Cannot open file for writing: " + path;
        } else {
            if (!data.empty())
                f.write(data.data(), static_cast<std::streamsize>(data.size()));
            r.ok = static_cast<bool>(f);
            if (!r.ok) r.data = "Failed to write file: " + path;
        }
        {
            std::lock_guard<std::mutex> lock(resultsMutex());
            resultsMap()[jobId.value] = std::move(r);
        }
    });
    return id.value;
}

struct CompletedResult {
    std::uint64_t id;
    Result result;
};

// Drain completed I/O results (main-thread only).
inline std::vector<CompletedResult> pumpResults() {
    auto completed = lovejs::async::TaskSystem::instance().drainCompleted();
    if (completed.empty()) return {};

    std::vector<CompletedResult> out;
    std::lock_guard<std::mutex> lock(resultsMutex());
    auto& m = resultsMap();
    for (auto& cj : completed) {
        auto it = m.find(cj.id.value);
        if (it != m.end()) {
            out.push_back({it->first, std::move(it->second)});
            m.erase(it);
        }
    }
    return out;
}

} // namespace lovejs::fs_async
