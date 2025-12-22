# LoveJS 文档

LoveJS是一个基于JavaScript的2D游戏引擎，仿照Love2D设计。

## 目录

- [回调驱动](./回调驱动.md) - 游戏循环和回调函数
- [Graphics](./Graphics.md) - 图形渲染模块

## 快速开始

### 1. 创建main.js

```javascript
import { setWindow, clear, present, setColor, circle } from 'graphics';

export function load() {
    setWindow("Hello LoveJS", 800, 600);
}

export function update(dt) {
}

export function draw() {
    clear(0.1, 0.1, 0.2, 1);
    setColor(1, 1, 1, 1);
    circle(400, 300, 50, true);
    present();
}

export function keypressed(key) {}
export function keyreleased(key) {}
export function mousepressed(x, y, button) {}
export function mousereleased(x, y, button) {}
export function wheelmoved(x, y) {}
```

### 2. 运行

```bash
./build/macosx/arm64/release/lovejs
```

## 架构概览

```
main.js (你的游戏代码)
    │
    ├── export load()        → 初始化
    ├── export update(dt)    → 每帧更新
    ├── export draw()        → 每帧绘制
    ├── export keypressed()  → 键盘按下
    ├── export keyreleased() → 键盘释放
    ├── export mousepressed()→ 鼠标按下
    ├── export mousereleased()→ 鼠标释放
    └── export wheelmoved()  → 滚轮滚动
```

## 模块列表

| 模块 | 说明 |
|-----|------|
| graphics | 图形渲染、窗口管理、变换、纹理 |

## 示例

查看 `examples/` 目录获取更多示例代码。