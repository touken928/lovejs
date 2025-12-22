/**
 * @file JSEngine.hpp
 * @brief 轻量级 QuickJS 引擎封装，支持 C++/JS 双向绑定
 * 
 * 特性:
 *   - 链式 API 注册函数、值和模块
 *   - 自动类型转换 (int, int64_t, double, bool, string, vector<T>)
 *   - 支持 ES6 模块和脚本两种执行模式
 *   - 支持嵌套子模块
 *   - 使用 QuickJS 官方 JSClassID + opaque 机制管理生命周期
 */

#pragma once
#include <quickjs.h>
#include <string>
#include <functional>
#include <vector>
#include <unordered_map>
#include <type_traits>
#include <tuple>
#include <fstream>
#include <sstream>
#include <memory>
#include <stdexcept>

//=============================================================================
// 类型转换器 - C++ <-> JSValue 自动转换（带错误处理）
//=============================================================================

template<typename T> struct JSConv;

template<> struct JSConv<int> {
    static int from(JSContext* c, JSValue v, bool& ok) {
        int32_t r;
        if (JS_ToInt32(c, &r, v) < 0) { ok = false; return 0; }
        ok = true;
        return r;
    }
    static JSValue to(JSContext* c, int v) { return JS_NewInt32(c, v); }
};

template<> struct JSConv<int64_t> {
    static int64_t from(JSContext* c, JSValue v, bool& ok) {
        int64_t r;
        if (JS_ToInt64(c, &r, v) < 0) { ok = false; return 0; }
        ok = true;
        return r;
    }
    static JSValue to(JSContext* c, int64_t v) { return JS_NewInt64(c, v); }
};

template<> struct JSConv<double> {
    static double from(JSContext* c, JSValue v, bool& ok) {
        double r;
        if (JS_ToFloat64(c, &r, v) < 0) { ok = false; return 0; }
        ok = true;
        return r;
    }
    static JSValue to(JSContext* c, double v) { return JS_NewFloat64(c, v); }
};

template<> struct JSConv<bool> {
    static bool from(JSContext* c, JSValue v, bool& ok) {
        ok = true;
        return JS_ToBool(c, v);
    }
    static JSValue to(JSContext* c, bool v) { return JS_NewBool(c, v); }
};

template<> struct JSConv<std::string> {
    static std::string from(JSContext* c, JSValue v, bool& ok) {
        const char* s = JS_ToCString(c, v);
        if (!s) { ok = false; return ""; }
        std::string r = s;
        JS_FreeCString(c, s);
        ok = true;
        return r;
    }
    static JSValue to(JSContext* c, const std::string& v) { return JS_NewString(c, v.c_str()); }
};

// vector<T> <-> JS Array 转换
template<typename T>
struct JSConv<std::vector<T>> {
    static std::vector<T> from(JSContext* c, JSValue v, bool& ok) {
        std::vector<T> result;
        if (!JS_IsArray(c, v)) {
            ok = false;
            JS_ThrowTypeError(c, "expected array");
            return result;
        }
        JSValue lenVal = JS_GetPropertyStr(c, v, "length");
        int64_t len = 0;
        if (JS_ToInt64(c, &len, lenVal) < 0) {
            JS_FreeValue(c, lenVal);
            ok = false;
            return result;
        }
        JS_FreeValue(c, lenVal);
        result.reserve(len);
        for (int64_t i = 0; i < len; i++) {
            JSValue elem = JS_GetPropertyUint32(c, v, i);
            bool elemOk;
            T val = JSConv<T>::from(c, elem, elemOk);
            JS_FreeValue(c, elem);
            if (!elemOk) { ok = false; return result; }
            result.push_back(std::move(val));
        }
        ok = true;
        return result;
    }
    static JSValue to(JSContext* c, const std::vector<T>& v) {
        JSValue arr = JS_NewArray(c);
        for (size_t i = 0; i < v.size(); i++) {
            JS_SetPropertyUint32(c, arr, i, JSConv<T>::to(c, v[i]));
        }
        return arr;
    }
};

template<typename T> using decay_t = std::remove_cv_t<std::remove_reference_t<T>>;

//=============================================================================
// 函数包装器 - 将 C++ 函数包装为 JS 可调用（使用 JSClassID + opaque）
//=============================================================================

struct FuncBase {
    virtual ~FuncBase() = default;
    virtual JSValue call(JSContext*, int, JSValue*) = 0;
    virtual int arity() const = 0;  // 期望的参数数量
};

