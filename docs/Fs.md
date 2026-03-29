# Fs 模块（异步文件 I/O）

`fs` 模块提供简单的**异步文件读写**能力，基于 C++ 侧管理的 Promise；原生实现使用 **uvw** `file_req`（libuv 线程池上的 `uv_fs_*`）。

## 导入

```javascript
import * as fs from 'fs';
// 或者：
// import { readFile, writeFile } from 'fs';
```

## API

### readFile(path): Promise\<string\>

异步读取文本文件，返回文件内容字符串。

```javascript
const text = await fs.readFile("README.md");
```

- `path`: 相对或绝对路径（相对路径相对于**当前工作目录**）。
- 返回：`Promise<string>`；失败时 reject `Error`。

### writeFile(path, data): Promise\<void\>

异步写入文本文件（覆盖写入）。

```javascript
await fs.writeFile("output.txt", "Hello");
```

- `data`: UTF-8 文本字符串。
- 返回：`Promise<void>`；失败时 reject `Error`。

## 示例

```javascript
import { log } from 'console';
import * as fs from 'fs';

try {
    const text = await fs.readFile('README.md');
    await fs.writeFile('out.txt', text.slice(0, 80));
    log('OK');
} catch (err) {
    log('Error: ' + err.message);
}
```

说明：进程在退出前会等待 `fs` 与 `net` 等原生 I/O 完成（见 `script_host.h` 中的 `drainAsyncWork`）。`readFile` / `writeFile` 使用 **libuv** 的 **`uv_fs_*`**（与 Node 同栈），完成回调经 **`event_loop::defer`** 在主线程上 resolve/reject Promise。
