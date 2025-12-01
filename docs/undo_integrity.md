# Project Integrity & Time-Travel Undo

This document describes the delta compression scheme, hash chain integrity, and deterministic replay.

## Overview

The undo system provides:
- Command-based undo/redo with delta compression
- Hash chain for state integrity verification
- Deterministic replay for project recovery
- Time-travel navigation through project history

## Architecture

### UndoService

Central service managing undo/redo operations:

```cpp
namespace cppmusic::services::undo {

class UndoService {
public:
    // Execute command and record for undo
    void execute(std::unique_ptr<Command> command);
    
    // Undo/redo operations
    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();
    
    // History navigation
    size_t getHistorySize() const;
    size_t getCurrentPosition() const;
    void jumpToPosition(size_t position);
    
    // State snapshot
    std::vector<uint8_t> captureSnapshot() const;
    void restoreSnapshot(const std::vector<uint8_t>& snapshot);
    
    // Integrity verification
    bool verifyIntegrity() const;
};

}
```

### Command Pattern

```cpp
class Command {
public:
    virtual ~Command() = default;
    
    // Execute the command
    virtual void execute() = 0;
    
    // Undo the command
    virtual void undo() = 0;
    
    // Get command description for UI
    virtual std::string getDescription() const = 0;
    
    // Serialize command for persistence
    virtual std::vector<uint8_t> serialize() const = 0;
    
    // Get state delta (for compression)
    virtual StateDelta getDelta() const = 0;
};
```

## Delta Compression Scheme

### StateDelta

Represents the minimal change from one state to another:

```cpp
struct StateDelta {
    // Type of change
    enum class Type {
        PropertyChange,   // Single value change
        CollectionInsert, // Item added to collection
        CollectionRemove, // Item removed from collection
        CollectionMove,   // Item position changed
        Compound          // Multiple deltas combined
    };
    
    Type type;
    std::string path;           // JSON path to changed element
    std::vector<uint8_t> oldValue;
    std::vector<uint8_t> newValue;
    
    // For compound deltas
    std::vector<StateDelta> children;
    
    // Compute compressed size
    size_t compressedSize() const;
};
```

### Compression Algorithm

```cpp
class DeltaCompressor {
public:
    // Compress delta for storage
    std::vector<uint8_t> compress(const StateDelta& delta);
    
    // Decompress delta
    StateDelta decompress(const std::vector<uint8_t>& data);
    
private:
    // Use zstd for compression
    // Typical compression ratio: 3-10x for similar states
};
```

### Delta Chain

```
┌─────────────────────────────────────────────────────────────┐
│                      Delta Chain                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐  │
│  │ State 0 │───►│ Delta 1 │───►│ Delta 2 │───►│ Delta 3 │  │
│  │ (base)  │    │ +5 notes│    │ -2 notes│    │ +param  │  │
│  └─────────┘    └─────────┘    └─────────┘    └─────────┘  │
│       │              │              │              │        │
│       │              │              │              │        │
│       ▼              ▼              ▼              ▼        │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐  │
│  │ Hash 0  │───►│ Hash 1  │───►│ Hash 2  │───►│ Hash 3  │  │
│  │ abc123  │    │ def456  │    │ ghi789  │    │ jkl012  │  │
│  └─────────┘    └─────────┘    └─────────┘    └─────────┘  │
│                                                             │
│  Periodic Checkpoints:                                      │
│                                                             │
│  ┌───────────────────────┐         ┌───────────────────────┐│
│  │ Checkpoint 0 (full)   │         │ Checkpoint 100 (full) ││
│  │ State 0               │         │ State 100             ││
│  └───────────────────────┘         └───────────────────────┘│
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Hash Chain Integrity

### StateHasher

Computes cryptographic hashes for state verification:

```cpp
namespace cppmusic::services::integrity {

class StateHasher {
public:
    // Hash entire state
    Hash256 hashState(const ProjectState& state);
    
    // Incremental hash update
    void updateHash(const StateDelta& delta);
    Hash256 getCurrentHash() const;
    