// 参数解包辅助
template<typename Tuple, size_t... I>
bool unpack_impl(JSContext* c, JSValue* v, Tuple& result, std::index_sequence<I...>) {
    bool ok = true;
    // 使用折叠表达式逐个解包
    ((ok = ok && [&]() {
        bool elemOk;
        std::get<I>(result) = JSConv<decay_t<std::tuple_element_t<I, Tuple>>>::from(c, v[I], elemOk);
        return elemOk;
    }()), ...);
    return ok;
}

template<typename... Args>
std::tuple<decay_t<Args>...> unpackArgs(JSContext* c, JSValue* v, bool& ok) {
    std::tuple<decay_t<Args>...> result{};
    ok = unpack_impl(c, v, result, std::index_sequence_for<Args...>{});
    return result;
}

template<typename Ret, typename... Args>
struct FuncWrap : FuncBase {
    std::function<Ret(Args...)> fn;
    FuncWrap(std::function<Ret(Args...)> f) : fn(std::move(f)) {}
    
    int arity() const override { return sizeof...(Args); }
    
    JSValue call(JSContext* c, int argc, JSValue* argv) override {
        // 参数数量校验
        constexpr int expected = sizeof...(Args);
        if (argc < expected) {
            return JS_ThrowTypeError(c, "expected %d arguments, got %d", expected, argc);
        }
        
        bool ok;
        auto args = unpackArgs<Args...>(c, argv, ok);
        if (!ok) {
            return JS_EXCEPTION;  // 类型转换已抛出异常
        }
        
        if constexpr (std::is_void_v<Ret>) {
            std::apply(fn, args);
            return JS_UNDEFINED;
        } else {
            return JSConv<Ret>::to(c, std::apply(fn, args));
        }
    }
};

class JSEngine;

//=============================================================================
// JSModule - 模块节点，支持链式注册函数、值和子模块
//=============================================================================

class JSModule {
    friend class JSEngine;
public:
    JSModule& module(const std::string& name) {
        auto& child = children_[name];
        if (!child) child = std::make_unique<JSModule>(name, this, engine_);
        return *child;
    }
    
    template<typename F>
    JSModule& func(const std::string& name, F&& f) {
        addFunc(name, std::forward<F>(f), &std::decay_t<F>::operator());
        return *this;
    }
    
    template<typename Ret, typename... Args>
    JSModule& func(const std::string& name, Ret(*f)(Args...)) {
        funcs_[name] = std::make_unique<FuncWrap<Ret, Args...>>(
            std::function<Ret(Args...)>(f));
        return *this;
    }
    
    template<typename T>
    JSModule& value(const std::string& name, T v) {
        values_[name] = [v](JSContext* c) { return JSConv<decay_t<T>>::to(c, v); };
        return *this;
    }

    const std::string& name() const { return name_; }

private:
    std::string name_;
    JSModule* parent_ = nullptr;
    JSEngine* engine_ = nullptr;
    std::unordered_map<std::string, std::unique_ptr<JSModule>> children_;
    std::unordered_map<std::string, std::unique_ptr<FuncBase>> funcs_;
    std::unordered_map<std::string, std::function<JSValue(JSContext*)>> values_;
    
public:
    JSModule(const std::string& name, JSModule* parent, JSEngine* engine)
        : name_(name), parent_(parent), engine_(engine) {}
    
private:
    template<typename F, typename Ret, typename C, typename... Args>
    void addFunc(const std::string& n, F&& f, Ret(C::*)(Args...) const) {
        funcs_[n] = std::make_unique<FuncWrap<Ret, Args...>>(
            std::function<Ret(Args...)>(std::forward<F>(f)));
    }
    template<typename F, typename Ret, typename C, typename... Args>
    void addFunc(const std::string& n, F&& f, Ret(C::*)(Args...)) {
        funcs_[n] = std::make_unique<FuncWrap<Ret, Args...>>(
            std::function<Ret(Args...)>(std::forward<F>(f)));
    }
};

//=============================================================================
// JSEngine - JS 引擎，管理运行时和模块（非单例，支持多实例）
//=============================================================================

// 全局 ClassID，所有引擎实例共享（QuickJS 要求 ClassID 全局唯一）
inline JSClassID g_funcClassId = 0;

