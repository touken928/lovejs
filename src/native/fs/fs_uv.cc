#include "native/fs/fs_uv.h"

#include "runtime/event_loop/event_loop.h"

#include <uvw.hpp>

#include <climits>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

#include <sys/stat.h>

namespace {

void schedule_reject(qjs::JSEngine::PromiseHandle ph, std::string msg) {
    qianjs::event_loop::defer(
        [ph, msg = std::move(msg)](qjs::JSEngine& e) {
            e.rejectPromise(ph, msg);
            e.freePromise(ph);
        });
}

void schedule_resolve_string(qjs::JSEngine::PromiseHandle ph, std::string data) {
    qianjs::event_loop::defer(
        [ph, data = std::move(data)](qjs::JSEngine& e) {
            e.resolvePromise(ph, data);
            e.freePromise(ph);
        });
}

void schedule_resolve_void(qjs::JSEngine::PromiseHandle ph) {
    qianjs::event_loop::defer([ph](qjs::JSEngine& e) {
        e.resolvePromiseVoid(ph);
        e.freePromise(ph);
    });
}

static bool stat_is_dir(const uv_stat_t& st) {
#ifdef S_ISDIR
    return S_ISDIR(static_cast<unsigned>(st.st_mode));
#else
    return (st.st_mode & _S_IFMT) == _S_IFDIR;
#endif
}

struct FsReadCtx {
    qjs::JSEngine::PromiseHandle ph{};
    std::string buffer;
    std::string fail_on_close;
    /** Keep `file_req` alive until libuv finishes (uvw fs requests do not pin `self_ptr`). */
    std::shared_ptr<uvw::file_req> req_keep;
};

void defer_release_read_req(const std::shared_ptr<FsReadCtx>& ctx) {
    qianjs::event_loop::defer([ctx](qjs::JSEngine&) { ctx->req_keep.reset(); });
}

void read_done_fail(const std::shared_ptr<FsReadCtx>& ctx, std::string msg) {
    qianjs::event_loop::end_operation();
    schedule_reject(ctx->ph, std::move(msg));
    defer_release_read_req(ctx);
}

struct FsWriteCtx {
    qjs::JSEngine::PromiseHandle ph{};
    std::string data;
    std::shared_ptr<uvw::file_req> req_keep;
};

void defer_release_write_req(const std::shared_ptr<FsWriteCtx>& ctx) {
    qianjs::event_loop::defer([ctx](qjs::JSEngine&) { ctx->req_keep.reset(); });
}

void write_done_fail(const std::shared_ptr<FsWriteCtx>& ctx, std::string msg) {
    qianjs::event_loop::end_operation();
    schedule_reject(ctx->ph, std::move(msg));
    defer_release_write_req(ctx);
}

} // namespace

qjs::RawJSValue fsReadFileAsync(qjs::JSEngine& engine, std::string path) {
    qjs::JSEngine::PromiseHandle ph = engine.createPromise();
    if (!ph.ptr)
        return engine.promiseValue(ph);

    auto ctx = std::make_shared<FsReadCtx>();
    ctx->ph = ph;
    auto loop = qianjs::event_loop::uv::uvw_loop();
    auto req = loop->resource<uvw::file_req>();
    ctx->req_keep = req;

    using ft = uvw::file_req::fs_type;

    req->on<uvw::error_event>([ctx](const uvw::error_event& e, auto&) {
        read_done_fail(ctx, e.what());
    });

    req->on<uvw::fs_event>([ctx](const uvw::fs_event& ev, uvw::file_req& r) {
        switch (ev.type) {
        case ft::OPEN:
            r.stat();
            break;
        case ft::FSTAT: {
            const uint64_t sz64 = static_cast<uint64_t>(ev.stat.st_size);
            if (stat_is_dir(ev.stat)) {
                ctx->fail_on_close = "EISDIR: path is a directory";
                r.close();
                break;
            }
            if (sz64 == 0) {
                ctx->buffer.clear();
                r.close();
                break;
            }
            if (sz64 > static_cast<uint64_t>(SIZE_MAX)) {
                read_done_fail(ctx, "file too large");
                break;
            }
            const std::size_t sz = static_cast<std::size_t>(sz64);
            if (sz > static_cast<std::size_t>(UINT_MAX)) {
                read_done_fail(ctx, "file too large for single read");
                break;
            }
            r.read(0, static_cast<unsigned>(sz));
            break;
        }
        case ft::READ:
            if (ev.result > 0 && ev.read.data)
                ctx->buffer.assign(ev.read.data.get(), ev.read.data.get() + ev.result);
            else
                ctx->buffer.clear();
            r.close();
            break;
        case ft::CLOSE:
            if (!ctx->fail_on_close.empty()) {
                auto m = std::move(ctx->fail_on_close);
                read_done_fail(ctx, std::move(m));
            } else {
                qianjs::event_loop::end_operation();
                schedule_resolve_string(ctx->ph, std::move(ctx->buffer));
                defer_release_read_req(ctx);
            }
            break;
        default:
            break;
        }
    });

    qianjs::event_loop::begin_operation();
    req->open(path, uvw::file_req::file_open_flags::RDONLY, 0);
    return engine.promiseValue(ph);
}

qjs::RawJSValue fsWriteFileAsync(qjs::JSEngine& engine, std::string path, std::string data) {
    qjs::JSEngine::PromiseHandle ph = engine.createPromise();
    if (!ph.ptr)
        return engine.promiseValue(ph);

    auto ctx = std::make_shared<FsWriteCtx>();
    ctx->ph = ph;
    ctx->data = std::move(data);
    auto loop = qianjs::event_loop::uv::uvw_loop();
    auto req = loop->resource<uvw::file_req>();
    ctx->req_keep = req;

    using ft = uvw::file_req::fs_type;

    req->on<uvw::error_event>([ctx](const uvw::error_event& e, auto&) {
        write_done_fail(ctx, e.what());
    });

    req->on<uvw::fs_event>([ctx](const uvw::fs_event& ev, uvw::file_req& r) {
        switch (ev.type) {
        case ft::OPEN: {
            const unsigned len = static_cast<unsigned>(ctx->data.size());
            if (len == 0) {
                auto buf = std::make_unique<char[]>(1);
                r.write(std::move(buf), 0, 0);
            } else {
                auto buf = std::make_unique<char[]>(ctx->data.size());
                std::memcpy(buf.get(), ctx->data.data(), ctx->data.size());
                r.write(std::move(buf), len, 0);
            }
            break;
        }
        case ft::WRITE:
            r.close();
            break;
        case ft::CLOSE:
            qianjs::event_loop::end_operation();
            schedule_resolve_void(ctx->ph);
            defer_release_write_req(ctx);
            break;
        default:
            break;
        }
    });

#ifdef _WIN32
    const auto flags = uvw::file_req::file_open_flags::WRONLY | uvw::file_req::file_open_flags::CREAT
        | uvw::file_req::file_open_flags::TRUNC;
    const int mode = _S_IREAD | _S_IWRITE;
#else
    const auto flags = uvw::file_req::file_open_flags::WRONLY | uvw::file_req::file_open_flags::CREAT
        | uvw::file_req::file_open_flags::TRUNC;
    const int mode = 0644;
#endif

    qianjs::event_loop::begin_operation();
    req->open(path, flags, mode);
    return engine.promiseValue(ph);
}
