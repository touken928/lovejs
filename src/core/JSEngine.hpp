/**
 * @file JSEngine.hpp
 * @brief 轻量级 QuickJS 引擎封装，支持 C++/JS 双向绑定
 * 
 * 特性:
 *   - 链式 API 注册函数、值和模块
 *   - 自动类型转换 (int, int64_t, double, float, bool, string, vector<T>)
 *   - 支持 ES6 模块执行模式
 *   - 支持嵌套子模块
 *   - 使用 QuickJS 官方 JSClassID + opaque 机制管理生命周期
 * 
 * 生命周期约束:
 *   - 本引擎设计为"进程级单例，一次性使用"模式
 *   - initialize() 只能调用一次，cleanup() 后不可重新初始化
 *   - 模块注册必须在首次 JS 代码执行前完成
 *   - cleanup() 后所有 JS 调用将被拒绝
 *   - 不支持运行时卸载/重装模块
 * 
 * 类型安全:
 *   - JSConv<T> 仅支持显式特化的类型
 *   - 不支持 const char* (生命周期不安全)，请使用 std::string
 *   - 数组长度限制为 1e6，防止 OOM
 *   - 函数参数数量严格校验
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
        if (!JS_IsBool(v)) {
            ok = false;
            return false;
        }
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

// 删除 const char* 特化 - 生命周期不安全，统一使用 std::string

template<> struct JSConv<float> {
    static float from(JSContext* c, JSValue v, bool& ok) {
        double r;
        if (JS_ToFloat64(c, &r, v) < 0) { ok = false; return 0; }
        ok = true;
        return static_cast<float>(r);
    }
    static JSValue to(JSContext* c, float v) { return JS_NewFloat64(c, v); }
};

// vector<T> <-> JS Array 转换
template<typename T>
struct JSConv<std::vector<T>> {
    static constexpr int64_t MAX_ARRAY_LENGTH = 1000000;  // 1e6 上限
    
    static std::vector<T> from(JSContext* c, JSValue v, bool& ok) {
        if (!JS_IsArray(c, v)) {
            ok = false;
            JS_ThrowTypeError(c, "expected array");
            return {};
        }
        JSValue lenVal = JS_GetPropertyStr(c, v, "length");
        int64_t len = 0;
        if (JS_ToInt64(c, &len, lenVal) < 0) {
            JS_FreeValue(c, lenVal);
            ok = false;
            return {};
        }
        JS_FreeValue(c, lenVal);
        
        // 检查数组长度上限
        if (len > MAX_ARRAY_LENGTH) {
            ok = false;
            JS_ThrowRangeError(c, "array length %lld exceeds maximum %lld", 
                             (long long)len, (long long)MAX_ARRAY_LENGTH);
            return {};
        }
        
        std::vector<T> result;
        result.reserve(len);
        for (int64_t i = 0; i < len; i++) {
            JSValue elem = JS_GetPropertyUint32(c, v, i);
            bool elemOk;
            T val = JSConv<T>::from(c, elem, elemOk);
            JS_FreeValue(c, elem);
            if (!elemOk) {
                ok = false;
                JS_ThrowTypeError(c, "array element at index %lld conversion failed", (long long)i);
                return {};  // 返回空 vector，避免部分构造
            }
            result.push_back(std::move(val));
        }
        ok = true;
        return result;
    }
    static JSValue to(JSContext* c, const std::vector<T>& v) {
        JSValue arr = JS_NewArray(c);
        if (JS_IsException(arr)) {
            return arr;  // 传播异常
        }
        for (size_t i = 0; i < v.size(); i++) {
            JSValue elem = JSConv<T>::to(c, v[i]);
            if (JS_IsException(elem)) {
                JS_FreeValue(c, arr);
                return elem;  // 传播异常
            }
            if (JS_SetPropertyUint32(c, arr, i, elem) < 0) {
                JS_FreeValue(c, arr);
                return JS_EXCEPTION;
            }
        }
        return arr;
    }
};

template<typename T> using decay_t = std::remove_cv_t<std::remove_reference_t<T>>;

// 辅助模板用于更好的 static_assert 错误信息
template<typename> struct dependent_false : std::false_type {};

// 未特化类型的编译期错误提示
template<typename T>
struct JSConv {
    static_assert(dependent_false<T>::value, 
        "JSConv<T> not specialized for this type. Supported types: int, int64_t, double, float, bool, std::string, std::vector<T>");
};

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
        if (argc != expected) {
            return JS_ThrowTypeError(c, "expected exactly %d arguments, got %d", expected, argc);
        }
        
        bool ok;
        auto args = unpackArgs<Args...>(c, argv, ok);
        if (!ok) {
            // 类型转换失败，需要抛出明确异常
            if (!JS_IsException(JS_GetException(c))) {
                return JS_ThrowTypeError(c, "argument type conversion failed");
            }
            return JS_EXCEPTION;
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
        if (!child) child = std::make_unique<JSModule>(name, this);
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
    std::unordered_map<std::string, std::unique_ptr<JSModule>> children_;
    std::unordered_map<std::string, std::unique_ptr<FuncBase>> funcs_;
    std::unordered_map<std::string, std::function<JSValue(JSContext*)>> values_;
    
public:
    JSModule(const std::string& name, JSModule* parent)
        : name_(name), parent_(parent) {}
    
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
    JSEngine() = delete;  // 禁止实例化
    JSEngine(const JSEngine&) = delete;
    JSEngine& operator=(const JSEngine&) = delete;
    
    /**
     * 初始化 JS 引擎
     */
    static void initialize() {
        if (rt_) return;  // 已经初始化
        
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
        
        // 设置 runtime opaque 为 nullptr（静态方案不需要 this）
        JS_SetRuntimeOpaque(rt_, nullptr);
        
        // 设置模块加载器
        JS_SetModuleLoaderFunc(rt_, nullptr, &JSEngine::loadModule, nullptr);
    }
    
    /**
     * 获取全局模块
     */
    static JSModule& global() { 
        if (!global_) {
            global_ = std::make_unique<JSModule>("global", nullptr);
        }
        return *global_; 
    }
    
    /**
     * 检查是否已安装
     */
    static bool isInstalled() {
        return installed_;
    }
    
    /**
     * 显式清理资源（在程序退出前调用）
     */
    static void cleanup() {
        if (!ctx_ && !rt_) return; // 已经清理过了
        
        cleanedUp_ = true;  // 设置清理标志
        
        // 清理模块映射
        modData_.clear();
        jsModuleCache_.clear();
        
        // 运行垃圾回收
        if (ctx_ && rt_) {
            JS_RunGC(rt_);
        }
        
        // 释放上下文和运行时
        if (ctx_) {
            JS_FreeContext(ctx_);
            ctx_ = nullptr;
        }
        if (rt_) {
            JS_FreeRuntime(rt_);
            rt_ = nullptr;
        }
        
        // 清理全局模块
        global_.reset();
    }
    
    /**
     * 设置错误回调（用于集成日志系统）
     */
    static void setErrorCallback(std::function<void(const std::string&)> callback) {
        errorCallback_ = std::move(callback);
    }
    
    /**
     * 调用全局 JS 函数（支持任意类型参数）
     * 使用示例：
     *   callGlobal("update", 0.016);
     *   callGlobal("keypressed", "space");
     *   callGlobal("mousepressed", 100, 200, 1);
     */
    template<typename... Args>
    static bool callGlobal(const char* name, Args... args) {
        if (cleanedUp_ || !ctx_) {
            fprintf(stderr, "Error: JSEngine has been cleaned up, cannot call JS functions\n");
            return false;
        }
        
        JSValue global = JS_GetGlobalObject(ctx_);
        JSValue func = JS_GetPropertyStr(ctx_, global, name);
        
        bool success = false;
        if (JS_IsFunction(ctx_, func)) {
            // 转换参数为 JSValue 数组
            constexpr size_t argc = sizeof...(Args);
            
            if constexpr (argc > 0) {
                JSValue argv[argc];
                convertArgs(argv, args...);
                
                // 调用函数
                JSValue result = JS_Call(ctx_, func, global, argc, argv);
                if (JS_IsException(result)) {
                    dumpError();
                } else {
                    success = true;
                }
                JS_FreeValue(ctx_, result);
                
                // 释放参数
                freeArgs(argv, argc);
            } else {
                // 零参数情况
                JSValue result = JS_Call(ctx_, func, global, 0, nullptr);
                if (JS_IsException(result)) {
                    dumpError();
                } else {
                    success = true;
                }
                JS_FreeValue(ctx_, result);
            }
        }
        
        JS_FreeValue(ctx_, func);
        JS_FreeValue(ctx_, global);
        return success;
    }
    
    static bool runFile(const std::string& path) {
        if (cleanedUp_ || !ctx_) {
            fprintf(stderr, "Error: JSEngine has been cleaned up\n");
            return false;
        }
        std::ifstream f(path);
        if (!f) { fprintf(stderr, "Cannot open: %s\n", path.c_str()); return false; }
        std::stringstream buf; buf << f.rdbuf();
        return eval(buf.str(), path);
    }
    
    // 运行内联代码作为模块
    static bool runFile(const std::string& name, const std::string& code) {
        if (cleanedUp_ || !ctx_) {
            fprintf(stderr, "Error: JSEngine has been cleaned up\n");
            return false;
        }
        return eval(code, name);
    }
    
    /**
     * 运行字节码
     * @param buf 字节码数据
     * @param bufLen 字节码长度
     * @return 成功返回 true
     */
    static bool runBytecode(const uint8_t* buf, size_t bufLen) {
        if (cleanedUp_ || !ctx_) {
            fprintf(stderr, "Error: JSEngine has been cleaned up\n");
            return false;
        }
        
        ensureInstalled();
        
        // 读取字节码对象
        JSValue obj = JS_ReadObject(ctx_, buf, bufLen, JS_READ_OBJ_BYTECODE);
        if (JS_IsException(obj)) {
            fprintf(stderr, "Error: Failed to read bytecode\n");
            dumpError();
            return false;
        }
        
        // 检查是否是模块
        bool isModule = (JS_VALUE_GET_TAG(obj) == JS_TAG_MODULE);
        JSModuleDef* moduleDef = nullptr;
        
        if (isModule) {
            // 保存模块定义指针
            moduleDef = (JSModuleDef*)JS_VALUE_GET_PTR(obj);
            
            // 解析模块依赖
            if (JS_ResolveModule(ctx_, obj) < 0) {
                fprintf(stderr, "Error: Failed to resolve module\n");
                dumpError();
                return false;
            }
        }
        
        // 执行字节码（注意：JS_EvalFunction 会消费 obj）
        JSValue result = JS_EvalFunction(ctx_, obj);
        if (JS_IsException(result)) {
            fprintf(stderr, "Error: Failed to evaluate bytecode\n");
            dumpError();
            return false;
        }
        JS_FreeValue(ctx_, result);
        
        // 如果是模块，获取命名空间并注册导出到全局
        if (isModule && moduleDef) {
            JSValue moduleNS = JS_GetModuleNamespace(ctx_, moduleDef);
            if (!JS_IsException(moduleNS)) {
                registerModuleExportsToGlobal(moduleNS);
                JS_FreeValue(ctx_, moduleNS);
            }
        }
        
        return true;
    }
    
    /**
     * 编译结果
     */
    struct CompileResult {
        bool success = false;
        std::string error;
        std::vector<uint8_t> bytecode;
    };
    
    /**
     * 编译 JS 代码为字节码（不执行）
     * @param code JS 源代码
     * @param filename 文件名（用于错误信息）
     * @return 编译结果，包含字节码或错误信息
     */
    static CompileResult compile(const std::string& code, const std::string& filename) {
        CompileResult result;
        
        // 创建独立的运行时用于编译
        JSRuntime* rt = JS_NewRuntime();
        if (!rt) {
            result.error = "Failed to create JS runtime";
            return result;
        }
        
        JSContext* ctx = JS_NewContext(rt);
        if (!ctx) {
            result.error = "Failed to create JS context";
            JS_FreeRuntime(rt);
            return result;
        }
        
        // 设置模块加载器（用于处理依赖）
        JS_SetModuleLoaderFunc(rt, nullptr, compileModuleLoader, nullptr);
        
        // 编译为字节码（不执行）
        JSValue obj = JS_Eval(ctx, code.c_str(), code.size(), filename.c_str(),
                              JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
        
        if (JS_IsException(obj)) {
            JSValue ex = JS_GetException(ctx);
            const char* msg = JS_ToCString(ctx, ex);
            result.error = msg ? msg : "Unknown compile error";
            if (msg) JS_FreeCString(ctx, msg);
            JS_FreeValue(ctx, ex);
            JS_FreeContext(ctx);
            JS_FreeRuntime(rt);
            return result;
        }
        
        // 序列化字节码
        size_t outSize = 0;
        uint8_t* outBuf = JS_WriteObject(ctx, &outSize, obj, JS_WRITE_OBJ_BYTECODE);
        JS_FreeValue(ctx, obj);
        
        if (!outBuf) {
            result.error = "Failed to serialize bytecode";
            JS_FreeContext(ctx);
            JS_FreeRuntime(rt);
            return result;
        }
        
        // 复制字节码到 vector
        result.bytecode.assign(outBuf, outBuf + outSize);
        result.success = true;
        
        js_free(ctx, outBuf);
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
        
        return result;
    }

private:
    inline static JSRuntime* rt_ = nullptr;
    inline static JSContext* ctx_ = nullptr;
    inline static std::unique_ptr<JSModule> global_;
    inline static bool installed_ = false;
    inline static std::unordered_map<JSModuleDef*, JSModule*> modData_;
    inline static std::unordered_map<std::string, JSModuleDef*> jsModuleCache_;
    inline static bool cleanedUp_ = false;
    inline static std::function<void(const std::string&)> errorCallback_;
    
    /**
     * 编译时模块加载器 - 为原生模块创建 stub
     */
    static JSModuleDef* compileModuleLoader(JSContext* ctx, const char* moduleName, void* opaque) {
        (void)opaque;
        
        // 尝试加载 JS 文件
        std::string path = moduleName;
        if (path.find(".js") == std::string::npos) path += ".js";
        
        std::ifstream f(path);
        if (f) {
            std::stringstream buf;
            buf << f.rdbuf();
            std::string code = buf.str();
            
            JSValue compiled = JS_Eval(ctx, code.c_str(), code.size(), moduleName,
                JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
            if (!JS_IsException(compiled)) {
                return (JSModuleDef*)JS_VALUE_GET_PTR(compiled);
            }
        }
        
        // 对于原生模块，创建空的 stub 模块
        JSModuleDef* m = JS_NewCModule(ctx, moduleName, [](JSContext*, JSModuleDef*) -> int {
            return 0;
        });
        
        if (!m) {
            JS_ThrowReferenceError(ctx, "could not load module '%s'", moduleName);
            return nullptr;
        }
        
        // 为 graphics 模块添加导出声明
        if (strcmp(moduleName, "graphics") == 0) {
            const char* exports[] = {
                "setWindow", "clear", "present", "setColor", 
                "circle", "rectangle", "line", "print",
                "getWidth", "getHeight"
            };
            for (const char* name : exports) {
                JS_AddModuleExport(ctx, m, name);
            }
        }
        
        return m;
    }
    
    //=========================================================================
    // 参数转换辅助函数
    //=========================================================================
    
    // 递归转换参数（统一使用 JSConv）
    template<typename T, typename... Rest>
    static void convertArgs(JSValue* argv, T first, Rest... rest) {
        argv[0] = JSConv<decay_t<T>>::to(ctx_, first);
        if constexpr (sizeof...(Rest) > 0) {
            convertArgs(argv + 1, rest...);
        }
    }
    
    // 释放参数数组
    static void freeArgs(JSValue* argv, size_t count) {
        for (size_t i = 0; i < count; i++) {
            JS_FreeValue(ctx_, argv[i]);
        }
    }
    
    //=========================================================================
    // 原有的私有方法
    //=========================================================================
    
    static bool eval(const std::string& code, const std::string& file = "<eval>") {
        ensureInstalled();
        JSValue r = JS_Eval(ctx_, code.c_str(), code.size(), file.c_str(), JS_EVAL_TYPE_MODULE);
        if (JS_IsException(r)) { dumpError(); JS_FreeValue(ctx_, r); return false; }
        JS_FreeValue(ctx_, r);
        return true;
    }
    
    static void ensureInstalled() {
        if (installed_) return;
        installed_ = true;
        
        JSValue g = JS_GetGlobalObject(ctx_);
        installToObject(g, *global_);
        JS_FreeValue(ctx_, g);
    }

    static void installToObject(JSValue obj, JSModule& mod) {
        for (auto& [name, wrapper] : mod.funcs_) {
            JSValue fn = createJSFunction(wrapper.get());
            if (JS_IsException(fn) || JS_SetPropertyStr(ctx_, obj, name.c_str(), fn) < 0) {
                fprintf(stderr, "Error: Failed to install function '%s'\n", name.c_str());
            }
        }
        for (auto& [name, creator] : mod.values_) {
            JSValue val = creator(ctx_);
            if (JS_IsException(val) || JS_SetPropertyStr(ctx_, obj, name.c_str(), val) < 0) {
                fprintf(stderr, "Error: Failed to install value '%s'\n", name.c_str());
            }
        }
        // 递归安装子模块
        for (auto& [name, child] : mod.children_) {
            JSValue childObj = JS_NewObject(ctx_);
            if (JS_IsException(childObj)) {
                fprintf(stderr, "Error: Failed to create child module object '%s'\n", name.c_str());
                continue;
            }
            installChildModule(ctx_, childObj, *child);
            if (JS_SetPropertyStr(ctx_, obj, name.c_str(), childObj) < 0) {
                fprintf(stderr, "Error: Failed to install child module '%s'\n", name.c_str());
            }
        }
    }
    
    // 使用 JSClassID + opaque 创建函数，避免 reinterpret_cast<int64_t>
    static JSValue createJSFunction(FuncBase* wrapper) {
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
        (void)opaque;  // 静态方案不使用 opaque
        
        // 检查是否已清理
        if (cleanedUp_) {
            JS_ThrowInternalError(ctx, "JSEngine has been cleaned up");
            return nullptr;
        }
        
        // 查找 C++ 模块
        JSModule* mod = findModule(name);
        if (mod) return createCppModule(ctx, name, mod);
        
        // 检查 JS 模块缓存
        std::string modulePath = name;
        auto cacheIt = jsModuleCache_.find(modulePath);
        if (cacheIt != jsModuleCache_.end()) {
            return cacheIt->second;
        }
        
        // 加载 JS 文件
        std::string path = modulePath;
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
        
        JSModuleDef* moduleDef = (JSModuleDef*)JS_VALUE_GET_PTR(compiled);
        // 注意：不能 FreeValue，因为 moduleDef 需要保持有效
        // compiled 的生命周期由 QuickJS 内部管理
        jsModuleCache_[modulePath] = moduleDef;  // 缓存模块
        return moduleDef;
    }
    
    static JSModule* findModule(const std::string& name) {
        if (!global_) return nullptr;
        auto it = global_->children_.find(name);
        return it != global_->children_.end() ? it->second.get() : nullptr;
    }
    
    static JSModuleDef* createCppModule(JSContext* ctx, const char* name, JSModule* mod) {
        JSModuleDef* m = JS_NewCModule(ctx, name, &JSEngine::initModuleStatic);
        if (!m) return nullptr;
        
        modData_[m] = mod;
        
        for (auto& [n, _] : mod->funcs_) JS_AddModuleExport(ctx, m, n.c_str());
        for (auto& [n, _] : mod->values_) JS_AddModuleExport(ctx, m, n.c_str());
        for (auto& [n, _] : mod->children_) JS_AddModuleExport(ctx, m, n.c_str());
        
        return m;
    }

    static int initModuleStatic(JSContext* ctx, JSModuleDef* m) {
        return initModule(ctx, m);
    }
    
    static int initModule(JSContext* ctx, JSModuleDef* m) {
        auto it = modData_.find(m);
        if (it == modData_.end()) return -1;
        JSModule* mod = it->second;
        
        // 导出函数
        for (auto& [name, wrapper] : mod->funcs_) {
            JSValue fn = createJSFunction(wrapper.get());
            if (JS_IsException(fn) || JS_SetModuleExport(ctx, m, name.c_str(), fn) < 0) {
                fprintf(stderr, "Error: Failed to export function '%s'\n", name.c_str());
                return -1;
            }
        }
        // 导出值
        for (auto& [name, creator] : mod->values_) {
            JSValue val = creator(ctx);
            if (JS_IsException(val) || JS_SetModuleExport(ctx, m, name.c_str(), val) < 0) {
                fprintf(stderr, "Error: Failed to export value '%s'\n", name.c_str());
                return -1;
            }
        }
        // 导出子模块为对象
        for (auto& [name, child] : mod->children_) {
            JSValue obj = JS_NewObject(ctx);
            if (JS_IsException(obj)) {
                fprintf(stderr, "Error: Failed to create child module object '%s'\n", name.c_str());
                return -1;
            }
            installChildModule(ctx, obj, *child);
            if (JS_SetModuleExport(ctx, m, name.c_str(), obj) < 0) {
                fprintf(stderr, "Error: Failed to export child module '%s'\n", name.c_str());
                return -1;
            }
        }
        return 0;
    }
    
    static void installChildModule(JSContext* ctx, JSValue obj, JSModule& mod) {
        for (auto& [name, wrapper] : mod.funcs_) {
            JSValue fn = createJSFunction(wrapper.get());
            if (JS_IsException(fn) || JS_SetPropertyStr(ctx, obj, name.c_str(), fn) < 0) {
                fprintf(stderr, "Error: Failed to install child function '%s'\n", name.c_str());
            }
        }
        for (auto& [name, creator] : mod.values_) {
            JSValue val = creator(ctx);
            if (JS_IsException(val) || JS_SetPropertyStr(ctx, obj, name.c_str(), val) < 0) {
                fprintf(stderr, "Error: Failed to install child value '%s'\n", name.c_str());
            }
        }
        for (auto& [name, child] : mod.children_) {
            JSValue childObj = JS_NewObject(ctx);
            if (JS_IsException(childObj)) {
                fprintf(stderr, "Error: Failed to create nested child module '%s'\n", name.c_str());
                continue;
            }
            installChildModule(ctx, childObj, *child);
            if (JS_SetPropertyStr(ctx, obj, name.c_str(), childObj) < 0) {
                fprintf(stderr, "Error: Failed to install nested child module '%s'\n", name.c_str());
            }
        }
    }
    
    static void dumpError() {
        JSValue ex = JS_GetException(ctx_);
        std::string errorMsg;
        
        // 优先获取 message 属性
        JSValue msgVal = JS_GetPropertyStr(ctx_, ex, "message");
        if (!JS_IsUndefined(msgVal) && !JS_IsException(msgVal)) {
            const char* msg = JS_ToCString(ctx_, msgVal);
            if (msg) {
                errorMsg = std::string("Error: ") + msg;
                JS_FreeCString(ctx_, msg);
            }
        }
        JS_FreeValue(ctx_, msgVal);
        
        // fallback 到直接转字符串
        if (errorMsg.empty()) {
            const char* s = JS_ToCString(ctx_, ex);
            if (s) { 
                errorMsg = std::string("Error: ") + s;
                JS_FreeCString(ctx_, s); 
            } else {
                errorMsg = "Error: [unable to convert exception to string]";
            }
        }
        
        JSValue stack = JS_GetPropertyStr(ctx_, ex, "stack");
        if (!JS_IsUndefined(stack) && !JS_IsException(stack)) {
            const char* st = JS_ToCString(ctx_, stack);
            if (st) { 
                errorMsg += std::string("\n") + st;
                JS_FreeCString(ctx_, st); 
            }
        }
        JS_FreeValue(ctx_, stack);
        JS_FreeValue(ctx_, ex);
        
        // 使用回调或默认输出
        if (errorCallback_) {
            errorCallback_(errorMsg);
        } else {
            fprintf(stderr, "%s\n", errorMsg.c_str());
        }
    }
    
    /**
     * 将模块导出的函数注册到全局对象
     * 动态遍历模块命名空间的所有导出
     */
    static void registerModuleExportsToGlobal(JSValue moduleNS) {
        if (JS_IsUndefined(moduleNS) || JS_IsException(moduleNS)) {
            return;
        }
        
        JSValue global = JS_GetGlobalObject(ctx_);
        
        // 获取模块命名空间的所有属性名
        JSPropertyEnum* props = nullptr;
        uint32_t propCount = 0;
        
        if (JS_GetOwnPropertyNames(ctx_, &props, &propCount, moduleNS, 
                                   JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) == 0) {
            for (uint32_t i = 0; i < propCount; i++) {
                JSValue val = JS_GetProperty(ctx_, moduleNS, props[i].atom);
                if (!JS_IsException(val)) {
                    // 将导出绑定到全局对象
                    JS_SetProperty(ctx_, global, props[i].atom, JS_DupValue(ctx_, val));
                }
                JS_FreeValue(ctx_, val);
            }
            JS_FreePropertyEnum(ctx_, props, propCount);
        }
        
        JS_FreeValue(ctx_, global);
    }
};
