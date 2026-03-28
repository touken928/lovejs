# QianJS

A lightweight **JavaScript runtime** and CLI built on **QuickJS** via the vendored **[qjs](https://github.com/touken928/qjs)** C++ wrapper (`third_party/qjs` git submodule), with native modules, ES modules, and bytecode tooling.

## Features

- ES module entry (`qianjs run main.js`)
- Bytecode compile and run (`build` / `run *.qbc`)
- Embed bytecode into a standalone executable (`embed`)
- Built-in modules: `console`, `fs` (async Promise-based file I/O)
- Cross-platform (macOS, Windows, Linux)
- The **qjs** C++ layer is usable as a **standalone static library** from other CMake projects (`qjs::qjs`)

## Quick start

```javascript
// main.js
import { log } from 'console';

log('Hello, QianJS!');
```

```bash
qianjs run main.js
```

## Build

### Prerequisites

- CMake 3.16+
- C++17 compiler (**MSVC is not supported** on Windows; use MinGW-w64 or Clang)

### Clone

```bash
git clone --recurse-submodules https://github.com/touken928/qianjs.git
cd qianjs
```

If the GitHub repository is still named differently, adjust the URL. **Submodules** (initialize after clone if you skipped `--recurse-submodules`):

```bash
git submodule update --init --recursive
```

- `third_party/qjs` — [touken928/qjs](https://github.com/touken928/qjs) (C++ engine wrapper; **QuickJS** is **FetchContent** inside `qjs`)
- `third_party/googletest` — [google/googletest](https://github.com/google/googletest) (pinned for **QianJS** tests only; not used via qjs FetchContent when this submodule is added first)
- `third_party/thread-pool` — [BS::thread_pool](https://github.com/bshoshany/thread-pool) (header-only; pinned tag **`v5.1.0`**)

### Configure and build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
```

The `qianjs` binary is in `build/bin/`.

### Tests

With **`QIANJS_BUILD_TESTS=ON`** (default), **`qianjs_tests`** link against **`third_party/googletest`** (git submodule — run `git submodule update --init third_party/googletest`). The **`qjs`** subdirectory defaults **`QJS_BUILD_TESTS=OFF`**, so the top-level **`qianjs`** build does **not** build **`qjs_tests`** or pull GTest for qjs. To run **qjs**’s own tests, configure from **`third_party/qjs`** (standalone default **`QJS_BUILD_TESTS=ON`**) or set **`QJS_BUILD_TESTS=ON`** before **`add_subdirectory(third_party/qjs)`**. First configure needs network for **QuickJS** FetchContent (and for GTest when those tests are enabled).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j8
ctest --test-dir build --output-on-failure
```

To skip tests: `cmake -B build -DQIANJS_BUILD_TESTS=OFF`.

## CLI

```bash
qianjs help

# Run a JS file (ES module)
qianjs run main.js

# Run bytecode
qianjs run app.qbc

# Compile JS → ./dist/<name>.qbc
qianjs build main.js

# Copy this executable with bytecode appended (see docs)
qianjs embed dist/main.qbc
```

### Distribution

```bash
qianjs build main.js
qianjs embed dist/main.qbc
./dist/main    # or dist/main.exe on Windows
```

With no arguments, `qianjs` tries embedded bytecode, then `<exe-name>.qbc` next to the executable or in the current directory.

**Note:** Bytecode executables embedded with the new footer magic (`QIANJSBC`) are not compatible with older `LOVEJSBC` footers.

## Using the `qjs` library from CMake

The targets **`qjs::qjs`** and **`qjs::quickjs`** come from **`add_subdirectory(third_party/qjs)`**. Initialize the **qjs** submodule; **QuickJS** is fetched automatically by **`qjs`** at a **pinned commit** inside `third_party/qjs/cmake/quickjs.cmake`.

### `add_subdirectory`

```cmake
set(QIANJS_BUILD_CLI OFF CACHE BOOL "" FORCE)     # do not build the `qianjs` executable
set(QIANJS_BUILD_TESTS OFF CACHE BOOL "" FORCE)   # optional: skip GoogleTest + engine tests
add_subdirectory(path/to/this/repo qianjs_build)

add_executable(myapp main.cc)
target_link_libraries(myapp PRIVATE qjs::qjs)
```

Public headers use `#include <js_engine.h>` (and other `js_*.h` in the same directory); C++ API lives in namespace **`qjs::`**.

A minimal project lives under **`examples/use_qjs/`** (`cmake -S examples/use_qjs -B build && cmake --build build`).

## Source layout

| Path | Role |
|------|------|
| `src/cli/` | `main.cc` — command-line entry |
| `src/runtime/` | Host runtime: `embed.h`, `headless_runtime.h`, `async/` (thread pool helpers) |
| `src/native/` | Built-in native plugins: root `default_plugins.h`; modules under `console/`, `fs/`, … |
| `third_party/qjs/` | **qjs** submodule — FetchContent **QuickJS**, optional **`qjs_tests`** when **`QJS_BUILD_TESTS`**, `src/`, `include/` (→ [touken928/qjs](https://github.com/touken928/qjs)) |
| `examples/use_qjs/` | Sample consumer that only links **`qjs::qjs`** (no CLI) via `add_subdirectory` |

Headers under `src/` use includes rooted at `src/` (e.g. `#include "runtime/embed.h"`). Engine API: **`#include <js_engine.h>`** via target **`qjs::qjs`** (headers from `third_party/qjs/include/`).

**third_party:** **`qjs/`**, **`googletest/`**, and **`thread-pool/`** are git submodules; **QuickJS** comes from **`qjs`** FetchContent.

## Documentation

See [docs/README.md](./docs/README.md) and [docs/Fs.md](./docs/Fs.md).

## Dependencies

- **QuickJS** — pulled by **`third_party/qjs`** via **FetchContent** from [bellard/quickjs](https://github.com/bellard/quickjs) (see its `LICENSE`)
- **qjs** (CMake target `qjs::qjs`, namespace `qjs::`) — git submodule `third_party/qjs` ([touken928/qjs](https://github.com/touken928/qjs))
- **BS::thread_pool** — submodule `third_party/thread-pool` ([bshoshany/thread-pool](https://github.com/bshoshany/thread-pool), MIT)

## License

MIT
