# DAW Project

A professional-grade Digital Audio Workstation built with JUCE, featuring AI-powered capabilities and ultra-hardened development standards.

## Overview

This project implements a zero-glitch, zero-crash DAW that prioritizes real-time audio performance, professional UX, and cross-platform consistency. The codebase follows strict development rules to ensure audio correctness, thread safety, and maintainability.

## Key Features

- **Real-Time Audio Engine**: Lock-free, allocation-free audio processing
- **AI Integration**: Background AI processing with graceful fallbacks
- **Professional UI**: Responsive, accessible, and fully dockable interface
- **Ultra UI (ImGui)**: Modern, GPU-accelerated interface with Dear ImGui + SDL2 + OpenGL
- **Premium Look & Feel**: Token-driven design system with flagship macro panels
- **Cross-Platform**: Windows, macOS, and Linux support with feature parity

## Ultra UI Preview

The cppmusic DAW features a modern ImGui-based interface designed for professional workflow:

### Features
- **Docking Layout**: Fully customizable panel arrangement with persistent layout
- **Command Palette**: Quick actions via `Ctrl+K` with fuzzy search
- **Theme System**: JSON-based tokens for colors, spacing, and typography
- **High-DPI Support**: Crisp rendering at any scale factor
- **Performance Overlay**: Real-time frame time and draw call monitoring (F12)

### Panels
- **Transport Bar**: Play/stop/record, BPM, time signature, CPU meter
- **Browser**: File tree with search and filter chips
- **Channel Rack**: Pattern step sequencer with velocity lane
- **Piano Roll**: MIDI editor with scale lock and velocity editing
- **Playlist**: Arrangement timeline with clip management
- **Mixer**: Channel strips with animated peak/RMS meters
- **Inspector**: Context-sensitive property editor

### Running the UI

```bash
# Build the ImGui UI application
cmake --build build --target cppmusic_imgui_app

# Run with default theme
./build/bin/cppmusic_imgui_app

# Run with custom theme
./build/bin/cppmusic_imgui_app --theme assets/themes/default.json

# Command line options
./build/bin/cppmusic_imgui_app --help
```

See [docs/ui/ultra_ui.md](docs/ui/ultra_ui.md) for architecture details.

## GODMODE UI System

The GODMODE UI upgrade provides enterprise-grade features for professional DAW development:

### Core Features

- **Reactive Data Binding**: Frame-coalesced `Signal<T>` system for efficient UI updates
- **Virtualized Views**: Handle 100k+ notes with stable 60fps scrolling
- **GPU Acceleration**: OpenGL-based waveform and meter rendering
- **Lua Scripting**: Sandboxed extension system for custom actions and panels
- **Diagnostics Overlay**: Live performance metrics and Chrome trace export

### Performance Targets

| Metric | Target |
|--------|--------|
| Mean frame time | < 4ms |
| P99 frame time | < 12ms |
| Large dataset scroll | 60fps stable |

### Documentation

- [GODMODE Overview](docs/ui/godmode_ui.md) - Architecture and rationale
- [Performance Budget](docs/ui/performance_budget.md) - Targets and measurement
- [Virtualization](docs/ui/virtualization.md) - Techniques and tuning
- [Lua Scripting](docs/ui/scripting.md) - API and examples
- [Theme Tokens](docs/ui/theme_tokens.md) - Token specification

### Build Options

```bash
# Enable Lua scripting (optional)
cmake -DENABLE_LUA_SCRIPTING=ON ..

# Run GODMODE tests
ctest --output-on-failure -R "Reactive|Lua"

# Run performance benchmarks
./build/tests/perf/benchmark_ui_pipeline
```

## Getting Started

### Prerequisites

- CMake 3.22 or higher
- C++20 compatible compiler (MSVC 2019+, GCC 10+, Clang 12+)
- JUCE framework (see [BUILDING.md](docs/BUILDING.md))

### Building

See [docs/BUILDING.md](docs/BUILDING.md) for detailed build instructions.

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Development

**CRITICAL**: All development work must follow the [DAW Development Rules](docs/DAW_DEV_RULES.md). These rules are mandatory and violations will result in code rejection.

### Quick Links

- [Development Rules](docs/DAW_DEV_RULES.md) - **MANDATORY READING**
- [Contributing Guidelines](docs/CONTRIBUTING.md)
- [Architecture Documentation](docs/ARCHITECTURE.md)
- [Build Instructions](docs/BUILDING.md)
- [Enforcement Guide](docs/ENFORCEMENT.md)
- [Development Roadmap](docs/ROADMAP.md)
- [Security Policy](docs/SECURITY.md)

### Key Principles

1. **No Glitches, No Pops**: Audio must never emit artifacts due to our code
2. **Real-Time First**: Audio correctness and timing > everything else
3. **Determinism**: Same project + same inputs = same output
4. **Cross-Platform Parity**: Feature parity across all supported platforms
5. **Professional UX**: Fast, predictable, undoable, and discoverable

## Project Structure

```
src/
  core/          # Core utilities, math, logging
  engine/        # Engine skeleton (AudioGraph, Transport)
  model/         # Model layer (Pattern, NoteEvent)
  audio/         # Audio engine, DSP, routing
  ai/            # AI models and inference
  project/       # Project model, tracks, automation
  ui/            # UI components and views
  platform/      # Platform-specific integration
tests/           # Unit and integration tests
  unit/          # Unit tests for engine/model
docs/            # Documentation
cmake/           # CMake modules and toolchain flags
```

## Testing

- Audio engine requires 80%+ code coverage
- All tests must pass before merging
- Real-time safety tests verify no allocations in audio thread
- Run tests with: `ctest --output-on-failure`

## Ubuntu Quick Start

### Install Dependencies

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    clang \
    cmake \
    ninja-build \
    pkg-config \
    libsndfile1-dev \
    libx11-dev \
    libxrandr-dev \
    libxcursor-dev \
    libxinerama-dev \
    libxext-dev \
    libfreetype6-dev \
    libcurl4-openssl-dev \
    libasound2-dev
```

### Build and Test

```bash
# Clone the repository
git clone <repository-url>
cd cppmusic

# Create build directory
mkdir build && cd build

# Configure (Release build)
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..

# Build
cmake --build .

# Run tests
ctest --output-on-failure
```

### Build with Sanitizers (Development)

```bash
# Debug build with AddressSanitizer
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON ..
cmake --build .
ctest --output-on-failure

# Debug build with UndefinedBehaviorSanitizer
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_UBSAN=ON ..
cmake --build .
ctest --output-on-failure
```

## License

[Add your license here]

## Contributing

Please read [CONTRIBUTING.md](docs/CONTRIBUTING.md) and [DAW_DEV_RULES.md](docs/DAW_DEV_RULES.md) before submitting pull requests.