class JSEngine {
public:
    JSEngine() : global_("global", nullptr, this) {
        rt_ = JS_NewRuntime();
        ctx_ = JS_NewContext(rt_);
        
        // 注册函数包装器的 JSClass（只注册一次）
        if (g_funcClassId == 0) {
            JS_NewClassID(&g_funcClassId);
        }
        JSClassDef classDef = {};
        classDef.class_name = "CppFunc";
        classDef.finalizer = nullptr;  // C++ 侧管理生命周期，JS 不释放
        JS_NewClass(rt_, g_funcClassId, &classDef);
        
        // 设置 runtime opaque 为 this，用于模块初始化回调
        JS_SetRuntimeOpaque(rt_, this);
        
        // 设置模块加载器，传递 this 指针
        JS_SetModuleLoaderFunc(rt_, nullptr, &JSEngine::loadModule, this);
    }
    
    ~JSEngine() {
        JS_FreeContext(ctx_);
        JS_FreeRuntime(rt_);
    }
    
    // 禁止拷贝
    JSEngine(const JSEngine&) = delete;
    JSEngine& operator=(const JSEngine&) = delete;
    
    /// 获取单例（兼容旧 API）
    static JSEngine& instance() {
        static JSEngine inst;
        return inst;
    }
    
    JSModule& global() { return global_; }
    
    JSContext* getContext() { return ctx_; }
    
    bool runFile(const std::string& path) {
        std::ifstream f(path);
        if (!f) { fprintf(stderr, "Cannot open: %s\n", path.c_str()); return false; }
        std::stringstream buf; buf << f.rdbuf();
        return eval(buf.str(), path);
    }
    
    // 运行内联代码作为模块
    bool runFile(const std::string& name, const std::string& code) {
        return eval(code, name);
    }
    
    bool runString(const std::string& code, const std::string& name = "<string>") {
        return eval(code, name);
    }
    
    bool runScript(const std::string& code, const std::string& name = "<script>") {
        ensureInstalled();
        JSValue r = JS_Eval(ctx_, code.c_str(), code.size(), name.c_str(), JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(r)) { dumpError(); JS_FreeValue(ctx_, r); return false; }
        JS_FreeValue(ctx_, r);
        return true;
    }

private:
    JSRuntime* rt_;
    JSContext* ctx_;
    JSModule global_;
    bool installed_ = false;  // 确保只安装一次
    std::unordered_map<JSModuleDef*, JSModule*> modData_;  // 实例级模块映射
    
    bool eval(const std::string& code, const std::string& file = "<eval>") {
        ensureInstalled();
        JSValue r = JS_Eval(ctx_, code.c_str(), code.size(), file.c_str(), JS_EVAL_TYPE_MODULE);
        if (JS_IsException(r)) { dumpError(); JS_FreeValue(ctx_, r); return false; }
        JS_FreeValue(ctx_, r);
        return true;
    }
    
    void ensureInstalled() {
        if (installed_) return;
        installed_ = true;
        
        JSValue g = JS_GetGlobalObject(ctx_);
        installToObject(g, global_);
        JS_FreeValue(ctx_, g);
    }

    void installToObject(JSValue obj, JSModule& mod) {
        for (auto& [name, wrapper] : mod.funcs_) {
            JSValue fn = createJSFunction(wrapper.get());
            JS_SetPropertyStr(ctx_, obj, name.c_str(), fn);
        }
        for (auto& [name, creator] : mod.values_) {
            JS_SetPropertyStr(ctx_, obj, name.c_str(), creator(ctx_));
        }
    }
    
    // 使用 JSClassID + opaque 创建函数，避免 reinterpret_cast<int64_t>
    JSValue createJSFunction(FuncBase* wrapper) {
        // 创建一个带 opaque 的对象来持有 FuncBase 指针
        JSValue funcData = JS_NewObjectClass(ctx_, g_funcClassId);
        JS_SetOpaque(funcData, wrapper);
        
        // 创建 C 函数，将 funcData 作为 data 传入
        JSValue fn = JS_NewCFunctionData(ctx_, &JSEngine::callFunc, 0, 0, 1, &funcData);
        JS_FreeValue(ctx_, funcData);  // fn 已持有引用
        return fn;
    }
    
    static JSValue callFunc(JSContext* c, JSValue thisVal, int argc, JSValue* argv, 
                            int magic, JSValue* data) {
        (void)thisVal; (void)magic;
        FuncBase* wrapper = static_cast<FuncBase*>(JS_GetOpaque(data[0], g_funcClassId));
        if (!wrapper) {
            return JS_ThrowInternalError(c, "invalid function wrapper");
        }
        return wrapper->call(c, argc, argv);
    }
    
