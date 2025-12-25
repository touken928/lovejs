# LoveJS 文档

LoveJS 是一个基于 JavaScript 的 2D 游戏引擎，仿照 Love2D 设计，使用 QuickJS 作为 JS 运行时，Sokol 作为图形后端。

## 目录

- [Callbacks](./Callbacks.md) - 游戏循环和回调函数
- [Graphics](./Graphics.md) - 图形渲染模块

## 快速开始

### 1. 创建 main.js

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
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8

# 运行 JS 文件
lovejs run main.js

# 运行示例
lovejs run examples/hello.js
```

## 命令行工具

```bash
# 显示帮助
lovejs help

# 运行 JS 源文件
lovejs run main.js

# 运行字节码文件
lovejs run game.qbc

# 编译 JS 到字节码（输出到 ./dist/<name>.qbc）
lovejs build main.js

# 运行同名字节码（查找 <可执行文件名>.qbc）
lovejs
```

### 发布游戏

可以将游戏打包为可执行文件 + 字节码的形式发布：

```bash
# 1. 编译游戏
lovejs build main.js

# 2. 重命名可执行文件和字节码，使其同名
cp build/bin/lovejs mygame
cp dist/main.qbc mygame.qbc

# 3. 运行 - 自动加载 mygame.qbc
./mygame
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

```bash
lovejs run examples/hello.js        # 入门示例
lovejs run examples/particles.js    # 粒子效果
lovejs run examples/game/tetris.js  # 俄罗斯方块
lovejs run examples/game/gomoku.js  # 五子棋
lovejs run examples/game/snap.js    # 贪吃蛇
```

## 技术特性

- **现代图形 API**: 基于 Sokol，支持 Metal/D3D11/OpenGL 硬件加速渲染
- **轻量级引擎**: 使用 QuickJS，启动快速，内存占用小
- **字节码编译**: 支持将 JS 编译为字节码，便于发布
- **ES6 模块**: 支持现代 JavaScript 模块系统
- **跨平台**: 支持 Windows、macOS、Linux
- **简单易用**: Love2D 风格的 API，学习成本低
