# Automation Curves 2.0 (Hierarchical Layers & Versioning)

This document describes the automation layer types, evaluation order, and version snapshots.

## Overview

Automation Curves 2.0 provides:
- Hierarchical layer system (Base, Override, Macro)
- Deterministic layer evaluation order
- Version snapshots for undo/comparison
- Interpolation curves (linear, bezier, step)

## Layer Types

### Base Layer

The foundational automation curve for a parameter:
- Always present for automated parameters
- Edited directly in automation lanes
- Lowest priority in evaluation

### Override Layer

Temporary overrides for specific regions:
- Higher priority than Base
- Time-bounded with fade in/out
- Used for punch-in recording, live adjustments

### Macro Layer

Global modifiers affecting all automation:
- Highest priority
- Applies scaling, offset, or gating
- Used for master controls, A/B switching

```
┌─────────────────────────────────────────────────────────────┐
│                    Layer Evaluation Stack                   │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ Macro Layer (highest priority)                      │   │
│  │ - Scale: 0.8                                        │   │
│  │ - Offset: +0.1                                      │   │
│  └─────────────────────────────────────────────────────┘   │
│                          │                                  │
│                          ▼                                  │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ Override Layer                                      │   │
│  │ [====ACTIVE====]        [====ACTIVE====]           │   │
│  │ Beat 4-8                Beat 16-20                 │   │
│  └─────────────────────────────────────────────────────┘   │
│                          │                                  │
│                          ▼                                  │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ Base Layer (lowest priority)                        │   │
│  │ ╱─────╲    ╱─────────╲    ╱───────────────         │   │
│  │╱       ╲__╱           ╲__╱                          │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Evaluation Order

### Algorithm

```cpp
float AutomationClip::evaluate(double beat) const {
    float value = 0.0f;
    
    // 1. Start with base layer
    if (baseLayer_) {
        value = baseLayer_->evaluate(beat);
    }
    
    // 2. Apply override layers (blend by region)
    for (const auto& override : overrideLayers_) {
        if (override->isActiveAt(beat)) {
            float blendFactor = override->getBlendFactor(beat);
            float overrideValue = override->evaluate(beat);
            value = lerp(value, overrideValue, blendFactor);
        }
    }
    
    // 3. Apply macro transformations
    if (macroLayer_) {
        value = macroLayer_->transform(value);
    }
    
    return clamp(value, 0.0f, 1.0f);
}
```

### Blend Factor Calculation

Override layers use smooth fade in/out:

```cpp
float OverrideLayer::getBlendFactor(double beat) const {
    if (beat < startBeat_) return 0.0f;
    if (beat > endBeat_) return 0.0f;
    
    // Fade in region
    if (beat < startBeat_ + fadeInBeats_) {
        return smoothstep(0.0f, 1.0f, 
            (beat - startBeat_) / fadeInBeats_);
    }
    
    // Fade out region
    if (beat > endBeat_ - fadeOutBeats_) {
        return smoothstep(1.0f, 0.0f,
            (beat - (endBeat_ - fadeOutBeats_)) / fadeOutBeats_);
    }
    
    return 1.0f;  // Full blend
}
```

## Version Snapshots

### AutomationVersionStore

Maintains versioned snapshots of automation state:

```cpp
class AutomationVersionStore {
public:
    // Create a snapshot of current state
    VersionId createSnapshot(const AutomationClip& clip);
    
    // Restore a previous snapshot
    void restoreSnapshot(AutomationClip& clip, VersionId version);
    
    // Compare two versions (returns delta)
    AutomationDelta compare(VersionId v1, VersionId v2);
    
    // Prune old versions
    void pruneOlderThan(std::chrono::seconds age);
};
```

### Version Hash

Each snapshot includes a deterministic hash for comparison:

```cpp
struct AutomationSnapshot {
    VersionId id;
    std::chrono::system_clock::time_point timestamp;
    uint64_t contentHash;  // xxHash of serialized state
    std::vector<uint8_t> data;  // Compressed automation data
};
```

### Deterministic Merge

For undo/redo consistency, merge operations are deterministic:

```cpp
AutomationClip AutomationClip::merge(
    const AutomationClip& other,
    MergeStrategy strategy) const {
    
    // Strategy determines conflict resolution
    switch (strategy) {
        case MergeStrategy::PreferLocal:
            // Local changes win
            break;
        case MergeStrategy::PreferRemote:
            // Remote changes win
            break;
        case MergeStrategy::Interleave:
            // Combine by timestamp
            break;
    }
    
    // Result hash must be identical regardless of merge order
    // for same inputs and strategy
}
```

## Interpolation Curves

### Curve Types

```cpp
enum class CurveType {
    Step,    // Instant change at breakpoint
    Linear,  // Straight line between points
    Bezier,  // Smooth cubic bezier
    SCurve   // Smooth step (ease in/out)
};
```

### Breakpoint Structure

```cpp
struct AutomationPoint {
    double beat;
    float value;
    CurveType curveToNext;
    
    // Bezier control points (if CurveType::Bezier)
    std::optional<BezierHandles> handles;
};

struct BezierHandles {
    float outTangentX, outTangentY;  // Control point leaving this point
    float inTangentX, inTangentY;    // Control point entering next point
};
```

## Data Model

### AutomationClip

```cpp
class AutomationClip {
public:
    // Point management
    void addPoint(const AutomationPoint& point);
    void removePoint(size_t index);
    void movePoint(size_t index, double beat, float value);
    
    // Layer management
    void setBaseLayer(std::unique_ptr<AutomationLayer> layer);
    void addOverrideLayer(std::unique_ptr<OverrideLayer> layer);
    void setMacroLayer(std::unique_ptr<MacroLayer> layer);
    
    // Evaluation
    float evaluate(double beat) const;
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    static AutomationClip deserialize(const std::vector<uint8_t>& data);
    
private:
    std::vector<AutomationPoint> points_;
    std::unique_ptr<AutomationLayer> baseLayer_;
    std::vector<std::unique_ptr<OverrideLayer>> overrideLayers_;
    std::unique_ptr<MacroLayer> macroLayer_;
};
```

## File Layout

```
src/engine/automation/
├── AutomationClip.hpp/.cpp
├── AutomationVersionStore.hpp/.cpp
└── CMakeLists.txt

src/ui/automation/
├── AutomationEditor.cpp
└── CMakeLists.txt
```

## Testing

Key test cases:
- Layer evaluation order produces expected values
- Override fade in/out blending is smooth
- Version hash is identical for same state
- Merge is deterministic (A merge B == B merge A where applicable)
