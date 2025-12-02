# Development Roadmap

This document outlines the phased development plan for the cppmusic DAW project.

## Phase 1: Core Foundations (Current)

**Status: In Progress**

- [x] Project structure and CMake build system
- [x] C++20 standard configuration
- [x] Basic JUCE integration
- [x] Engine skeleton (AudioGraph, Transport)
- [x] Model layer (Pattern, NoteEvent)
- [x] Initial unit test harness (CTest)
- [x] CI pipeline with sanitizers
- [x] Security hardening flags
- [x] Documentation (ARCHITECTURE, ROADMAP, CONTRIBUTING, SECURITY)
- [ ] Audio device enumeration and selection
- [ ] Basic MIDI input handling
- [ ] File I/O for project persistence

## Phase 2: Mixer & Automation

**Status: Planned**

- [ ] Track strip DSP (gain, pan, mute, solo)
- [ ] Mixer bus routing architecture
- [ ] Automation curves and lanes
- [ ] Parameter smoothing for real-time control
- [ ] Send/return effects routing
- [ ] Master bus processing
- [ ] Metering (peak, RMS, LUFS)
- [ ] Offline bounce/export

## Phase 3: UI Prototype – FL-Style Layout

**Status: Complete**

The first fully functional JUCE-based UI implementing a pattern-oriented DAW workflow:

- [x] Main window layout (arrange view, mixer view)
- [x] Top transport bar with play/stop/record, tempo, metronome
- [x] FL-style workspace layout:
  - [x] Left browser panel (collapsible)
  - [x] Center playlist/arrangement view
  - [x] Bottom channel rack / step sequencer
  - [x] Mixer panel (dockable/toggleable)
- [x] Track header components
- [x] Clip/region display on timeline with drag & drop
- [x] Piano roll editor with velocity lane
- [x] Mixer strip UI with peak/RMS meters
- [x] Browser panel for samples/presets
- [x] Inspector panel for properties
- [x] Keyboard shortcuts and command system (F5-F9 view switching)
- [x] Central design system (NI/iZotope-grade aesthetic)
- [x] AppState for centralized UI state management
- [x] Premium Look & Feel with dark theme

### Follow-up Tasks (After Phase 3)
- [x] UI-Engine integration layer (PlaybackController, ProjectController)
- [x] Built-in demo projects for quick-start experience
- [ ] Topological sort & cycle detection in AudioGraph with UI feedback
- [ ] Audio device abstraction & configuration UI
- [ ] Visual refinement & micro-animations
- [ ] Accessibility & keyboard-only navigation
- [ ] Performance profiling & rendering optimization
- [ ] Playlist playhead sync with timer-based position updates
- [ ] Pattern playback from Channel Rack to audio engine
- [ ] Mixer routing with fader/pan parameter wiring

## Phase 3.5: Engine Integration (Current)

**Status: In Progress**

Advanced UI-engine integration building on Phase 3:

- [x] PlaybackController for thread-safe transport control
  - [x] Play/stop/loop/seek methods
  - [x] Lock-free state snapshots
  - [x] Listener interface for UI updates
- [x] ProjectController for project model management
  - [x] Pattern/track/clip helper methods
  - [x] Active pattern tracking for piano roll
  - [x] Change notification system
- [x] Built-in demo projects (DemoProjects module)
  - [x] Simple demo (4-channel drum + bass)
  - [x] Advanced demo (8-channel multi-pattern)
  - [x] Minimal demo for testing
- [ ] Playlist playhead sync
  - [ ] Timer-based position updates from PlaybackController
  - [ ] Seek to beats on timeline click
- [ ] Pattern playback integration
  - [ ] Wire pattern events to audio engine
  - [ ] Real-time note triggering from patterns
- [ ] Mixer integration
  - [ ] Fader/pan controls to engine parameters
  - [ ] Meter display from engine

## Phase 4: Plugin & AI Integration

**Status: In Progress**

- [x] AI model integration infrastructure
- [x] Chord progression advisor
- [x] Melody generation suggestions
- [x] Groove and rhythm analysis
- [x] Background AI processing with lock-free queues
- [ ] Plugin hosting framework (LV2/VST3)
- [ ] Plugin sandboxing for crash isolation
- [ ] Plugin parameter automation

## Phase 5: Performance & Packaging

**Status: Planned**

- [ ] SIMD optimization for DSP routines
- [ ] Multi-threaded audio graph processing
- [ ] Memory pool optimizations
- [ ] CPU load profiling and optimization
- [ ] Linux AppImage packaging
- [ ] Installation and update mechanisms
- [ ] Crash reporting and diagnostics
- [ ] Performance regression testing in CI

## Phase 6: Advanced Features

**Status: Planned**

- [ ] Sample editor with waveform display
- [ ] Time-stretching and pitch-shifting
- [ ] Audio-to-MIDI conversion
- [ ] Collaboration features (optional)
- [ ] Cloud project sync (optional)
- [ ] Custom scripting/macro system
- [ ] Advanced MIDI editing (velocity, CC)
- [ ] Notation view (optional)

---

## Milestone Definitions

### MVP (Minimum Viable Product)
- Phases 1-3 complete
- Basic recording and playback
- Simple mixing capabilities
- Usable UI for arrangement

### Production Ready
- Phases 1-5 complete
- Plugin hosting operational
- AI features integrated
- Packaged for distribution

### Feature Complete
- All phases complete
- Full feature set as designed
- Performance optimized
- Documentation complete

---

## Version Targets

| Version | Phase | Description |
|---------|-------|-------------|
| 0.1.0   | 1     | Core foundations, engine skeleton |
| 0.2.0   | 2     | Mixer and automation |
| 0.3.0   | 3     | UI prototype |
| 0.5.0   | 4     | Plugin and AI integration |
| 0.8.0   | 5     | Performance and packaging |
| 1.0.0   | 6     | Feature complete release |

---

## Contributing to the Roadmap

Feature requests and priority adjustments can be discussed via GitHub Issues. Please reference the relevant phase when proposing changes.
