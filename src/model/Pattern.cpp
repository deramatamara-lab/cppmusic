/**
 * @file Pattern.cpp
 * @brief Implementation of pattern note storage.
 */

#include "Pattern.hpp"
#include <algorithm>
#include <cmath>

namespace cppmusic::model {

Pattern::Pattern() noexcept = default;

Pattern::Pattern(const std::string& name, int lengthBars, int beatsPerBar) noexcept
    : name_(name)
    , lengthBeats_(static_cast<double>(lengthBars * beatsPerBar)) {
}

// =============================================================================
// Pattern Properties
// =============================================================================

const std::string& Pattern::getName() const noexcept {
    return name_;
}

void Pattern::setName(const std::string& name) noexcept {
    name_ = name;
}

double Pattern::getLengthBeats() const noexcept {
    return lengthBeats_;
}

void Pattern::setLengthBeats(double beats) noexcept {
    lengthBeats_ = std::max(0.0, beats);
}

double Pattern::computeContentLength() const noexcept {
    if (notes_.empty()) {
        return lengthBeats_;
    }

    double maxEndBeat = 0.0;
    for (const auto& note : notes_) {
        const double endBeat = note.getEndBeat();
        if (endBeat > maxEndBeat) {
            maxEndBeat = endBeat;
        }
    }

    // Return the greater of pattern length or content length
    return std::max(lengthBeats_, maxEndBeat);
}

// =============================================================================
// Note Management
// =============================================================================

void Pattern::addNote(const NoteEvent& note) {
    notes_.push_back(note);
    sortNotes();
}

bool Pattern::removeNote(std::size_t index) noexcept {
    if (index >= notes_.size()) {
        return false;
    }
    notes_.erase(notes_.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

std::size_t Pattern::removeNotesMatching(const NoteEvent& note) noexcept {
    const auto originalSize = notes_.size();
    notes_.erase(
        std::remove(notes_.begin(), notes_.end(), note),
        notes_.end());
    return originalSize - notes_.size();
}

void Pattern::clearNotes() noexcept {
    notes_.clear();
}

std::size_t Pattern::getNoteCount() const noexcept {
    return notes_.size();
}

const std::vector<NoteEvent>& Pattern::getNotes() const noexcept {
    return notes_;
}

const NoteEvent* Pattern::getNote(std::size_t index) const noexcept {
    if (index >= notes_.size()) {
        return nullptr;
    }
    return &notes_[index];
}

std::vector<NoteEvent> Pattern::getNotesInRange(double startBeat, double endBeat) const {
    std::vector<NoteEvent> result;
    for (const auto& note : notes_) {
        if (note.overlapsRange(startBeat, endBeat)) {
            result.push_back(note);
        }
    }
    return result;
}

// =============================================================================
// Probability and Condition Evaluation
// =============================================================================

bool Pattern::evaluateNoteCondition(
    const NoteEvent& note,
    std::uint32_t loopIteration,
    std::uint64_t seed) noexcept {
    
    // First check the condition
    switch (note.condition) {
        case NoteCondition::Always:
            break;  // Continue to probability check
            
        case NoteCondition::FirstOnly:
            if (loopIteration != 0) {
                return false;
            }
            break;
            
        case NoteCondition::Nth:
            // Play on every Nth iteration (1-indexed in param)
            if (note.conditionParam == 0) {
                return false;  // Invalid param
            }
            if ((loopIteration + 1) % note.conditionParam != 0) {
                return false;
            }
            break;
            
        case NoteCondition::EveryN:
            // Play every N iterations starting from first
            if (note.conditionParam == 0) {
                return false;  // Invalid param
            }
            if (loopIteration % note.conditionParam != 0) {
                return false;
            }
            break;
            
        case NoteCondition::SkipM:
            // Skip first M iterations
            if (loopIteration < note.conditionParam) {
                return false;
            }
            break;
            
        case NoteCondition::Random:
            // Will be evaluated with probability below
            break;
    }
    
    // Now evaluate probability
    if (note.probability < 1.0f) {
        // Create deterministic random value based on seed and note properties
        // This ensures same result for same seed/note combination
        std::uint64_t noteSeed = seed;
        noteSeed ^= static_cast<std::uint64_t>(note.pitch) << 8;
        noteSeed ^= static_cast<std::uint64_t>(note.startBeat * 1000.0);
        noteSeed ^= static_cast<std::uint64_t>(loopIteration) << 32;
        
        // Simple deterministic hash for probability check
        std::mt19937_64 rng(noteSeed);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        if (dist(rng) >= note.probability) {
            return false;
        }
    }
    
    return true;
}

std::vector<NoteEvent> Pattern::getPlayableNotes(
    std::uint32_t loopIteration,
    std::uint64_t seed) const {
    
    std::vector<NoteEvent> result;
    result.reserve(notes_.size());
    
    for (const auto& note : notes_) {
        if (evaluateNoteCondition(note, loopIteration, seed)) {
            result.push_back(note);
        }
    }
    
    return result;
}

double Pattern::getSwingAdjustedBeat(const NoteEvent& note) const noexcept {
    if (swingResolution_ <= 0.0) {
        return note.startBeat;
    }
    
    // Determine effective swing amount (note override or pattern default)
    float effectiveSwing = (note.swingAmount != 0.0f) ? note.swingAmount : swingAmount_;
    
    if (effectiveSwing == 0.0f) {
        return note.startBeat;
    }
    
    // Calculate position within the swing grid
    double gridPosition = note.startBeat / swingResolution_;
    double gridIndex = std::floor(gridPosition);
    double fractionalPosition = gridPosition - gridIndex;
    
    // Apply swing to off-beat notes (odd positions in the grid)
    // Swing affects the second note in each pair
    auto intGridIndex = static_cast<std::int64_t>(gridIndex);
    if (intGridIndex % 2 == 1) {
        // This is an off-beat note - apply swing
        // Positive swing = late, Negative swing = early
        double swingOffset = effectiveSwing * swingResolution_ * 0.5;
        return note.startBeat + swingOffset;
    }
    
    // On-beat note or non-swing position
    // Just account for fractional position with swing interpolation
    if (fractionalPosition > 0.0) {
        // Interpolate swing for notes between grid positions
        double nextSwingOffset = effectiveSwing * swingResolution_ * 0.5;
        if (intGridIndex % 2 == 0) {
            // Between on-beat and off-beat
            return note.startBeat + fractionalPosition * nextSwingOffset;
        }
    }
    
    return note.startBeat;
}

// =============================================================================
// Utility Methods
// =============================================================================

bool Pattern::isEmpty() const noexcept {
    return notes_.empty();
}

void Pattern::sortNotes() {
    std::sort(notes_.begin(), notes_.end());
}

} // namespace cppmusic::model
