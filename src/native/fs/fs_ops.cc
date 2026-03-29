#include "native/fs/fs_uv.h"

#include "runtime/event_loop/event_loop.h"

#include <uvw.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <sys/stat.h>

namespace {

void schedule_reject(qjs::JSEngine::PromiseHandle ph, std::string msg) {
    qianjs::event_loop::defer(
        [ph, msg = std::move(msg)](qjs::JSEngine& e) {
            e.rejectPromise(ph, msg);
            e.freePromise(ph);
        });
}

void schedule_resolve_void(qjs::JSEngine::PromiseHandle ph) {
    qianjs::event_loop::defer([ph](qjs::JSEngine& e) {
        e.resolvePromiseVoid(ph);
        e.freePromise(ph);
    });
}

void schedule_resolve_string_array(qjs::JSEngine::PromiseHandle ph, std::vector<std::string> names) {
    qianjs::event_loop::defer(
        [ph, names = std::move(names)](qjs::JSEngine& e) {
            JSContext* c = e.ctx();
            JSValue arr = JS_NewArray(c);
            if (JS_IsException(arr)) {
                e.rejectPromise(ph, "failed to allocate array");
                e.freePromise(ph);
                return;
            }
            for (uint32_t i = 0; i < names.size(); i++) {
                JSValue s = JS_NewString(c, names[i].c_str());
                if (JS_IsException(s) || JS_SetPropertyUint32(c, arr, i, s) < 0) {
                    JS_FreeValue(c, arr);
                    e.rejectPromise(ph, "failed to build readdir result");
                    e.freePromise(ph);
                    return;
                }
            }
            e.resolvePromiseJSValue(ph, arr);
            e.freePromise(ph);
        });
}

static int64_t timespec_to_ms(const uv_timespec_t& t) {
    return static_cast<int64_t>(t.tv_sec) * 1000 + static_cast<int64_t>(t.tv_nsec) / 1000000;
}

static JSValue build_stat_object(JSContext* c, const uv_stat_t& st) {
    JSValue o = JS_NewObject(c);
    if (JS_IsException(o))
        return o;

    auto def = [&](const char* name, JSValue v) {
        if (JS_IsException(v) || JS_DefinePropertyValueStr(c, o, name, v, JS_PROP_C_W_E) < 0) {
            JS_FreeValue(c, o);
            return false;
        }
        return true;
    };

#ifndef S_IFMT
#define S_IFMT 0170000
#endif
#ifndef S_IFREG
#define S_IFREG 0100000
#endif
#ifndef S_IFDIR
#define S_IFDIR 0040000
#endif
#ifndef S_IFLNK
#define S_IFLNK 0120000
#endif
#ifndef S_IFCHR
#define S_IFCHR 0020000
#endif
#ifndef S_IFBLK
#define S_IFBLK 0060000
#endif
#ifndef S_IFIFO
#define S_IFIFO 0010000
#endif
#ifndef S_IFSOCK
#define S_IFSOCK 0140000
#endif

    const uint64_t mode = st.st_mode;
    const bool is_file = (mode & S_IFMT) == S_IFREG;
    const bool is_dir = (mode & S_IFMT) == S_IFDIR;
    const bool is_symlink = (mode & S_IFMT) == S_IFLNK;
    const bool is_chr = (mode & S_IFMT) == S_IFCHR;
    const bool is_blk = (mode & S_IFMT) == S_IFBLK;
    const bool is_fifo = (mode & S_IFMT) == S_IFIFO;
    const bool is_sock = (mode & S_IFMT) == S_IFSOCK;

    if (!def("dev", JS_NewInt64(c, static_cast<int64_t>(st.st_dev)))
        || !def("ino", JS_NewInt64(c, static_cast<int64_t>(st.st_ino)))
        || !def("mode", JS_NewInt64(c, static_cast<int64_t>(st.st_mode)))
        || !def("nlink", JS_NewInt64(c, static_cast<int64_t>(st.st_nlink)))
        || !def("uid", JS_NewInt64(c, static_cast<int64_t>(st.st_uid)))
        || !def("gid", JS_NewInt64(c, static_cast<int64_t>(st.st_gid)))
        || !def("rdev", JS_NewInt64(c, static_cast<int64_t>(st.st_rdev)))
        || !def("size", JS_NewInt64(c, static_cast<int64_t>(st.st_size)))
        || !def("blksize", JS_NewInt64(c, static_cast<int64_t>(st.st_blksize)))
        || !def("blocks", JS_NewInt64(c, static_cast<int64_t>(st.st_blocks)))
        || !def("atimeMs", JS_NewFloat64(c, static_cast<double>(timespec_to_ms(st.st_atim))))
        || !def("mtimeMs", JS_NewFloat64(c, static_cast<double>(timespec_to_ms(st.st_mtim))))
        || !def("ctimeMs", JS_NewFloat64(c, static_cast<double>(timespec_to_ms(st.st_ctim))))
        || !def("birthtimeMs", JS_NewFloat64(c, static_cast<double>(timespec_to_ms(st.st_birthtim))))
        || !def("isFile", JS_NewBool(c, is_file))
        || !def("isDirectory", JS_NewBool(c, is_dir))
        || !def("isSymbolicLink", JS_NewBool(c, is_symlink))
        || !def("isCharacterDevice", JS_NewBool(c, is_chr))
        || !def("isBlockDevice", JS_NewBool(c, is_blk))
        || !def("isFIFO", JS_NewBool(c, is_fifo))
        || !def("isSocket", JS_NewBool(c, is_sock))) {
        return JS_EXCEPTION;
    }

    return o;
}

void schedule_resolve_stat(qjs::JSEngine::PromiseHandle ph, const uv_stat_t& st) {
    qianjs::event_loop::defer([ph, st](qjs::JSEngine& e) {
        JSContext* c = e.ctx();
        JSValue o = build_stat_object(c, st);
        if (JS_IsException(o)) {
            e.rejectPromise(ph, "failed to build stat object");
            e.freePromise(ph);
            return;
        }
        e.resolvePromiseJSValue(ph, o);
        e.freePromise(ph);
    });
}

} // namespace

