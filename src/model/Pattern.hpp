#pragma once
/**
 * @file Pattern.hpp
 * @brief Note event storage with simple length computation.
 *
 * This is part of the foundational model layer for the cppmusic DAW.
 * Stores MIDI note events for pattern-based sequencing with advanced
 * editing features including slides, probability, conditions, and micro-timing.
 */

#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace cppmusic::model {

/**
 * @brief Condition types for conditional note triggering.
 *
 * Notes can be conditioned to play only under specific circumstances
 * within a loop for creating variation and humanization.
 */
enum class NoteCondition : std::uint8_t {
    Always = 0,    ///< Always play (default)
    FirstOnly,     ///< Play only on first loop iteration
    Nth,           ///< Play on every Nth iteration (uses conditionParam)
    EveryN,        ///< Play every N iterations (uses conditionParam)
    SkipM,         ///< Skip first M iterations (uses conditionParam)
    Random         ///< Random based on probability
};

/**
 * @brief Slide/portamento mode for note transitions.
 */
enum class SlideMode : std::uint8_t {
    None = 0,      ///< No slide
    Legato,        ///< Voice-level legato glide
    Portamento,    ///< Per-note portamento
    MPEPitchBend   ///< MPE-style pitch bend slide
};

/**
 * @brief A single MIDI note event in a pattern.
 *
 * Extended to support FL Studio-style deep editing capabilities:
 * - Slides and portamento
 * - Per-note pitch offset
 * - Release velocity
 * - Probability and conditional triggering
 * - Micro-timing offsets
 */
struct NoteEvent {
    // === Core note properties ===
    std::uint8_t pitch = 60;          ///< MIDI note number (0-127)
    std::uint8_t velocity = 100;      ///< Note-on velocity (0-127)
    double startBeat = 0.0;           ///< Start position in beats
    double durationBeats = 0.25;      ///< Duration in beats
    std::uint8_t channel = 0;         ///< MIDI channel (0-15)

    // === Advanced editing properties ===
    std::uint8_t releaseVelocity = 64;  ///< Note-off velocity (0-127)
    float pitchOffset = 0.0f;           ///< Per-note pitch offset in semitones (-48 to +48)
    
    // === Slide/Portamento ===
    SlideMode slideMode = SlideMode::None;  ///< Type of slide effect
    float slideTime = 0.0f;                 ///< Slide duration in beats (0.0 = instant)
    std::int8_t slideToPitch = 0;           ///< Target pitch for slide (relative semitones)
    
    // === Probability and conditions ===
    float probability = 1.0f;             ///< Probability of note playing [0.0, 1.0]
    NoteCondition condition = NoteCondition::Always;  ///< Conditional trigger type
    std::uint8_t conditionParam = 1;      ///< Parameter for condition (e.g., N for EveryN)
    
    // === Micro-timing ===
    std::int32_t microTimingOffset = 0;   ///< Sub-tick offset in samples (can be negative)
    float swingAmount = 0.0f;             ///< Per-note swing override (-1.0 to 1.0, 0 = use pattern)

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
 * querying and manipulating the note data. Extended to support
 * polymeter, swing, and probability/condition evaluation.
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
    // Swing and Timing Properties
    // =========================================================================

    /**
     * @brief Get the pattern-level swing amount.
     * @return Swing amount from -1.0 (early) to 1.0 (late).
     */
    [[nodiscard]] float getSwingAmount() const noexcept { return swingAmount_; }

    /**
     * @brief Set the pattern-level swing amount.
     * @param swing Swing from -1.0 to 1.0 (0.0 = no swing).
     */
    void setSwingAmount(float swing) noexcept { swingAmount_ = std::max(-1.0f, std::min(1.0f, swing)); }

    /**
     * @brief Get the swing grid resolution in beats.
     */
    [[nodiscard]] double getSwingResolution() const noexcept { return swingResolution_; }

    /**
     * @brief Set the swing grid resolution (e.g., 0.5 for 8th notes).
     */
    void setSwingResolution(double beats) noexcept { swingResolution_ = std::max(0.0625, beats); }

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
     * @brief Get a mutable reference to notes for editing.
     */
    [[nodiscard]] std::vector<NoteEvent>& getNotesRef() noexcept { return notes_; }

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
    // Probability and Condition Evaluation
    // =========================================================================

    /**
     * @brief Evaluate if a note should play based on probability and conditions.
     * @param note The note to evaluate.
     * @param loopIteration Current loop iteration (0-indexed).
     * @param seed Random seed for deterministic evaluation.
     * @return true if the note should play, false otherwise.
     */
    [[nodiscard]] static bool evaluateNoteCondition(
        const NoteEvent& note,
        std::uint32_t loopIteration,
        std::uint64_t seed) noexcept;

    /**
     * @brief Get notes that should play at the current iteration.
     * @param loopIteration Current loop iteration.
     * @param seed Random seed for deterministic probability evaluation.
     * @return Vector of notes that pass condition and probability checks.
     */
    [[nodiscard]] std::vector<NoteEvent> getPlayableNotes(
        std::uint32_t loopIteration,
        std::uint64_t seed) const;

    /**
     * @brief Calculate the swing-adjusted start beat for a note.
     * @param note The note to adjust.
     * @return Adjusted start beat accounting for pattern and note swing.
     */
    [[nodiscard]] double getSwingAdjustedBeat(const NoteEvent& note) const noexcept;

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
    
    // Swing settings
    float swingAmount_{0.0f};      ///< Pattern-level swing (-1.0 to 1.0)
    double swingResolution_{0.5};  ///< Swing grid in beats (0.5 = 8th notes)
};

} // namespace cppmusic::model
