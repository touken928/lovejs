# net 模块（异步 TCP 客户端）

基于 **uvw** `tcp_handle` / `get_addr_info_req`（底层 libuv）的 TCP 客户端；连接用 **数字 socket id**（`connect` 的 `Promise` 结果）配合 `write` / `read` / `readBytes` / `close` 使用。

## 导入

```javascript
import * as net from 'net';
```

## API

### connect(port, host)

- `port`：1–65535。
- `host`：主机名或 IP；传 **`''`** 表示本机 **`127.0.0.1`**。
- 返回：`Promise<number>`（socket id）。

### write(socketId, data)

- `data`：与 `fs.writeFile` 相同，可为 **字符串**、**`ArrayBuffer`** 或 **TypedArray**（按原始字节发送）。
- 返回：`Promise<void>`。

### read(socketId)

返回：`Promise<string>`。将收到的字节按 **UTF-8** 解码为字符串（非法序列由引擎处理）。同一时间每个 socket **仅允许一个未完成的 `read` 或 `readBytes`**。

### readBytes(socketId)

返回：`Promise<ArrayBuffer>` 原始字节；EOF 后为 **`byteLength === 0`** 的 buffer。与 `read` 互斥（同上，单次仅一个挂起读）。

### close(socketId)

同步关闭并释放句柄；会拒绝挂起的 `read` / `readBytes` / 队列中的 `write`。

## 读语义说明

一次 `read` / `readBytes` 交付的是 **当前读缓冲里已累积的数据**（或 EOF），不是「固定长度」也不是「每个 TCP 段一次」。

## 示例

```javascript
import { log } from 'console';
import * as net from 'net';

const sock = await net.connect(6379, '');
try {
  await net.write(sock, 'PING\r\n');
  const reply = await net.read(sock);
  log(reply.trim());

  const enc = new TextEncoder().encode('HELLO');
  await net.write(sock, enc);
  const raw = await net.readBytes(sock);
  log(new Uint8Array(raw).length);
} finally {
  net.close(sock);
}
```

## 与 Node 的差异

- 无 `Socket` 类与 `EventEmitter`；**id + 函数式 API**。
- 无 `connectLocal`；本机请用 **`connect(port, '')`**。
- 无 `createServer`（后续可用 `uv_tcp_bind` / `listen` 扩展）。
