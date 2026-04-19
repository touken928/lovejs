#pragma once

#include "runtime/event_loop/event_loop.h"

#include <js_engine.h>

#include <string>
#include <utility>

namespace qianjs::fs::schedule {

inline void reject(qjs::JSEngine::PromiseHandle ph, std::string msg, const std::string& code = {}) {
    qianjs::event_loop::defer(
        [ph, msg = std::move(msg), code](qjs::JSEngine& e) {
            if (code.empty())
                e.rejectPromise(ph, msg);
            else
                e.rejectPromise(ph, msg, code);
            e.freePromise(ph);
        });
}

inline void resolve_void(qjs::JSEngine::PromiseHandle ph) {
    qianjs::event_loop::defer([ph](qjs::JSEngine& e) {
        e.resolvePromiseVoid(ph);
        e.freePromise(ph);
    });
}

inline void resolve_string(qjs::JSEngine::PromiseHandle ph, std::string data) {
    qianjs::event_loop::defer(
        [ph, data = std::move(data)](qjs::JSEngine& e) {
            e.resolvePromise(ph, data);
            e.freePromise(ph);
        });
}

inline void resolve_bytes(qjs::JSEngine::PromiseHandle ph, std::string buffer) {
    qianjs::event_loop::defer(
        [ph, buf = std::move(buffer)](qjs::JSEngine& e) {
            const uint8_t* p = reinterpret_cast<const uint8_t*>(buf.data());
            e.resolvePromiseBytes(ph, p, buf.size());
            e.freePromise(ph);
        });
}

} // namespace qianjs::fs::schedule