qjs::RawJSValue fsReaddirAsync(qjs::JSEngine& engine, std::string path) {
    qjs::JSEngine::PromiseHandle ph = engine.createPromise();
    if (!ph.ptr)
        return engine.promiseValue(ph);

    struct Ctx {
        qjs::JSEngine::PromiseHandle ph{};
        std::shared_ptr<uvw::fs_req> req_keep;
        std::vector<std::string> names;
    };
    auto ctx = std::make_shared<Ctx>();
    ctx->ph = ph;
    auto loop = qianjs::event_loop::uv::uvw_loop();
    ctx->req_keep = loop->resource<uvw::fs_req>();

    using ft = uvw::fs_req::fs_type;
    auto req = ctx->req_keep;

    req->on<uvw::error_event>([ctx](const uvw::error_event& e, auto&) {
        qianjs::event_loop::end_operation();
        schedule_reject(ctx->ph, e.what());
        qianjs::event_loop::defer([ctx](qjs::JSEngine&) { ctx->req_keep.reset(); });
    });

    req->on<uvw::fs_event>([ctx](const uvw::fs_event& ev, uvw::fs_req& r) {
        switch (ev.type) {
        case ft::OPENDIR:
            r.readdir();
            break;
        case ft::READDIR:
            if (ev.dirent.eos) {
                r.closedir();
            } else if (ev.dirent.name) {
                ctx->names.emplace_back(ev.dirent.name);
                r.readdir();
            } else {
                r.closedir();
            }
            break;
        case ft::CLOSEDIR:
            qianjs::event_loop::end_operation();
            schedule_resolve_string_array(ctx->ph, std::move(ctx->names));
            qianjs::event_loop::defer([ctx](qjs::JSEngine&) { ctx->req_keep.reset(); });
            break;
        default:
            break;
        }
    });

    qianjs::event_loop::begin_operation();
    req->opendir(path);
    return engine.promiseValue(ph);
}

qjs::RawJSValue fsStatAsync(qjs::JSEngine& engine, std::string path, bool followSymlink) {
    qjs::JSEngine::PromiseHandle ph = engine.createPromise();
    if (!ph.ptr)
        return engine.promiseValue(ph);

    struct Ctx {
        qjs::JSEngine::PromiseHandle ph{};
        std::shared_ptr<uvw::fs_req> req_keep;
    };
    auto ctx = std::make_shared<Ctx>();
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
        if (ev.type == ft::STAT || ev.type == ft::LSTAT) {
            qianjs::event_loop::end_operation();
            schedule_resolve_stat(ctx->ph, ev.stat);
            qianjs::event_loop::defer([ctx](qjs::JSEngine&) { ctx->req_keep.reset(); });
        }
    });

    qianjs::event_loop::begin_operation();
    if (followSymlink)
        req->stat(path);
    else
        req->lstat(path);
    return engine.promiseValue(ph);
}

