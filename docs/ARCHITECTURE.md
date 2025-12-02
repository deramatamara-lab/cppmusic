# Architecture Documentation

## Overview

This document describes the comprehensive architecture of the CPPMusic DAW project, following ultra-hardened development rules. The system is designed as a layered, modular architecture that prioritizes real-time audio performance, thread safety, maintainability, and cutting-edge audio processing capabilities.

**Latest Updates**: The architecture has been significantly enhanced with advanced DSP processors, neural inference engines, professional animation systems, comprehensive performance monitoring, and studio-grade audio components.

## System Architecture

### Layered Architecture

The DAW follows a strict layered architecture with clear separation of concerns:

```
Integration Layer (App, MainWindow, Platform)
    ↓
UI Layer (Components, Views, LookAndFeel)
    ↓
Project Model Layer (Tracks, Clips, Automation, Undo)
    ↓
AI & Analysis Layer (AI Models, Inference, Analyzers)
    ↓
Audio Engine Layer (Graph, Routing, DSP Nodes, Transport)
    ↓
Core Layer (Math, Utilities, Logging, Job System)
```

**Critical Constraint**: Lower layers must never depend on higher layers. UI only communicates with engine/model via well-defined interfaces.

## Module Organization

### Core Module (`cppmusic::core`)
**Location**: `src/core/`  
**Purpose**: Foundational utilities and infrastructure with advanced real-time capabilities.  
**Components**: 
- **Engine System**: `EngineContext` (unified engine interface), `RTMemoryPool` (real-time memory management)
- **Threading**: `JobSystem` (worker thread pool), `RealtimeMessageQueue` (lock-free messaging)
- **Utilities**: Lock-free collections (`LockFreeQueue`, `LockFreeStack`, `LockFreeHashMap`), `MathUtils`
- **Performance**: `BenchmarkSystem`, `PerformanceMonitor`, `CPUMonitor`, `MemoryTracker`
- **Infrastructure**: `Logger`, atomic operations, wait-free algorithms

**Key Classes**: `EngineContext`, `RTMemoryPool`, `RealtimeMessageQueue`, `JobSystem`, `BenchmarkSystem`, `MathUtils`.  
**Thread Safety**: All components designed for lock-free audio thread communication with comprehensive real-time safety guarantees.

### Audio Engine Module (`daw::audio::engine`)
**Location**: `src/audio/engine/`  
**Purpose**: Real-time audio processing and transport control.  
**Components**: `DawEngine` (main coordinator), `Transport` (playback state), `AudioGraph` (processing graph), `EngineContext` (UI facade).  
**Real-Time Safety**: All `processBlock()` methods are noexcept, allocation-free, and lock-free. Parameter updates use atomics or lock-free queues.  
**Threading**: Audio thread processes blocks, UI thread controls via EngineContext, communication via lock-free atomics.

### DSP Module (`daw::audio::dsp`)
**Location**: `src/audio/dsp/`  
**Purpose**: Audio processing units and DSP primitives.  
**Components**: `TrackStrip` (gain/pan/mute/solo/metering), `Envelope` (ADSR), `LFO` (modulation).  
**Real-Time Safety**: Inline processing, allocation-free, numerically stable with denormal handling.

### Effects Module (`daw::audio::effects`)
**Location**: `src/audio/effects/`  
**Purpose**: Audio effects processing (Compressor, Reverb, Delay).  
**Real-Time Safety**: Pre-allocated buffers, no allocations in processBlock(), stable algorithms.

### Synthesis Module (`daw::audio::synthesis`)
**Location**: `src/audio/synthesis/`  
**Purpose**: Sound generation (Oscillator, Synthesizer).  
**Real-Time Safety**: All synthesis is real-time safe with pre-allocated state.

### Project Model Module (`daw::project`)
**Location**: `src/project/`  
**Purpose**: Project data model and persistence.  
**Components**: `ProjectModel` (container), `Track` (track representation), `Clip` (clip representation), `SelectionModel` (selection state), `ProjectSerializer` (save/load).  
**Thread Safety**: Accessed from UI thread, synchronized with engine via EngineContext.

