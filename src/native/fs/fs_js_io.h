#pragma once

#include <quickjs.h>

#include <cstdint>
#include <vector>

/** Read JS string, ArrayBuffer, or TypedArray view into raw bytes (for writeFile). */
inline bool fs_read_js_bytes(JSContext* c, JSValue v, std::vector<uint8_t>& out) {
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
