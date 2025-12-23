# LoveJS

A lightweight 2D game engine for JavaScript, inspired by [LÖVE](https://love2d.org/).

## Features

- Love2D-style callback API
- ES6 module support
- SDL2-based rendering
- Keyboard, mouse, and wheel input

## Quick Start

```javascript
// main.js
import * as graphics from 'graphics';

export function load() {
    graphics.setWindow("My Game", 800, 600);
}

export function update(dt) {
}

export function draw() {
    graphics.clear(0.1, 0.1, 0.2, 1);
    graphics.setColor(0, 1, 0, 1);
    graphics.circle(400, 300, 50, true);
    graphics.present();
}

export function keypressed(key) {}
export function keyreleased(key) {}
export function mousepressed(x, y, button) {}
export function mousereleased(x, y, button) {}
export function wheelmoved(x, y) {}
```

## Build

```bash
# 初始化子模块
git submodule update --init --recursive

# 配置并构建
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
```

## Run

```bash
./build/bin/lovejs           # runs main.js
./build/bin/lovejs game.js   # runs game.js
```

## Callbacks

| Callback | Description |
|----------|-------------|
| `load()` | Called once at startup |
| `update(dt)` | Called every frame |
| `draw()` | Called every frame after update |
| `keypressed(key)` | Key pressed |
| `keyreleased(key)` | Key released |
| `mousepressed(x, y, button)` | Mouse button pressed |
| `mousereleased(x, y, button)` | Mouse button released |
| `wheelmoved(x, y)` | Mouse wheel scrolled |

## Dependencies

- CMake 3.16+
- C++17 compiler
- Git (for submodules)

Third-party libraries (managed as git submodules):
- SDL2
- SDL2_image
- QuickJS

## Documentation

See [docs/](./docs/) for detailed documentation.

## License

MIT