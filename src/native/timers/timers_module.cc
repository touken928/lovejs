#include "native/timers/timers_module.h"

#include "runtime/event_loop/event_loop.h"

#include <js_engine.h>
#include <js_module.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace {

struct TimerCtx {
    int64_t id = 0;
    int64_t delay_ms = 0;
    bool repeat = false;
    std::atomic<bool> cancelled{false};
    JSValue callback = JS_UNDEFINED;
};

std::mutex g_timer_mu;
std::unordered_map<int64_t, std::shared_ptr<TimerCtx>> g_timers;
std::atomic<int64_t> g_next_timer_id{1};

void clear_timer_impl(int64_t id) {
    std::shared_ptr<TimerCtx> timer;
    {
        std::lock_guard<std::mutex> lock(g_timer_mu);
        auto it = g_timers.find(id);
        if (it == g_timers.end())
            return;
        timer = it->second;
        g_timers.erase(it);
    }

    if (timer->cancelled.exchange(true))
        return;
    qianjs::event_loop::end_operation();
    qianjs::event_loop::defer([timer](qjs::JSEngine& e) {
        JSContext* c = e.ctx();
        JS_FreeValue(c, timer->callback);
        timer->callback = JS_UNDEFINED;
    });
}

void run_callback(qjs::JSEngine& engine, const std::shared_ptr<TimerCtx>& timer) {
    if (timer->cancelled.load(std::memory_order_relaxed))
        return;

    JSContext* c = engine.ctx();
    JSValue ret = JS_Call(c, timer->callback, JS_UNDEFINED, 0, nullptr);
    if (JS_IsException(ret)) {
        JSValue exc = JS_GetException(c);
        const char* msg = JS_ToCString(c, exc);
        if (msg) {
            std::cerr << "timers callback exception: " << msg << '\n';
            JS_FreeCString(c, msg);
        } else {
            std::cerr << "timers callback exception\n";
        }
        JS_FreeValue(c, exc);
    } else {
        JS_FreeValue(c, ret);
    }

    if (!timer->repeat) {
        if (timer->cancelled.exchange(true))
            return;
        {
            std::lock_guard<std::mutex> lock(g_timer_mu);
            auto it = g_timers.find(timer->id);
            if (it != g_timers.end() && it->second.get() == timer.get())
                g_timers.erase(it);
        }
        JS_FreeValue(c, timer->callback);
        timer->callback = JS_UNDEFINED;
        qianjs::event_loop::end_operation();
    }
}

void launch_timer_thread(const std::shared_ptr<TimerCtx>& timer) {
    std::thread([timer]() {
        using namespace std::chrono_literals;
        const auto delay = std::chrono::milliseconds(timer->delay_ms);

        if (!timer->repeat) {
            std::this_thread::sleep_for(delay);
            if (timer->cancelled.load(std::memory_order_relaxed))
                return;
            qianjs::event_loop::defer([timer](qjs::JSEngine& e) { run_callback(e, timer); });
            return;
        }

        while (!timer->cancelled.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(delay);
            if (timer->cancelled.load(std::memory_order_relaxed))
                return;
            qianjs::event_loop::defer([timer](qjs::JSEngine& e) { run_callback(e, timer); });
        }
    }).detach();
}

int64_t create_timer(JSContext* c, JSValue fn, int64_t delay_ms, bool repeat) {
    auto timer = std::make_shared<TimerCtx>();
    timer->id = g_next_timer_id.fetch_add(1, std::memory_order_relaxed);
    if (delay_ms < 0)
        delay_ms = 0;
    if (repeat && delay_ms == 0)
        delay_ms = 1;
    timer->delay_ms = delay_ms;
    timer->repeat = repeat;
    timer->callback = JS_DupValue(c, fn);

    {
        std::lock_guard<std::mutex> lock(g_timer_mu);
        g_timers[timer->id] = timer;
    }
    qianjs::event_loop::begin_operation();
    launch_timer_thread(timer);
    return timer->id;
}

} // namespace

const char* TimersPlugin::name() const {
    return "timers";
}

void TimersPlugin::install(qjs::JSEngine& engine, qjs::JSModule& root) {
    (void)engine;
    qianjs::event_loop::ensure_started();
    auto& m = root.module("timers");

    m.funcDynamic("setTimeout", 2, 2, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        if (!JS_IsFunction(c, argv[0]))
            return JS_ThrowTypeError(c, "setTimeout: callback must be function");
        int64_t delay = 0;
        if (JS_ToInt64(c, &delay, argv[1]) < 0)
            return JS_EXCEPTION;
        const int64_t id = create_timer(c, argv[0], delay, false);
        return JS_NewInt64(c, id);
    });

    m.funcDynamic("setInterval", 2, 2, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        if (!JS_IsFunction(c, argv[0]))
            return JS_ThrowTypeError(c, "setInterval: callback must be function");
        int64_t delay = 0;
        if (JS_ToInt64(c, &delay, argv[1]) < 0)
            return JS_EXCEPTION;
        const int64_t id = create_timer(c, argv[0], delay, true);
        return JS_NewInt64(c, id);
    });

    m.func("clearTimeout", [](int64_t id) { clear_timer_impl(id); });
    m.func("clearInterval", [](int64_t id) { clear_timer_impl(id); });
}
