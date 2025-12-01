# GODMODE UI System

## Overview

The GODMODE UI system is a comprehensive upgrade to the cppmusic DAW user interface, implementing:

- **Reactive Data Binding**: Frame-coalesced signal system for efficient UI updates
- **Virtualized Views**: Efficient rendering for large note counts and complex arrangements
- **GPU Acceleration**: OpenGL-based waveform and meter rendering
- **Scripting Extensibility**: Lua sandbox for user-defined actions and panels
- **Accessibility**: Font scaling, high-contrast themes, keyboard navigation
- **Diagnostics**: Live performance metrics and trace export

## Architecture

### Core Systems

```
src/ui/
├── core/
│   ├── reactive/          # Signal<T> and frame coalescing
│   │   ├── Signal.hpp     # Base reactive signal
│   │   └── ParameterSignal.hpp  # DAW-specific signals
│   ├── input/
│   │   └── InputRouter.hpp  # Semantic action dispatch
│   ├── render/
│   │   ├── WaveformRenderer.hpp  # GPU waveform rendering
│   │   └── WaveformRenderer.cpp
│   ├── layout/
│   │   ├── LayoutSerializer.hpp  # Layout persistence
│   │   └── LayoutSerializer.cpp
│   └── diagnostics/
│       ├── DiagnosticsOverlay.hpp  # Live metrics
│       └── DiagnosticsOverlay.cpp
├── theme/
│   ├── ThemeManager.hpp   # Enhanced theme system
│   └── ThemeManager.cpp
├── script/
│   ├── LuaVM.hpp          # Lua scripting sandbox
│   └── LuaVM.cpp
└── i18n/
    ├── I18n.hpp           # Internationalization
    └── I18n.cpp
```

### Data Flow

```
Engine Thread                    UI Thread
     │                              │
     ▼                              │
  [Audio Processing]               │
     │                              │
     ▼                              │
  [Lock-Free Queue] ──────────────▶ [SignalAggregator]
                                    │
                                    ▼
                               [flush() - once per frame]
                                    │
                                    ▼
                               [Signal<T> subscribers]
                                    │
                                    ▼
                               [UI Panel Updates]
```

## Reactive Data Binding

### Signal<T>

The core reactive primitive with frame-coalesced flushing:

```cpp
#include "ui/core/reactive/Signal.hpp"

using namespace daw::ui::reactive;

// Create a signal
Signal<float> volumeSignal(0.8f);

// Subscribe to changes
auto subscription = volumeSignal.subscribe([](float value) {
    updateUI(value);
});

// Update from any thread (thread-safe)
volumeSignal.set(0.5f);

// Flush during frame update (UI thread only)
getGlobalAggregator().flush();
```

### Collection Signals

Specialized signals for notes, mixer channels, and clips:

```cpp
NoteCollectionSignal noteSignal;

// Add notes
noteSignal.addNote({.pitch = 60, .startBeats = 0, .lengthBeats = 1});

// Subscribe
auto sub = noteSignal.subscribe([](const auto& notes) {
    redrawPianoRoll(notes);
});

// Get visible notes (virtualized)
auto visible = noteSignal.getVisibleNotes(startBeat, endBeat, minPitch, maxPitch);
```

## Virtualized Views

The piano roll and playlist panels use virtualization to handle large datasets efficiently:

- **Spatial Queries**: Only query notes/clips within visible bounds
- **Adaptive Grid Thinning**: Reduce grid lines at lower zoom levels
- **Culling**: Skip rendering for off-screen elements

Target: 100+ patterns × 10,000+ notes each with stable 60fps scrolling.

## GPU Acceleration

### WaveformRenderer

Asynchronous waveform generation with mipmap levels:

```cpp
auto& renderer = getGlobalWaveformRenderer();
renderer.initialize();

// Generate waveform asynchronously
auto future = renderer.generateAsync("sample1", samples, sampleRate, channels);

// Render when ready
if (auto data = renderer.getCached("sample1")) {
    renderer.renderImGui(drawList, *data, x, y, width, height, startSample, endSample);
}
```

