# QianJS

基于 **QuickJS** 的轻量 **JavaScript 运行时**与命令行工具，通过 C++ 封装 **[qjs](https://github.com/touken928/qjs)**（`third_party/qjs`）接入引擎。支持 ES 模块、可选的字节码编译/运行、将字节码嵌入拷贝后的可执行文件，以及一组 **native 内置模块**（`console`、`fs`、`net`）。

---

## 特性

- **`qianjs run`** — 运行 ES 模块（`.js`）或字节码（`.qbc`）
- **`qianjs build`** — 将 JS 编译为 `./dist/<name>.qbc`
- **`qianjs embed`** — 把字节码附加到本 CLI 的副本，生成独立二进制
- **内置模块** — `console`；`fs` / `fs.sync`；`net`（TCP 客户端）。均可在配置阶段单独关闭
- **`qjs::qjs`** — 引擎以普通 CMake 静态库形式提供，其他工程可直接链接（无需构建 CLI）
- **平台** — macOS、Linux、Windows（**不支持 MSVC**；Windows 请用 MinGW-w64 或 Clang）

---

## 快速体验

```javascript
// main.js
import { log } from 'console';

log('Hello, QianJS!');
```

```bash
qianjs run main.js
```

---

## 构建

### 工具链

| 要求 | 说明 |
|------|------|
| CMake | 3.16 及以上 |
| C++ 编译器 | C++17；Windows 上**不要**用 MSVC |

### 克隆与子模块

```bash
git clone --recurse-submodules https://github.com/touken928/qianjs.git
cd qianjs
```

若克隆时未拉子模块：

```bash
git submodule update --init --recursive
```

| 子模块 | 作用 |
|--------|------|
| `third_party/qjs` | C++ 引擎 API 与 CMake；通过 **FetchContent** 拉取 **QuickJS**（首次配置需联网） |
| `third_party/googletest` | 仅用于顶层 **QianJS** 测试（`QIANJS_BUILD_TESTS`） |
| `third_party/libuv` | 事件循环 / 异步 I/O（开启 CLI 时始终配置相关 target） |
| `third_party/uvw` | 基于 libuv 的 C++ 头文件封装 |

### 配置与编译

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
```

可执行文件一般在 **`build/bin/qianjs`**（具体路径随生成器略有不同）。

**首次配置**需要能访问网络，以便 **`qjs`** 拉取 **QuickJS**。

---

## CMake 选项

| 选项 | 默认 | 含义 |
|------|------|------|
| `QIANJS_BUILD_CLI` | `ON` | 构建 **`qianjs`** 可执行文件、native 胶水代码；为 `ON` 时**始终** `include` **`cmake/libuv.cmake`** / **`cmake/uvw.cmake`**，使对应第三方 target 进入工程 |
| `QIANJS_BUILD_TESTS` | `ON` | 使用 `third_party/googletest` 构建 **`qianjs_tests`** |
| `QIANJS_MODULE_CONSOLE` | `ON` | 内置 `console` 插件 |
| `QIANJS_MODULE_FS` | `ON` | 内置 `fs` / `fs.sync` 源码 |
| `QIANJS_MODULE_NET` | `ON` | 内置 `net` 源码 |

当 **`QIANJS_MODULE_FS`** 与 **`QIANJS_MODULE_NET`** 均为 **`OFF`** 时，**`qianjs`** **不链接** libuv/uvw，**`QIANJS_HAVE_LIBUV`** 为假：共享 **`event_loop`** 仍会处理延后任务，但 **`tick`** 不会驱动 libuv 循环。

生成头文件（请勿手改）：**`${CMAKE_BINARY_DIR}/generated/qianjs_modules.h`**、**`qianjs_default_plugins.g.h`**。

**新增内置模块**：在 **`src/native/native_modules.cmake`** 注册；在 **`src/native/CMakeLists.txt`** 用 **`if(QIANJS_MODULE_…)`** 追加 **`target_sources`**；第三方与链接约定见 **[`src/native/README.md`](src/native/README.md)**。

---

## 测试

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j8
ctest --test-dir build --output-on-failure
```

关闭测试：**`-DQIANJS_BUILD_TESTS=OFF`**。

**`qjs`** 子目录默认 **`QJS_BUILD_TESTS=OFF`**，因此顶层工程默认**不会**构建 **`qjs`** 自带的 GTest，除非在 **`third_party/qjs`** 内另行打开。

---

## 命令行

```bash
qianjs help

qianjs run main.js          # ES 模块
qianjs run app.qbc          # 字节码

qianjs build main.js        # 输出 ./dist/main.qbc

qianjs embed dist/main.qbc  # 本程序副本 + 载荷（见下）
```

### 独立可执行文件

```bash
qianjs build main.js
qianjs embed dist/main.qbc
./dist/main                 # Windows 上为 dist\main.exe
```

无参数启动时，**`qianjs`** 会查找尾部魔数为 **`QIANJSBC`** 的嵌入字节码，再尝试可执行文件旁或当前目录下的 **`<exe 名>.qbc`**。

---

## 在其他 CMake 工程中使用 `qjs::qjs`

```cmake
set(QIANJS_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(QIANJS_BUILD_TESTS OFF CACHE BOOL "" FORCE)   # 可选
add_subdirectory(path/to/qianjs qianjs_build)

add_executable(myapp main.cc)
target_link_libraries(myapp PRIVATE qjs::qjs)
```

公开 API：**`#include <js_engine.h>`** 及同目录相关头文件；C++ 命名空间 **`qjs::`**。

---

## 仓库结构

CMake 与第三方封装策略：**[`cmake/README.md`](cmake/README.md)**。

| 路径 | 说明 |
|------|------|
| `cmake/` | 仅封装 **`third_party`**（`qjs`、`libuv`、`uvw`、`googletest` 等），见 **[`cmake/README.md`](cmake/README.md)** |
| `src/cli/` | CLI 入口（`main.cc`） |
| `src/runtime/` | 嵌入辅助、脚本宿主、**`event_loop/`** |
| `src/native/` | 内置插件；**`native_modules.cmake`**（模块选项与生成头）、**`CMakeLists.txt`**（源码）；详见 **[`src/native/README.md`](src/native/README.md)** |
| `tests/` | **`qianjs_tests`** |
| `third_party/qjs` | 引擎子模块 |

**`src/`** 下源码以 **`src/`** 为 include 根（例如 **`#include "runtime/embed.h"`**）。

---

## 模块文档（JavaScript API）

- [`console`](src/native/console/README.md)
- [`fs`](src/native/fs/README.md)
- [`net`](src/native/net/README.md)

内置模块的 CMake / C++ 接线：**[`src/native/README.md`](src/native/README.md)**。

---

## 依赖一览

| 组件 | 来源 |
|------|------|
| **QuickJS** | 由 **`third_party/qjs`** 通过 **FetchContent** 从 [bellard/quickjs](https://github.com/bellard/quickjs) 获取 |
| **qjs**（库） | Git 子模块 [touken928/qjs](https://github.com/touken928/qjs) |
| **libuv** / **uvw** | Git 子模块；**`QIANJS_BUILD_CLI=ON`** 时始终参与配置；仅在开启 **`fs`** 或 **`net`** 时**链接**进 **`qianjs`** |
| **GoogleTest** | 子模块，用于 **`QIANJS_BUILD_TESTS`** |

---

## 许可证

本仓库中由 QianJS 项目维护的原创代码默认以 [**Apache License 2.0**](https://www.apache.org/licenses/LICENSE-2.0.txt) 发布，全文见根目录 [`LICENSE`](LICENSE)。

第三方依赖（QuickJS、libuv、uvw、GoogleTest 等）保留各自许可证，见 `third_party/**/LICENSE` 或上游说明。通过 FetchContent 获取的 QuickJS 源码遵循 [bellard/quickjs](https://github.com/bellard/quickjs) 的许可；`third_party/qjs` 子模块内为 **Apache-2.0 全文** [`LICENSE`](third_party/qjs/LICENSE)，版权与第三方说明见 [`NOTICE`](third_party/qjs/NOTICE)。
