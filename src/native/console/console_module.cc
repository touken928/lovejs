#include "native/console/console_module.h"

#include <js_module.h>
#include <quickjs.h>

#include <iostream>
#include <string>

namespace {

/** Pretty line: `ToString` each argument, space-separated, newline. */
JSValue write_line(std::ostream& os, JSContext* c, int argc, JSValue* argv) {
    std::string out;
    out.reserve(static_cast<size_t>(argc) * 8u);
    for (int i = 0; i < argc; i++) {
        if (i > 0)
            out.push_back(' ');
        JSValue str = JS_ToString(c, argv[i]);
        if (JS_IsException(str))
            return JS_EXCEPTION;
        size_t len = 0;
        const char* p = JS_ToCStringLen(c, &len, str);
        JS_FreeValue(c, str);
        if (!p)
            return JS_EXCEPTION;
        out.append(p, len);
        JS_FreeCString(c, p);
    }
    os << out << '\n';
    return JS_UNDEFINED;
}

} // namespace

const char* ConsolePlugin::name() const {
    return "console";
}

void ConsolePlugin::install(qjs::JSEngine&, qjs::JSModule& root) {
    auto& c = root.module("console");

    c.funcDynamic("log", 0, 32, [](JSContext* ctx, int argc, JSValue* argv) -> JSValue {
        return write_line(std::cout, ctx, argc, argv);
    });
    c.funcDynamic("info", 0, 32, [](JSContext* ctx, int argc, JSValue* argv) -> JSValue {
        return write_line(std::cout, ctx, argc, argv);
    });
    c.funcDynamic("debug", 0, 32, [](JSContext* ctx, int argc, JSValue* argv) -> JSValue {
        return write_line(std::cout, ctx, argc, argv);
    });
    c.funcDynamic("warn", 0, 32, [](JSContext* ctx, int argc, JSValue* argv) -> JSValue {
        return write_line(std::cerr, ctx, argc, argv);
    });
    c.funcDynamic("error", 0, 32, [](JSContext* ctx, int argc, JSValue* argv) -> JSValue {
        return write_line(std::cerr, ctx, argc, argv);
    });
}
