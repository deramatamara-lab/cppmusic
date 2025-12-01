# Performance Budget

## Overview

The GODMODE UI system targets real-time performance suitable for professional DAW use. This document defines performance targets and measurement approaches.

## Frame Time Budget

| Category | Budget | Notes |
|----------|--------|-------|
| **Total Frame** | < 16.67ms | 60fps target |
| Signal Flush | < 0.5ms | Coalesced updates |
| Layout Pass | < 1.0ms | ImGui layout calculation |
| Render Pass | < 4.0ms | Draw commands + GPU submit |
| GPU Present | < 8.0ms | VSync dependent |
| **Reserve** | 3.0ms | Headroom for spikes |

### Mean vs P99 Targets

| Metric | Target |
|--------|--------|
| Mean frame time | < 4ms |
| P95 frame time | < 8ms |
| P99 frame time | < 12ms |
| Max spike | < 33ms (maintains 30fps minimum) |

## Memory Budget

| Component | Budget | Notes |
|-----------|--------|-------|
| Base UI | < 50MB | Core ImGui + theme |
| Waveform Cache | < 100MB | Mipmapped audio |
| Icon Atlas | < 10MB | Preloaded icons |
| Lua VM | < 16MB | Per-script sandbox |
| Layout Data | < 5MB | Panel state, history |
| **Total UI** | < 200MB | All UI systems |

## Draw Call Budget

| Scenario | Draw Calls | Vertices |
|----------|------------|----------|
| Idle UI | < 50 | < 10k |
| Piano Roll (1k notes) | < 100 | < 50k |
| Piano Roll (10k notes) | < 150 | < 100k |
| Mixer (32 channels) | < 100 | < 50k |
| Full DAW | < 300 | < 200k |

## Virtualization Efficiency

For virtualized views (piano roll, playlist):

| Dataset Size | Visible % | Draw Reduction |
|--------------|-----------|----------------|
| 1k notes | 20% | 5x |
| 10k notes | 2% | 50x |
| 100k notes | 0.2% | 500x |

Target: Drawn element count proportional to viewport, not total dataset.

## Audio Thread Impact

UI operations must never block the audio thread:

| Constraint | Limit |
|------------|-------|
| Shared mutex hold | < 100μs |
| Lock-free queue ops | < 10μs |
| Memory allocation | None (audio thread) |

## Measurement Approach

### Built-in Diagnostics

Toggle with F12:

```cpp
auto& diag = getGlobalDiagnostics();

// Per-frame metrics
diag.getCurrentStats().frameTimeMs;
diag.getCurrentStats().drawCalls;
diag.getAverageFrameTime();
diag.get99thPercentileFrameTime();
```

### Chrome Trace Export

Capture detailed timing:

```cpp
diag.startTraceCapture();
// Perform test scenario
diag.stopTraceCapture();
diag.exportTrace("trace.json");
// Open in chrome://tracing
```

### Scoped Profiling

Use macros for section timing:

```cpp
void renderPianoRoll() {
    DAW_PROFILE_SCOPE("PianoRoll");
    
    {
        DAW_PROFILE_SCOPE_CAT("Grid", "Render");
        drawGrid();
    }
    
    {
        DAW_PROFILE_SCOPE_CAT("Notes", "Render");
        drawNotes();
    }
}
```

### Automated Tests

```cpp
// tests/perf/BenchmarkUIPipeline.cpp

TEST_CASE("Frame time budget") {
    auto& app = getTestApp();
    
    std::vector<float> frameTimes;
    for (int i = 0; i < 500; ++i) {
        auto start = high_resolution_clock::now();
        app.renderFrame();
        auto end = high_resolution_clock::now();
        frameTimes.push_back(duration<float, milli>(end - start).count());
    }
    
    float mean = calculateMean(frameTimes);
    float p99 = calculatePercentile(frameTimes, 99);
    
    REQUIRE(mean < 4.0f);
    REQUIRE(p99 < 12.0f);
}
```

## Optimization Guidelines

### DO

- ✅ Use virtualization for large datasets
- ✅ Batch draw calls where possible
- ✅ Use mipmap levels for waveforms
- ✅ Coalesce signal updates per frame
- ✅ Cache computed values
- ✅ Use lock-free queues for cross-thread communication

### DON'T

- ❌ Allocate memory in render loop
- ❌ Hold locks while rendering
- ❌ Recalculate layout unnecessarily
- ❌ Load textures synchronously
- ❌ Block on file I/O in UI thread
- ❌ Create/destroy GPU resources per frame

## Regression Prevention

Performance tests run in CI:

```yaml
- name: Performance Tests
  run: |
    cmake --build build --target perf_tests
    ./build/bin/perf_tests --benchmark
```

Alerts trigger if:
- Mean frame time increases > 10%
- P99 frame time increases > 20%
- Memory usage increases > 15%

## Platform Considerations

### Ubuntu (Primary)

- OpenGL 3.3+ required
- X11/Wayland compositor overhead varies
- DPI scaling: 1.0, 1.25, 1.5, 2.0 validated

### Windows (Future)

- DirectX backend option
- DPI-aware per-monitor scaling

### macOS (Future)

- Metal backend option
- Retina display support
