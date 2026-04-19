# console 模块

提供轻量日志输出的内置模块；与 `process`、`timers` 同属「最小可用宿主」核心。

## 导入

```javascript
import { log } from 'console';
```

## API

所有日志函数均为可变参数（最多 **32** 个）。每个参数先经引擎 **`ToString`** 再按空格拼接，与仅对原值做 C 字符串转换相比，**数字、一般对象** 的显示更稳定；若某次转换失败，会按异常向传播（与常见 `throw` 行为一致）。

### `log` / `info` / `debug` / `warn` / `error`

- 行为与参数规则相同，仅流不同：
  - `log` / `info` / `debug` → `stdout`
  - `warn` / `error` → `stderr`
- 行末自动换行。

## 示例

```javascript
import { log, info, warn, error } from 'console';

log('Hello, QianJS!');
info('port', 8080, { ok: true });
warn('disk space low');
error('request failed', 500);
```

## 说明

- 无 Node 的格式占位符（如 `%s`）；显示效果与 Node `util.format` 不同。
- 对象、数组的字符串化由 QuickJS 的 `ToString` 决定，不保证与 Node 的 `util.inspect` 一致。
