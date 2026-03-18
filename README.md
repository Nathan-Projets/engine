# FallGuysClone Engine

A modern OpenGL-based 3D game engine with a material-driven rendering pipeline, entity component system (ECS), and async resource management.

## Quick Start

### Prerequisites

- **C++23** compiler (MSVC, GCC, or Clang)
- **CMake 3.25+**
- **OpenGL 4.6+**
- **Git** (with submodules support)

### Building

```bash
# Clone repository with submodules
git clone --recurse-submodules <repo-url>
cd engine

# Create build directory
mkdir build
cd build

# Generate build files
cmake ..

# Build the project
cmake --build . --config Debug
```

### Running

```bash
# From the build directory
cmake --build . --config Debug
./bin/Debug/FallGuysClone.exe
```

## Documentation

This project includes comprehensive API documentation generated from source code comments using **Doxygen**.

Quick commands:

```bash
cmake --build build -t documentation
```

```bash
doxygen Doxyfile
```

Output: `docs/html/index.html`

For full documentation workflow, install options, CI usage, and troubleshooting, see `DOCUMENTATION.md`.

## Architecture

### Core Subsystems

- Resource Management: async loading, deduplication, and handle-based access
- Entity Component System (ECS): component composition and system-driven updates
- Rendering: OpenGL backend with material-driven shading

See generated API docs for class-level architecture details.

## Project Structure

```
engine/
├── core/              # Scene and world management
├── ecs/               # Entity component system
│   ├── components/    # Component definitions
│   └── systems/       # System implementations
├── render/            # Rendering subsystem
│   ├── camera/        # Camera systems
│   └── renderer.hpp   # GPU state management
├── resources/         # Resource management
│   ├── loaders/       # Asset loaders (mesh, shader, texture, material)
│   └── units/         # Resource types
├── helpers/           # Utility functions and logging
└── input/             # Input handling

game/
├── components/        # Game-specific components
├── systems/           # Game-specific systems
└── scene/             # Game scene implementation

src/
└── main.cpp           # Entry point

assets/
├── meshes/            # 3D models
├── shaders/           # GLSL shader files
├── materials/         # Material definitions (JSON)
├── textures/          # Texture images
└── scenes/            # Scene files (JSON)
```

## Key Features

- **Material-Driven Rendering**: Shaders and textures driven by material definitions
- **Async Resource Loading**: Non-blocking asset loading with deduplication
- **GPU Callbacks**: Explicit control over GPU upload and release phases
- **Scene Format**: JSON-based scene definitions with component composition
- **Shader Auto-Discovery**: Automatic shader stage discovery from file extensions
- **Hot Reload**: SceneLoader API enables live scene reloading during development

## Development

### Adding New Components

1. Define the component in `engine/ecs/components/`
2. Register a loader in `game/scene/game_scene.cpp`
3. Reference in scene JSON with component type name

### Adding New Materials

1. Create material JSON in `assets/materials/`
2. Specify shader and texture slots
3. Define material properties
4. Reference in scene entity Render components

### Adding Assets

- **Shaders**: Place in `assets/shaders/` and use base path (auto-discovery finds `.vert`/`.frag`)
- **Models**: Place in `assets/meshes/` and reference by filename
- **Textures**: Place in `assets/textures/` and reference in materials
- **Materials**: Define JSON in `assets/materials/`

## Documentation Files

- `DOCUMENTATION.md` - Detailed documentation guide
- `docs/` - Generated Doxygen documentation

## License

[Add your license here]

## References

- [Doxygen Documentation](https://www.doxygen.nl/manual/)
- [OpenGL API Reference](https://www.khronos.org/opengl/wiki/)
- [GLM Mathematics Library](https://github.com/g-truc/glm)
- [simdjson](https://github.com/simdjson/simdjson)
