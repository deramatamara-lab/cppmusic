#pragma once
/**
 * @file Pattern.hpp
 * @brief Note event storage with simple length computation.
 *
 * This is part of the foundational model layer for the cppmusic DAW.
 * Stores MIDI note events for pattern-based sequencing.
 */

#include <cstdint>
#include <string>
#include <vector>

namespace cppmusic::model {

/**
 * @brief A single MIDI note event in a pattern.
 */
struct NoteEvent {
    std::uint8_t pitch = 60;        ///< MIDI note number (0-127)
    std::uint8_t velocity = 100;    ///< Note velocity (0-127)
    double startBeat = 0.0;         ///< Start position in beats
    double durationBeats = 0.25;    ///< Duration in beats
    std::uint8_t channel = 0;       ///< MIDI channel (0-15)

    /**
     * @brief Calculate the end beat of this note.
     */
    [[nodiscard]] double getEndBeat() const noexcept {
        return startBeat + durationBeats;
    }

    /**
     * @brief Check if this note overlaps with a given beat range.
     * @param rangeStart Start of the range in beats.
     * @param rangeEnd End of the range in beats.
     */
    [[nodiscard]] bool overlapsRange(double rangeStart, double rangeEnd) const noexcept {
        return startBeat < rangeEnd && getEndBeat() > rangeStart;
    }

    /**
     * @brief Comparison for sorting by start beat.
     *
     * Note: Uses exact floating-point comparison for sorting stability.
     * Start beat values are typically set from quantized grid positions,
     * where exact equality is meaningful. For range queries, use overlapsRange().
     */
    bool operator<(const NoteEvent& other) const noexcept {
        if (startBeat != other.startBeat) {
            return startBeat < other.startBeat;
        }
        return pitch < other.pitch;
    }

    /**
     * @brief Equality comparison.
     *
     * Note: Uses exact floating-point comparison. This is intentional for
     * matching specific note events in remove operations. For approximate
     * matching, implement a separate comparison with epsilon tolerance.
     */
    bool operator==(const NoteEvent& other) const noexcept {
        return pitch == other.pitch &&
               velocity == other.velocity &&
               startBeat == other.startBeat &&
               durationBeats == other.durationBeats &&
               channel == other.channel;
    }
};

/**
 * @brief A pattern containing a sequence of note events.
 *
 * Patterns are the basic unit of musical content in the DAW.
 * They store MIDI note events and provide utilities for
 * querying and manipulating the note data.
 */
class Pattern {
public:
    /**
     * @brief Default constructor creates an empty 4-bar pattern.
     */
    Pattern() noexcept;

    /**
     * @brief Construct a pattern with a name and length.
     * @param name Pattern name.
     * @param lengthBars Length in bars (default: 4).
     * @param beatsPerBar Beats per bar (default: 4 for 4/4 time).
     */
    explicit Pattern(const std::string& name, int lengthBars = 4, int beatsPerBar = 4) noexcept;

    ~Pattern() = default;

    // Copyable and movable
    Pattern(const Pattern&) = default;
    Pattern& operator=(const Pattern&) = default;
    Pattern(Pattern&&) noexcept = default;
    Pattern& operator=(Pattern&&) noexcept = default;

    // =========================================================================
    // Pattern Properties
    // =========================================================================

    /**
     * @brief Get the pattern name.
     */
    [[nodiscard]] const std::string& getName() const noexcept;

    /**
     * @brief Set the pattern name.
     */
    void setName(const std::string& name) noexcept;

    /**
     * @brief Get the pattern length in beats.
     */
    [[nodiscard]] double getLengthBeats() const noexcept;

    /**
     * @brief Set the pattern length in beats.
     */
    void setLengthBeats(double beats) noexcept;

    /**
     * @brief Compute the actual content length based on note events.
     *
     * Returns the end position of the last note, or the pattern length
     * if the pattern is empty.
     */
    [[nodiscard]] double computeContentLength() const noexcept;

    // =========================================================================
    // Note Management
    // =========================================================================

    /**
     * @brief Add a note event to the pattern.
     * @param note The note event to add.
     *
     * Notes are kept sorted by start beat.
     */
    void addNote(const NoteEvent& note);

    /**
     * @brief Remove a note event at the specified index.
     * @param index Index of the note to remove.
     * @return true if the note was removed, false if index out of range.
     */
    bool removeNote(std::size_t index) noexcept;

    /**
     * @brief Remove all notes matching the given criteria.
     * @param note The note event to match.
     * @return Number of notes removed.
     */
    std::size_t removeNotesMatching(const NoteEvent& note) noexcept;

    /**
     * @brief Clear all notes from the pattern.
     */
    void clearNotes() noexcept;

    /**
     * @brief Get the number of notes in the pattern.
     */
    [[nodiscard]] std::size_t getNoteCount() const noexcept;

    /**
     * @brief Get all notes in the pattern.
     */
    [[nodiscard]] const std::vector<NoteEvent>& getNotes() const noexcept;

    /**
     * @brief Get a specific note by index.
     * @param index Index of the note.
     * @return Pointer to the note, or nullptr if index out of range.
     */
    [[nodiscard]] const NoteEvent* getNote(std::size_t index) const noexcept;

    /**
     * @brief Get notes within a beat range.
     * @param startBeat Start of the range.
     * @param endBeat End of the range.
     * @return Vector of notes that overlap the range.
     */
    [[nodiscard]] std::vector<NoteEvent> getNotesInRange(double startBeat, double endBeat) const;

    // =========================================================================
    // Utility Methods
    // =========================================================================

    /**
     * @brief Check if the pattern has any notes.
     */
    [[nodiscard]] bool isEmpty() const noexcept;

    /**
     * @brief Sort notes by start beat (called automatically after addNote).
     */
    void sortNotes();

private:
    std::string name_{"Untitled"};
    double lengthBeats_{16.0};  // Default: 4 bars at 4/4
    std::vector<NoteEvent> notes_;
};

} // namespace cppmusic::model
