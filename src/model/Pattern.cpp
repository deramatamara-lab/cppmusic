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
// Utility Methods
// =============================================================================

bool Pattern::isEmpty() const noexcept {
    return notes_.empty();
}

void Pattern::sortNotes() {
    std::sort(notes_.begin(), notes_.end());
}

} // namespace cppmusic::model
