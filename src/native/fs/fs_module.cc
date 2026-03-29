#include "native/fs/fs_module.h"

#include "native/fs/fs_sync.h"
#include "native/fs/fs_uv.h"

#include <js_engine.h>
#include <js_module.h>
#include <js_types.h>

#include <string>
#include <vector>

namespace {

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

} // namespace

const char* FsPlugin::name() const {
    return "fs";
}

void FsPlugin::install(qjs::JSEngine& engine, qjs::JSModule& root) {
    qjs::JSEngine* eng = &engine;
    auto& m = root.module("fs");

    m.funcDynamic("readFile", 1, 1, [eng](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok = false;
        std::string path = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        qjs::RawJSValue r = fsReadFileAsync(*eng, std::move(path), false);
        return qjs::JSConv<qjs::RawJSValue>::to(c, r);
    });

    m.funcDynamic("readFileBytes", 1, 1, [eng](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok = false;
        std::string path = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        qjs::RawJSValue r = fsReadFileAsync(*eng, std::move(path), true);
        return qjs::JSConv<qjs::RawJSValue>::to(c, r);
    });

    m.funcDynamic("writeFile", 2, 2, [eng](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok = false;
        std::string path = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        std::vector<uint8_t> bytes;
        if (!read_js_bytes(c, argv[1], bytes))
            return JS_ThrowTypeError(c, "writeFile: data must be string, ArrayBuffer, or TypedArray");
        qjs::RawJSValue r = fsWriteFileAsync(*eng, std::move(path), std::move(bytes));
        return qjs::JSConv<qjs::RawJSValue>::to(c, r);
    });

    m.funcDynamic("mkdir", 1, 1, [eng](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok = false;
        std::string path = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        qjs::RawJSValue r = fsMkdirAsync(*eng, std::move(path), false);
        return qjs::JSConv<qjs::RawJSValue>::to(c, r);
    });

    m.funcDynamic("mkdirRecursive", 1, 1, [eng](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok = false;
        std::string path = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        qjs::RawJSValue r = fsMkdirAsync(*eng, std::move(path), true);
        return qjs::JSConv<qjs::RawJSValue>::to(c, r);
    });

    m.funcDynamic("readdir", 1, 1, [eng](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok = false;
        std::string path = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        qjs::RawJSValue r = fsReaddirAsync(*eng, std::move(path));
        return qjs::JSConv<qjs::RawJSValue>::to(c, r);
    });

    m.funcDynamic("stat", 1, 1, [eng](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok = false;
        std::string path = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        qjs::RawJSValue r = fsStatAsync(*eng, std::move(path));
        return qjs::JSConv<qjs::RawJSValue>::to(c, r);
    });

    m.funcDynamic("unlink", 1, 1, [eng](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok = false;
        std::string path = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        qjs::RawJSValue r = fsUnlinkAsync(*eng, std::move(path));
        return qjs::JSConv<qjs::RawJSValue>::to(c, r);
    });

    m.funcDynamic("rmdir", 1, 1, [eng](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok = false;
        std::string path = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        qjs::RawJSValue r = fsRmdirAsync(*eng, std::move(path));
        return qjs::JSConv<qjs::RawJSValue>::to(c, r);
    });

    install_fs_sync(m.module("sync"));
}