    static JSModuleDef* loadModule(JSContext* ctx, const char* name, void* opaque) {
        auto* self = static_cast<JSEngine*>(opaque);
        
        // 查找 C++ 模块
        JSModule* mod = self->findModule(name);
        if (mod) return self->createCppModule(ctx, name, mod);
        
        // 加载 JS 文件
        std::string path = name;
        if (path.find(".js") == std::string::npos) path += ".js";
        std::ifstream f(path);
        if (!f) { 
            JS_ThrowReferenceError(ctx, "module not found: %s", name);
            return nullptr; 
        }
        
        std::stringstream buf; buf << f.rdbuf();
        std::string code = buf.str();
        
        JSValue compiled = JS_Eval(ctx, code.c_str(), code.size(), name,
            JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
        if (JS_IsException(compiled)) {
            return nullptr;
        }
        return (JSModuleDef*)JS_VALUE_GET_PTR(compiled);
    }
    
    JSModule* findModule(const std::string& name) {
        auto it = global_.children_.find(name);
        return it != global_.children_.end() ? it->second.get() : nullptr;
    }
    
    JSModuleDef* createCppModule(JSContext* ctx, const char* name, JSModule* mod) {
        JSModuleDef* m = JS_NewCModule(ctx, name, &JSEngine::initModuleStatic);
        if (!m) return nullptr;
        
        modData_[m] = mod;  // 实例级映射
        
        for (auto& [n, _] : mod->funcs_) JS_AddModuleExport(ctx, m, n.c_str());
        for (auto& [n, _] : mod->values_) JS_AddModuleExport(ctx, m, n.c_str());
        for (auto& [n, _] : mod->children_) JS_AddModuleExport(ctx, m, n.c_str());
        
        return m;
    }

    static int initModuleStatic(JSContext* ctx, JSModuleDef* m) {
        // 通过 runtime opaque 获取 engine 实例
        JSRuntime* rt = JS_GetRuntime(ctx);
        void* opaque = JS_GetRuntimeOpaque(rt);
        if (!opaque) {
            // fallback: 使用单例
            return JSEngine::instance().initModule(ctx, m);
        }
        return static_cast<JSEngine*>(opaque)->initModule(ctx, m);
    }
    
    int initModule(JSContext* ctx, JSModuleDef* m) {
        auto it = modData_.find(m);
        if (it == modData_.end()) return -1;
        JSModule* mod = it->second;
        
        // 导出函数
        for (auto& [name, wrapper] : mod->funcs_) {
            JSValue fn = createJSFunction(wrapper.get());
            JS_SetModuleExport(ctx, m, name.c_str(), fn);
        }
        // 导出值
        for (auto& [name, creator] : mod->values_) {
            JS_SetModuleExport(ctx, m, name.c_str(), creator(ctx));
        }
        // 导出子模块为对象
        for (auto& [name, child] : mod->children_) {
            JSValue obj = JS_NewObject(ctx);
            installChildModule(ctx, obj, *child);
            JS_SetModuleExport(ctx, m, name.c_str(), obj);
        }
        return 0;
    }
    
    void installChildModule(JSContext* ctx, JSValue obj, JSModule& mod) {
        for (auto& [name, wrapper] : mod.funcs_) {
            JSValue fn = createJSFunction(wrapper.get());
            JS_SetPropertyStr(ctx, obj, name.c_str(), fn);
        }
        for (auto& [name, creator] : mod.values_) {
            JS_SetPropertyStr(ctx, obj, name.c_str(), creator(ctx));
        }
        for (auto& [name, child] : mod.children_) {
            JSValue childObj = JS_NewObject(ctx);
            installChildModule(ctx, childObj, *child);
            JS_SetPropertyStr(ctx, obj, name.c_str(), childObj);
        }
    }
    
    void dumpError() {
        JSValue ex = JS_GetException(ctx_);
        const char* s = JS_ToCString(ctx_, ex);
        if (s) { fprintf(stderr, "Error: %s\n", s); JS_FreeCString(ctx_, s); }
        JSValue stack = JS_GetPropertyStr(ctx_, ex, "stack");
        if (!JS_IsUndefined(stack)) {
            const char* st = JS_ToCString(ctx_, stack);
            if (st) { fprintf(stderr, "%s\n", st); JS_FreeCString(ctx_, st); }
        }
        JS_FreeValue(ctx_, stack);
        JS_FreeValue(ctx_, ex);
    }
};