### UI Module (`daw::ui`)
**Location**: `src/ui/`  
**Purpose**: User interface components and views.  
**Components**: `App` (application lifecycle), `MainWindow` (main window), `MainView` (root view), `ArrangeView` (timeline), `MixerView` (mixer), `TransportBar` (transport controls), `BrowserPanel` (library), `InspectorPanel` (property editor).  
**Design System**: Centralized DesignSystem tokens (colors, typography, spacing). No magic numbers.  
**Performance**: 60fps target, dirty-rect repainting, cached visuals, no allocations in paint().

#### FL-Style DAW UI Architecture

The UI implements a pattern-oriented DAW workflow inspired by FL Studio's channel rack/playlist/mixer paradigm:

**Core UI Architecture (`src/ui/core/`):**
- `AppState`: Central non-audio state management (view state, theme, zoom, snap settings)
- `Commands`: Command IDs and keyboard mappings for transport, views, editing
- `AnimationHelper`: Animation utilities for smooth UI transitions

**Style/Look & Feel (`src/ui/style/`, `src/ui/lookandfeel/`):**
- `CppMusicLookAndFeel`: Premium NI/iZotope-grade aesthetic
- `UltraDesignSystem`: Token-driven design system (colors, typography, spacing)
- Uniform rounding, shadows, and gradients for professional appearance

**Main Views (`src/ui/views/`):**
- `MainView`: Root layout manager with resizable panels
- `TransportBar`: Play/stop/record, tempo, metronome, snap settings
- `ArrangeView`: Playlist/timeline with tracks and pattern clips
- `PianoRollView`: MIDI note editor with velocity lane
- `MixerView`: Horizontal mixer strips with meters and faders
- `BrowserPanel`: Collapsible browser with tabs (Project, Samples, Presets)

**Components (`src/ui/components/`):**
- `PatternSequencerPanel`: Channel rack / step sequencer
- `SessionLauncherView`: Session view for clip launching
- `DrumMachine`: Drum pattern programming
- `StatusStrip`: Project name, CPU meter, status indicators

**UI/Engine Boundary:**
- UI interacts with engine/model via thread-safe controller objects
- State changes dispatched via lock-free message queues
- Transport controls map to engine Transport methods
- Playhead position updated via timer callbacks

### AI Module (`daw::ai`)
**Location**: `src/ai/`  
**Purpose**: AI model integration and inference.  
**Components**: `AIModelManager` (model management), `InferenceEngine` (background inference), `AIProcessor` (result processing).  
**Models**: `ChordAdvisor`, `MelodyGenerator`, `GrooveExtractor`.  
**Thread Safety**: AI runs ONLY on background threads. Audio thread never calls AI. Results via lock-free queues.

### Plugins Module (`daw::plugins`)
**Location**: `src/plugins/`  
**Purpose**: Third-party plugin hosting (`PluginHost`, `PluginDatabase`).  
**Isolation**: Plugins run in separate processes where possible. Crashes don't crash DAW.

## Threading Model

### Thread Hierarchy

1. **Audio Thread** (Highest Priority): Real-time audio processing, never blocks/allocates/locks, uses atomics and lock-free structures.
2. **UI Thread** (JUCE MessageThread): All UI updates, can block (file I/O, dialogs), communicates via EngineContext.
3. **AI Thread(s)** (Background Workers): AI model loading and inference, never blocks audio/UI, bounded queues.
4. **Job System Threads** (Background Workers): General background tasks (file I/O, analysis).

### Inter-Thread Communication

- **Audio ↔ UI**: Lock-free atomics, lock-free queues
- **AI ↔ Audio**: Lock-free queues (AI → Audio only)
- **AI ↔ UI**: Message passing, callbacks
- **Project ↔ Engine**: EngineContext (thread-safe facade)

## Data Flow

### Playback Flow
```
AudioDeviceManager → DawEngine::processBlock() → AudioGraph::processBlock() 
→ TrackStrip::processBlock() (per track) → Effects/Plugins → Master Bus → Output
```

### UI Update Flow
```
User Interaction (UI) → EngineContext → DawEngine (atomics) 
→ Audio Thread (reads atomics) → Metering Data (lock-free queue) → UI (updates meters)
```

### AI Processing Flow
```
UI Request → InferenceEngine (Background) → AI Model → Results (Lock-Free Queue) 
→ Audio Thread (if needed) / UI Thread (updates UI)
```

