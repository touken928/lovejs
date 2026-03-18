# Fs 模块（异步文件 I/O）

`fs` 模块提供简单的**异步文件读写**能力，基于 C++ 侧管理的 Promise，不会阻塞游戏主循环。

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

- `path`: 相对或绝对路径  
  - 相对路径默认相对于当前工作目录（一般是运行时所在目录）。
- 返回：`Promise<string>`，resolve 为文件内容，如果读取失败则 reject 一个 `Error`。

### writeFile(path, data): Promise\<void\>

异步写入文本文件（覆盖写入）。

```javascript
await fs.writeFile("output.txt", "Hello LoveJS");
```

- `path`: 目标文件路径
- `data`: 要写入的字符串（会被转为 UTF-8 字节流）
- 返回：`Promise<void>`，写入失败时 reject 一个 `Error`。

## 使用模式

结合 `async/await` 在 `load` 或其他异步流程中使用：

```javascript
import * as graphics from 'graphics';
import * as fs from 'fs';

let message = "loading...";

export async function load() {
    graphics.setWindow("FS Demo", 800, 600);

    try {
        const text = await fs.readFile("README.md");
        await fs.writeFile("tmp_fs_demo.txt", text.slice(0, 80));
        message = "FS OK: read README.md and wrote tmp_fs_demo.txt";
    } catch (err) {
        message = "FS Error: " + err.message;
    }
}
```

注意：

- 所有文件 I/O 都在后台线程执行，不会卡顿渲染。
- Promise 的创建和 resolve/reject 完全在 C++ 侧管理，JS 端只需要正常 `await` 即可。

