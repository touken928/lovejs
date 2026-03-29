#include "native/fs/fs_sync.h"

#include "native/fs/fs_stat_js.h"

#include "runtime/event_loop/event_loop.h"

#include <js_module.h>
#include <js_types.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <uv.h>

namespace {

namespace fs = std::filesystem;

bool read_js_bytes(JSContext* c, JSValue v, std::vector<uint8_t>& out) {
    if (JS_IsString(v)) {
        size_t plen = 0;
        const char* p = JS_ToCStringLen(c, &plen, v);
        if (!p)
            return false;
        out.assign(reinterpret_cast<const uint8_t*>(p), reinterpret_cast<const uint8_t*>(p) + plen);
        JS_FreeCString(c, p);
        return true;
    }

    size_t sz = 0;
    uint8_t* raw = JS_GetArrayBuffer(c, &sz, v);
    if (raw) {
        out.assign(raw, raw + sz);
        return true;
    }

    size_t boff = 0, blen = 0, bpe = 0;
    JSValue buf = JS_GetTypedArrayBuffer(c, v, &boff, &blen, &bpe);
    if (JS_IsException(buf))
        return false;
    size_t ablen = 0;
    uint8_t* base = JS_GetArrayBuffer(c, &ablen, buf);
    JS_FreeValue(c, buf);
    if (!base || boff + blen > ablen || boff > ablen)
        return false;
    out.assign(base + boff, base + boff + blen);
    return true;
}

JSValue throw_err(JSContext* c, const char* prefix, const std::error_code& ec) {
    std::string msg = prefix;
    msg += ec.message();
    return JS_ThrowTypeError(c, "%s", msg.c_str());
}

} // namespace

void install_fs_sync(qjs::JSModule& sync) {
    sync.funcDynamic("readFile", 1, 1, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok = false;
        std::string path = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        std::ifstream in(path, std::ios::binary);
        if (!in)
            return JS_ThrowTypeError(c, "readFile: cannot open file");
        std::ostringstream oss;
        oss << in.rdbuf();
        const std::string& s = oss.str();
        return JS_NewStringLen(c, s.data(), s.size());
    });

    sync.funcDynamic("readFileBytes", 1, 1, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok = false;
        std::string path = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        std::ifstream in(path, std::ios::binary);
        if (!in)
            return JS_ThrowTypeError(c, "readFileBytes: cannot open file");
        std::vector<char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        return JS_NewArrayBufferCopy(c, reinterpret_cast<const uint8_t*>(data.data()), data.size());
    });

    sync.funcDynamic("writeFile", 2, 2, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok = false;
        std::string path = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        std::vector<uint8_t> bytes;
        if (!read_js_bytes(c, argv[1], bytes))
            return JS_ThrowTypeError(c, "writeFile: data must be string, ArrayBuffer, or TypedArray");
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out)
            return JS_ThrowTypeError(c, "writeFile: cannot open file for write");
        if (!bytes.empty())
            out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!out)
            return JS_ThrowTypeError(c, "writeFile: write failed");
        return JS_UNDEFINED;
    });

    sync.funcDynamic("mkdir", 1, 1, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok = false;
        const std::string pathStr = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        std::error_code ec;
        fs::create_directory(fs::path(pathStr), ec);
        if (ec)
            return throw_err(c, "mkdir: ", ec);
        return JS_UNDEFINED;
    });

    sync.funcDynamic("mkdirRecursive", 1, 1, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok = false;
        const std::string pathStr = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        std::error_code ec;
        fs::create_directories(fs::path(pathStr), ec);
        if (ec)
            return throw_err(c, "mkdirRecursive: ", ec);
        return JS_UNDEFINED;
    });

    sync.funcDynamic("readdir", 1, 1, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok = false;
        const std::string pathStr = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        std::error_code ec;
        const fs::path p(pathStr);
        const fs::file_status st = fs::status(p, ec);
        if (ec)
            return throw_err(c, "readdir: ", ec);
        if (!fs::is_directory(st))
            return JS_ThrowTypeError(c, "readdir: not a directory");
        JSValue arr = JS_NewArray(c);
        if (JS_IsException(arr))
            return arr;
        uint32_t i = 0;
        for (const auto& ent : fs::directory_iterator(p, fs::directory_options::skip_permission_denied, ec)) {
            if (ec) {
                JS_FreeValue(c, arr);
                return throw_err(c, "readdir: ", ec);
            }
            JSValue s = JS_NewString(c, ent.path().filename().string().c_str());
            if (JS_IsException(s) || JS_SetPropertyUint32(c, arr, i, s) < 0) {
                JS_FreeValue(c, arr);
                return JS_EXCEPTION;
            }
            i++;
        }
        return arr;
    });

    sync.funcDynamic("stat", 1, 1, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok = false;
        std::string path = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        uv_fs_t req;
        uv_loop_t* lp = qianjs::event_loop::uv::loop();
        const int r = uv_fs_stat(lp, &req, path.c_str(), nullptr);
        if (r < 0) {
            std::string err = uv_strerror(r);
            uv_fs_req_cleanup(&req);
            return JS_ThrowTypeError(c, "%s", err.c_str());
        }
        JSValue o = fs_stat_to_js(c, req.statbuf);
        uv_fs_req_cleanup(&req);
        return o;
    });

    sync.funcDynamic("unlink", 1, 1, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok = false;
        const std::string pathStr = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        const fs::path p(pathStr);
        std::error_code ec;
        const auto sl = fs::symlink_status(p, ec);
        if (ec)
            return throw_err(c, "unlink: ", ec);
        if (fs::is_directory(sl) && !fs::is_symlink(sl))
            return JS_ThrowTypeError(c, "unlink: path is a directory");
        if (!fs::remove(p, ec))
            return throw_err(c, "unlink: ", ec);
        return JS_UNDEFINED;
    });

    sync.funcDynamic("rmdir", 1, 1, [](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok = false;
        const std::string pathStr = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        const fs::path p(pathStr);
        std::error_code ec;
        const fs::file_status st = fs::status(p, ec);
        if (ec)
            return throw_err(c, "rmdir: ", ec);
        if (!fs::is_directory(st))
            return JS_ThrowTypeError(c, "rmdir: not a directory");
        if (!fs::is_empty(p, ec))
            return throw_err(c, "rmdir: ", ec);
        if (!fs::remove(p, ec))
            return throw_err(c, "rmdir: ", ec);
        return JS_UNDEFINED;
    });
}