## Memory Management

### Principles
- **RAII**: All resources managed via RAII
- **Smart Pointers**: `std::unique_ptr`, `std::shared_ptr` for ownership
- **JUCE Containers**: `OwnedArray`, `Array` for JUCE types
- **No Raw Pointers**: No `new`/`delete` in production code

### Audio Thread Memory
- **Pre-allocated Buffers**: All buffers allocated in `prepareToPlay()`
- **Stack Allocation**: Small temporary buffers on stack
- **No Heap**: Zero heap allocations in `processBlock()`

### UI Memory
- **Component Ownership**: Components owned by parent, no manual delete
- **Cached Resources**: Images, fonts, paths cached and reused
- **Smart Pointers**: UI components use smart pointers for dynamic allocation

## Build System

### CMake Structure
- **Root CMakeLists.txt**: Main project configuration
- **Module CMakeLists.txt**: Each module has its own CMakeLists.txt
- **JUCE Integration**: Uses `cmake/JUCE.cmake` for JUCE setup
- **Tests**: Separate test targets, linked to main modules

### Build Targets
- **Standalone App**: Main application
- **VST3 Plugin**: Plugin format (if configured)
- **AU Plugin**: macOS plugin format (if configured)
- **Tests**: Unit and integration tests

## Testing Strategy

### Test Organization
- **Unit Tests**: Per-module tests in `tests/` directory
- **Audio Tests**: Real-time safety tests, allocation checks
- **Project Tests**: Save/load roundtrip, undo/redo
- **UI Tests**: Layout and state snapshot tests

### Coverage Requirements
- **Audio Engine**: 80%+ line coverage required
- **Project Model**: 80%+ line coverage required
- **CI Enforcement**: Tests must pass, coverage must meet threshold

## Performance Targets

### Audio Thread
- **48kHz, 128 samples**: < 1.5 ms per buffer
- **48kHz, 256 samples**: < 3.0 ms per buffer
- **96kHz, 128 samples**: < 0.8 ms per buffer

### UI Thread
- **60fps**: Smooth UI updates
- **Paint Operations**: No allocations, cached resources
- **Layout**: Responsive, no jank during resize

### AI Thread
- **Clip Analysis**: < few seconds for typical song
- **Real-Time Assist**: < 50 ms perceived delay
- **Background Jobs**: Never block playback or UI

## Error Handling

### Audio Thread
- **No Exceptions**: All audio code is noexcept
- **Graceful Degradation**: Mute or bypass on error
- **No Crashes**: Never crash user's system

### UI Thread
- **Exception Handling**: Catch and display user-friendly messages
- **Error States**: Visual feedback for errors
- **Recovery**: Allow user to recover from errors

### AI Thread
- **Fallback**: Non-AI fallback on failure
- **Non-Intrusive**: Warnings in status bar, no modals during playback

## Platform Support

### Supported Platforms
- **Windows 10/11**: x64
- **macOS**: Current major -1 and newer (Intel + Apple Silicon)
- **Linux**: Supported where JUCE supports it

### Platform-Specific Code
- **Guarded**: Platform-specific code guarded with `#ifdef`
- **Documented**: All platform-specific behavior documented
- **Parity**: Feature parity across platforms where possible

## Future Extensions

### Planned Features
- **Plugin Sandboxing**: Separate process for unstable plugins
- **Cloud Collaboration**: Secure cloud sync (HTTPS only)
- **Advanced AI**: More AI models and features
- **Workspace System**: Saved layouts ("Mix", "Arrange", "Edit", "Live")

### Migration Roadmap
- **Legacy Components**: Migrating `oldbutgold/` components to modern architecture
- **Design System**: Expanding design tokens and components
- **Performance**: Continuous profiling and optimization

## Enhanced Systems (2024 Upgrades)

### Advanced DSP Processing

