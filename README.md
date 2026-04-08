# 3D Engine

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
./bin/Debug/Editor.exe
```

### Executables

- `Editor`: author scenes, inspect entities, tweak content, and iterate in-editor.
- `Game`: run the gameplay bootstrap scene without editor UI or debug panels.
- `Executable`: compatibility target that currently maps to the editor build.

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
- **Runtime Audio**: Minimal engine-level music and one-shot sound playback for `Game.exe`
- **Scene Format**: JSON-based scene definitions with component composition
- **Shader Auto-Discovery**: Automatic shader stage discovery from file extensions
- **Hot Reload**: SceneLoader API enables live scene reloading during development

## Development

### Game Development Workflow

1. Implement reusable engine/runtime features in `engine/`.
2. Implement game-specific components, systems, and scene bootstrap in `game/`.
3. Use `Editor.exe` to build scenes and tune component data in `assets/scenes/`.
4. Use `Game.exe` to validate the actual player-facing runtime without editor tooling.
5. Pass a scene path to `Game.exe` when you want to run the JSON-backed `EditorScene` runtime instead of the default gameplay bootstrap scene.

For gameplay collision logic, add a `PhysicsListener` component to any entity that should react to collisions or triggers. The runtime wiring now fills that component automatically each frame from Bullet events, so gameplay systems can query the entity-local listener instead of scanning the global physics event stream manually.

`SimpleGameScene` now includes a concrete trigger example: the player cube has a `PhysicsListener`, entering the `GoalTrigger` zone opens the `GoalDoor` cube, boosts the fill light, and plays a confirmation sound; exiting the zone closes it again with a second cue. `Game.exe` also now shows a minimal runtime HUD and pause menu, and the default bootstrap scene starts looping background music, so there is a real player-facing menu/HUD/audio path instead of editor-only tooling. The editor inspector also supports adding and removing `PhysicsListener` visually, so the component can be authored like the other gameplay-facing components.

### Build Targets

```bash
# Editor build
cmake --build build --config Release --target Editor

# Runtime game build
cmake --build build --config Release --target Game
```

Release outputs are generated under `build/bin/Release/`.

Examples:

```bash
# Default gameplay bootstrap scene
./build/bin/Release/Game.exe

# Run a JSON-authored scene with the heavier EditorScene runtime
./build/bin/Release/Game.exe assets/scenes/backpack_scene.json

# Run the sample physics authoring scene with box/sphere/capsule/trigger setup
./build/bin/Release/Game.exe assets/scenes/physics_authoring_scene.json
```

In `Game.exe`, `ESC` now opens a minimal pause menu with Resume, Restart, and Quit. The default bootstrap scene also shows a HUD objective/status overlay.

Audio assets for the default runtime demo live under `assets/audio/`. The engine audio service is currently Windows-backed and intended as a lightweight runtime seam for simple games and prototype-scale content.

Example usage in gameplay code:

```cpp
PhysicsListener *listener = world.GetComponent<PhysicsListener>(playerEntity);
if (listener && listener->HasEvent(PhysicsEventType::TriggerEnter))
{
	// react to entering a trigger this frame
}
```

The full in-repo example lives in `game/scene/simple_game_scene.cpp` and `game/systems/physics_listener_demo_system.hpp`.

### Shipping

For a playable build, package:

- `build/bin/Release/Game.exe`
- the `assets/` directory
- any runtime DLLs produced beside the executable in the build output

### Adding New Components

1. Define the component in `engine/ecs/components/`
2. Register a loader in `game/components/component_registration.cpp`
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
