#include "native/fs/fs_uv.h"

#include "runtime/event_loop/event_loop.h"

#include <uvw.hpp>

#include <climits>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <sys/stat.h>

namespace {

using file_flags = uvw::file_req::file_open_flags;

void schedule_reject(qjs::JSEngine::PromiseHandle ph, std::string msg, const std::string& code = {}) {
    qianjs::event_loop::defer(
        [ph, msg = std::move(msg), code](qjs::JSEngine& e) {
            if (code.empty())
                e.rejectPromise(ph, msg);
            else
                e.rejectPromise(ph, msg, code);
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

void schedule_resolve_bytes(qjs::JSEngine::PromiseHandle ph, std::string buffer) {
    qianjs::event_loop::defer(
        [ph, buf = std::move(buffer)](qjs::JSEngine& e) {
            const uint8_t* p = reinterpret_cast<const uint8_t*>(buf.data());
            e.resolvePromiseBytes(ph, p, buf.size());
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

file_flags parse_read_flag(std::string fl) {
    if (fl.empty())
        fl = "r";
    if (fl == "r+" || fl == "rs+" || fl == "sr+")
        return file_flags::RDWR;
    return file_flags::RDONLY;
}

file_flags parse_write_flag(std::string fl, bool append_default) {
    if (fl.empty())
        fl = append_default ? "a" : "w";
    if (fl == "a")
        return file_flags::WRONLY | file_flags::CREAT | file_flags::APPEND;
    if (fl == "ax")
        return file_flags::WRONLY | file_flags::CREAT | file_flags::APPEND | file_flags::EXCL;
    if (fl == "a+")
        return file_flags::RDWR | file_flags::CREAT | file_flags::APPEND;
    if (fl == "ax+")
        return file_flags::RDWR | file_flags::CREAT | file_flags::APPEND | file_flags::EXCL;
    if (fl == "w")
        return file_flags::WRONLY | file_flags::CREAT | file_flags::TRUNC;
    if (fl == "wx")
        return file_flags::WRONLY | file_flags::CREAT | file_flags::TRUNC | file_flags::EXCL;
    if (fl == "w+")
        return file_flags::RDWR | file_flags::CREAT | file_flags::TRUNC;
    if (fl == "wx+")
        return file_flags::RDWR | file_flags::CREAT | file_flags::TRUNC | file_flags::EXCL;
    if (fl == "r+")
        return file_flags::RDWR | file_flags::CREAT | file_flags::TRUNC;
    return file_flags::WRONLY | file_flags::CREAT | file_flags::TRUNC;
}

struct FsReadCtx {
    qjs::JSEngine::PromiseHandle ph{};
    std::string buffer;
    std::string fail_on_close;
    std::shared_ptr<uvw::file_req> req_keep;
    bool as_buffer = false;
};

void defer_release_read_req(const std::shared_ptr<FsReadCtx>& ctx) {
    qianjs::event_loop::defer([ctx](qjs::JSEngine&) { ctx->req_keep.reset(); });
}

void read_done_fail(const std::shared_ptr<FsReadCtx>& ctx, std::string msg) {
    qianjs::event_loop::end_operation();
    schedule_reject(ctx->ph, std::move(msg));
    defer_release_read_req(ctx);
}

void read_resolve_ok(const std::shared_ptr<FsReadCtx>& ctx) {
    qianjs::event_loop::end_operation();
    if (ctx->as_buffer)
        schedule_resolve_bytes(ctx->ph, std::move(ctx->buffer));
    else
        schedule_resolve_string(ctx->ph, std::move(ctx->buffer));
    defer_release_read_req(ctx);
}

struct FsWriteCtx {
    qjs::JSEngine::PromiseHandle ph{};
    std::vector<uint8_t> data;
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

qjs::RawJSValue fsReadFileAsync(qjs::JSEngine& engine, std::string path, bool asBuffer, std::string flag) {
    qjs::JSEngine::PromiseHandle ph = engine.createPromise();
    if (!ph.ptr)
        return engine.promiseValue(ph);

    auto ctx = std::make_shared<FsReadCtx>();
    ctx->ph = ph;
    ctx->as_buffer = asBuffer;
    auto loop = qianjs::event_loop::uv::uvw_loop();
    auto req = loop->resource<uvw::file_req>();
    ctx->req_keep = req;

    using ft = uvw::file_req::fs_type;
    const file_flags openFlags = parse_read_flag(std::move(flag));

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
                ctx->fail_on_close = "EISDIR: illegal operation on a directory";
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
                read_resolve_ok(ctx);
            }
            break;
        default:
            break;
        }
    });

    qianjs::event_loop::begin_operation();
    req->open(path, openFlags, 0);
    return engine.promiseValue(ph);
}

static qjs::RawJSValue write_file_impl(qjs::JSEngine& engine, std::string path, std::vector<uint8_t> data, file_flags flags,
    int mode) {
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

    qianjs::event_loop::begin_operation();
    req->open(path, flags, mode);
    return engine.promiseValue(ph);
}

qjs::RawJSValue fsWriteFileAsync(qjs::JSEngine& engine, std::string path, std::vector<uint8_t> data, std::string flag,
    int mode) {
    return write_file_impl(engine, std::move(path), std::move(data), parse_write_flag(std::move(flag), false), mode);
}

qjs::RawJSValue fsAppendFileAsync(qjs::JSEngine& engine, std::string path, std::vector<uint8_t> data, int mode) {
    return write_file_impl(engine, std::move(path), std::move(data), parse_write_flag("a", true), mode);
}

qjs::RawJSValue fsMkdirAsync(qjs::JSEngine& engine, std::string path, bool recursive, int mode) {
    qjs::JSEngine::PromiseHandle ph = engine.createPromise();
    if (!ph.ptr)
        return engine.promiseValue(ph);

    if (recursive) {
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path), ec);
        if (ec) {
            schedule_reject(ph, ec.message());
            return engine.promiseValue(ph);
        }
        schedule_resolve_void(ph);
        return engine.promiseValue(ph);
    }

    struct MkdirAsyncCtx {
        qjs::JSEngine::PromiseHandle ph{};
        std::shared_ptr<uvw::fs_req> req_keep;
    };
    auto ctx = std::make_shared<MkdirAsyncCtx>();
    ctx->ph = ph;
    auto loop = qianjs::event_loop::uv::uvw_loop();
    auto req = loop->resource<uvw::fs_req>();
    ctx->req_keep = req;

    using ft = uvw::fs_req::fs_type;

    req->on<uvw::error_event>([ctx](const uvw::error_event& e, auto&) {
        qianjs::event_loop::end_operation();
        schedule_reject(ctx->ph, e.what());
        qianjs::event_loop::defer([ctx](qjs::JSEngine&) { ctx->req_keep.reset(); });
    });

    req->on<uvw::fs_event>([ctx](const uvw::fs_event& ev, uvw::fs_req&) {
        if (ev.type == ft::MKDIR) {
            qianjs::event_loop::end_operation();
            schedule_resolve_void(ctx->ph);
            qianjs::event_loop::defer([ctx](qjs::JSEngine&) { ctx->req_keep.reset(); });
        }
    });

    qianjs::event_loop::begin_operation();
    req->mkdir(path, mode);
    return engine.promiseValue(ph);
}
