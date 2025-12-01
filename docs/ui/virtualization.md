# Virtualization

## Overview

Virtualization techniques enable the cppmusic DAW to handle large datasets efficiently. This document covers the implementation of virtualized views in the piano roll, playlist, and other panels.

## Problem Statement

Without virtualization, rendering performance degrades linearly with dataset size:

| Notes | Naive Render Time |
|-------|-------------------|
| 1,000 | ~2ms |
| 10,000 | ~20ms |
| 100,000 | ~200ms (impossible at 60fps) |

With virtualization, render time stays constant regardless of dataset size.

## Core Concepts

### Viewport Culling

Only render elements within the visible viewport:

```cpp
// Get visible bounds in beat/pitch space
double startBeat = scrollX / pixelsPerBeat;
double endBeat = (scrollX + viewportWidth) / pixelsPerBeat;
int minPitch = 127 - int((scrollY + viewportHeight) / noteHeight);
int maxPitch = 127 - int(scrollY / noteHeight);

// Query only visible notes
auto visibleNotes = noteSignal.getVisibleNotes(startBeat, endBeat, minPitch, maxPitch);
```

### Spatial Indexing

For datasets > 10k notes, use spatial indexing:

```cpp
// R-tree or grid-based index
class SpatialIndex {
public:
    void insert(const NoteEvent& note);
    void remove(uint32_t noteId);
    std::vector<NoteEvent> query(double startBeat, double endBeat,
                                  int minPitch, int maxPitch);
};
```

### Adaptive Grid Thinning

Reduce grid line density at lower zoom levels:

```cpp
int getGridSubdivision(float pixelsPerBeat) {
    if (pixelsPerBeat < 8) return 1;   // Bar lines only
    if (pixelsPerBeat < 16) return 4;  // Quarter notes
    if (pixelsPerBeat < 32) return 8;  // Eighth notes
    if (pixelsPerBeat < 64) return 16; // Sixteenth notes
    return 32;  // Full resolution
}
```

## Implementation Details

### Piano Roll Virtualization

```cpp
void PianoRollPanel::drawNotes(const Theme& theme) {
    // 1. Calculate visible bounds
    ImVec2 canvasMin = ImGui::GetWindowContentRegionMin();
    ImVec2 canvasMax = ImGui::GetWindowContentRegionMax();
    
    double viewStartBeat = scrollX_ / (pixelsPerBeat_ * zoomX_);
    double viewEndBeat = viewStartBeat + (canvasMax.x - canvasMin.x) / 
                         (pixelsPerBeat_ * zoomX_);
    
    int viewTopPitch = yToPitch(canvasMin.y);
    int viewBottomPitch = yToPitch(canvasMax.y);
    
    // 2. Query visible notes only
    auto visibleNotes = noteSignal_.getVisibleNotes(
        viewStartBeat, viewEndBeat, viewBottomPitch, viewTopPitch);
    
    // 3. Record for diagnostics
    getGlobalDiagnostics().setVisibleNotes(visibleNotes.size());
    
    // 4. Render visible notes
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    for (const auto& note : visibleNotes) {
        drawNote(drawList, note, theme);
    }
}
```

### Playlist Virtualization

```cpp
void PlaylistPanel::drawClips(const Theme& theme) {
    // Calculate visible track range
    int topTrack = int(scrollY_ / trackHeight_);
    int bottomTrack = topTrack + int(viewportHeight_ / trackHeight_) + 1;
    
    // Calculate visible time range
    double startBeat = scrollX_ / pixelsPerBeat_;
    double endBeat = startBeat + viewportWidth_ / pixelsPerBeat_;
    
    // Query visible clips
    auto visibleClips = clipSignal_.getVisibleClips(
        startBeat, endBeat, topTrack, bottomTrack);
    
    // Render
    for (const auto& clip : visibleClips) {
        drawClip(drawList, clip, theme);
    }
}
```

### Waveform Virtualization

