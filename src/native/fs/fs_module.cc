#include "native/fs/fs_module.h"

#include "native/fs/fs_js_io.h"
#include "native/fs/fs_sync.h"
#include "native/fs/fs_uv.h"

#include <js_engine.h>
#include <js_module.h>
#include <js_types.h>

#include <string>
#include <vector>

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
        if (!fs_read_js_bytes(c, argv[1], bytes))
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
