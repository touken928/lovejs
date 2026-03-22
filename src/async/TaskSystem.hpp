#pragma once

#include "tinytp.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace lovejs::async {

// Opaque job identifier (monotonic, per-process)
struct JobId {
    std::uint64_t value{};

    bool operator==(const JobId& other) const noexcept { return value == other.value; }
    bool operator!=(const JobId& other) const noexcept { return value != other.value; }
};

// Job payload executed on a background worker.
// The JobId of the enqueued task is provided to the function so that
// higher layers can correlate results.
using JobFunc = std::function<void(JobId)>;

struct CompletedJob {
    JobId id;
};

// Background execution via tinytp thread pool (see third_party/tinytp).
// - enqueue is thread-safe (tinytp serializes push); workers run jobs concurrently
// - completions are buffered and drained on the main thread
class TaskSystem {
public:
    static TaskSystem& instance() {
        static TaskSystem sys;
        return sys;
    }

    JobId enqueue(JobFunc job) {
        JobId id{nextId_++};
        (void)pool_.push([this, id, job = std::move(job)]() mutable {
            if (job) {
                job(id);
            }
            {
                std::lock_guard<std::mutex> lock(mutex_);
                completed_.push({id});
            }
        });
        return id;
    }

    std::vector<CompletedJob> drainCompleted() {
        std::vector<CompletedJob> out;
        std::lock_guard<std::mutex> lock(mutex_);
        while (!completed_.empty()) {
            out.push_back(completed_.front());
            completed_.pop();
        }
        return out;
    }

private:
    TaskSystem() : nextId_(1) {
        unsigned n = std::thread::hardware_concurrency();
        if (n == 0) n = 4;
        pool_.resize(n);
    }

    ~TaskSystem() = default;

    TaskSystem(const TaskSystem&) = delete;
    TaskSystem& operator=(const TaskSystem&) = delete;

    tinytp::thread_pool pool_;
    std::mutex mutex_;
    std::queue<CompletedJob> completed_;
    std::atomic<std::uint64_t> nextId_;
};

} // namespace lovejs::async
