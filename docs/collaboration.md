# Collaborative Live Session Skeleton (CRDT-based Pattern Editing)

This document describes the CRDT design, session protocol, conflict resolution, and security considerations.

## Overview

The collaboration system enables:
- Real-time multi-user pattern editing
- CRDT-based conflict-free synchronization
- Optimistic local updates with eventual consistency
- Session management and user presence

## CRDT Design

### PatternCRDT

A hybrid CRDT combining G-Counter for IDs and LWW-Register for note properties:

```cpp
namespace cppmusic::services::collab {

class PatternCRDT {
public:
    // Note operations
    NoteId insertNote(const NoteEvent& note);
    void deleteNote(NoteId id);
    void updateNote(NoteId id, const NoteEvent& note);
    
    // Merge with remote state
    void merge(const PatternCRDT& remote);
    
    // Get canonical state
    std::vector<NoteEvent> getCanonicalNotes() const;
    
    // Serialization for network transport
    std::vector<uint8_t> serialize() const;
    static PatternCRDT deserialize(const std::vector<uint8_t>& data);
};

}
```

### CRDT Structure

```
┌─────────────────────────────────────────────────────────────┐
│                     PatternCRDT Structure                   │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ Note Set (G-Set with tombstones)                    │   │
│  │                                                     │   │
│  │  NoteId → { NoteEvent, timestamp, peerId, deleted } │   │
│  │  ─────────────────────────────────────────────────  │   │
│  │  001 → { C4, q=0.5, vel=100, t=1234, peer=A, del=F }│   │
│  │  002 → { E4, q=1.0, vel=80,  t=1235, peer=B, del=F }│   │
│  │  003 → { G4, q=0.5, vel=90,  t=1230, peer=A, del=T }│   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ Vector Clock                                        │   │
│  │  { A: 5, B: 3, C: 2 }                              │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### NoteId Generation

Unique IDs using Lamport timestamps + peer ID:

```cpp
struct NoteId {
    uint64_t timestamp;  // Lamport clock
    uint32_t peerId;     // Unique peer identifier
    uint32_t sequence;   // Tie-breaker for same timestamp
    
    bool operator<(const NoteId& other) const {
        if (timestamp != other.timestamp) 
            return timestamp < other.timestamp;
        if (peerId != other.peerId)
            return peerId < other.peerId;
        return sequence < other.sequence;
    }
};
```

## Session Protocol

### Message Types

```cpp
enum class MessageType : uint8_t {
    // Session management
    JoinRequest,
    JoinAccept,
    JoinReject,
    Leave,
    
    // Synchronization
    StateRequest,
    StateFull,
    StateDelta,
    
    // Operations
    NoteInsert,
    NoteDelete,
    NoteUpdate,
    
    // Presence
    CursorUpdate,
    SelectionUpdate,
    Heartbeat
};
```

### Session Lifecycle

```
┌─────────────┐     JoinRequest     ┌─────────────┐
│   Client    │─────────────────────│   Server    │
│   (new)     │                     │   (host)    │
└─────────────┘                     └─────────────┘
       │                                   │
       │◄──────── JoinAccept ──────────────│
       │          (session_id, peer_id)    │
       │                                   │
       │────────── StateRequest ──────────►│
       │                                   │
       │◄──────── StateFull ───────────────│
       │          (full CRDT state)        │
       │                                   │
       ├─────────────────────────────────────────────┐
       │              Active Session                 │
       │                                             │
       │  Operations (insert/delete/update)          │
       │◄────────────────────────────────────────────►
       │                                             │
       │  Presence updates (cursor, selection)       │
       │◄────────────────────────────────────────────►
       │                                             │
       │  Periodic heartbeats                        │
       │◄────────────────────────────────────────────►
       └─────────────────────────────────────────────┘
```

### SessionServer

```cpp
class SessionServer {
public:
    // Start hosting a session
    SessionId createSession(const SessionConfig& config);
    
    // Connect to existing session
    bool joinSession(SessionId id, const std::string& address);
    
    // Leave current session
    void leaveSession();
    
    // Send operation
    void sendOperation(const Operation& op);
    
    // Register callbacks
    void onRemoteOperation(std::function<void(const Operation&)> callback);
    void onPresenceUpdate(std::function<void(PeerId, const Presence&)> callback);
    void onConnectionStateChange(std::function<void(ConnectionState)> callback);
};
```

## Conflict Resolution

### LWW (Last-Writer-Wins)

For note property updates, the most recent write wins:

```cpp
void PatternCRDT::updateNote(NoteId id, const NoteEvent& note) {
    auto it = notes_.find(id);
    if (it == notes_.end() || it->second.deleted) {
        return;  // Note doesn't exist
    }
    
    auto newTimestamp = lamportClock_.tick();
    
    // Only update if our timestamp is newer
    if (newTimestamp > it->second.timestamp) {
        it->second.note = note;
        it->second.timestamp = newTimestamp;
        it->second.peerId = localPeerId_;
    }
}
```

### Concurrent Insert Resolution

When two peers insert at the same beat position:

1. Both notes are kept (no conflict)
2. Notes are ordered by NoteId for canonical ordering
3. UI displays both notes at same position

### Delete vs Update Conflict

When one peer deletes while another updates:

- Delete wins (tombstone preserved)
- Update is discarded
- Can be undone locally if needed

## Security Notes

### Authentication

```cpp
struct SessionConfig {
    std::string sessionName;
    bool requirePassword;
    std::string passwordHash;  // bcrypt
    
    // Permission levels
    enum class Permission {
        ReadOnly,   // Can view but not edit
        Edit,       // Can edit patterns
        Admin       // Can manage session
    };
};
```

### Transport Security

- All network communication uses TLS 1.3
- Session tokens are short-lived (1 hour)
- Rate limiting prevents DoS
- Input validation on all operations

### Trust Model

```
┌─────────────────────────────────────────────────────────────┐
│                      Trust Boundaries                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌───────────────────────────┐                             │
│  │    Local Client (trusted) │                             │
│  │    - Full project access  │                             │
│  │    - Undo/redo local ops  │                             │
│  └───────────────────────────┘                             │
│               │                                             │
│               │ TLS + Auth                                  │
│               ▼                                             │
│  ┌───────────────────────────┐                             │
│  │ Session Server (semi-trust)│                             │
│  │ - Validates operations    │                             │
│  │ - Broadcasts to peers     │                             │
│  │ - No project file access  │                             │
│  └───────────────────────────┘                             │
│               │                                             │
│               │ TLS + Auth                                  │
│               ▼                                             │
│  ┌───────────────────────────┐                             │
│  │  Remote Peers (untrusted) │                             │
│  │  - Operations validated   │                             │
│  │  - Rate limited           │                             │
│  │  - Can be kicked          │                             │
│  └───────────────────────────┘                             │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## File Layout

```
src/services/collab/
├── PatternCRDT.hpp/.cpp
├── SessionServer.hpp/.cpp
└── CMakeLists.txt

src/ui/collab/
├── CollabOverlay.cpp
└── CMakeLists.txt
```

## Testing

Key test cases:
- Two operations at same timestamp resolve deterministically
- Merge is commutative (A merge B == B merge A)
- Merge is associative ((A merge B) merge C == A merge (B merge C))
- Delete tombstones are preserved through merge
- Canonical ordering is stable
