# LoveJS

A lightweight 2D game engine for JavaScript, inspired by [LÖVE](https://love2d.org/).

## Features

- Love2D-style callback API
- ES6 module support
- Bytecode compilation for distribution
- Sokol-based rendering with modern graphics pipeline (Metal/D3D11/OpenGL)
- Keyboard, mouse, and wheel input
- Built-in text rendering
- Geometric shapes and basic graphics primitives
- Texture loading support
- Matrix transformations (translate, rotate, scale)
- Cross-platform (macOS, Windows, Linux)

## Quick Start

```javascript
// main.js
import { setWindow, clear, present, setColor, circle, print } from 'graphics';

export function load() {
    setWindow("My Game", 800, 600);
}

export function update(dt) {
    // Game logic here
}

export function draw() {
    clear(0.1, 0.1, 0.2, 1);
    
    setColor(0, 1, 0, 1);
    circle(400, 300, 50, true);
    
    setColor(1, 1, 1, 1);
    print("Hello, LoveJS!", 350, 250);
    
    present();
}

export function keypressed(key) {}
export function keyreleased(key) {}
export function mousepressed(x, y, button) {}
export function mousereleased(x, y, button) {}
export function wheelmoved(x, y) {}
```

## Build

### Prerequisites
- CMake 3.16+
- C++17 compatible compiler (**Note: MSVC is not supported**)
- Git (for submodules)

### Supported Compilers
- **Linux**: GCC 7+ or Clang 5+
- **macOS**: Clang (Xcode Command Line Tools)
- **Windows**: MinGW-w64 or Clang (MSVC is not supported)

### Build Steps

```bash
# Clone the repository with submodules
git clone --recursive --depth 1 https://github.com/user/lovejs.git
cd lovejs

# Configure and build (static linking by default)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
```

## CLI Usage

```bash
# Show help
lovejs help

# Run a JavaScript file
lovejs run main.js

# Run a bytecode file
lovejs run game.qbc

# Compile JS to bytecode (output to ./dist/<name>.qbc)
lovejs build main.js

# Embed bytecode into standalone executable
lovejs embed dist/main.qbc
```

### Distribution

You can distribute your game as a single standalone executable:

```bash
# 1. Compile your game to bytecode
lovejs build main.js

# 2. Embed bytecode into executable (generates dist/main)
lovejs embed dist/main.qbc

# 3. Run the standalone executable
./dist/main
```

## Rendering Backend

LoveJS uses **Sokol** as its rendering backend:

- **macOS**: Metal (native, high performance)
- **Windows**: Direct3D 11
- **Linux**: OpenGL 3.3 Core

## Examples

The `examples/` directory contains several demo programs:

```bash
lovejs run examples/hello.js
lovejs run examples/particles.js
lovejs run examples/game/tetris.js
lovejs run examples/game/gomoku.js
lovejs run examples/game/snap.js
```

## Dependencies

Third-party libraries (managed as git submodules):
- **Sokol** - Modern cross-platform graphics library
- **QuickJS** - Lightweight JavaScript engine

## Documentation

See [docs/](./docs/) for detailed documentation:

- [Getting Started Guide](./docs/README.md)
- [Callback System](./docs/Callbacks.md)
- [Graphics API Reference](./docs/Graphics.md)

## License

MIT
