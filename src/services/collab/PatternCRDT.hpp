#pragma once
/**
 * @file PatternCRDT.hpp
 * @brief CRDT-based pattern editing for collaborative sessions.
 */

#include "model/Pattern.hpp"
#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace cppmusic::services::collab {

/**
 * @brief Unique identifier for a peer in a collaborative session.
 */
using PeerId = std::uint32_t;

/**
 * @brief Invalid peer ID sentinel value.
 */
constexpr PeerId InvalidPeerId = 0;

/**
 * @brief Unique identifier for a note in the CRDT.
 * 
 * Combines Lamport timestamp, peer ID, and sequence for uniqueness.
 */
struct NoteId {
    std::uint64_t timestamp = 0;  ///< Lamport clock value
    PeerId peerId = InvalidPeerId;
    std::uint32_t sequence = 0;   ///< Tie-breaker for same timestamp
    
    bool operator<(const NoteId& other) const noexcept {
        if (timestamp != other.timestamp) return timestamp < other.timestamp;
        if (peerId != other.peerId) return peerId < other.peerId;
        return sequence < other.sequence;
    }
    
    bool operator==(const NoteId& other) const noexcept {
        return timestamp == other.timestamp &&
               peerId == other.peerId &&
               sequence == other.sequence;
    }
    
    bool operator!=(const NoteId& other) const noexcept {
        return !(*this == other);
    }
};

/**
 * @brief A note entry in the CRDT with metadata.
 */
struct CRDTNoteEntry {
    NoteId id;
    model::NoteEvent note;
    std::uint64_t lastModified = 0;  ///< Timestamp of last modification
    PeerId modifiedBy = InvalidPeerId;
    bool deleted = false;  ///< Tombstone marker
};

/**
 * @brief Vector clock for causality tracking.
 */
class VectorClock {
public:
    /**
     * @brief Increment the clock for a peer.
     */
    void tick(PeerId peer);
    
    /**
     * @brief Get the current value for a peer.
     */
    [[nodiscard]] std::uint64_t get(PeerId peer) const;
    
    /**
     * @brief Merge with another vector clock.
     */
    void merge(const VectorClock& other);
    
    /**
     * @brief Check if this clock is concurrent with another.
     */
    [[nodiscard]] bool isConcurrent(const VectorClock& other) const;
    
    /**
     * @brief Check if this clock happens-before another.
     */
    [[nodiscard]] bool happensBefore(const VectorClock& other) const;
    
    /**
     * @brief Serialize the vector clock.
     */
    [[nodiscard]] std::vector<std::uint8_t> serialize() const;
    
    /**
     * @brief Deserialize a vector clock.
     */
    static VectorClock deserialize(const std::vector<std::uint8_t>& data);
    
private:
    std::map<PeerId, std::uint64_t> clocks_;
};

/**
 * @brief CRDT for collaborative pattern editing.
 * 
 * Implements a hybrid CRDT combining:
 * - G-Set with tombstones for note add/delete
 * - LWW-Register for note property updates
 * 
 * Guarantees:
 * - Convergence: All replicas converge to same state
 * - Commutativity: Merge order doesn't matter
 * - Idempotency: Merging same state multiple times is safe
 */
class PatternCRDT {
public:
    /**
     * @brief Construct a new CRDT for the given peer.
     */
    explicit PatternCRDT(PeerId localPeer);
    ~PatternCRDT();
    
    // Copyable for state transfer
    PatternCRDT(const PatternCRDT& other);
    PatternCRDT& operator=(const PatternCRDT& other);
    PatternCRDT(PatternCRDT&&) noexcept = default;
    PatternCRDT& operator=(PatternCRDT&&) noexcept = default;
    
    // =========================================================================
    // Note Operations
    // =========================================================================
    
    /**
     * @brief Insert a new note.
     * @return The unique ID assigned to the note.
     */
    NoteId insertNote(const model::NoteEvent& note);
    
    /**
     * @brief Delete a note by ID.
     * @return true if the note was found and deleted.
     */
    bool deleteNote(const NoteId& id);
    
    /**
     * @brief Update a note's properties.
     * @return true if the note was found and updated.
     */
    bool updateNote(const NoteId& id, const model::NoteEvent& note);
    
    /**
     * @brief Get a note by ID.
     * @return The note, or nullopt if not found or deleted.
     */
    [[nodiscard]] std::optional<model::NoteEvent> getNote(const NoteId& id) const;
    
    // =========================================================================
    // State Access
    // =========================================================================
    
    /**
     * @brief Get all notes in canonical ordering.
     * 
     * Returns notes sorted by start beat, then by NoteId for stability.
     * Deleted notes are excluded.
     */
    [[nodiscard]] std::vector<model::NoteEvent> getCanonicalNotes() const;
    
    /**
     * @brief Get all note entries (including metadata).
     */
    [[nodiscard]] const std::map<NoteId, CRDTNoteEntry>& getAllEntries() const;
    
    /**
     * @brief Get the number of non-deleted notes.
     */
    [[nodiscard]] std::size_t getNoteCount() const;
    
    /**
     * @brief Get the local peer ID.
     */
    [[nodiscard]] PeerId getLocalPeerId() const noexcept { return localPeer_; }
    
    // =========================================================================
    // Merge Operations
    // =========================================================================
    
    /**
     * @brief Merge with another CRDT instance.
     * 
     * After merge, both CRDTs have equivalent state.
     * Merge is commutative and idempotent.
     */
    void merge(const PatternCRDT& remote);
    
    /**
     * @brief Get the current vector clock.
     */
    [[nodiscard]] const VectorClock& getVectorClock() const { return clock_; }
    
    // =========================================================================
    // Serialization
    // =========================================================================
    
    /**
     * @brief Serialize the CRDT state for network transfer.
     */
    [[nodiscard]] std::vector<std::uint8_t> serialize() const;
    
    /**
     * @brief Deserialize from binary data.
     */
    static PatternCRDT deserialize(const std::vector<std::uint8_t>& data, PeerId localPeer);
    
private:
    /**
     * @brief Generate a unique NoteId for a new note.
     */
    NoteId generateNoteId();
    
    /**
     * @brief Get the current Lamport timestamp.
     */
    std::uint64_t getLamportTime() const;
    
    PeerId localPeer_;
    VectorClock clock_;
    std::map<NoteId, CRDTNoteEntry> notes_;
    std::uint32_t localSequence_ = 0;
};

} // namespace cppmusic::services::collab