#### AnalogModeledEQ (`cppmusic::audio::AnalogModeledEQ`)
**Location**: `src/audio/AnalogModeledEQ.h` (single translation unit)
**Purpose**: Professional 5-band parametric EQ with vintage analog hardware emulation
**Key Features**:
- **RBJ Filter Cookbook Implementation**: Industry-standard biquad filters (peaking, shelving, highpass, lowpass)
- **Analog Modeling**: Multiple vintage hardware models (Neve, SSL, Tube preamps) with transformer pre/post filtering
- **Saturation Processing**: Lookup table-based tube and tape saturation with configurable drive amounts
- **Optional 2x Oversampling**: Anti-aliasing with Butterworth filters for ultra-high quality processing
- **Per-Band Controls**: Frequency (20Hz-20kHz), gain (±20dB), Q (0.1-10), drive, saturation, mix
- **Real-Time Safety**: Zero heap allocations, lock-free parameter updates, numerical stability with denormal handling
- **JUCE Unit Tests**: Embedded unit tests validate filter responses, stability, and performance

#### EqualizerService (`cppmusic::integration::EqualizerService`)
**Location**: `src/integration/EqualizerService.h/.cpp`
**Purpose**: Integration layer connecting EQ audio processing with UI and EngineContext
**Key Features**:
- **Real-Time Parameter Messaging**: Lock-free queues for audio thread parameter updates
- **Automation Support**: Full parameter automation with DAW integration
- **MIDI Control**: CC-to-parameter mapping with smooth interpolation
- **A/B Preset Management**: Quick preset comparison and management
- **Performance Metrics**: Real-time CPU usage, processing time, and quality metrics
- **Professional UI Integration**: Connects to AnalogEQEditor for studio-ready interface

#### AnalogEQEditor (`cppmusic::ui::AnalogEQEditor`)
**Location**: `src/ui/AnalogEQEditor.h/.cpp`
**Purpose**: Professional studio-grade UI for the AnalogModeledEQ
**Key Features**:
- **Per-Band Controls**: Rotary knobs for frequency, gain, Q, drive, saturation, mix with real-time visual feedback
- **Frequency Response Plot**: Real-time frequency response visualization with interactive band selection
- **A/B Preset System**: Quick preset comparison with visual indicators and smooth transitions
- **Professional Styling**: Analog-inspired design with studio-grade visual feedback and metering
- **Real-Time Updates**: Smooth parameter interpolation with 60fps refresh rate
- **JUCE Component Integration**: Full JUCE component architecture with proper lifecycle management

### Neural Inference Engine

#### Advanced AI Integration (`cppmusic::ai`)
**Location**: `src/ai/`
**Purpose**: Cutting-edge AI model integration for music analysis and generation
**Key Components**:
- **InferenceEngine**: Background AI processing with GPU acceleration support
- **Model Management**: Dynamic model loading, caching, and version management
- **TensorFlow Integration**: Native TensorFlow Lite integration for mobile-optimized models
- **ONNX Support**: Cross-platform model format support for maximum compatibility
- **Thread Pool**: Dedicated AI worker threads with priority scheduling
- **Result Processing**: Lock-free result queues for real-time integration

**AI Models**:
- **ChordProgressionAnalyzer**: Advanced harmonic analysis with context awareness
- **MelodyGenerator**: Contextual melody generation based on harmonic analysis
- **GrooveExtractor**: Rhythm and groove pattern analysis and synthesis
- **StyleTransfer**: Musical style transfer between different genres and artists
- **MasteringAssistant**: AI-powered mastering suggestions and processing

### Professional Animation System

#### ProfessionalAnimationEngine (`cppmusic::ui::animation`)
**Location**: `src/ui/animation/ProfessionalAnimationEngine.h/.cpp`
**Purpose**: High-performance animation system targeting 120fps with physics-based motion
**Key Features**:
- **Advanced Easing**: 15+ easing curves including elastic, bounce, and custom bezier curves
- **Physics Integration**: Spring-damper systems, momentum, and realistic motion dynamics
- **Performance Optimization**: Adaptive frame rate, dirty region tracking, SIMD acceleration
- **Design System Integration**: Animation tokens, consistent timing, and motion design patterns
- **Event System**: Comprehensive animation lifecycle events and callback system
- **120fps Target**: Ultra-smooth motion for professional audio applications

