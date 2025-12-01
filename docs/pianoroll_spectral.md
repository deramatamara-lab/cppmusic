# Spectral Piano Roll & Harmonic Analyzer

This document describes the spectral overlay methodology, harmonic tension metrics, and chord labeling approach.

## Overview

The spectral piano roll extends the traditional MIDI editor with:
- Real-time spectral visualization overlaid on notes
- Harmonic tension metric for chord progressions
- Automatic chord labeling and scale degree display
- Pitch class distribution histogram

## Spectral Overlay Methodology

### Visualization Layers

```
┌───────────────────────────────────────────────────────────────┐
│ Piano Roll with Spectral Overlay                              │
├───────────────────────────────────────────────────────────────┤
│  C5 ─────────────────────────────────────────────────────────│
│  B4 ─────────────────────────────────────────────────────────│
│  A4 ─────░░░░░░░░░░░░░░░░░────────────────────░░░░░░░░░░░───│ ← Spectral
│  G4 ─────▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓────────────────────▓▓▓▓▓▓▓▓▓▓▓───│ ← intensity
│  F4 ─────────────────────────────────────────────────────────│
│  E4 ─────────░░░░░░░░░░░░────────────────────░░░░░░░░░░░░───│
│  D4 ─────────────────────────────────────────────────────────│
│  C4 ─────▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓────────────────────▓▓▓▓▓▓▓▓▓▓▓───│
│        │    Beat 1    │    Beat 2    │    Beat 3    │       │
├───────────────────────────────────────────────────────────────┤
│  Chord: Cmaj          │    (rest)    │    Am               │
│  Tension: 0.12        │              │    0.45             │
└───────────────────────────────────────────────────────────────┘
```

### Spectral Data Pipeline

1. **Audio Analysis**: FFT of audio output (if available)
2. **MIDI Synthesis**: Virtual sine synthesis for MIDI-only analysis
3. **Bin Mapping**: Map FFT bins to piano roll rows
4. **Intensity Scaling**: Logarithmic scaling for display
5. **Overlay Blending**: Alpha-blend spectral data behind notes

## Harmonic Analyzer

### Pitch Class Vector (PCV)

A 12-dimensional vector representing the intensity of each pitch class:

```cpp
struct PitchClassVector {
    std::array<float, 12> values;  // C, C#, D, ..., B
    
    // Normalize to sum = 1
    void normalize();
    
    // Get dominant pitch classes
    std::vector<int> getDominant(int count = 3) const;
};
```

### Tension Metric

The harmonic tension metric quantifies dissonance:

```cpp
class HarmonicAnalyzer {
public:
    // Compute tension from pitch class vector (0 = consonant, 1 = dissonant)
    float computeTension(const PitchClassVector& pcv) const;
    
    // Factors considered:
    // - Minor second intervals (high tension)
    // - Tritone intervals (moderate tension)
    // - Perfect fifth/octave (low tension)
    // - Number of distinct pitch classes
};
```

#### Tension Formula

```
tension = w1 * minorSecondCount + w2 * tritoneCount - w3 * perfectFifthCount
          + w4 * pitchClassDensity
```

Weights are tuned empirically:
- `w1 = 0.3` (minor seconds)
- `w2 = 0.2` (tritones)
- `w3 = 0.15` (perfect fifths reduce tension)
- `w4 = 0.1` (more notes = more tension)

## Chord Labeling Approach

### Recognition Algorithm

1. Extract pitch class vector from active notes
2. Match against chord templates
3. Score each template by similarity
4. Select best match above threshold

### Chord Templates

```cpp
const std::map<std::string, std::array<bool, 12>> chordTemplates = {
    {"maj",  {1,0,0,0,1,0,0,1,0,0,0,0}},  // C E G
    {"min",  {1,0,0,1,0,0,0,1,0,0,0,0}},  // C Eb G
    {"dim",  {1,0,0,1,0,0,1,0,0,0,0,0}},  // C Eb Gb
    {"aug",  {1,0,0,0,1,0,0,0,1,0,0,0}},  // C E G#
    {"7",    {1,0,0,0,1,0,0,1,0,0,1,0}},  // C E G Bb
    {"maj7", {1,0,0,0,1,0,0,1,0,0,0,1}},  // C E G B
    {"min7", {1,0,0,1,0,0,0,1,0,0,1,0}},  // C Eb G Bb
    // ... additional templates
};
```

### Scale Degree Display

Shows Roman numeral analysis relative to detected key:

| Chord | In C major | In A minor |
|-------|------------|------------|
| Cmaj  | I          | III        |
| Dm    | ii         | iv         |
| Em    | iii        | v          |
| Fmaj  | IV         | VI         |
| G7    | V7         | VII7       |
| Am    | vi         | i          |

## Harmonic Lane

A dedicated lane below the piano roll showing:
- Chord labels with duration bars
- Tension curve graph
- Key detection indicator

```
┌───────────────────────────────────────────────────────────────┐
│ Harmonic Lane                                                 │
├───────────────────────────────────────────────────────────────┤
│  Key: C major (confidence: 0.85)                             │
│                                                               │
│  ┌──────────┐    ┌──────────┐    ┌──────────────────────────┐│
│  │  C maj   │    │   F maj  │    │       G7                 ││
│  │    I     │    │    IV    │    │       V7                 ││
│  └──────────┘    └──────────┘    └──────────────────────────┘│
│                                                               │
│  Tension: ─────╲____/─────────\    /───────────────────────  │
│           0.1  0.2  0.1  0.3  0.4  0.6  0.5  0.4  0.3       │
└───────────────────────────────────────────────────────────────┘
```

## File Layout

```
src/ai/harmony/
├── HarmonicAnalyzer.hpp/.cpp
└── CMakeLists.txt

src/ui/pianoroll/
├── PianoRollWidget.cpp  (spectral overlay, harmonic lane)
└── CMakeLists.txt
```

## Future Enhancements

- Scale lock (constrain notes to scale)
- Ghost note comparisons
- Voicing suggestions
- Part writing rule checking
