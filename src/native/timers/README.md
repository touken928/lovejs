# timers 模块（基础定时器）

提供 `setTimeout` / `setInterval` 以及对应清理函数。

## 导入

```javascript
import { setTimeout, setInterval, clearTimeout, clearInterval } from 'timers';
```

## API

### setTimeout(callback, delayMs)

- `callback`：无参数函数。
- `delayMs`：毫秒，负数按 `0` 处理。
- 返回：`number`（timer id）。

### setInterval(callback, delayMs)

- `callback`：无参数函数。
- `delayMs`：毫秒，负数按 `0` 处理（内部最小 1ms，避免空转）。
- 返回：`number`（timer id）。

### clearTimeout(id)

- 取消 `setTimeout` 创建的 timer。
- 允许重复调用或取消不存在 id（无副作用）。

### clearInterval(id)

- 取消 `setInterval` 创建的 timer。
- 允许重复调用或取消不存在 id（无副作用）。

## 示例

```javascript
import { setTimeout, setInterval, clearInterval } from 'timers';
import { log } from 'console';

setTimeout(() => {
  log('once');
}, 50);

let count = 0;
const id = setInterval(() => {
  count++;
  log('tick', count);
  if (count >= 3) clearInterval(id);
}, 20);
```
