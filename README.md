# LoveJS

A lightweight 2D game engine for JavaScript, inspired by [LÖVE](https://love2d.org/).

## Features

- Love2D-style callback API
- ES6 module support
- SDL3-based rendering with modern graphics pipeline
- Keyboard, mouse, and wheel input
- Built-in text rendering
- Geometric shapes and basic graphics primitives
- Texture loading (BMP format)
- Matrix transformations (translate, rotate, scale)

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
- C++17 compatible compiler
- Git (for submodules)

### Build Steps

```bash
# Clone the repository
git clone https://github.com/touken928/lovejs.git
cd lovejs

# Initialize submodules
git submodule update --init --recursive

# Configure and build
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
```

## Examples

The `examples/` directory contains several demo programs:

- `examples/hello.js` - Basic "Hello World" example
- `examples/particles.js` - Particle system demonstration
- `examples/game/tetris.js` - Full Tetris implementation
- `examples/game/gomoku.js` - Five-in-a-row game (Gomoku)

## Dependencies

- CMake 3.16+
- C++17 compiler
- Git (for submodules)

Third-party libraries (managed as git submodules):
- **SDL3** - Modern cross-platform graphics and input
- **QuickJS** - Lightweight JavaScript engine

## Documentation

See [docs/](./docs/) for detailed documentation:

- [Getting Started Guide](./docs/README.md)
- [Callback System](./docs/Callbacks.md)
- [Graphics API Reference](./docs/Graphics.md)

## License

MIT