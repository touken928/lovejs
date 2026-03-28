#pragma once

#include "BS_thread_pool.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <mutex>
#include <utility>
#include <vector>

namespace qianjs::async {

namespace detail {
inline constexpr std::chrono::seconds kFuturePollTimeout{0};
}

inline BS::light_thread_pool& globalThreadPool() {
    static BS::light_thread_pool pool;
    return pool;
}

template<typename T>
class AsyncOps {
public:
    std::uint64_t enqueue(std::function<T()> work) {
        const std::uint64_t id = next_id_++;
        auto fut = globalThreadPool().submit_task(std::move(work));
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.push_back(Entry{id, std::move(fut)});
        return id;
    }

    std::vector<std::pair<std::uint64_t, T>> drainReady() {
        std::vector<std::pair<std::uint64_t, T>> out;
        std::lock_guard<std::mutex> lock(mutex_);
        if (pending_.empty()) {
            return out;
        }
        out.reserve(pending_.size());
        // Swap-with-back removes O(1) per ready task; order of pending_ is not meaningful for callers.
        for (std::size_t i = 0; i < pending_.size();) {
            if (pending_[i].fut.wait_for(detail::kFuturePollTimeout) == std::future_status::ready) {
                out.emplace_back(pending_[i].id, pending_[i].fut.get());
                if (i + 1 < pending_.size()) {
                    pending_[i] = std::move(pending_.back());
                }
                pending_.pop_back();
            } else {
                ++i;
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

template<>
class AsyncOps<void> {
public:
    std::uint64_t enqueue(std::function<void()> work) {
        const std::uint64_t id = next_id_++;
        auto fut = globalThreadPool().submit_task(std::move(work));
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.push_back(Entry{id, std::move(fut)});
        return id;
    }

    std::vector<std::uint64_t> drainReady() {
        std::vector<std::uint64_t> out;
        std::lock_guard<std::mutex> lock(mutex_);
        if (pending_.empty()) {
            return out;
        }
        out.reserve(pending_.size());
        for (std::size_t i = 0; i < pending_.size();) {
            if (pending_[i].fut.wait_for(detail::kFuturePollTimeout) == std::future_status::ready) {
                pending_[i].fut.get();
                out.push_back(pending_[i].id);
                if (i + 1 < pending_.size()) {
                    pending_[i] = std::move(pending_.back());
                }
                pending_.pop_back();
            } else {
                ++i;
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

} // namespace qianjs::async
