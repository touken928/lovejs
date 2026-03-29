# console 模块

向 **stdout** 输出一行文本的内置模块。

## 导入

```javascript
import { log } from 'console';
```

## API

### log(message)

- `message`：`string`，打印一行（末尾换行）。

## 示例

```javascript
import { log } from 'console';

log('Hello, QianJS!');
```

## 说明

- 无 Node 的 `console.log` 多参数拼接与格式化占位符；本模块仅提供 **`log(string)`**。