#### AdaptiveAnimationManager (`cppmusic::ui::AdaptiveAnimationManager`)
**Location**: `src/ui/AdaptiveAnimationManager.h/.cpp`
**Purpose**: Intelligent animation management with adaptive performance
**Key Features**:
- **Performance Monitoring**: Real-time frame rate monitoring and adaptive quality adjustment
- **Animation Pooling**: Efficient animation object reuse and memory management
- **Batch Processing**: Optimized batch animation updates for complex UIs
- **Motion Blur**: Optional motion blur for ultra-smooth visual feedback
- **Accessibility**: Respects user motion preferences and accessibility settings

### Performance Monitoring & CI/CD

#### BenchmarkSystem (`cppmusic::performance::BenchmarkSystem`)
**Location**: `src/core/performance/BenchmarkSystem.h/.cpp`
**Purpose**: Comprehensive performance analysis and regression detection
**Key Features**:
- **High-Resolution Timing**: Nanosecond precision timing with statistical analysis
- **CPU Profiling**: Detailed CPU usage tracking with call stack analysis
- **Memory Tracking**: Allocation tracking, leak detection, and peak usage monitoring
- **Automated Regression Detection**: Statistical analysis of performance changes over time
- **Multi-Format Reports**: JSON, CSV, HTML dashboard generation
- **CI/CD Integration**: GitHub Actions integration with automated performance testing

#### AudioProcessingBenchmarks (`cppmusic::performance::AudioBenchmarks`)
**Location**: `src/core/performance/AudioBenchmarks.h/.cpp`
**Purpose**: Specialized benchmarks for audio processing components
**Key Features**:
- **EQ Performance Analysis**: Detailed AnalogEQ performance profiling across configurations
- **Latency Measurement**: Precise audio latency measurement and jitter analysis
- **Load Testing**: Voice polyphony and CPU usage load testing
- **SIMD Optimization Verification**: Performance comparison of scalar vs SIMD implementations
- **Real-Time Safety Validation**: Verification of real-time processing constraints

#### GitHub Actions CI/CD Pipeline
**Location**: `.github/workflows/performance.yml`
**Purpose**: Automated performance testing and regression detection
**Key Features**:
- **Multi-Configuration Testing**: Tests across different sample rates, buffer sizes, and quality settings
- **Regression Analysis**: Automated comparison against baseline performance metrics
- **Memory Leak Detection**: Valgrind integration for comprehensive memory analysis
- **Performance Profiling**: CPU profiling with perf and cache analysis
- **Dashboard Generation**: Automated HTML dashboard generation with interactive charts
- **PR Comments**: Automatic performance analysis comments on pull requests

### Lock-Free Utilities & Real-Time Safety

#### Advanced Lock-Free Collections (`cppmusic::core`)
**Location**: `src/core/utilities/`
**Purpose**: High-performance lock-free data structures for real-time audio
**Components**:
- **LockFreeQueue**: Michael & Scott lock-free queue with ABA protection
- **LockFreeStack**: Treiber stack with hazard pointers
- **LockFreeHashMap**: Optimistic lock-free hash table for concurrent access
- **RealtimeMessageQueue**: Specialized queue for audio thread communication
- **AtomicRingBuffer**: Single-producer/single-consumer ring buffer

**Real-Time Guarantees**:
- **Wait-Free Operations**: Bounded execution time for audio thread operations
- **Memory Ordering**: Explicit memory ordering for all atomic operations
- **ABA Prevention**: Hazard pointers and epoch-based memory management
- **Cache Optimization**: Cache-friendly data layout and access patterns

#### RTMemoryPool (`cppmusic::core::RTMemoryPool`)
**Location**: `src/core/RTMemoryPool.h`
**Purpose**: Real-time safe memory management for audio processing
**Key Features**:
- **Pre-Allocated Pools**: Fixed-size memory pools allocated at initialization
- **O(1) Allocation**: Constant-time allocation and deallocation
- **Fragmentation Prevention**: Intelligent pool sizing and defragmentation
- **Thread Safety**: Lock-free allocation suitable for audio thread
- **Debug Support**: Allocation tracking and leak detection in debug builds

### Integration Architecture

