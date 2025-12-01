# Piano Roll Deep Edit

Advanced piano roll editing capabilities for FL-style workflow parity.

## Tool Modes

The piano roll provides a comprehensive tool palette accessible via toolbar buttons or keyboard shortcuts:

| Tool | Key | Description |
|------|-----|-------------|
| Draw | D | Draw new notes at cursor position |
| Select | V | Select and move notes, marquee selection |
| Slice | S | Slice notes at cursor position |
| Glue | G | Glue adjacent notes together |
| Stretch | - | Time-stretch selected notes |
| Warp | - | Insert warp markers for time transformation |
| Erase | E | Erase notes under cursor |
| Velocity Paint | - | Paint velocity values across notes |
| Time Nudge | - | Micro-adjust note timing |

## Gestures and Modifiers

### Drag Modifiers
- **Shift**: Fine adjustment (smaller increments)
- **Alt**: Ignore snap grid
- **Ctrl**: Duplicate while dragging
- **Ctrl+Shift**: Array duplicate (multiple copies)

### Pan and Zoom
- **Mouse Wheel**: Vertical scroll
- **Shift + Mouse Wheel**: Horizontal scroll
- **Ctrl + Mouse Wheel**: Zoom in/out
- **Middle Mouse Button Drag**: Free pan with inertia

### Scrub Playback
Hold **Space** and drag in the timeline ruler to audition the selection.

## Note Properties

### Per-Note Deep Edit Properties

Each note can have advanced properties:

```cpp
struct NoteEvent {
    // Core properties
    uint8_t pitch;           // MIDI note 0-127
    uint8_t velocity;        // Note-on velocity 0-127
    double startBeat;        // Start position in beats
    double durationBeats;    // Duration in beats
    uint8_t channel;         // MIDI channel 0-15
    
    // Advanced properties
    uint8_t releaseVelocity; // Note-off velocity
    float pitchOffset;       // Per-note pitch offset (semitones)
    
    // Slide/Portamento
    SlideMode slideMode;     // None, Legato, Portamento, MPEPitchBend
    float slideTime;         // Duration in beats
    int8_t slideToPitch;     // Target pitch (relative semitones)
    
    // Probability & Conditions
    float probability;       // Play probability [0.0, 1.0]
    NoteCondition condition; // Always, FirstOnly, Nth, EveryN, SkipM, Random
    uint8_t conditionParam;  // Parameter for condition
    
    // Micro-timing
    int32_t microTimingOffset; // Sub-tick offset in samples
    float swingAmount;         // Per-note swing override
};
```

## Slide/Portamento

Slides create smooth pitch transitions between notes:

1. Select a note
2. Enable slide in the inspector or use the Slide tool
3. Adjust slide time and target pitch
4. Visual tie line shows the slide connection

### Slide Modes
- **None**: No slide effect
- **Legato**: Voice-level legato glide
- **Portamento**: Per-note portamento
- **MPE Pitch Bend**: MPE-style pitch bend slide

## Scale Lock

Lock notes to a musical scale:

1. Enable Scale Lock in toolbar
2. Select root note (C, C#, D, etc.)
3. Non-scale notes are dimmed
4. Drawing snaps to scale degrees

### Supported Scales
- Major
- Minor (Natural, Harmonic, Melodic)
- Pentatonic
- Blues
- Custom patterns

## Ghost Notes

View notes from other patterns for reference:

1. Enable Ghost Notes in toolbar/View menu
2. Other pattern notes appear semi-transparent
3. Helps align melodies and harmonies

## Fold Mode

Show only pitches that contain notes:

1. Enable Fold Mode in toolbar
2. Piano keyboard collapses to used notes only
3. Useful for editing specific instruments

## Warp Markers

Insert warp markers for non-linear time transformation:

1. Select Warp tool
2. Click to place markers
3. Drag markers to remap time
4. Useful for tempo changes within patterns

## Command Palette

Access editing commands quickly:

**Ctrl+P** opens the command palette

### Available Commands
- **Quantize** (Q): Snap notes to grid
- **Legato** (L): Extend notes to next note
- **Scale Lock Toggle**: Enable/disable scale lock
- **Randomize Velocity**: Add velocity variation
- **Randomize Timing**: Humanize timing
- **Make Unique**: Detach from parent pattern
- **Consolidate**: Merge selection to single clip

## Velocity Lane

Visual velocity editing below the note grid:

- Click and drag to adjust individual velocities
- Supports brush and ramp tools
- Color-coded by velocity level (green/yellow/red)

## Probability Lane

Per-note probability visualization:

- Diamond markers show probability
- Size indicates probability value
- Color indicates probability level
- Click to adjust, right-click for presets

## Micro-Timing Lane

Sub-tick timing adjustments:

- Markers show offset from grid
- Blue = late, Orange = early
- Center line = on grid
- Drag to adjust offset in samples
