# Development Rules Summary

## Overview

This document provides a concise summary of the mandatory development rules for the DAW project. For complete details, see [DAW_DEV_RULES.md](DAW_DEV_RULES.md).

**CRITICAL**: These rules are MANDATORY. Violations result in code rejection.

## Global Principles

**No Glitches, No Pops**: Audio must never emit pops/clicks. Non-recoverable issues must mute or bypass gracefully.

**Real-Time First**: Audio correctness and timing > everything else. No feature compromises real-time safety.

**Determinism**: Same project + same inputs = same output. No hidden randomness.

**Cross-Platform Parity**: macOS + Windows (and Linux) must have feature parity. Consistent UX.

**Professional UX**: Fast, predictable, undoable, discoverable. "Feels like a toy" = bug.

## JUCE/C++ Core Standards

### Memory Management (CRITICAL)
- **NO** raw `new`/`delete`/`malloc`/`free`
- Use RAII and smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- Use JUCE containers (`OwnedArray`, `Array`)
- Enforcement: `-Werror`, clang-tidy, code review

### Thread Safety (CRITICAL)
- Audio thread must NEVER block or allocate
- No locks, waits, or heap operations in `processBlock()`
- Use atomics and lock-free structures
- Example: `std::atomic<float> gain { 1.0f };`

### C++ Standards
- C++17 minimum, C++20 preferred
- Use `auto`, `constexpr`, `noexcept`, `[[nodiscard]]`
- Structured bindings, `std::optional`, `std::variant` where appropriate

### Exception Handling
- Audio thread: logically `noexcept`, no throwing
- UI/IO threads: handle exceptions, show user-friendly messages

## Audio Processing Rules

### Real-Time Thread Safety (CRITICAL)
- `processBlock()` must finish within time budget
- **NO** heap allocation, locks, file/network/console I/O
- **NO** logging to disk (only lock-free ring buffer if needed)

### Sample Rate & Buffer Size
- Fully dynamic, no hardcoded values
- Handle any sample rate and buffer size

### Latency & Delay Compensation
- Report correct latency, keep it deterministic
- Automation accounts for latency

### DSP Stability
- Numerically stable, denormal-safe, clamped
- Handle NaN/Inf, avoid catastrophic cancellation

### Plugin Formats
- Use JUCE abstractions, no format-specific hacks
- Must pass validation tools (pluginval)
- Identical behavior: offline render vs live, different buffer sizes

### Automation & Modulation
- Sample-accurate where possible, always artifact-free
- Smooth fast changes (no zipper noise)

## AI Integration Standards

### No AI on Audio Thread (CRITICAL)
- AI runs on background workers only
- Audio thread only reads results via atomics/lock-free queues
- Hard rule: `processBlock()` call stack must never call AI

### Model Lifecycle
- Models loaded on background threads
- Progress and failure surfaced to UI
- Large allocations done once and reused

### Inference Threading
- Dedicated inference worker pool
- Bounded queues (e.g., 4 items max)
- Backpressure: drop oldest or decline new requests

### Failure & Fallback
- On AI failure: audio continues with non-AI fallback
- Non-intrusive warning in UI (status bar)
- No modal that blocks playback

### Performance
- Targets: P95 < 50 ms (clip-level), < 200 ms (track-level)
- If exceeded: degrade quality or apply approximation

## UI/UX Professional Standards

### LookAndFeel & Design System
- Single, centralized design system
- One global `CustomLookAndFeel`
- `DesignSystem` namespace: Colors, Typography, Spacing, Radii, Shadows
- **NO** magic numbers in paint/layout

### Layout & Responsiveness
- Fully resizable, dockable, remembers layout
- Main areas: Transport, Arrangement, Mixer, Browser, Inspector
- No fixed pixel layouts

### Input Model & Shortcuts
- Fully usable via keyboard + mouse
- Unified input manager: every command has ID, shortcut, menu entry
- Common: Space (Play/Stop), Ctrl/Cmd+Z/Y (Undo/Redo), Ctrl/Cmd+S (Save)

### Accessibility
- Keyboard focus traversal works everywhere
- High-contrast theme supported, font sizes scalable
- Accessible names/descriptions for controls

### UI Performance (60fps Target)
- Dirty-rect repainting: `repaint(region)` not `repaint()`
- Cache static visuals, no allocations in `paint()`
- Heavy layout only on resize/structural changes

