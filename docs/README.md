# QianJS 文档

QianJS 是一个轻量级 **JavaScript 运行时** 与命令行工具，基于 QuickJS。引擎 C++ 封装来自 git 子模块 **`third_party/qjs`**（[touken928/qjs](https://github.com/touken928/qjs)）；QuickJS 本体由 **`qjs` 的 CMake FetchContent** 拉取，无需在本仓库单独子模块化 QuickJS。

## 目录

- [Fs 模块](./Fs.md) — 异步文件读写

## 快速开始

### 1. 编写 `main.js`

```javascript
import { log } from 'console';

log('Hello, QianJS!');
```

### 2. 构建与运行

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8

qianjs run main.js
```

## 命令行

```bash
qianjs help
qianjs run <file.js|file.qbc>
qianjs build <file.js>
qianjs embed <file.qbc>
```

无参数启动时，会依次尝试：可执行文件内嵌字节码、同目录下 `<可执行文件名>.qbc`、当前目录下同名 `.qbc`。

## 内置模块

| 模块    | 说明 |
|---------|------|
| console | `log(message: string)` 输出一行到 stdout |
| fs      | 异步 `readFile` / `writeFile`（Promise） |

## 技术说明

- **ES 模块**：入口文件按 ES module 执行（与 `qjs` 引擎行为一致）。
- **字节码**：`build` 将模块编译为 `.qbc`，`run` 可直接加载。
- **异步 I/O**：`fs` 在后台线程执行，主线程在退出前会排空 Promise 与微任务队列。
