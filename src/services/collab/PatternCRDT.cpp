/**
 * @file PatternCRDT.cpp
 * @brief Implementation of the collaborative pattern CRDT.
 */

#include "PatternCRDT.hpp"
#include <algorithm>
#include <cstring>

namespace cppmusic::services::collab {

// =============================================================================
// VectorClock Implementation
// =============================================================================

void VectorClock::tick(PeerId peer) {
    ++clocks_[peer];
}

std::uint64_t VectorClock::get(PeerId peer) const {
    auto it = clocks_.find(peer);
    if (it != clocks_.end()) {
        return it->second;
    }
    return 0;
}

void VectorClock::merge(const VectorClock& other) {
    for (const auto& [peer, time] : other.clocks_) {
        clocks_[peer] = std::max(clocks_[peer], time);
    }
}

bool VectorClock::isConcurrent(const VectorClock& other) const {
    return !happensBefore(other) && !other.happensBefore(*this);
}

bool VectorClock::happensBefore(const VectorClock& other) const {
    bool strictlyLess = false;
    
    for (const auto& [peer, time] : clocks_) {
        auto otherTime = other.get(peer);
        if (time > otherTime) {
            return false;  // Local is ahead
        }
        if (time < otherTime) {
            strictlyLess = true;
        }
    }
    
    // Check for any peers in other that we don't have
    for (const auto& [peer, time] : other.clocks_) {
        if (clocks_.find(peer) == clocks_.end() && time > 0) {
            strictlyLess = true;
        }
    }
    
    return strictlyLess;
}

std::vector<std::uint8_t> VectorClock::serialize() const {
    std::vector<std::uint8_t> data;
    
    // Write peer count
    std::uint32_t count = static_cast<std::uint32_t>(clocks_.size());
    data.resize(sizeof(count));
    std::memcpy(data.data(), &count, sizeof(count));
    
    // Write each peer:time pair
    for (const auto& [peer, time] : clocks_) {
        std::size_t offset = data.size();
        data.resize(offset + sizeof(PeerId) + sizeof(std::uint64_t));
        std::memcpy(data.data() + offset, &peer, sizeof(PeerId));
        std::memcpy(data.data() + offset + sizeof(PeerId), &time, sizeof(std::uint64_t));
    }
    
    return data;
}

VectorClock VectorClock::deserialize(const std::vector<std::uint8_t>& data) {
    VectorClock clock;
    
    if (data.size() < sizeof(std::uint32_t)) {
        return clock;
    }
    
    std::uint32_t count = 0;
    std::memcpy(&count, data.data(), sizeof(count));
    
    std::size_t offset = sizeof(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        if (offset + sizeof(PeerId) + sizeof(std::uint64_t) > data.size()) {
            break;
        }
        
        PeerId peer = 0;
        std::uint64_t time = 0;
        std::memcpy(&peer, data.data() + offset, sizeof(PeerId));
        std::memcpy(&time, data.data() + offset + sizeof(PeerId), sizeof(std::uint64_t));
        
        clock.clocks_[peer] = time;
        offset += sizeof(PeerId) + sizeof(std::uint64_t);
    }
    
    return clock;
}

// =============================================================================
// PatternCRDT Implementation
// =============================================================================

PatternCRDT::PatternCRDT(PeerId localPeer)
    : localPeer_(localPeer) {
}

PatternCRDT::~PatternCRDT() = default;

PatternCRDT::PatternCRDT(const PatternCRDT& other)
    : localPeer_(other.localPeer_)
    , clock_(other.clock_)
    , notes_(other.notes_)
    , localSequence_(other.localSequence_) {
}

PatternCRDT& PatternCRDT::operator=(const PatternCRDT& other) {
    if (this != &other) {
        localPeer_ = other.localPeer_;
        clock_ = other.clock_;
        notes_ = other.notes_;
        localSequence_ = other.localSequence_;
    }
    return *this;
}

NoteId PatternCRDT::generateNoteId() {
    clock_.tick(localPeer_);
    
    NoteId id;
    id.timestamp = getLamportTime();
    id.peerId = localPeer_;
    id.sequence = ++localSequence_;
    
    return id;
}

std::uint64_t PatternCRDT::getLamportTime() const {
    std::uint64_t maxTime = 0;
    for (const auto& [noteId, entry] : notes_) {
        maxTime = std::max(maxTime, noteId.timestamp);
        maxTime = std::max(maxTime, entry.lastModified);
    }
    return std::max(maxTime, clock_.get(localPeer_));
}

NoteId PatternCRDT::insertNote(const model::NoteEvent& note) {
    NoteId id = generateNoteId();
    
    CRDTNoteEntry entry;
    entry.id = id;
    entry.note = note;
    entry.lastModified = id.timestamp;
    entry.modifiedBy = localPeer_;
    entry.deleted = false;
    
    notes_[id] = entry;
    
    return id;
}

bool PatternCRDT::deleteNote(const NoteId& id) {
    auto it = notes_.find(id);
    if (it == notes_.end()) {
        return false;
    }
    
    if (it->second.deleted) {
        return false;  // Already deleted
    }
    
    clock_.tick(localPeer_);
    it->second.deleted = true;
    it->second.lastModified = getLamportTime();
    it->second.modifiedBy = localPeer_;
    
    return true;
}

bool PatternCRDT::updateNote(const NoteId& id, const model::NoteEvent& note) {
    auto it = notes_.find(id);
    if (it == notes_.end() || it->second.deleted) {
        return false;
    }
    
    clock_.tick(localPeer_);
    std::uint64_t newTime = getLamportTime();
    
    // LWW: only update if we have a newer timestamp
    if (newTime > it->second.lastModified) {
        it->second.note = note;
        it->second.lastModified = newTime;
        it->second.modifiedBy = localPeer_;
        return true;
    }
    
    return false;
}

std::optional<model::NoteEvent> PatternCRDT::getNote(const NoteId& id) const {
    auto it = notes_.find(id);
    if (it != notes_.end() && !it->second.deleted) {
        return it->second.note;
    }
    return std::nullopt;
}

std::vector<model::NoteEvent> PatternCRDT::getCanonicalNotes() const {
    std::vector<std::pair<NoteId, model::NoteEvent>> sortedNotes;
    
    for (const auto& [id, entry] : notes_) {
        if (!entry.deleted) {
            sortedNotes.push_back({id, entry.note});
        }
    }
    
    // Sort by start beat, then by NoteId for stability
    std::sort(sortedNotes.begin(), sortedNotes.end(),
        [](const auto& a, const auto& b) {
            if (a.second.startBeat != b.second.startBeat) {
                return a.second.startBeat < b.second.startBeat;
            }
            return a.first < b.first;
        });
    
    std::vector<model::NoteEvent> result;
    result.reserve(sortedNotes.size());
    for (const auto& [id, note] : sortedNotes) {
        result.push_back(note);
    }
    
    return result;
}

const std::map<NoteId, CRDTNoteEntry>& PatternCRDT::getAllEntries() const {
    return notes_;
}

std::size_t PatternCRDT::getNoteCount() const {
    std::size_t count = 0;
    for (const auto& [id, entry] : notes_) {
        if (!entry.deleted) {
            ++count;
        }
    }
    return count;
}

void PatternCRDT::merge(const PatternCRDT& remote) {
    // Merge vector clocks
    clock_.merge(remote.clock_);
    
    // Merge notes
    for (const auto& [id, remoteEntry] : remote.notes_) {
        auto it = notes_.find(id);
        
        if (it == notes_.end()) {
            // New note from remote
            notes_[id] = remoteEntry;
        } else {
            // Existing note - resolve conflict
            CRDTNoteEntry& localEntry = it->second;
            
            // Delete wins over update
            if (remoteEntry.deleted && !localEntry.deleted) {
                localEntry.deleted = true;
                localEntry.lastModified = std::max(
                    localEntry.lastModified, remoteEntry.lastModified);
            } else if (!remoteEntry.deleted && localEntry.deleted) {
                // Local delete already happened, keep deleted
            } else if (!remoteEntry.deleted && !localEntry.deleted) {
                // Both have updates - LWW
                if (remoteEntry.lastModified > localEntry.lastModified) {
                    localEntry.note = remoteEntry.note;
                    localEntry.lastModified = remoteEntry.lastModified;
                    localEntry.modifiedBy = remoteEntry.modifiedBy;
                } else if (remoteEntry.lastModified == localEntry.lastModified) {
                    // Tie-breaker: lower peer ID wins
                    if (remoteEntry.modifiedBy < localEntry.modifiedBy) {
                        localEntry.note = remoteEntry.note;
                        localEntry.modifiedBy = remoteEntry.modifiedBy;
                    }
                }
            }
        }
    }
}

std::vector<std::uint8_t> PatternCRDT::serialize() const {
    std::vector<std::uint8_t> data;
    
    // Write local peer ID
    data.resize(sizeof(PeerId));
    std::memcpy(data.data(), &localPeer_, sizeof(PeerId));
    
    // Write vector clock
    auto clockData = clock_.serialize();
    std::uint32_t clockSize = static_cast<std::uint32_t>(clockData.size());
    std::size_t offset = data.size();
    data.resize(offset + sizeof(clockSize) + clockSize);
    std::memcpy(data.data() + offset, &clockSize, sizeof(clockSize));
    std::memcpy(data.data() + offset + sizeof(clockSize), clockData.data(), clockSize);
    
    // Write note count
    offset = data.size();
    std::uint32_t noteCount = static_cast<std::uint32_t>(notes_.size());
    data.resize(offset + sizeof(noteCount));
    std::memcpy(data.data() + offset, &noteCount, sizeof(noteCount));
    
    // Write each note entry (simplified)
    for (const auto& [id, entry] : notes_) {
        offset = data.size();
        
        // Write NoteId
        data.resize(offset + sizeof(NoteId));
        std::memcpy(data.data() + offset, &id, sizeof(NoteId));
        offset += sizeof(NoteId);
        
        // Write deleted flag
        data.resize(offset + 1);
        data[offset] = entry.deleted ? 1 : 0;
        offset += 1;
        
        // Write lastModified
        data.resize(offset + sizeof(std::uint64_t));
        std::memcpy(data.data() + offset, &entry.lastModified, sizeof(std::uint64_t));
        offset += sizeof(std::uint64_t);
        
        // Write note data (simplified - just key fields)
        data.resize(offset + sizeof(model::NoteEvent));
        std::memcpy(data.data() + offset, &entry.note, sizeof(model::NoteEvent));
    }
    
    return data;
}

PatternCRDT PatternCRDT::deserialize(const std::vector<std::uint8_t>& data, PeerId localPeer) {
    PatternCRDT crdt(localPeer);
    
    if (data.size() < sizeof(PeerId)) {
        return crdt;
    }
    
    // Skip stored peer ID, use provided localPeer
    std::size_t offset = sizeof(PeerId);
    
    // Read vector clock size
    if (offset + sizeof(std::uint32_t) > data.size()) {
        return crdt;
    }
    std::uint32_t clockSize = 0;
    std::memcpy(&clockSize, data.data() + offset, sizeof(clockSize));
    offset += sizeof(clockSize);
    
    // Read vector clock
    if (offset + clockSize > data.size()) {
        return crdt;
    }
    std::vector<std::uint8_t> clockData(data.begin() + static_cast<std::ptrdiff_t>(offset),
                                         data.begin() + static_cast<std::ptrdiff_t>(offset + clockSize));
    crdt.clock_ = VectorClock::deserialize(clockData);
    offset += clockSize;
    
    // Read note count
    if (offset + sizeof(std::uint32_t) > data.size()) {
        return crdt;
    }
    std::uint32_t noteCount = 0;
    std::memcpy(&noteCount, data.data() + offset, sizeof(noteCount));
    offset += sizeof(noteCount);
    
    // Read notes
    for (std::uint32_t i = 0; i < noteCount; ++i) {
        if (offset + sizeof(NoteId) + 1 + sizeof(std::uint64_t) + sizeof(model::NoteEvent) > data.size()) {
            break;
        }
        
        CRDTNoteEntry entry;
        std::memcpy(&entry.id, data.data() + offset, sizeof(NoteId));
        offset += sizeof(NoteId);
        
        entry.deleted = (data[offset] != 0);
        offset += 1;
        
        std::memcpy(&entry.lastModified, data.data() + offset, sizeof(std::uint64_t));
        offset += sizeof(std::uint64_t);
        
        std::memcpy(&entry.note, data.data() + offset, sizeof(model::NoteEvent));
        offset += sizeof(model::NoteEvent);
        
        crdt.notes_[entry.id] = entry;
    }
    
    return crdt;
}

} // namespace cppmusic::services::collab
