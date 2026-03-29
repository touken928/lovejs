# net 模块（异步 TCP 客户端）

与 Node 类似，基于 **uvw** `tcp_handle` / `get_addr_info_req`（底层 libuv `uv_tcp_t` / `uv_getaddrinfo`）的轻量 TCP 客户端；句柄用 **数字 id**（`Promise<number>` 的 `connect` 结果）表示，便于在不扩展 qjs 宿主对象的前提下使用。

## 导入

```javascript
import * as net from 'net';
```

## API

### connect(port, host)

- `port`: 1–65535。
- `host`: 主机名或 IP；传 `''` 时使用 `127.0.0.1`。
- 返回：`Promise<number>`，解析为 **socket id**（供 `write` / `read` / `close` 使用）。

### connectLocal(port)

等价于 `connect(port, '')`（仅本机回环）。

### write(socketId, data)

- `data`: 字符串（按字节发送，与当前 `fs` 一致为原始字节视图）。
- 返回：`Promise<void>`。

### read(socketId)

返回：`Promise<string>`。同一时间每个 socket **仅允许一个未完成的 `read`**；有数据或 EOF 时结算。EOF 后读到空字符串。

### close(socketId)

同步关闭并释放底层句柄；会拒绝挂起的 `read` / 未发送的 `write` 队列。

## 示例

```javascript
import { log } from 'console';
import * as net from 'net';

const sock = await net.connectLocal(6379); // 或 connect(6379, '127.0.0.1')
try {
  await net.write(sock, 'PING\r\n');
  const reply = await net.read(sock);
  log(reply.trim());
} finally {
  net.close(sock);
}
```

## 与 Node 的差异

- 无 `Socket` 类与 `EventEmitter`；用 **id + 函数式 API**。
- 无内置 `createServer`（可在后续版本用 `uv_tcp_bind` / `uv_listen` 扩展）。
