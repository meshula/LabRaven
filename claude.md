# LabRaven Technical Architecture Documentation

This document provides a comprehensive technical overview of the LabRaven architecture, design patterns, and implementation details.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Core Concepts](#core-concepts)
3. [Component Breakdown](#component-breakdown)
4. [Design Patterns](#design-patterns)
5. [Data Flow](#data-flow)
6. [Plugin System](#plugin-system)
7. [Build System](#build-system)
8. [Platform Support](#platform-support)
9. [Code Organization](#code-organization)
10. [Extending the System](#extending-the-system)

## Architecture Overview

LabRaven implements a **Studio-Activity-Provider** architecture pattern that enables modular, composable multimedia applications. The system is built on several key architectural principles:

### Architectural Layers

```
┌─────────────────────────────────────────────────┐
│                   Studios                       │  ← Workspace Configurations
├─────────────────────────────────────────────────┤
│                  Activities                     │  ← UI/Functionality Modules
├─────────────────────────────────────────────────┤
│                  Providers                      │  ← Data/Service Layer
├─────────────────────────────────────────────────┤
│                 Orchestrator                    │  ← Core Coordination
├─────────────────────────────────────────────────┤
│            CSP Engine (ZeroMQ)                  │  ← Event System
├─────────────────────────────────────────────────┤
│         Transaction/Journal System              │  ← Undo/Redo
├─────────────────────────────────────────────────┤
│          Platform Abstraction Layer             │  ← OS/Graphics APIs
└─────────────────────────────────────────────────┘
```

### Key Files

- **Core Architecture**: `src/Lab/StudioCore.hpp`
- **Application Framework**: `src/Lab/App.h`
- **CSP Engine**: `src/Lab/CSP.hpp`
- **Platform Entry Points**:
  - macOS: `src/Lab/main_macos.mm`
  - Windows: `src/Lab/main_win32.cpp`
  - Linux/GLFW: `src/Lab/main_glfw.cpp`
  - Web: `src/Lab/main_emscripten.cpp`

## Core Concepts

### 1. Studio

A **Studio** represents a complete workspace configuration. It defines:
- Which Activities are active
- Which Providers are available
- The initial layout and state
- Coordination between Activities

**Location**: `src/Studios/`

**Current Studios**:
- **Color Studio** (`src/Studios/Color/`) - Color science exploration
- **OpenUSD Studio** (`src/Studios/OpenUSD/`) - USD authoring
- **OpenUSD Insights Studio** (`src/Studios/OpenUSDInsights/`) - USD analysis
- **Texture Studio** (`src/Studios/Texture/`) - Texture tools

**Example Studio Implementation**:
```cpp
class ColorStudio : public Studio {
public:
    void setup(Orchestrator* orch) override {
        // Initialize providers
        orch->getProvider<ColorProvider>();

        // Activate activities
        orch->activateActivity<ColorSpaceActivity>();
        orch->activateActivity<SpectralActivity>();
        orch->activateActivity<ConeSensitivityActivity>();
    }
};
```

### 2. Activity

An **Activity** is a discrete, composable piece of functionality. Each Activity:
- Has its own ImGui UI implementation
- Can subscribe to Provider data
- Can publish events via CSP
- Is independently testable
- Can be activated/deactivated dynamically

**Location**: `src/Activities/`

**Activity Categories**:
- **Color Tools**: Spectral analysis, color space visualization, CMF plots
- **USD Tools**: Scene authoring, component editing, insights
- **Timeline**: OTIO-based sequencing
- **Animation**: Skeletal animation, BVH import
- **Audio**: Sound editing, MIDI control
- **2D Graphics**: Sprites, tiles, textures
- **Developer Tools**: Console, node graph, preferences, journal

**Activity Interface**:
```cpp
class Activity {
public:
    virtual void init(Orchestrator* orch) = 0;
    virtual void update(float dt) = 0;
    virtual void render() = 0;
    virtual void shutdown() = 0;
};
```

### 3. Provider

A **Provider** is a singleton service that supplies data and functionality. Providers:
- Are globally accessible through the Orchestrator
- Manage their own state
- Can be queried by any Activity
- Often wrap external libraries or data sources

**Location**: `src/Providers/`

**Core Providers**:
- **ColorProvider** (`src/CoreProviders/Color/`) - Color management, spectral data
- **CameraProvider** (`src/Providers/Camera/`) - Camera state management
- **SoundProvider** (`src/Providers/Sound/`) - Audio engine (LabSound)
- **MidiProvider** (`src/Providers/Midi/`) - MIDI I/O (LabMidi)
- **OpenUSDProvider** (`src/Providers/OpenUSD/`) - USD scene management
- **AssetProvider** (`src/Providers/Assets/`) - Asset loading and management
- **SelectionProvider** (`src/Providers/Selection/`) - Selection state
- **SchemeProvider** (`src/Providers/Scheme/`) - s7 Scheme scripting
- **AnimationProvider** (`src/Providers/Animation/`) - Animation data
- **AstroProvider** (`src/Providers/Astro/`) - Astronomical calculations
- **TilengineProvider** (`src/Providers/Tilengine/`) - 2D rendering

**Provider Interface**:
```cpp
class Provider {
public:
    virtual void init(Orchestrator* orch) = 0;
    virtual void update() = 0;
    virtual void shutdown() = 0;
};
```

### 4. Orchestrator

The **Orchestrator** is the central coordinator that manages:
- Studio lifecycle
- Activity activation/deactivation
- Provider registration and access
- Transaction system
- CSP event routing

**Key Responsibilities**:
- Dependency injection for Providers
- Activity lifecycle management
- Event bus coordination
- State synchronization

### 5. CSP Engine

The **Communicating Sequential Processes (CSP)** engine provides event-driven programming:
- Built on ZeroMQ for message passing
- Enables reactive, state-machine-based Activities
- Decouples Activities from each other
- Supports asynchronous operations

**Implementation**: `src/Lab/CSP.hpp`

**CSP Pattern**:
```cpp
// Activity subscribes to events
csp_subscribe("color.changed", [](const Event& evt) {
    // React to color change
});

// Activity publishes events
csp_publish("color.changed", colorData);
```

### 6. Transaction/Journal System

The **Transaction System** provides comprehensive undo/redo:
- Wraps all state mutations
- Records changes to a journal
- Supports multi-level undo/redo
- Can serialize/deserialize state

**Features**:
- Transactional state changes
- Journal visualization (Activity)
- State rollback
- History inspection

## Component Breakdown

### Activities

#### Color Activities (`src/Activities/Color/`)

The Color Activity suite provides sophisticated color science tools:

**Components**:
- **Spectral Locus Visualization**: CIE 1964 10-degree standard observer
- **Cone Sensitivity Plots**: L, M, S photoreceptor response curves
- **CMF (Color Matching Functions)**: Tristimulus values
- **Spectral Response Editor**: Interactive spectral curve editing
- **Color Space Comparison**: sRGB, Display P3, Rec.2020, ACES, etc.
- **Planckian Locus**: Blackbody radiation curves
- **ISO17321 Charts**: Color calibration targets

**Key Files**:
- Activity implementations in `src/Activities/Color/`
- Color provider in `src/CoreProviders/Color/`
- Color library: nanocolor

#### OpenUSD Activities (`src/Activities/OpenUSD/`)

USD (Universal Scene Description) integration:
- **Scene Authoring**: Create and modify USD stages
- **Component Editing**: Edit prims, attributes, relationships
- **Hydra Integration**: Real-time rendering
- **USD Insights**: Stage analysis, dependency graphs

**Dependencies**:
- Pixar USD library (optional, controlled by `HAVE_OPENUSD`)
- OpenUSDProvider for scene management

#### Timeline Activity (`src/Activities/Timeline/`)

Timeline editing using OpenTimelineIO:
- **Media Sequencing**: Arrange clips, gaps, transitions
- **OTIO Support**: Import/export OTIO files
- **Playback Control**: Timeline scrubbing, playback

**Dependencies**:
- OpenTimelineIO library (controlled by `HAVE_OTIO`)

#### Animation Activity (`src/Activities/Animation/`)

Skeletal animation tools:
- **BVH Import**: Motion capture data
- **Skeletal Playback**: ozz-animation integration
- **Animation Editing**: Keyframe manipulation

**Dependencies**:
- ozz-animation library
- AnimationProvider

#### Audio/MIDI Activities

**Sound Activity** (`src/Activities/Sound/`):
- Audio playback and editing
- Waveform visualization
- LabSound integration

**Joystick Activity** (`src/Activities/Joystick/`):
- MIDI controller input
- Joystick/gamepad support
- MIDI mapping

**Dependencies**:
- LabSound (audio engine)
- LabMidi/rtmidi (MIDI I/O)

#### 2D Graphics Activities

**Sprite Activity** (`src/Activities/Sprite/`):
- Sprite editing and animation
- Sprite sheet management

**Tilengine Activity** (`src/Activities/Tilengine/`):
- 2D tile-based graphics
- Tilemap editing

**Texture Activity** (`src/Activities/Texture/`):
- Texture inspection
- HDR texture tools
- OpenEXR support

**Dependencies**:
- Tilengine (2D engine)
- OpenEXR (HDR images)
- tex_inspect (texture viewer)

#### Developer Tools Activities

**Console Activity** (`src/Activities/Console/`):
- Interactive REPL
- Scheme scripting (s7)
- Command execution
- Uses imgui_console

**Node Graph Activity** (`src/Activities/NodeGraph/`):
- Visual node-based programming
- Graph editing
- Node execution

**Journal Activity** (`src/Activities/Journal/`):
- Transaction history visualization
- Undo/redo UI
- State inspection

**Preferences Activity** (`src/Activities/Preferences/`):
- User settings
- Configuration management

**Plugin Activity** (`src/Activities/Plugin/`):
- Plugin management UI
- Plugin loading/unloading
- Plugin browser

### Providers

#### ColorProvider (`src/CoreProviders/Color/`)

Manages color science data:
- Spectral power distributions (SPDs)
- Color matching functions (CMFs)
- Color space definitions
- Illuminant data (D65, A, etc.)
- Planckian locus calculations

**nanocolor Library**:
Custom color science implementation providing:
- CIE standard observer data
- Color space transformations
- Spectral calculations
- Perceptual metrics

#### OpenUSDProvider (`src/Providers/OpenUSD/`)

USD scene management:
- Stage creation and loading
- Prim traversal
- Attribute editing
- Hydra delegate management

#### SoundProvider (`src/Providers/Sound/`)

Audio engine wrapper:
- LabSound integration
- Audio graph management
- Playback control
- Audio file loading

#### MidiProvider (`src/Providers/Midi/`)

MIDI I/O management:
- MIDI input handling
- MIDI output
- Device enumeration
- Message routing

#### AssetProvider (`src/Providers/Assets/`)

Asset management system:
- Asset loading (images, models, audio)
- Asset caching
- Resource lifecycle

#### SchemeProvider (`src/Providers/Scheme/`)

Embedded Scheme interpreter:
- s7 Scheme implementation
- Script execution
- API bindings
- REPL support

#### AstroProvider (`src/Providers/Astro/`)

Astronomical calculations:
- Sun/moon position calculations
- Sky models
- Atmospheric scattering
- Celestial coordinates

## Design Patterns

### 1. Plugin Architecture

The entire system is built on plugin principles:
- **Activities**: UI plugins
- **Providers**: Service plugins
- **Studios**: Configuration plugins

### 2. Dependency Injection

The Orchestrator provides dependency injection:
```cpp
auto* colorProvider = orchestrator->getProvider<ColorProvider>();
```

### 3. Event-Driven Programming

CSP enables reactive programming:
```cpp
// Publish-subscribe pattern
csp_subscribe("event.name", callback);
csp_publish("event.name", data);
```

### 4. Immediate Mode UI

Using Dear ImGui for UI:
- No retained UI state
- Declarative UI code
- Efficient for tools and debugging

### 5. Auto-Registration

CMake automatically generates registration code:
- Activities are discovered and registered
- No manual registration required
- Reduces boilerplate

### 6. Transaction Pattern

State mutations are wrapped in transactions:
```cpp
transaction.begin();
// Make changes
transaction.commit(); // Or rollback()
```

## Data Flow

### Typical Data Flow

```
User Input
    ↓
Activity (ImGui)
    ↓
Provider API
    ↓
Provider State Change
    ↓
Transaction Recorded
    ↓
CSP Event Published
    ↓
Other Activities Subscribe
    ↓
UI Update
```

### Example: Color Change Flow

1. User adjusts color in **Color Activity**
2. Activity calls `ColorProvider::setColor()`
3. Provider wraps change in transaction
4. Transaction recorded in journal
5. Provider publishes `color.changed` event via CSP
6. Other Activities (e.g., Texture Activity) receive event
7. Activities update their UI to reflect new color

## Plugin System

### Dynamic Plugin Loading

LabRaven supports dynamic plugins via C ABI:

**Plugin Interface**:
```cpp
extern "C" {
    void plugin_init(Orchestrator* orch);
    void plugin_update(float dt);
    void plugin_shutdown();
}
```

**Plugin Discovery**:
- Plugins placed in `plugins/` directory
- Auto-discovered at startup
- Can be loaded/unloaded at runtime

**Example Plugin**: `examples/zig_plugin/`

### Code Signing (macOS)

macOS plugins require code signing:
- Automatic signing via `AppleCodeSigning.cmake`
- Ad-hoc signing for development
- Proper entitlements for loading

## Build System

### CMake Structure

```
CMakeLists.txt (root)
    ├── cmake-scripts/
    │   ├── MetalShaderSupport.cmake
    │   ├── AppleCodeSigning.cmake
    │   └── AppleMakeBundle.cmake
    └── src/
        ├── CMakeLists.txt
        ├── Activities/CMakeLists.txt
        ├── Providers/CMakeLists.txt
        └── Studios/CMakeLists.txt
```

### Auto-Generated Code

**RegisterAllActivities.cpp**:
CMake generates activity registration code automatically:

```cmake
set_property(TARGET MyActivity PROPERTY ACTIVITY_CLASS "MyActivity")
```

This generates:
```cpp
extern Activity* create_MyActivity();
registry["MyActivity"] = create_MyActivity;
```

### Metal Shader Compilation (macOS)

Metal shaders are compiled automatically:
- `.metal` files → `.metallib`
- Embedded in app bundle
- Loaded at runtime

### Platform-Specific Builds

#### macOS
- **Universal Binary**: arm64 + x86_64
- **Bundle**: Creates `.app` bundle
- **Code Signing**: Automatic ad-hoc signing
- **Metal**: Shader compilation
- **ARC**: Automatic Reference Counting enabled

#### Windows
- **DirectX 11**: Default backend
- **Unicode**: UNICODE/_UNICODE defined
- **MSVC**: Visual Studio 2022+

#### Linux
- **OpenGL**: Default backend
- **GCC/Clang**: Modern C++20 support

#### Emscripten
- **WebAssembly**: WASM output
- **SDL2**: Backend for web
- **Pthreads**: Threading support
- **SharedArrayBuffer**: For multithreading

## Platform Support

### Graphics Backends

- **OpenGL 3.0+**: Default for Windows/Linux
- **Metal**: Default for macOS
- **DirectX 11**: Optional for Windows

### Window Management

- **GLFW**: Cross-platform window/input
- **SDL2**: Emscripten/web
- **Native**: macOS NSWindow wrapper

### Threading

- **C++20 threads**: Standard threading
- **ZeroMQ**: Message passing
- **Emscripten pthreads**: Web workers

## Code Organization

### Directory Layout

```
src/
├── Lab/                    # Core framework
│   ├── App.h              # Application interface
│   ├── StudioCore.hpp     # Architecture definitions
│   ├── CSP.hpp            # CSP engine
│   ├── main_*.cpp/mm      # Platform entry points
│   ├── imgui/             # Dear ImGui library
│   ├── implot/            # ImPlot library
│   └── glfw/              # GLFW library
│
├── Activities/            # Activity modules
│   ├── Animation/
│   ├── App/
│   ├── Camera/
│   ├── Color/             # Color science
│   ├── Console/
│   ├── Journal/
│   ├── Joystick/
│   ├── NodeGraph/
│   ├── OpenUSD/
│   ├── Plugin/
│   ├── Preferences/
│   ├── Providers/
│   ├── Sound/
│   ├── Sprite/
│   ├── Texture/
│   ├── Tilengine/
│   └── Timeline/
│
├── Providers/             # Service providers
│   ├── Animation/
│   ├── Assets/
│   ├── Astro/
│   ├── Camera/
│   ├── Midi/
│   ├── OpenUSD/
│   ├── Scheme/
│   ├── Selection/
│   ├── Sound/
│   ├── Sprite/
│   └── Tilengine/
│
├── Studios/               # Studio configurations
│   ├── Color/
│   ├── OpenUSD/
│   ├── OpenUSDInsights/
│   └── Texture/
│
├── CoreProviders/         # Core providers
│   └── Color/
│
└── resources/             # Embedded resources
    └── fonts/
```

### Coding Conventions

- **C++20**: Modern C++ features
- **Header Files**: `.hpp` for C++-only, `.h` for C-compatible
- **Namespaces**: Generally avoided, rely on prefixes
- **Naming**: PascalCase for types, camelCase for functions
- **ImGui**: Immediate mode UI throughout

## Extending the System

### Creating a New Activity

1. **Create Directory**: `src/Activities/MyActivity/`

2. **Implement Activity**:
```cpp
// MyActivity.hpp
#include "Lab/StudioCore.hpp"

class MyActivity : public Activity {
public:
    void init(Orchestrator* orch) override;
    void update(float dt) override;
    void render() override;
    void shutdown() override;
};
```

3. **Add to CMake**: `src/Activities/CMakeLists.txt`
```cmake
add_subdirectory(MyActivity)
```

4. **Activity CMakeLists.txt**: `src/Activities/MyActivity/CMakeLists.txt`
```cmake
add_library(MyActivity STATIC
    MyActivity.cpp
)
target_link_libraries(MyActivity Lab)
set_property(TARGET MyActivity PROPERTY ACTIVITY_CLASS "MyActivity")
```

5. **Auto-Registration**: CMake generates registration automatically

### Creating a New Provider

1. **Create Directory**: `src/Providers/MyProvider/`

2. **Implement Provider**:
```cpp
// MyProvider.hpp
#include "Lab/StudioCore.hpp"

class MyProvider : public Provider {
public:
    void init(Orchestrator* orch) override;
    void update() override;
    void shutdown() override;

    // Provider-specific API
    void doSomething();
};
```

3. **Add to CMake**: `src/Providers/CMakeLists.txt`

4. **Register in Orchestrator**: Providers are typically registered at startup

### Creating a New Studio

1. **Create Directory**: `src/Studios/MyStudio/`

2. **Implement Studio**:
```cpp
// MyStudio.hpp
#include "Lab/StudioCore.hpp"

class MyStudio : public Studio {
public:
    void setup(Orchestrator* orch) override {
        // Initialize providers
        orch->getProvider<MyProvider>();

        // Activate activities
        orch->activateActivity<MyActivity>();
    }
};
```

3. **Add to CMake**: `src/Studios/CMakeLists.txt`

4. **Register Studio**: Studios are discovered and registered

### Adding a New Dependency

1. **Add to `ext/`**: Place dependency source/submodule

2. **Update root CMakeLists.txt**:
```cmake
add_subdirectory(ext/mydependency)
```

3. **Link in Activity/Provider**:
```cmake
target_link_libraries(MyActivity mydependency)
```

4. **Optional: Add CMake Option**:
```cmake
option(HAVE_MYDEPENDENCY "Enable MyDependency" ON)
```

## Testing Strategy

### Unit Testing

- Test Activities in isolation
- Mock Providers for testing
- Test Provider logic independently

### Integration Testing

- Test Studio configurations
- Test Activity interactions via CSP
- Test transaction rollback

### Visual Testing

- ImGui makes visual testing straightforward
- Screenshot comparison for UI tests
- Color accuracy tests using known standards

## Performance Considerations

### Immediate Mode UI

ImGui is efficient but requires:
- Minimize state queries per frame
- Cache expensive computations
- Use ImGui's ID stack properly

### Provider Caching

Providers should cache expensive computations:
- Color space transformations
- USD stage queries
- Audio buffer processing

### CSP Event Filtering

Subscribe to specific events, not all events:
```cpp
// Good
csp_subscribe("color.changed", callback);

// Bad
csp_subscribe("*", callback); // Too broad
```

### Transaction Overhead

Minimize transaction granularity:
- Batch related changes
- Don't create transactions for reads
- Consider transaction cost vs. benefit

## Security Considerations

### Plugin Loading

- Verify plugin signatures (macOS)
- Sandboxing for untrusted plugins
- API surface minimization

### Script Execution

- Scheme sandboxing
- Resource limits
- Safe API exposure

### Asset Loading

- Validate file formats
- Prevent path traversal
- Resource limits (memory, file size)

## Future Directions

### Planned Features

1. **Original Raven Timeline**: Reintroduce OTIO-based timeline as Activity
2. **More Studios**: Additional workspace configurations
3. **Plugin Marketplace**: Discover and install plugins
4. **Collaboration**: Multi-user support
5. **Scripting**: Enhanced scripting with Python/Lua

### Architectural Improvements

1. **Better Provider Discovery**: Dynamic provider loading
2. **Activity Presets**: Save/restore Activity configurations
3. **Studio Templates**: Pre-configured Studio templates
4. **Performance Profiling**: Built-in profiler Activity

## Conclusion

LabRaven's architecture provides a flexible, extensible framework for multimedia applications. The Studio-Activity-Provider pattern, combined with CSP-based event handling and a comprehensive transaction system, enables complex, composable workflows while maintaining clean separation of concerns.

The immediate mode UI approach with Dear ImGui allows for rapid development of tools and debugging interfaces, while the modular plugin architecture ensures that the system can grow organically without requiring extensive refactoring.

Whether you're building color science tools, USD authoring workflows, or custom multimedia applications, LabRaven provides a solid foundation with clear extension points and well-defined patterns.