static qjs::RawJSValue fs_one_path_void(qjs::JSEngine& engine, std::string path,
    void (*start)(uvw::fs_req&, const std::string&), uvw::fs_req::fs_type doneType) {
    qjs::JSEngine::PromiseHandle ph = engine.createPromise();
    if (!ph.ptr)
        return engine.promiseValue(ph);

    struct Ctx {
        qjs::JSEngine::PromiseHandle ph{};
        std::shared_ptr<uvw::fs_req> req_keep;
    };
    auto ctx = std::make_shared<Ctx>();
    ctx->ph = ph;
    auto loop = qianjs::event_loop::uv::uvw_loop();
    auto req = loop->resource<uvw::fs_req>();
    ctx->req_keep = req;

    req->on<uvw::error_event>([ctx](const uvw::error_event& e, auto&) {
        qianjs::event_loop::end_operation();
        schedule_reject(ctx->ph, e.what());
        qianjs::event_loop::defer([ctx](qjs::JSEngine&) { ctx->req_keep.reset(); });
    });

    req->on<uvw::fs_event>([ctx, doneType](const uvw::fs_event& ev, uvw::fs_req&) {
        if (ev.type == doneType) {
            qianjs::event_loop::end_operation();
            schedule_resolve_void(ctx->ph);
            qianjs::event_loop::defer([ctx](qjs::JSEngine&) { ctx->req_keep.reset(); });
        }
    });

    qianjs::event_loop::begin_operation();
    start(*req, path);
    return engine.promiseValue(ph);
}

qjs::RawJSValue fsUnlinkAsync(qjs::JSEngine& engine, std::string path) {
    return fs_one_path_void(
        engine, std::move(path), [](uvw::fs_req& r, const std::string& p) { r.unlink(p); }, uvw::fs_req::fs_type::UNLINK);
}

qjs::RawJSValue fsRmdirAsync(qjs::JSEngine& engine, std::string path) {
    return fs_one_path_void(
        engine, std::move(path), [](uvw::fs_req& r, const std::string& p) { r.rmdir(p); }, uvw::fs_req::fs_type::RMDIR);
}

qjs::RawJSValue fsRenameAsync(qjs::JSEngine& engine, std::string oldPath, std::string newPath) {
    qjs::JSEngine::PromiseHandle ph = engine.createPromise();
    if (!ph.ptr)
        return engine.promiseValue(ph);

    struct Ctx {
        qjs::JSEngine::PromiseHandle ph{};
        std::shared_ptr<uvw::fs_req> req_keep;
    };
    auto ctx = std::make_shared<Ctx>();
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
        if (ev.type == ft::RENAME) {
            qianjs::event_loop::end_operation();
            schedule_resolve_void(ctx->ph);
            qianjs::event_loop::defer([ctx](qjs::JSEngine&) { ctx->req_keep.reset(); });
        }
    });

    qianjs::event_loop::begin_operation();
    req->rename(oldPath, newPath);
    return engine.promiseValue(ph);
}

qjs::RawJSValue fsCopyFileAsync(qjs::JSEngine& engine, std::string src, std::string dest, int copyFlags) {
    qjs::JSEngine::PromiseHandle ph = engine.createPromise();
    if (!ph.ptr)
        return engine.promiseValue(ph);

    struct Ctx {
        qjs::JSEngine::PromiseHandle ph{};
        std::shared_ptr<uvw::fs_req> req_keep;
    };
    auto ctx = std::make_shared<Ctx>();
    ctx->ph = ph;
    auto loop = qianjs::event_loop::uv::uvw_loop();
    auto req = loop->resource<uvw::fs_req>();
    ctx->req_keep = req;

    using ft = uvw::fs_req::fs_type;
    const auto flags = static_cast<uvw::fs_req::copy_file_flags>(copyFlags);

    req->on<uvw::error_event>([ctx](const uvw::error_event& e, auto&) {
        qianjs::event_loop::end_operation();
        schedule_reject(ctx->ph, e.what());
        qianjs::event_loop::defer([ctx](qjs::JSEngine&) { ctx->req_keep.reset(); });
    });

    req->on<uvw::fs_event>([ctx](const uvw::fs_event& ev, uvw::fs_req&) {
        if (ev.type == ft::COPYFILE) {
            qianjs::event_loop::end_operation();
            schedule_resolve_void(ctx->ph);
            qianjs::event_loop::defer([ctx](qjs::JSEngine&) { ctx->req_keep.reset(); });
        }
    });

    qianjs::event_loop::begin_operation();
    req->copyfile(src, dest, flags);
    return engine.promiseValue(ph);
}