#### Comprehensive Module Integration
**Pattern**: All new systems follow consistent integration patterns:
- **Engine Integration**: All audio components integrate via EngineContext
- **UI Integration**: All UI components follow the design system and animation patterns
- **Performance Integration**: All components include performance monitoring and benchmarking
- **Thread Safety**: All components respect the threading model and real-time constraints
- **Testing Integration**: Comprehensive unit tests, integration tests, and performance benchmarks

#### CMake Build System Enhancement
**Improvements**:
- **Modular Targets**: Each subsystem has its own CMake target with proper dependencies
- **Performance Flags**: Optimized compiler flags for each component type
- **Test Integration**: Automated test discovery and execution
- **Documentation**: Generated API documentation integration
- **Continuous Integration**: Full CI/CD pipeline with performance regression detection

## Performance Specifications

### Audio Processing Performance
- **AnalogEQ Processing**: < 0.5ms per block at 48kHz/512 samples with 5 bands enabled
- **AI Inference**: < 50ms for chord analysis, < 200ms for melody generation
- **Animation Rendering**: 120fps target with < 8ms frame times
- **Memory Usage**: < 100MB baseline, < 500MB with large projects loaded
- **CPU Usage**: < 25% CPU usage for typical mixing scenarios

### Real-Time Safety Validation
- **Zero Heap Allocations**: All audio thread operations are allocation-free
- **Bounded Execution Time**: All audio operations have deterministic maximum execution time
- **Lock-Free Communication**: No blocking operations in audio thread
- **Numerical Stability**: Comprehensive denormal handling and range checking
- **Exception Safety**: All audio code is noexcept with graceful error handling

### Continuous Integration Metrics
- **Build Time**: < 10 minutes for full build and test suite
- **Test Coverage**: > 80% line coverage for all audio and core components
- **Performance Regression Detection**: Automated detection of > 5% performance degradation
- **Memory Leak Detection**: Zero memory leaks in comprehensive test scenarios
- **Cross-Platform Validation**: Automated testing on Windows, macOS, and Linux

## Phase 1 Foundational Architecture

### Engine Skeleton (`cppmusic::engine`)

**Location**: `src/engine/`

The engine skeleton provides the foundational audio graph and transport infrastructure:

#### AudioGraph
- **Node Registration**: Register/unregister `AudioNode` instances with unique IDs
- **Edge Connections**: Connect node output ports to input ports
- **Topology Rebuild**: Topological sort using Kahn's algorithm for cycle detection
- **Block Processing**: Process nodes in sorted order (placeholder for DSP)

#### Transport
- **Sample-Accurate Position**: Beat and sample position tracking with atomics
- **Tempo Control**: BPM setting with clamping (20-999 BPM)
- **Time Signature**: Configurable numerator/denominator
- **Thread Safety**: All state accessed via `std::atomic` for lock-free audio thread access

### Model Layer (`cppmusic::model`)

**Location**: `src/model/`

The model layer provides pattern and note event storage:

#### Pattern
- **Note Storage**: Vector of `NoteEvent` structs (pitch, velocity, start, duration, channel)
- **Length Computation**: `computeContentLength()` returns max of pattern length or last note end
- **Range Queries**: `getNotesInRange()` for playback and display
- **Automatic Sorting**: Notes sorted by start beat on insertion

## Scheduling & Patterns

### Pattern Playback Model
- Patterns are first-class containers of MIDI note events
- Each pattern has a defined length in beats
- Patterns can be placed on tracks at specific positions
- Looping and one-shot playback modes supported

### Timing Resolution
- Internal timing in double-precision beats
- Sample-accurate scheduling via Transport
- Sub-beat precision for humanization and micro-timing

### Quantization
- Grid-based quantization (1/4, 1/8, 1/16, etc.)
- Swing and groove templates (planned)
- Non-destructive quantization with original timing preserved

## Mixer Routing Plan

### Signal Flow
```
Track Sources (Patterns, Audio Clips)
    ↓
Track Strip (Gain, Pan, Mute, Solo)
    ↓
Insert Effects (per-track)
    ↓
Send/Return Buses
    ↓
Group Buses (submix)
    ↓
Master Bus (final processing)
    ↓
Audio Output
```

### Bus Types
- **Track Buses**: One per track, mono or stereo
- **Aux Buses**: For sends/returns (reverb, delay)
- **Group Buses**: Submix multiple tracks (drums, vocals)
- **Master Bus**: Final output processing

