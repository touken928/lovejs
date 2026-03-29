#include "runtime/event_loop/event_loop.h"

#if QIANJS_HAVE_LIBUV
#include <uvw.hpp>
#endif

#include <atomic>
#include <cstdlib>
#include <mutex>
#include <utility>
#include <vector>

namespace {

#if QIANJS_HAVE_LIBUV
std::mutex g_loop_mutex;
std::shared_ptr<uvw::loop> g_uvw_loop;
#endif

std::mutex g_js_mutex;
std::vector<std::function<void(qjs::JSEngine&)>> g_js_pending;

std::atomic<int> g_pending_ops{0};

} // namespace

#if QIANJS_HAVE_LIBUV

namespace qianjs::event_loop::uv {

std::shared_ptr<uvw::loop> uvw_loop() {
    std::lock_guard<std::mutex> lock(g_loop_mutex);
    if (!g_uvw_loop)
        g_uvw_loop = uvw::loop::create();
    return g_uvw_loop;
}

uv_loop_t* loop() { return uvw_loop()->raw(); }

} // namespace qianjs::event_loop::uv

#endif

namespace qianjs::event_loop {

#if QIANJS_HAVE_LIBUV
void ensure_started() { (void)uv::uvw_loop(); }

void tick() { uv::uvw_loop()->run(uvw::loop::run_mode::NOWAIT); }
#else
void ensure_started() {}

void tick() {}
#endif

void defer(std::function<void(qjs::JSEngine&)> fn) {
    std::lock_guard<std::mutex> lock(g_js_mutex);
    g_js_pending.push_back(std::move(fn));
}

void run_deferred(qjs::JSEngine& engine) {
    std::vector<std::function<void(qjs::JSEngine&)>> batch;
    {
        std::lock_guard<std::mutex> lock(g_js_mutex);
        batch.swap(g_js_pending);
    }
    for (auto& f : batch)
        f(engine);
}

void begin_operation() { g_pending_ops.fetch_add(1, std::memory_order_relaxed); }

void end_operation() { g_pending_ops.fetch_sub(1, std::memory_order_relaxed); }

int pending_operations() { return g_pending_ops.load(std::memory_order_relaxed); }

void shutdown() {}

} // namespace qianjs::event_loop
