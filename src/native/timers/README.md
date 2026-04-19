# timers 模块（基础定时器）

提供 `setTimeout` / `setInterval` 以及对应清理函数，依赖宿主在脚本结束后 **`drainAsyncWork`**（`qianjs run` / 嵌入路径已包含）：在引擎侧排空微任务并把通过 `event_loop::defer` 投递的回调跑完。

## 导入

```javascript
import { setTimeout, setInterval, clearTimeout, clearInterval } from 'timers';
```

## API

### `setTimeout(callback, delayMs)`

- `callback`：无参数函数。
- `delayMs`：毫秒，负数按 `0` 处理。
- 返回：`number`（timer id），供 `clearTimeout` 使用。
- 实现：独立线程休眠后在 **JS 线程**上通过 `event_loop::defer` 调用回调（与 Node 的单线程模型不同，但回调仍在引擎上下文执行）。

### `setInterval(callback, delayMs)`

- `callback`：无参数函数。
- `delayMs`：毫秒，负数按 `0`；周期为 `0` 时在实现中会抬到 **1ms**，避免忙等。
- 返回：`number`（timer id）。

### `clearTimeout(id)` / `clearInterval(id)`

- 取消对应 timer；对无效或已结束的 id **幂等**，可重复调用。

## 插件初始化

加载本模块时会调用 `event_loop::ensure_started()`；在启用 `fs` 时会确保 libuv 循环已创建，与 `fs` 异步 I/O 共用同一循环。

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

## 说明

- 与 Node 不同：不提供全局 `setTimeout`，须从 `'timers'` 导入。
- 长时间运行的 `setInterval` 应在脚本结束前 `clearInterval`，以免后台线程仍存活（宿主退出前一般仍会排空已投递的 defer；显式清理更稳妥）。
