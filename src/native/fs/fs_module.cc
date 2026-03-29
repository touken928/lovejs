#include "native/fs/fs_module.h"

#include "native/fs/fs_uv.h"

#include <js_engine.h>
#include <js_module.h>
#include <js_types.h>

#include <sys/stat.h>

#include <cctype>
#include <cstring>
#include <string>
#include <vector>

namespace {

std::string to_lower(std::string s) {
    for (char& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

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

struct ReadOpts {
    bool as_buffer = false;
    std::string flag = "r";
};

ReadOpts parse_read_options(JSContext* c, JSValue v) {
    ReadOpts o;
    if (JS_IsUndefined(v) || JS_IsNull(v))
        return o;
    if (JS_IsString(v)) {
        const char* s = JS_ToCString(c, v);
        if (s) {
            std::string low = to_lower(s);
            JS_FreeCString(c, s);
            if (low == "utf8" || low == "utf-8")
                o.as_buffer = false;
        }
        return o;
    }
    if (!JS_IsObject(v))
        return o;

    JSValue enc = JS_GetPropertyStr(c, v, "encoding");
    if (JS_IsNull(enc))
        o.as_buffer = true;
    else if (JS_IsString(enc)) {
        const char* s = JS_ToCString(c, enc);
        if (s) {
            std::string low = to_lower(s);
            if (low == "utf8" || low == "utf-8")
                o.as_buffer = false;
            else if (low == "buffer" || low == "binary")
                o.as_buffer = true;
            JS_FreeCString(c, s);
        }
    }
    JS_FreeValue(c, enc);

    JSValue fl = JS_GetPropertyStr(c, v, "flag");
    if (JS_IsString(fl)) {
        const char* s = JS_ToCString(c, fl);
        if (s) {
            o.flag = s;
            JS_FreeCString(c, s);
        }
    }
    JS_FreeValue(c, fl);
    return o;
}

struct WriteOpts {
    std::string flag = "w";
    int mode =
#ifdef _WIN32
        _S_IREAD | _S_IWRITE;
#else
        0644;
#endif
};

WriteOpts parse_write_options(JSContext* c, JSValue v, bool append_default) {
    WriteOpts o;
    if (append_default)
        o.flag = "a";
    if (JS_IsUndefined(v) || JS_IsNull(v))
        return o;
    if (!JS_IsObject(v))
        return o;

    JSValue fl = JS_GetPropertyStr(c, v, "flag");
    if (JS_IsString(fl)) {
        const char* s = JS_ToCString(c, fl);
        if (s) {
            o.flag = s;
            JS_FreeCString(c, s);
        }
    }
    JS_FreeValue(c, fl);

    JSValue m = JS_GetPropertyStr(c, v, "mode");
    if (JS_IsNumber(m)) {
        int32_t mm = 0;
        if (JS_ToInt32(c, &mm, m) == 0)
            o.mode = mm;
    }
    JS_FreeValue(c, m);
    return o;
}

struct MkdirOpts {
    bool recursive = false;
    int mode =
#ifdef _WIN32
        0777;
#else
        0777;
#endif
};

MkdirOpts parse_mkdir_options(JSContext* c, JSValue v) {
    MkdirOpts o;
    if (JS_IsUndefined(v) || JS_IsNull(v) || !JS_IsObject(v))
        return o;
    JSValue rec = JS_GetPropertyStr(c, v, "recursive");
    if (JS_IsBool(rec))
        o.recursive = JS_ToBool(c, rec);
    JS_FreeValue(c, rec);
    JSValue m = JS_GetPropertyStr(c, v, "mode");
    if (JS_IsNumber(m)) {
        int32_t mm = 0;
        if (JS_ToInt32(c, &mm, m) == 0)
            o.mode = mm;
    }
    JS_FreeValue(c, m);
    return o;
}

} // namespace

const char* FsPlugin::name() const {
    return "fs";
}

void FsPlugin::install(qjs::JSEngine& engine, qjs::JSModule& root) {
    qjs::JSEngine* eng = &engine;
    auto& m = root.module("fs");

    m.funcDynamic("readFile", 1, 2, [eng](JSContext* c, int argc, JSValue* argv) -> JSValue {
        bool ok = false;
        std::string path = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        ReadOpts ro;
        if (argc >= 2)
            ro = parse_read_options(c, argv[1]);
        qjs::RawJSValue r = fsReadFileAsync(*eng, std::move(path), ro.as_buffer, std::move(ro.flag));
        return qjs::JSConv<qjs::RawJSValue>::to(c, r);
    });

    m.funcDynamic("writeFile", 2, 3, [eng](JSContext* c, int argc, JSValue* argv) -> JSValue {
        bool ok = false;
        std::string path = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        std::vector<uint8_t> bytes;
        if (!read_js_bytes(c, argv[1], bytes))
            return JS_ThrowTypeError(c, "writeFile: data must be string, ArrayBuffer, or TypedArray");
        WriteOpts wo;
        if (argc >= 3)
            wo = parse_write_options(c, argv[2], false);
        qjs::RawJSValue r = fsWriteFileAsync(*eng, std::move(path), std::move(bytes), std::move(wo.flag), wo.mode);
        return qjs::JSConv<qjs::RawJSValue>::to(c, r);
    });

    m.funcDynamic("appendFile", 2, 3, [eng](JSContext* c, int argc, JSValue* argv) -> JSValue {
        bool ok = false;
        std::string path = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        std::vector<uint8_t> bytes;
        if (!read_js_bytes(c, argv[1], bytes))
            return JS_ThrowTypeError(c, "appendFile: data must be string, ArrayBuffer, or TypedArray");
        WriteOpts wo = parse_write_options(c, argc >= 3 ? argv[2] : JS_UNDEFINED, true);
        qjs::RawJSValue r = fsAppendFileAsync(*eng, std::move(path), std::move(bytes), wo.mode);
        return qjs::JSConv<qjs::RawJSValue>::to(c, r);
    });

    m.funcDynamic("mkdir", 1, 2, [eng](JSContext* c, int argc, JSValue* argv) -> JSValue {
        bool ok = false;
        std::string path = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        MkdirOpts mo;
        if (argc >= 2)
            mo = parse_mkdir_options(c, argv[1]);
        qjs::RawJSValue r = fsMkdirAsync(*eng, std::move(path), mo.recursive, mo.mode);
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
        qjs::RawJSValue r = fsStatAsync(*eng, std::move(path), true);
        return qjs::JSConv<qjs::RawJSValue>::to(c, r);
    });

    m.funcDynamic("lstat", 1, 1, [eng](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok = false;
        std::string path = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        qjs::RawJSValue r = fsStatAsync(*eng, std::move(path), false);
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

    m.funcDynamic("rename", 2, 2, [eng](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok1 = false, ok2 = false;
        std::string a = qjs::JSConv<std::string>::from(c, argv[0], ok1);
        std::string b = qjs::JSConv<std::string>::from(c, argv[1], ok2);
        if (!ok1 || !ok2)
            return JS_EXCEPTION;
        qjs::RawJSValue r = fsRenameAsync(*eng, std::move(a), std::move(b));
        return qjs::JSConv<qjs::RawJSValue>::to(c, r);
    });

    m.funcDynamic("copyFile", 2, 3, [eng](JSContext* c, int argc, JSValue* argv) -> JSValue {
        bool ok1 = false, ok2 = false;
        std::string src = qjs::JSConv<std::string>::from(c, argv[0], ok1);
        std::string dest = qjs::JSConv<std::string>::from(c, argv[1], ok2);
        if (!ok1 || !ok2)
            return JS_EXCEPTION;
        int flags = 0;
        if (argc >= 3) {
            int32_t f = 0;
            if (JS_ToInt32(c, &f, argv[2]) == 0)
                flags = f;
        }
        qjs::RawJSValue r = fsCopyFileAsync(*eng, std::move(src), std::move(dest), flags);
        return qjs::JSConv<qjs::RawJSValue>::to(c, r);
    });
}
