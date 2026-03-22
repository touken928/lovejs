#pragma once

#include "tinytp.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace lovejs::async {

// Shared process-wide thread pool (tinytp). Resize once on first use.
inline tinytp::thread_pool& globalThreadPool() {
    static tinytp::thread_pool pool;
    static const bool ready = [] {
        unsigned n = std::thread::hardware_concurrency();
        if (n == 0) n = 4;
        pool.resize(std::max(1u, n));
        return true;
    }();
    (void)ready;
    return pool;
}

// Reusable bridge: submit work to globalThreadPool(), track std::future with a stable id,
// drain completed work on the main thread without blocking (wait_for(0)).
//
// Each subsystem (fs, audio decode, …) typically holds one static AsyncOps<T> for its result type T.
template<typename T>
class AsyncOps {
public:
    std::uint64_t enqueue(std::function<T()> work) {
        const std::uint64_t id = next_id_++;
        auto fut = globalThreadPool().push(std::move(work));
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.push_back(Entry{id, std::move(fut)});
        return id;
    }

    std::vector<std::pair<std::uint64_t, T>> drainReady() {
        std::vector<std::pair<std::uint64_t, T>> out;
        std::lock_guard<std::mutex> lock(mutex_);
        static const auto zero = std::chrono::seconds(0);
        for (auto it = pending_.begin(); it != pending_.end();) {
            if (it->fut.wait_for(zero) == std::future_status::ready) {
                out.emplace_back(it->id, it->fut.get());
                it = pending_.erase(it);
            } else {
                ++it;
            }
        }
        return out;
    }

private:
    struct Entry {
        std::uint64_t id;
        std::future<T> fut;
    };
    std::mutex mutex_;
    std::vector<Entry> pending_;
    std::atomic<std::uint64_t> next_id_{1};
};

// For fire-and-forget async work where only completion order/id matters (no return value).
template<>
class AsyncOps<void> {
public:
    std::uint64_t enqueue(std::function<void()> work) {
        const std::uint64_t id = next_id_++;
        auto fut = globalThreadPool().push(std::move(work));
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.push_back(Entry{id, std::move(fut)});
        return id;
    }

    std::vector<std::uint64_t> drainReady() {
        std::vector<std::uint64_t> out;
        std::lock_guard<std::mutex> lock(mutex_);
        static const auto zero = std::chrono::seconds(0);
        for (auto it = pending_.begin(); it != pending_.end();) {
            if (it->fut.wait_for(zero) == std::future_status::ready) {
                it->fut.get();
                out.push_back(it->id);
                it = pending_.erase(it);
            } else {
                ++it;
            }
        }
        return out;
    }

private:
    struct Entry {
        std::uint64_t id;
        std::future<void> fut;
    };
    std::mutex mutex_;
    std::vector<Entry> pending_;
    std::atomic<std::uint64_t> next_id_{1};
};

} // namespace lovejs::async
