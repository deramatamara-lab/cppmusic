# DAW Project

A professional-grade Digital Audio Workstation built with JUCE, featuring AI-powered capabilities and ultra-hardened development standards.

## Overview

This project implements a zero-glitch, zero-crash DAW that prioritizes real-time audio performance, professional UX, and cross-platform consistency. The codebase follows strict development rules to ensure audio correctness, thread safety, and maintainability.

## Graphical DAW Application

The `cppmusic-daw` executable provides a fully-featured FL Studio-style interface:

**FL-Style Workspace Layout:**
- **Top Transport Bar**: Play/stop/record, tempo control, metronome, snap settings
- **Left Browser Panel**: Collapsible file browser with tabs (Project, Samples, Presets)
- **Center Playlist**: Arrangement timeline with tracks and pattern clips
- **Bottom Channel Rack**: Step sequencer with pattern management
- **Mixer Panel**: Dockable mixer with peak/RMS meters, faders, and pan controls
- **Piano Roll**: MIDI note editor with velocity lane and AI-assisted composition

**Premium Design System:**
- Production-grade dark theme inspired by FL Studio, NI, and iZotope products
- Token-driven styling (colors, typography, spacing, animations) via JSON theme files
- 60 FPS smooth animations with physics-based easing functions (cubic, elastic, back)
- High-DPI support with vector icons and global UI scaling (100%-200%)
- Micro-interactions: hover/press animations, smooth value interpolation, animated panels
- Performance-optimized: zero allocations in paint, cached resources, < 8ms frame time

**Quick Start:**
```bash
# Build and run
cmake -B build -DENABLE_JUCE=ON
cmake --build build
./build/src/main/DAWProject
```

## Key Features

- **Real-Time Audio Engine**: Lock-free, allocation-free audio processing
- **AI Integration**: Background AI processing with graceful fallbacks
- **Professional UI**: Responsive, accessible, fully animated interface with tooltips and keyboard shortcuts
- **Ultra UI (ImGui)**: Modern, GPU-accelerated interface with Dear ImGui + SDL2 + OpenGL
- **Premium Look & Feel**: Production-grade design system with smooth animations and HiDPI support
- **Cross-Platform**: Windows, macOS, and Linux support with feature parity



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

### Feature Flags

The following feature flags control optional subsystems:

| Flag | Default | Description |
|------|---------|-------------|
| `ENABLE_GPU` | OFF | GPU DSP offloading (requires Vulkan) |
| `ENABLE_COLLAB` | OFF | Collaborative session features |
| `ENABLE_AI` | ON | AI-powered features (arrangement, tagging) |
| `ENABLE_SANDBOX` | OFF | Plugin sandboxing |
| `ENABLE_PERF_ADAPTIVE` | ON | Adaptive performance system |

Example with all features enabled:
```bash
cmake -DENABLE_GPU=ON -DENABLE_COLLAB=ON -DENABLE_SANDBOX=ON ..
```

## Development

**CRITICAL**: All development work must follow the [DAW Development Rules](docs/DAW_DEV_RULES.md). These rules are mandatory and violations will result in code rejection.

### Quick Links

- [Development Rules](docs/DAW_DEV_RULES.md) - **MANDATORY READING**
- [Mega Overview](docs/MEGA_OVERVIEW.md) - Advanced subsystems architecture
- [Contributing Guidelines](docs/CONTRIBUTING.md)
- [Architecture Documentation](docs/ARCHITECTURE.md)
- [Build Instructions](docs/BUILDING.md)
- [Enforcement Guide](docs/ENFORCEMENT.md)
- [Development Roadmap](docs/ROADMAP.md)
- [Security Policy](docs/SECURITY.md)

### Feature Documentation

- [Parameters & Modulation](docs/parameters.md)
- [Spectral Piano Roll](docs/pianoroll_spectral.md)
- [GPU Offload](docs/gpu_offload.md)
- [Automation Layers](docs/automation_layers.md)
- [Collaboration](docs/collaboration.md)
- [Asset Browser](docs/assets.md)
- [Adaptive Performance](docs/performance_adaptive.md)
- [Undo & Integrity](docs/undo_integrity.md)
- [AI Arrangement](docs/ai_arrangement.md)
- [Plugin Sandbox](docs/plugin_sandbox.md)

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
    parameters/  # Reactive parameter system
    automation/  # Automation clips and layers
    performance/ # Adaptive performance system
    concurrency/ # Lock-free async dispatcher
    dsp/         # DSP processing nodes
      offload/   # GPU offload framework
      nodes/     # FFT, convolution nodes
  model/         # Model layer (Pattern, NoteEvent)
    assets/      # Asset database
  audio/         # Audio engine, DSP, routing
  ai/            # AI models and inference
    harmony/     # Harmonic analysis
    tagging/     # Feature extraction and classification
    arrangement/ # Arrangement analysis and suggestions
  project/       # Project model, tracks, automation
  services/      # Background services
    undo/        # Undo service with delta compression
    integrity/   # State hash verification
    collab/      # Collaboration session handling
  platform/      # Platform-specific integration
    sandbox/     # Plugin sandboxing
    plugins/     # Plugin inspection
  ui/            # UI components and views
tests/           # Unit and integration tests
  unit/          # Unit tests for engine/model
  perf/          # Performance benchmarks
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
    libasound2-dev \
    libvulkan-dev \
    sqlite3 \
    libsqlite3-dev
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
