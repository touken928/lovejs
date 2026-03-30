# process 模块（最小运行时信息）

提供运行参数、环境变量快照和退出码控制。

## 导入

```javascript
import * as process from 'process';
```

## API

### argv()

- 返回：`string[]`，脚本参数数组。
- `qianjs run app.js a b` 时，返回 `["app.js", "a", "b"]`。

### env()

- 返回：`Record<string, string>`，启动时捕获的环境变量快照（只读）。

### env(key)

- `key`：`string`。
- 返回：对应环境变量字符串；不存在时返回 `undefined`。

### getExitCode()

- 返回：当前退出码（`number`，默认 `0`）。

### setExitCode(code)

- `code`：`number`。
- 设置运行结束后 CLI 的退出码。

## 示例

```javascript
import * as process from 'process';
import { log } from 'console';

const args = process.argv();
log('args:', args.length);

const home = process.env('HOME') ?? process.env('USERPROFILE') ?? '';
log('home:', home);

if (args.includes('--fail')) {
  process.setExitCode(2);
}
```
