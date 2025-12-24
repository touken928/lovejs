# LoveJS

A lightweight 2D game engine for JavaScript, inspired by [LÖVE](https://love2d.org/).

## Features

- Love2D-style callback API
- ES6 module support
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

export function keypressed(key) {
    if (key === 'escape') {
        // Handle escape key
    }
}
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
# Clone the repository
git clone https://github.com/touken928/lovejs.git
cd lovejs

# Initialize submodules (IMPORTANT!)
git submodule update --init --recursive

# Configure and build (static linking by default)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
```

## Run

```bash
# Run the default main.js
./build/bin/lovejs

# Run a specific JavaScript file
./build/bin/lovejs examples/hello.js

# Run example games
./build/bin/lovejs examples/game/tetris.js
./build/bin/lovejs examples/game/gomoku.js
./build/bin/lovejs examples/game/snap.js
```

## Rendering Backend

LoveJS uses **Sokol** as its rendering backend:

- **macOS**: Metal (native, high performance)
- **Windows**: Direct3D 11
- **Linux**: OpenGL 3.3 Core

Sokol provides:
- Lightweight, modern graphics API
- Excellent cross-platform support
- Batch rendering for better performance
- Minimal dependencies

See [SOKOL_SUMMARY.md](./SOKOL_SUMMARY.md) for technical details.

## Examples

The `examples/` directory contains several demo programs:

- `examples/hello.js` - Basic "Hello World" example
- `examples/particles.js` - Particle system demonstration
- `examples/game/tetris.js` - Full Tetris implementation
- `examples/game/gomoku.js` - Five-in-a-row game (Gomoku)
- `examples/game/snap.js` - Snake game

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
