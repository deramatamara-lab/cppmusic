# Reactive Parameter & Modulation Graph

This document describes the reactive parameter system with signal-based change propagation and modulation blending.

## Overview

The parameter system provides:
- Centralized parameter registry with unique identifiers
- Reactive signal objects with observer notification
- Modulation matrix for source-to-target routing
- Blending operators for combining modulation values
- Cycle detection to prevent infinite update loops

## Architecture

### ParamRegistry

The central registry manages all parameters in the system:

```cpp
namespace cppmusic::engine::parameters {

class ParamRegistry {
public:
    // Register a parameter and get its unique ID
    ParamId registerParam(const ParamSpec& spec);
    
    // Unregister and remove a parameter
    bool unregisterParam(ParamId id);
    
    // Get parameter by ID
    ParamSignal* getParam(ParamId id);
    
    // Iterate all parameters
    void forEachParam(std::function<void(ParamSignal&)> fn);
    
    // Cycle detection
    bool hasCycle() const;
};

}
```

### ParamSignal

A reactive signal object that notifies observers when changed:

```cpp
class ParamSignal {
public:
    // Value access
    float getValue() const noexcept;
    void setValue(float value);
    void setValueNormalized(float normalized);  // 0-1 range
    
    // Range and metadata
    float getMinValue() const noexcept;
    float getMaxValue() const noexcept;
    float getDefaultValue() const noexcept;
    const std::string& getName() const noexcept;
    
    // Observer pattern
    void addObserver(ParamObserver* observer);
    void removeObserver(ParamObserver* observer);
    
    // Modulation
    float getModulatedValue() const noexcept;
    void setModulationAmount(float amount);
};
```

### ModMatrix

Routes modulation sources to parameter targets:

```cpp
class ModMatrix {
public:
    // Connection management
    ModSlotId connect(ModSource source, ParamId target, float amount);
    void disconnect(ModSlotId slot);
    
    // Update all modulations (called per-block)
    void process() noexcept;
    
    // Blending operators
    enum class BlendMode {
        Add,        // target += mod * amount
        Multiply,   // target *= (1 + mod * amount)
        Replace,    // target = mod * amount
        Bipolar     // target += (mod - 0.5) * 2 * amount
    };
    
    void setBlendMode(ModSlotId slot, BlendMode mode);
};
```

## Change Propagation

Parameters use a push-based reactive model:

1. Value changed via `setValue()` or modulation
2. Signal marks itself as dirty
3. Observers are notified synchronously
4. Dependent parameters update in topological order

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│ User Input  │────►│ ParamSignal │────►│  Observers  │
│ or Modulate │     │   (dirty)   │     │  (notify)   │
└─────────────┘     └─────────────┘     └─────────────┘
                           │
                           ▼
                    ┌─────────────┐
                    │ Dependent   │
                    │ Parameters  │
                    └─────────────┘
```

## Cycle Detection

The registry uses Kahn's algorithm to detect cycles:

```cpp
bool ParamRegistry::hasCycle() const {
    // Build dependency graph from modulation connections
    // Perform topological sort
    // Return true if any nodes remain unprocessed
}
```

Cycle detection is performed:
- When modulation connections are added
- Before processing begins
- Cycles are rejected and logged

## Thread Safety

- Parameter values use atomic reads/writes for audio thread access
- Observer notifications happen on the thread that triggered the change
- The modulation matrix maintains a lock-free snapshot for audio processing

## Modulation Blending Operators

| Mode | Formula | Use Case |
|------|---------|----------|
| Add | `target += mod * amount` | LFO modulation |
| Multiply | `target *= (1 + mod * amount)` | Velocity scaling |
| Replace | `target = mod * amount` | Full automation |
| Bipolar | `target += (mod - 0.5) * 2 * amount` | Pitch bend |

## File Layout

```
src/engine/parameters/
├── ParamRegistry.hpp/.cpp
├── ParamSignal.hpp/.cpp
├── ModMatrix.hpp/.cpp
└── CMakeLists.txt
```

## Testing

Key test cases:
- Add parameters, verify unique IDs
- Connect modulation, verify propagation
- Create cycle, verify detection and rejection
- Multi-threaded value updates
