#include "native/process/process_module.h"

#include "runtime/runtime_context.h"

#include <js_engine.h>
#include <js_module.h>
#include <js_types.h>

#include <string>
#include <utility>
#include <vector>

namespace {

JSValue argv_to_js(JSContext* c, const std::vector<std::string>& argv) {
    JSValue arr = JS_NewArray(c);
    if (JS_IsException(arr))
        return arr;
    for (uint32_t i = 0; i < argv.size(); i++) {
        JSValue s = JS_NewString(c, argv[i].c_str());
        if (JS_IsException(s) || JS_SetPropertyUint32(c, arr, i, s) < 0) {
            JS_FreeValue(c, arr);
            return JS_EXCEPTION;
        }
    }
    return arr;
}

JSValue env_to_js(JSContext* c, const std::vector<std::pair<std::string, std::string>>& env) {
    JSValue obj = JS_NewObject(c);
    if (JS_IsException(obj))
        return obj;
    for (const auto& kv : env) {
        JSValue v = JS_NewString(c, kv.second.c_str());
        if (JS_IsException(v) || JS_SetPropertyStr(c, obj, kv.first.c_str(), v) < 0) {
            JS_FreeValue(c, obj);
            return JS_EXCEPTION;
        }
    }
    return obj;
}

} // namespace

const char* ProcessPlugin::name() const {
    return "process";
}

void ProcessPlugin::install(qjs::JSEngine& engine, qjs::JSModule& root) {
    qianjs::RuntimeContext* runtime = engine.host<qianjs::RuntimeContext>();
    auto& m = root.module("process");

    m.funcDynamic("argv", 0, 0, [runtime](JSContext* c, int argc, JSValue* argv) -> JSValue {
        (void)argc;
        (void)argv;
        if (!runtime)
            return JS_NewArray(c);
        return argv_to_js(c, runtime->argv);
    });

    m.funcDynamic("env", 0, 1, [runtime](JSContext* c, int argc, JSValue* argv) -> JSValue {
        if (!runtime)
            return JS_UNDEFINED;
        if (argc == 0)
            return env_to_js(c, runtime->env);

        bool ok = false;
        std::string key = qjs::JSConv<std::string>::from(c, argv[0], ok);
        if (!ok)
            return JS_EXCEPTION;
        for (const auto& kv : runtime->env) {
            if (kv.first == key)
                return JS_NewString(c, kv.second.c_str());
        }
        return JS_UNDEFINED;
    });

    m.func("getExitCode", [runtime]() -> int {
        return runtime ? runtime->exit_code : 0;
    });

    m.func("setExitCode", [runtime](int code) {
        if (runtime)
            runtime->exit_code = code;
    });
}
