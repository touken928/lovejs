# LoveJS 文档

LoveJS 是一个基于 JavaScript 的 2D 游戏引擎，仿照 Love2D 设计，使用 QuickJS 作为 JS 运行时，SDL2 作为图形后端。

## 目录

- [Callbacks](./Callbacks.md) - 游戏循环和回调函数
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

### 2. 构建与运行

```bash
# 构建
xmake

# 运行默认的 main.js
xmake run

# 运行指定的 JS 文件
xmake run lovejs examples/basic_shapes.js
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

查看 `examples/` 目录获取更多示例代码：

- `hello.js` - 最简单的入门示例
- `particles.js` - 粒子效果，展示动态图形
- `game/snap.js` - 贪吃蛇游戏

运行示例：

```bash
xmake run lovejs examples/hello.js
xmake run lovejs examples/particles.js
xmake run lovejs examples/game/snap.js
```