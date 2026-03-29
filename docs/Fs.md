# Fs 模块（异步文件 I/O，贴近 `fs.promises`）

`fs` 模块提供 **Promise 风格** 的文件系统 API；底层为 **uvw** / **libuv** `uv_fs_*`，完成回调经 **`event_loop::defer`** 回到 JS 线程 settle Promise。

## 导入

```javascript
import * as fs from 'fs';
```

## 二进制与编码

- **`readFile(path)`** / **`readFile(path, { encoding: 'utf8' })`**：返回 **`Promise<string>`**（UTF-8 文本）。
- **`readFile(path, { encoding: null })`**：返回 **`Promise<ArrayBuffer>`**（原始字节）。需要 `Uint8Array` 视图时用 **`new Uint8Array(buf)`**（本运行时无 Node 的 `Buffer`）。
- **`writeFile` / `appendFile` 的 `data`**：可为 **字符串**、**`ArrayBuffer`** 或 **TypedArray**（按字节写入）。

## API 一览

| 函数 | 说明 |
|------|------|
| `readFile(path, options?)` | 读整个文件。`options`: `encoding`（`'utf8'` / `null`）、`flag`（如 `'r'`、`'r+'`）。 |
| `writeFile(path, data, options?)` | 覆盖写。`options`: `flag`（`'w'`、`'wx'`、`'a'`、`'a+'` 等）、`mode`（八进制，默认 `0o644` / Windows 等价）。 |
| `appendFile(path, data, options?)` | 追加写；等价于以追加方式打开。`options.mode` 可设。 |
| `mkdir(path, options?)` | `options.recursive` 为真时用 **`std::filesystem::create_directories`** 同步创建（在同一线程上执行）；否则单次 `uv_fs_mkdir`。`options.mode` 默认 `0o777`。 |
| `readdir(path)` | **`Promise<string[]>`** 文件名（不含 `.` / `..`）。 |
| `stat(path)` | 跟随符号链接。 |
| `lstat(path)` | 不跟随符号链接。 |
| `unlink(path)` | 删除文件。 |
| `rmdir(path)` | 删除空目录。 |
| `rename(oldPath, newPath)` | 重命名 / 移动。 |
| `copyFile(src, dest, flags?)` | `flags` 为 libuv 拷贝标志；**`1`** = `COPYFILE_EXCL`（目标存在则失败）。 |

`stat` / `lstat` 解析值为普通对象，含 **`size`**、**`*Ms` 时间戳**、**`isFile` / `isDirectory` / `isSymbolicLink`** 等布尔字段（与 Node 的 `fs.Stats` 形状接近，无方法原型）。

## 示例

```javascript
import { log } from 'console';
import * as fs from 'fs';

const text = await fs.readFile('README.md');
const raw = await fs.readFile('README.md', { encoding: null });
log('utf8 length ' + text.length);
log('bytes ' + raw.byteLength);

const enc = new TextEncoder().encode('hello');
await fs.writeFile('out.bin', enc, { flag: 'w', mode: 0o644 });

await fs.mkdir('dist/nested', { recursive: true });
const names = await fs.readdir('.');
const st = await fs.stat('README.md');
log('stat ' + st.isFile + ' ' + st.size + ' ' + st.mtimeMs);
```

说明：进程退出前仍会排空挂起的 `fs` / `net`（见 `script_host.h` 中的 `drainAsyncWork`）。

## 与 Node 的差异（简要）

- 无 **`fs.promises`** 命名空间；本模块即 Promise 风格。
- 无 **`Buffer`**；二进制为 **`ArrayBuffer`** + 可选 **`Uint8Array`**。
- 无 Stream、`open`/`fd`、`watch`、`access` 等（可后续再加）。
- `reject` 时部分错误带 **`error.code`**（两参数 `rejectPromise`）；未覆盖全部 errno 映射。