    // Verify chain integrity
    bool verifyChain(const std::vector<Hash256>& chain,
                     const std::vector<StateDelta>& deltas);
    
private:
    // Use BLAKE3 for fast cryptographic hashing
    blake3_hasher hasher_;
};

}
```

### Hash Structure

```cpp
struct Hash256 {
    std::array<uint8_t, 32> bytes;
    
    bool operator==(const Hash256& other) const {
        return bytes == other.bytes;
    }
    
    std::string toHex() const;
    static Hash256 fromHex(const std::string& hex);
};
```

### Chain Verification

```cpp
bool StateHasher::verifyChain(
    const std::vector<Hash256>& hashes,
    const std::vector<StateDelta>& deltas) {
    
    if (hashes.empty()) return true;
    if (hashes.size() != deltas.size() + 1) return false;
    
    Hash256 currentHash = hashes[0];
    
    for (size_t i = 0; i < deltas.size(); ++i) {
        // Compute next hash from current hash + delta
        blake3_hasher hasher;
        blake3_hasher_init(&hasher);
        blake3_hasher_update(&hasher, currentHash.bytes.data(), 32);
        
        auto deltaBytes = serializeDelta(deltas[i]);
        blake3_hasher_update(&hasher, deltaBytes.data(), deltaBytes.size());
        
        Hash256 computedHash;
        blake3_hasher_finalize(&hasher, computedHash.bytes.data(), 32);
        
        // Verify against stored hash
        if (computedHash != hashes[i + 1]) {
            LOG_ERROR("Hash chain broken at position {}", i + 1);
            return false;
        }
        
        currentHash = computedHash;
    }
    
    return true;
}
```

## Deterministic Replay

### Replay Engine

Reconstructs any historical state by replaying deltas:

```cpp
class ReplayEngine {
public:
    // Replay from checkpoint to target position
    ProjectState replayTo(size_t targetPosition);
    
    // Get nearest checkpoint
    size_t findNearestCheckpoint(size_t targetPosition) const;
    
private:
    // Apply delta to state
    void applyDelta(ProjectState& state, const StateDelta& delta);
    
    // Reverse delta (for undo)
    void reverseDelta(ProjectState& state, const StateDelta& delta);
};
```

### Determinism Guarantee

For identical inputs, replay produces identical outputs:

1. **Floating-point determinism**: Use fixed-point for critical values
2. **Order stability**: Sort collections before hashing
3. **Random seed capture**: Store RNG states if used
4. **Timestamp normalization**: Use logical clocks, not wall time

```cpp
struct ProjectState {
    // Use deterministic serialization
    std::vector<uint8_t> serialize() const {
        // Sort all collections by stable key before serializing
        // Use canonical JSON-like format
        // Round floats to fixed precision
    }
};
```

## Time-Travel UI

### UndoTimeTravelWidget

Visual timeline of project history:

```
┌─────────────────────────────────────────────────────────────┐
│  Time-Travel Undo                                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  History Timeline:                                          │
│                                                             │
│  ├──●──●──●──●──●──●──●──◆──○──○──○──○──○──│               │
│  0  5  10 15 20 25 30 35 40 45 50 55 60                    │
│                          ▲                                  │
│                       Current                               │
│                                                             │
│  Selected: Position 40                                      │
│  Action: "Added 5 notes to Pattern 1"                      │
│  Hash: abc123def456...                                      │
│                                                             │
│  [◄ Undo] [Redo ►] [Jump to Selection] [Verify Integrity]  │
│                                                             │
│  Legend: ● = Command, ◆ = Current, ○ = Redo available      │
│          █ = Checkpoint                                     │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## File Layout

```
src/services/undo/
├── UndoService.hpp/.cpp
├── Command.hpp
├── DeltaCompressor.hpp/.cpp
└── CMakeLists.txt

src/services/integrity/
├── StateHasher.hpp/.cpp
└── CMakeLists.txt

src/ui/undo/
├── UndoTimeTravelWidget.cpp
└── CMakeLists.txt
```

## Testing

Key test cases:
- Undo/redo sequence produces original state
- Hash changes when state mutates
- Hash chain verification detects corruption
- Replay from checkpoint matches direct state
- Concurrent modifications serialize deterministically
