# DAW Project

A professional-grade Digital Audio Workstation built with JUCE, featuring AI-powered capabilities and ultra-hardened development standards.

## Overview

This project implements a zero-glitch, zero-crash DAW that prioritizes real-time audio performance, professional UX, and cross-platform consistency. The codebase follows strict development rules to ensure audio correctness, thread safety, and maintainability.

## Key Features

- **Real-Time Audio Engine**: Lock-free, allocation-free audio processing
- **AI Integration**: Background AI processing with graceful fallbacks
- **Professional UI**: Responsive, accessible, and fully dockable interface
- **Premium Look & Feel**: Token-driven JUCE design system with flagship macro panels
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

## Development

**CRITICAL**: All development work must follow the [DAW Development Rules](docs/DAW_DEV_RULES.md). These rules are mandatory and violations will result in code rejection.

### Quick Links

- [Development Rules](docs/DAW_DEV_RULES.md) - **MANDATORY READING**
- [Contributing Guidelines](docs/CONTRIBUTING.md)
- [Architecture Documentation](docs/ARCHITECTURE.md)
- [Build Instructions](docs/BUILDING.md)
- [Enforcement Guide](docs/ENFORCEMENT.md)

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
  audio/         # Audio engine, DSP, routing
  ai/            # AI models and inference
  project/       # Project model, tracks, automation
  ui/            # UI components and views
  platform/      # Platform-specific integration
tests/           # Unit and integration tests
docs/            # Documentation
```

## Testing

- Audio engine requires 80%+ code coverage
- All tests must pass before merging
- Real-time safety tests verify no allocations in audio thread

## License

[Add your license here]

## Contributing

Please read [CONTRIBUTING.md](docs/CONTRIBUTING.md) and [DAW_DEV_RULES.md](docs/DAW_DEV_RULES.md) before submitting pull requests.
