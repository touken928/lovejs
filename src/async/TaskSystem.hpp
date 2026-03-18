#pragma once

#include <atomic>
#include <condition_variable>
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
// This is intentionally minimal to keep the core extensible:
// concrete subsystems (fs, http, etc.) can capture their own data
// via the std::function.
using JobFunc = std::function<void(JobId)>;

// Record that a job has completed. For now this only carries the JobId;
// higher-level systems can keep their own maps from JobId -> result/continuation.
struct CompletedJob {
    JobId id;
};

// Simple single-worker task system:
// - thread-safe enqueue from any thread
// - one background worker thread executes jobs FIFO
// - completions are buffered and drained on the main thread
//
// This is deliberately designed so it can be evolved into:
// - a small fixed-size thread pool
// - a priority queue
// without changing the public surface much.
class TaskSystem {
public:
    static TaskSystem& instance() {
        static TaskSystem sys;
        return sys;
    }

    // Enqueue a job for background execution.
    // Returns a JobId which can be used to correlate results at a higher layer.
    JobId enqueue(JobFunc job) {
        JobId id{nextId_++};
        {
            std::lock_guard<std::mutex> lock(mutex_);
            jobs_.push({id, std::move(job)});
        }
        cv_.notify_one();
        return id;
    }

    // Drain all completed jobs since the last call.
    // Expected usage is from the main thread (e.g. inside AppLoop::frame_cb),
    // which can then look up per-JobId continuations or Promise resolvers.
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
    struct PendingJob {
        JobId id;
        JobFunc func;
    };

    TaskSystem()
        : stop_(false),
          nextId_(1),
          worker_([this] { workerLoop(); }) {}

    ~TaskSystem() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    TaskSystem(const TaskSystem&) = delete;
    TaskSystem& operator=(const TaskSystem&) = delete;

    void workerLoop() {
        for (;;) {
            PendingJob job;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] { return stop_ || !jobs_.empty(); });
                if (stop_ && jobs_.empty()) {
                    return;
                }
                job = std::move(jobs_.front());
                jobs_.pop();
            }

            // Run job outside the lock, passing the JobId to the function.
            if (job.func) {
                job.func(job.id);
            }

            {
                std::lock_guard<std::mutex> lock(mutex_);
                completed_.push({job.id});
            }
        }
    }

    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<PendingJob> jobs_;
    std::queue<CompletedJob> completed_;
    std::atomic<bool> stop_;
    std::atomic<std::uint64_t> nextId_;
    std::thread worker_;
};

} // namespace lovejs::async