Use mipmap levels based on zoom:

```cpp
void WaveformRenderer::renderImGui(...) {
    // Calculate samples per pixel
    int64_t sampleRange = endSample - startSample;
    int samplesPerPixel = sampleRange / width;
    
    // Select appropriate mipmap level
    const WaveformMipmap* mipmap = data.getMipmapForScale(samplesPerPixel);
    
    // Render from mipmap (constant time regardless of total samples)
    for (int px = 0; px < width; ++px) {
        int64_t idx = mapPixelToMipmapIndex(px, ...);
        float min = mipmap->minPeaks[idx];
        float max = mipmap->maxPeaks[idx];
        drawWaveformColumn(px, min, max);
    }
}
```

## Performance Characteristics

### Time Complexity

| Operation | Naive | Virtualized |
|-----------|-------|-------------|
| Render N notes | O(N) | O(V) where V = visible |
| Query visible | N/A | O(log N + V) with spatial index |
| Scroll/Pan | O(N) | O(V) |
| Zoom | O(N) | O(V) |

### Space Complexity

| Structure | Size |
|-----------|------|
| Note collection | O(N) |
| Spatial index | O(N) |
| Visible cache | O(V) |
| Waveform mipmaps | O(N) total, O(1) per render |

## Testing Virtualization

### Correctness Test

```cpp
TEST_CASE("Virtualization shows correct note count") {
    NoteCollectionSignal notes;
    
    // Add 10,000 notes across 100 bars
    for (int i = 0; i < 10000; ++i) {
        notes.addNote({
            .pitch = rand() % 128,
            .startBeats = (rand() % 400),
            .lengthBeats = 0.5
        });
    }
    notes.flush();
    
    // Query 4-bar window
    auto visible = notes.getVisibleNotes(0, 16, 0, 127);
    
    // Should see roughly 10000 * (16/400) = 400 notes
    REQUIRE(visible.size() > 350);
    REQUIRE(visible.size() < 450);
    
    // All returned notes should be in bounds
    for (const auto& note : visible) {
        REQUIRE(note.startBeats < 16);
        REQUIRE(note.endBeats() > 0);
    }
}
```

### Performance Test

```cpp
TEST_CASE("Virtualization maintains constant render time") {
    std::vector<int> noteCounts = {1000, 10000, 100000};
    std::vector<float> renderTimes;
    
    for (int count : noteCounts) {
        setupNotes(count);
        
        auto start = high_resolution_clock::now();
        for (int i = 0; i < 100; ++i) {
            auto visible = notes.getVisibleNotes(0, 16, 60, 72);
            renderNotes(visible);
        }
        auto end = high_resolution_clock::now();
        
        float avgTime = duration<float, milli>(end - start).count() / 100;
        renderTimes.push_back(avgTime);
    }
    
    // Times should be similar regardless of dataset size
    // Allow 50% variance due to cache effects
    float ratio = renderTimes[2] / renderTimes[0];
    REQUIRE(ratio < 1.5f);
}
```

## Tuning Parameters

| Parameter | Default | Notes |
|-----------|---------|-------|
| Viewport padding | 10% | Extra notes loaded for smooth scrolling |
| Grid thinning thresholds | 8/16/32/64 px | Adjust per display DPI |
| Mipmap levels | 11 | Powers of 2: 1-1024 samples/pixel |
| Spatial index cell size | 4 bars × 12 semitones | Balance query vs memory |

## Known Limitations

1. **Initial Load**: First query after data change may be slower while index rebuilds
2. **Selection Across Views**: Selected items outside viewport still tracked but not rendered
3. **Waveform Generation**: Initial mipmap generation is O(N) but cached
4. **Memory Overhead**: Spatial index adds ~10-20% memory overhead

## Future Improvements

- GPU-based spatial queries
- Progressive loading for very large projects
- Predictive pre-fetching based on scroll velocity
- LOD for note rendering at extreme zoom levels
