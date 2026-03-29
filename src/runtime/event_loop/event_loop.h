#pragma once

#include <js_engine.h>

#include <functional>
#include <memory>

namespace uvw {
class loop;
}

struct uv_loop_s;
typedef struct uv_loop_s uv_loop_t;

namespace qianjs::event_loop::uv {

/** Shared uvw loop owned by the runtime (`event_loop.cc`). */
std::shared_ptr<uvw::loop> uvw_loop();

/** Underlying `uv_loop_t*`; same lifetime as `uvw_loop()`. */
uv_loop_t* loop();

} // namespace qianjs::event_loop::uv

namespace qianjs::event_loop {

/**
 * Host-side event driver (backed by uvw/libuv inside the runtime).
 * Native I/O code includes this header for `defer` / `tick` / `uvw_loop()`, and `<uvw.hpp>` for uvw types.
 */

/** Idempotent; ensures the loop exists (used implicitly on first `tick()` / `uvw_loop()`). */
void ensure_started();

/** One non-blocking uvw/libuv pass (`UV_RUN_NOWAIT`). */
void tick();

/**
 * Queue work that must run on the JS thread (Promise settle, engine APIs).
 * Safe to call from libuv callbacks; runs on the next `run_deferred`.
 */
void defer(std::function<void(qjs::JSEngine&)> fn);

void run_deferred(qjs::JSEngine& engine);

/** Book-keeping for fs/net ops so the host knows when the process can go idle. */
void begin_operation();
void end_operation();
int pending_operations();

void shutdown();

} // namespace qianjs::event_loop
