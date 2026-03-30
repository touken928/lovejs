#include "native/console/console_module.h"

#include <js_module.h>

#include <iostream>
#include <string>

namespace {

std::string join_args(JSContext* c, int argc, JSValue* argv, bool& ok) {
    ok = true;
    std::string out;
    for (int i = 0; i < argc; i++) {
        if (i > 0)
            out.push_back(' ');
        const char* s = JS_ToCString(c, argv[i]);
        if (!s) {
            ok = false;
            return {};
        }
        out += s;
        JS_FreeCString(c, s);
    }
    return out;
}

void write_line(std::ostream& os, JSContext* c, int argc, JSValue* argv) {
    bool ok = false;
    std::string msg = join_args(c, argc, argv, ok);
    if (!ok)
        return;
    os << msg << '\n';
}

} // namespace

const char* ConsolePlugin::name() const {
    return "console";
}

void ConsolePlugin::install(qjs::JSEngine&, qjs::JSModule& root) {
    auto& c = root.module("console");

    c.funcDynamic("log", 0, 32, [](JSContext* ctx, int argc, JSValue* argv) -> JSValue {
        write_line(std::cout, ctx, argc, argv);
        return JS_UNDEFINED;
    });
    c.funcDynamic("info", 0, 32, [](JSContext* ctx, int argc, JSValue* argv) -> JSValue {
        write_line(std::cout, ctx, argc, argv);
        return JS_UNDEFINED;
    });
    c.funcDynamic("debug", 0, 32, [](JSContext* ctx, int argc, JSValue* argv) -> JSValue {
        write_line(std::cout, ctx, argc, argv);
        return JS_UNDEFINED;
    });
    c.funcDynamic("warn", 0, 32, [](JSContext* ctx, int argc, JSValue* argv) -> JSValue {
        write_line(std::cerr, ctx, argc, argv);
        return JS_UNDEFINED;
    });
    c.funcDynamic("error", 0, 32, [](JSContext* ctx, int argc, JSValue* argv) -> JSValue {
        write_line(std::cerr, ctx, argc, argv);
        return JS_UNDEFINED;
    });
}