### DAW-Specific UX
- **Timeline**: Drag to create/move/trim clips, snap to grid, smooth zoom
- **Mixer**: Consistent channel strips, large faders/meters
- **Editors**: Multi-tool model, crisp visuals under zoom
- **Undo/Redo**: Every operation undoable, semantic undo steps

## Code Quality & Architecture

### SOLID Principles
- No "god classes" mixing audio engine + UI + IO
- Single responsibility, clear interfaces

### Layered Architecture
- Clear layers: Core → Audio Engine → AI & Analysis → Project Model → UI → Integration
- Lower layers never depend on higher layers
- UI only talks to engine/model via well-defined interfaces

### Module & Namespace Layout
- Structure: `src/core/`, `src/audio/engine/`, `src/audio/dsp/`, `src/project/`, `src/ui/`, `src/ai/`
- Namespaces: `daw::core`, `daw::audio::engine`, `daw::ui::views`, etc.

### Documentation
- All public classes + methods: Doxygen-style comments
- Complex components: "How to use" section

### Testing
- No merging critical engine code without tests
- Coverage: Audio engine + project model: 80%+ required

## Performance Requirements

### Audio Thread Budget (CRITICAL)
- 48kHz, 128 samples: < 1.5 ms per buffer
- 48kHz, 256 samples: < 3.0 ms per buffer
- 96kHz, 128 samples: < 0.8 ms per buffer
- No single plugin/node > 30% of budget

### Multi-Core & Graph Scheduling
- Audio graph designed for parallel execution
- Identify independent subgraphs, avoid unnecessary serialization

### UI Performance Budget
- Paint operations: no dynamic allocations, no cache thrash
- Timeline and mixer: responsive at 60fps during zoom/scroll

### AI Performance Budget
- Real-time assist: < 50 ms perceived delay
- Clip analysis: < few seconds for typical song
- Background jobs never block playback/UI/save/load

### Profiling & Regression
- Regular benchmarks on reference machines
- Detect regressions: > 10% performance drop = review required

## Security & Privacy

### Plugin Isolation
- Host third-party plugins with extreme suspicion
- Validate formats, sandboxing where possible
- Plugin crash must not crash DAW

### File System & Network
- Project files: clear structured format with versioning
- Cloud/collaboration: HTTPS only, credentials in OS keychain
- No secrets in logs

### Telemetry & Analytics
- Opt-in and transparent
- No audio content uploaded without explicit consent

### Crash Handling
- Crash handler: captures stack trace, offers to save report
- Project autosave: configurable interval

## Build & Deployment

### Toolchain
- Single CMake project: standalone app, VST3, AU (AAX if licensed)
- Compiler warnings: `-Wall -Wextra -Wpedantic` (or MSVC equivalents)
- Treat warnings as errors

### Target Matrix
- Windows 10/11 x64, macOS (current major -1 and newer, Intel + Apple Silicon)
- CI must: build all formats on all platforms, run tests + plugin validation

## Enforcement & Tooling

### Static Analysis & Formatting
- clang-tidy with real-time rules
- clang-format for code style
- All checks run in CI and locally via pre-commit hooks

### Mandatory Code Review Checklist
Before merge, confirm:
- No raw owning pointers
- No locks/allocations in audio thread
- C++17+ features used sensibly
- Public APIs documented
- Tests written and passing, coverage OK
- UI uses design system (no magic values)
- Layout responsive and non-janky
- Thread safety sound
- Error paths and fallbacks implemented
- No secrets or file paths hardcoded

### AI Dev / Assistant Guardrails
- Never change real-time audio code to add logs/allocations/locks
- Prefer small, local changes over massive rewrites
- For new features: update design system (if UI), add tests (if engine/model), update docs (if public API)
- If in doubt: choose stability over "cool feature"

## Quick Reference

### Audio Thread: DO
- Use atomics for parameters, pre-allocate buffers
- Use lock-free structures, keep processing deterministic

### Audio Thread: DON'T
- Allocate memory, use locks, block or wait
- Call AI or file I/O, throw exceptions

### UI Thread: DO
- Use design system tokens, cache resources
- Dirty-rect repainting, handle exceptions gracefully

### UI Thread: DON'T
- Block audio thread, allocate in `paint()`
- Use magic numbers, create platform-specific hacks

## Enforcement

- **CI**: Automated checks (lint, tidy, tests, pluginval)
- **Code Review**: Mandatory checklist verification
- **Static Analysis**: Real-time rules enforcement
- **Coverage**: Minimum thresholds enforced

**Remember**: These rules are not suggestions. They are mandatory requirements for production code.
