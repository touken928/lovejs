#include "native/net/net_module.h"

#include "native/net/net_uv.h"

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

const char* NetPlugin::name() const {
    return "net";
}

void NetPlugin::install(qjs::JSEngine& engine, qjs::JSModule& root) {
    qjs::JSEngine* eng = &engine;
    auto& m = root.module("net");

    m.func("connect", [eng](int port, const std::string& host) -> qjs::RawJSValue {
        return netConnectAsync(*eng, port, host);
    });

    m.funcDynamic("write", 2, 2, [eng](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        bool ok = false;
        const int64_t sock = qjs::JSConv<int64_t>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        std::vector<uint8_t> bytes;
        if (!read_js_bytes(c, argv[1], bytes))
            return JS_ThrowTypeError(c, "write: data must be string, ArrayBuffer, or TypedArray");
        qjs::RawJSValue r = netWriteAsync(*eng, sock, std::move(bytes));
        return qjs::JSConv<qjs::RawJSValue>::to(c, r);
    });

    m.func("read", [eng](int64_t sock) -> qjs::RawJSValue { return netReadAsync(*eng, sock); });

    m.func("readBytes", [eng](int64_t sock) -> qjs::RawJSValue { return netReadBytesAsync(*eng, sock); });

    m.func("close", [](int64_t sock) { netClose(sock); });
}
