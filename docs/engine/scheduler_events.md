# Scheduler and Events

Sample-accurate scheduling infrastructure for the cppmusic DAW engine.

## Overview

The Scheduler provides:
- Sample-accurate beat-to-frame conversion
- Tempo and time signature handling
- Polymeter support (independent pattern lengths)
- Warp transformations (piecewise linear time mapping)

## Beat-to-Frame Conversion

### Formulas

```
samples_per_beat = (60 / tempo_bpm) * sample_rate
frames = beats * samples_per_beat
beats = frames / samples_per_beat
```

### Example at 120 BPM, 44100 Hz

| Beats | Frames |
|-------|--------|
| 0.25  | 5,512  |
| 0.5   | 11,025 |
| 1.0   | 22,050 |
| 4.0   | 88,200 |

## API Reference

### Scheduler Class

```cpp
class Scheduler {
public:
    // Configuration
    void setSampleRate(double sampleRate);
    double getSampleRate() const;
    void setTempo(double bpm);  // 20-999 BPM
    double getTempo() const;
    void setTimeSignature(int numerator, int denominator);
    
    // Beat/Frame Conversion
    int64_t beatsToFrames(double beats) const;
    double framesToBeats(int64_t frames) const;
    double getSamplesPerBeat() const;
    double getSamplesPerBar() const;
    
    // Polymeter Support
    static double getPatternBeat(double globalBeat, double patternLengthBeats);
    static uint32_t getLoopIteration(double globalBeat, double patternLengthBeats);
    
    // Micro-Timing
    static int64_t applyMicroTiming(int64_t framePosition, int32_t microOffset);
};
```

## Polymeter Support

Patterns can have independent lengths from the global bar structure:

```cpp
// 7-beat pattern against 4/4 global time
double patternLength = 7.0;
double globalBeat = 15.0;

// Get position within pattern (looped)
double patternBeat = Scheduler::getPatternBeat(globalBeat, patternLength);
// Result: 1.0 (15 % 7 = 1)

// Get loop iteration
uint32_t iteration = Scheduler::getLoopIteration(globalBeat, patternLength);
// Result: 2 (15 / 7 = 2)
```

## Probability and Conditions

Notes can have conditional triggering:

### Condition Types

```cpp
enum class NoteCondition : uint8_t {
    Always = 0,    // Always play
    FirstOnly,     // Play only on first loop
    Nth,           // Play on every Nth iteration
    EveryN,        // Play every N iterations starting from 0
    SkipM,         // Skip first M iterations
    Random         // Random based on probability
};
```

### Probability Evaluation

```cpp
// Deterministic evaluation using seeded RNG
bool shouldPlay = Pattern::evaluateNoteCondition(
    note,           // The note to evaluate
    loopIteration,  // Current loop iteration (0-indexed)
    seed            // Random seed for determinism
);
```

The same seed produces the same result, ensuring playback is reproducible.

### Examples

```cpp
NoteEvent note;
note.condition = NoteCondition::EveryN;
note.conditionParam = 2;  // Play on iterations 0, 2, 4, 6...
note.probability = 0.75f; // 75% chance when condition passes
```

## Slide/Portamento

Notes can slide to a target pitch:

```cpp
NoteEvent note;
note.slideMode = SlideMode::Portamento;
note.slideTime = 0.25f;  // Slide over 1/4 beat
note.slideToPitch = 2;   // Slide up 2 semitones
```

### Slide Modes

| Mode | Description |
|------|-------------|
| None | No slide |
| Legato | Voice-level legato glide |
| Portamento | Per-note portamento |
| MPEPitchBend | MPE-style pitch bend |

## Micro-Timing

Sub-tick timing adjustments in samples:

```cpp
NoteEvent note;
note.microTimingOffset = 50;  // 50 samples late
note.microTimingOffset = -30; // 30 samples early

// Apply to frame position
int64_t framePos = scheduler.beatsToFrames(note.startBeat);
int64_t adjustedPos = Scheduler::applyMicroTiming(framePos, note.microTimingOffset);
```

## Swing

### Pattern-Level Swing

```cpp
Pattern pattern;
pattern.setSwingAmount(0.5f);      // 50% swing
pattern.setSwingResolution(0.5);   // Swing on 8th notes
```

### Per-Note Swing Override

```cpp
NoteEvent note;
note.swingAmount = -0.3f;  // Override pattern swing
// 0.0 means use pattern swing, non-zero overrides
```

### Swing Calculation

Swing affects off-beat notes (odd positions in the swing grid):

```cpp
double adjustedBeat = pattern.getSwingAdjustedBeat(note);
// Off-beat notes are shifted by swing_amount * swing_resolution * 0.5
```

## Warp Transformations

Non-linear time mapping using piecewise linear interpolation.

### WarpMap Class

```cpp
class WarpMap {
public:
    void addMarker(const WarpMarker& marker);
    bool removeMarker(size_t index);
    void clearMarkers();
    
    bool isActive() const;  // Requires at least 2 markers
    
    // Time mapping
    double sourceToTarget(double sourceBeat) const;
    double targetToSource(double targetBeat) const;
};
```

### WarpMarker

```cpp
struct WarpMarker {
    double sourceBeat;  // Position in source timeline
    double targetBeat;  // Position in warped timeline
};
```

### Example: 2x Time Stretch

```cpp
WarpMap warpMap;
warpMap.addMarker({0.0, 0.0});   // Start aligned
warpMap.addMarker({8.0, 16.0}); // 8 beats -> 16 beats (half speed)

double targetBeat = warpMap.sourceToTarget(4.0);
// Result: 8.0 (source beat 4 maps to target beat 8)
```

### Example: Piecewise Tempo Change

```cpp
WarpMap warpMap;
warpMap.addMarker({0.0, 0.0});
warpMap.addMarker({4.0, 2.0});  // First 4 beats at 2x speed
warpMap.addMarker({8.0, 8.0});  // Next 4 beats at 0.67x speed
```

## Thread Safety

The Scheduler class is designed for real-time audio:
- Non-copyable to prevent accidental state duplication
- All methods are inline for performance
- No allocations in hot paths