### Routing Flexibility
- Pre-fader and post-fader sends
- Sidechain routing for compressors/gates
- Multi-output instruments (planned)

## Automation Concept

### Automation Lanes
- One or more automation lanes per track
- Each lane controls a single parameter
- Parameters can be any exposed track/plugin value

### Automation Data
- Stored as time-stamped control points
- Interpolation modes: linear, curved, stepped
- Sample-accurate playback via lock-free queues

### Automation Recording
- Touch: Write while touching control
- Latch: Write until stop
- Write: Overwrite existing automation
- Read: Playback only

## Plugin Hosting Roadmap

### Phase 1 (Current): No Plugin Hosting
- Internal effects and instruments only
- Focus on core engine stability

### Phase 2: LV2 Support
- Linux-native plugin format
- Process isolation via separate processes
- IPC for audio/MIDI data

### Phase 3: VST3 Support
- Cross-platform plugin format
- Steinberg VST3 SDK integration
- GUI hosting in separate window

### Plugin Sandboxing
- Each plugin in isolated process
- Crash recovery without host restart
- CPU/memory resource limits
- Timeout detection for stalled plugins

## Persistence Format

### Project File Structure
```
project.dawproj/
  ├── project.json       # Main project metadata
  ├── tracks/            # Track data
  │   ├── track-001.json
  │   └── track-002.json
  ├── patterns/          # Pattern data
  │   ├── pattern-001.json
  │   └── pattern-002.json
  ├── automation/        # Automation data
  │   └── auto-001.json
  └── assets/            # Audio files, samples
      └── audio/
```

### Serialization
- JSON for human-readable metadata
- Binary for audio waveform data
- Version field for migration compatibility
- Checksum for integrity verification

### Undo/Redo Persistence
- Command pattern for all edits
- Undo stack persisted in project
- Redo stack cleared on new action

## Concurrency Model

### Thread Hierarchy

| Thread | Priority | Purpose | Constraints |
|--------|----------|---------|-------------|
| Audio | Highest | `processBlock()` | No alloc, no lock, no exceptions |
| MIDI | High | MIDI event processing | Bounded latency |
| UI | Normal | User interface | Can block for I/O |
| AI | Low | Background inference | Never block audio |
| I/O | Low | File operations | Fully async |

### Communication Patterns
- **Audio ↔ UI**: Lock-free ring buffers, atomics
- **Audio ↔ MIDI**: Lock-free queues
- **UI ↔ AI**: Async callbacks, message passing
- **UI ↔ I/O**: Async file operations with callbacks

### Synchronization Primitives
- `std::atomic<T>` for simple values
- Lock-free queues for message passing
- No mutexes in audio thread
- Memory ordering explicit (`acquire`/`release`)

## Extensibility

### Plugin API (Future)
- C++ plugin interface for internal extensions
- Versioned API with backward compatibility
- Plugin discovery and loading system

### Scripting (Future)
- Lua or JavaScript scripting for automation
- Script access to project model (read-only during playback)
- User-defined MIDI transformations

### Custom DSP Nodes
- `AudioNode` base class for custom processors
- Register nodes with `AudioGraph`
- Hot-reload support (planned)

## Hardening

### Build-Time Hardening
- `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnull-dereference`
- `-Werror` in CI (warnings as errors)
- `-fstack-protector-strong` for stack protection
- `-D_FORTIFY_SOURCE=2` for runtime buffer checks
- `-fPIE -pie` for ASLR
- `-Wl,-z,relro,-z,now` for GOT protection

### Runtime Hardening
- Address Sanitizer (ASAN) in CI
- UndefinedBehavior Sanitizer (UBSAN) in CI
- No raw `new`/`delete` in production code
- RAII for all resource management
- Bounds checking in debug builds

### Audio Thread Safety
- Static analysis for allocation detection
- Runtime allocation tracking (debug builds)
- `noexcept` on all audio thread functions
- Graceful degradation on error (mute, bypass)

This enhanced architecture represents a significant evolution of the CPPMusic DAW, incorporating cutting-edge audio processing, AI integration, professional animation systems, and comprehensive performance monitoring - making it truly "JAW DROPPING" in its capabilities and engineering excellence.
