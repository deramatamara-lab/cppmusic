# Playlist Warp and Clip Stretch

Advanced clip manipulation for the playlist arranger.

## Overview

The playlist supports:
- Clip time-stretching
- Warp markers for non-linear stretching
- Slip editing
- Per-instance transpose and gain
- Loop regions

## Clip Properties

```cpp
struct PatternClip {
    // Basic properties
    std::string name;
    int trackIndex;
    double startBeats;
    double lengthBeats;
    ImVec4 color;
    bool selected;
    bool muted;
    
    // Deep-edit properties
    bool ghosted;           // Ghost state (visible but not playing)
    int colorGroup;         // Color group for organization
    
    // Per-instance modifiers
    int transpose;          // Transpose in semitones (-24 to +24)
    float gain;             // Gain multiplier (0 to 2)
    
    // Warp/stretch
    bool stretchEnabled;    // Time-stretch enabled
    double stretchRatio;    // Stretch ratio (0.5 = half speed, 2 = double)
    std::vector<ClipWarpMarker> warpMarkers;
    
    // Slip editing
    double slipOffset;      // Content offset within clip
};
```

## Tool Modes

| Tool | Description |
|------|-------------|
| Select | Select and move clips |
| Slice | Slice clips at cursor |
| Slip | Adjust content offset within clip bounds |
| Stretch | Time-stretch clips |
| Draw | Draw new clips |

## Time Stretching

### Simple Stretch

Set a constant stretch ratio for the entire clip:

```cpp
clip.stretchEnabled = true;
clip.stretchRatio = 1.5;  // 150% speed (faster playback)
```

| Ratio | Effect |
|-------|--------|
| 0.5 | Half speed (2x duration) |
| 1.0 | Original speed |
| 1.5 | 150% speed |
| 2.0 | Double speed (0.5x duration) |

### Fit to Tempo

Automatically adjust clip tempo to match project tempo:

```cpp
void fitClipToTempo(PatternClip* clip, double targetBPM);
```

This calculates the appropriate stretch ratio based on the clip's original tempo.

## Warp Markers

For non-linear time manipulation, use warp markers:

### ClipWarpMarker

```cpp
struct ClipWarpMarker {
    double sourceBeat;  // Position in original clip
    double targetBeat;  // Position in warped clip
    bool selected;
};
```

### Placing Warp Markers

1. Select the Warp tool
2. Click in the clip to place a marker
3. Drag markers to adjust time mapping
4. The region between markers stretches/compresses accordingly

### Example: Slow Down Ending

```cpp
clip.warpMarkers.clear();
clip.warpMarkers.push_back({0.0, 0.0});    // Start aligned
clip.warpMarkers.push_back({6.0, 6.0});    // First 6 beats unchanged
clip.warpMarkers.push_back({8.0, 12.0});   // Last 2 beats stretched to 6
```

## Slip Editing

Slip editing adjusts which portion of the clip's content is visible:

```cpp
clip.slipOffset = 2.0;  // Content starts from beat 2 of source
```

### Use Cases
- Align specific beats to bar lines
- Hide intro/outro sections
- Create variations from same source

### UI Interaction

1. Select Slip tool
2. Click and drag within a clip
3. Content slides while clip boundaries stay fixed

## Slicing Clips

Split a clip at a specific position:

```cpp
void sliceClipAtPosition(PatternClip* clip, double position);
```

This creates two clips:
- Original clip truncated at slice point
- New clip starting at slice point with adjusted slip offset

## Consolidation

Merge multiple clips into one:

```cpp
void consolidateSelection();
```

Requirements:
- Multiple clips selected
- All clips on same track

Result:
- Single clip spanning entire selection range
- Original clips removed

## Make Unique

Detach a clip from its pattern source:

```cpp
void makeClipUnique(PatternClip* clip);
```

This creates an independent copy of the pattern data for editing without affecting other instances.

## Per-Instance Modifiers

### Transpose

Shift pitch of all notes in clip:

```cpp
clip.transpose = -12;  // One octave down
clip.transpose = 7;    // Up a fifth
```

Range: -24 to +24 semitones

### Gain

Adjust clip volume:

```cpp
clip.gain = 0.5f;  // Half volume
clip.gain = 1.5f;  // 150% volume
```

Range: 0.0 to 2.0

## Ghost State

Ghost clips are visible but don't play:

```cpp
clip.ghosted = true;
```

Use for:
- Reference arrangements
- Disabled alternatives
- Visual planning

## Mute State

Muted clips are hidden from playback:

```cpp
clip.muted = true;
```

Unlike ghost, muted clips may be visually indicated differently.

## Loop Region

Set a loop region in the timeline:

```cpp
playlist.loopEnabled = true;
playlist.loopStart = 0.0;   // Start at beat 0
playlist.loopEnd = 16.0;    // Loop back at beat 16
```

### Visual Indicators
- Loop region highlighted in timeline
- Start/end markers at boundaries
- Bracket indicators at corners

## Color Groups

Organize clips by color group:

```cpp
clip.colorGroup = 1;  // Assign to group 1
```

Clips in the same group share visual styling for organization.

## Toolbar Controls

| Control | Function |
|---------|----------|
| Tool buttons | Select active tool |
| Snap toggle | Enable/disable grid snapping |
| Snap division | Set snap grid size |
| Loop toggle | Enable/disable loop region |
| Zoom +/- | Adjust horizontal zoom |
