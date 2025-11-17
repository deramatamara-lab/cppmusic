# Architecture Documentation

## Overview

This document describes the architecture of the DAW project, following the hardened development rules.

## Module Organization

### Audio Module (`daw::audio`)

The audio module handles all real-time audio processing. It is divided into:

- **Processors**: Base classes for audio processing
- **Effects**: Audio effects (compressor, reverb, delay)
- **Synthesis**: Sound generation and synthesis

**Critical Requirements:**
- All `processBlock()` methods must be real-time safe
- No memory allocations in audio thread
- No locks or blocking operations
- Deterministic execution time

### UI Module (`daw::ui`)

The UI module provides the user interface components:

- **Components**: Reusable UI components
- **LookAndFeel**: Custom styling and design system

**Critical Requirements:**
- Maintain 60fps performance
- Responsive design for different screen sizes
- Accessibility support (keyboard navigation, screen readers)
- Consistent design system

#### Legacy Component Migration Roadmap

We are actively porting proven UI widgets from `oldbutgold/` into the modern `daw::ui` tree:

- **UltraWaveformVisualizer → `WaveformViewer` (DONE)**: migrated into `src/ui/components/WaveformViewer.*`, wired to the new design tokens, JUCE DSP, and the flagship layout.
- **StepSequencer (PLANNED)**: will become a dockable `PatternSequencerPanel` with Zustand-style state syncing. Needs: design-token pass, responsive grid, MIDI routing hooks, tests for pattern generation utilities.
- **ClipLauncherSystem (PLANNED)**: target is a `SessionLauncherView` living beside the flagship macro board. Tasks: split data model vs. presentation, add accessibility focus handling, expose scene/clip commands for automation.

Each migration follows the hardened workflow: audit legacy code, extract core logic into `daw::core` or `daw::audio` services where appropriate, wrap UI with the premium LookAndFeel, and add regression/unit tests before wiring into `MainComponent`.
### AI Module (`daw::ai`)

The AI module handles AI model integration:

- **Models**: AI model loading and management
- **Inference**: AI inference engine running on dedicated threads

**Critical Requirements:**
- AI operations NEVER on audio thread
- Async model loading
- Thread-safe inference queue
- Graceful error handling with fallbacks

### Core Module (`daw::core`)

The core module provides foundational utilities:

- **Utilities**: Math functions, logging, etc.
- **State**: Application state management

## Threading Model

1. **Audio Thread**: Real-time audio processing (highest priority)
2. **UI Thread**: User interface updates (JUCE MessageThread)
3. **AI Thread(s)**: AI inference processing (dedicated worker threads)

Communication between threads uses:
- Lock-free data structures (atomics, FIFOs) for audio thread
- Message passing for UI thread
- Thread-safe queues for AI thread

## Memory Management

- Smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- RAII for all resources
- Pre-allocated buffers for audio processing
- No manual memory management

## Build System

CMake-based build system with:
- Module-level CMakeLists.txt files
- Dependency management
- Test integration
- Cross-platform support

