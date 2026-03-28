# Fs 模块（异步文件 I/O）

`fs` 模块提供简单的**异步文件读写**能力，基于 C++ 侧管理的 Promise。

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

说明：进程在退出前会等待 `fs` 相关的 Promise 结算（见运行时实现中的 async drain）。
