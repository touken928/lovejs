# console 模块

提供轻量日志输出的内置模块。

## 导入

```javascript
import { log } from 'console';
```

## API

### log(message)

- 参数：`...args`（最多 32 个），每个参数按字符串化后以空格拼接。
- 输出：`stdout`，末尾自动换行。

### info(...args)

- 与 `log` 相同，输出到 `stdout`。

### debug(...args)

- 与 `log` 相同，输出到 `stdout`。

### warn(...args)

- 参数与拼接规则同 `log`。
- 输出到 `stderr`，末尾自动换行。

### error(...args)

- 参数与拼接规则同 `log`。
- 输出到 `stderr`，末尾自动换行。

## 示例

```javascript
import { log, info, warn, error } from 'console';

log('Hello, QianJS!');
info('port', 8080, { ok: true });
warn('disk space low');
error('request failed', 500);
```

## 说明

- 无 Node 的格式化占位符（如 `%s`）。
- 对象会走引擎默认字符串化，不保证与 Node 一致。