### MeterRenderer

Batched meter rendering with smoothing:

```cpp
auto& meters = getGlobalMeterRenderer();

// Update each frame
meters.updateMeter(0, peakL, peakR, rmsL, rmsR, deltaTime);

// Render
meters.renderImGui(drawList, 0, x, y, width, height, true);
```

## Input Router

Semantic action dispatch with gesture tracking:

```cpp
auto& router = getGlobalInputRouter();

// Register handlers
router.registerHandler(ActionType::DragNote, [](const InputAction& action) {
    moveNote(action.targetId, action.beatDelta, action.pitchPosition);
    return true;
});

// Dispatch actions
router.dispatch(action);
```

### Snap Settings

```cpp
auto& snap = router.getSnapSettings();
snap.enabled = true;
snap.division = 8;  // 1/8 note
snap.triplet = false;

double snapped = snap.snapBeat(3.7, 4.0);  // Snaps to nearest 1/8
```

## Lua Scripting

### Security Model

The Lua sandbox restricts:
- File I/O to extensions directory only
- No network access by default
- Instruction count limits (1M per call)
- Memory limits (16MB default)

### API

```lua
-- Register an action
register_action({
    id = "my.action",
    name = "My Action",
    category = "Custom",
    callback = function()
        log("info", "Action executed!")
    end
})

-- Create a panel
create_panel({
    id = "my.panel",
    title = "My Panel",
    draw = function()
        imgui.text("Hello from Lua!")
    end
})

-- Subscribe to parameters
subscribe_param("transport.bpm", function(value)
    log("debug", "BPM: " .. value)
end)

-- Get selection
local selection = get_selection()
local notes = selection:get_note_ids()
```

## Theme System

### Token Structure

```cpp
ThemeTokens tokens;
tokens.colors.primary = Color::fromHex("#4D80CC");
tokens.colors.bgPrimary = Color::fromHex("#14141A");
tokens.spacing.md = 16.0f;
tokens.radii.sm = 2.0f;
tokens.fontScale = 1.0f;
```

### Live Reload

```cpp
auto& theme = getGlobalThemeManager();
theme.loadFromFile("assets/themes/default.json");

// In update loop
if (theme.reloadIfModified()) {
    // Theme was hot-reloaded
}
```

## Accessibility

### Font Scaling

```cpp
auto& theme = getGlobalThemeManager();
theme.setFontScale(1.5f);  // 150% font size
```

### High Contrast Mode

```cpp
theme.setTheme("High Contrast");
```

### Keyboard Navigation

Tab order cycles through panels. Focus ring visible when navigating with keyboard.

## Diagnostics

### Live Metrics

Press F12 to toggle diagnostics overlay:
- Frame time (ms)
- FPS
- Draw calls
- Vertex count
- Audio thread occupancy
- Dirty signal count

### Trace Export

```cpp
auto& diag = getGlobalDiagnostics();
diag.startTraceCapture();
// ... perform operations ...
diag.stopTraceCapture();
diag.exportTrace("/tmp/trace.json");
// Open in chrome://tracing
```

## Layout Persistence

Layout saves automatically on change (2s debounce):

```cpp
auto& layout = getGlobalLayoutSerializer();
layout.enableAutosave("config/layout.json", 2000);

// Manual save
layout.saveNow(currentState);
```

## Build Configuration

Enable Lua scripting:

```cmake
cmake -DENABLE_LUA_SCRIPTING=ON ..
```

## Performance Targets

| Metric | Target |
|--------|--------|
| Mean frame time | < 4ms |
| 99th percentile | < 12ms |
| Large dataset scroll | 60fps stable |
| Virtualization overhead | < 1% margin |

## Future Work

- Vulkan rendering backend
- CRDT collaborative editing
- Plugin GUI embedding
- Harmonic analysis overlays
