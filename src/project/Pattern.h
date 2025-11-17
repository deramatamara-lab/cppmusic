#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <memory>

namespace daw::project
{

/**
 * @brief Pattern clip model with MIDI playback data
 * 
 * Pattern clips are first-class citizens with cached MIDI data
 * for efficient playback. Supports quantization and variations.
 */
class Pattern
{
public:
    struct MIDINote
    {
        uint8_t note;
        uint8_t velocity;
        double startBeat;
        double lengthBeats;
        uint8_t channel;
    };

    Pattern() noexcept;
    Pattern(const std::string& name, int numSteps = 16) noexcept;
    ~Pattern() = default;

    // Non-copyable, movable
    Pattern(const Pattern&) = delete;
    Pattern& operator=(const Pattern&) = delete;
    Pattern(Pattern&&) noexcept = default;
    Pattern& operator=(Pattern&&) noexcept = default;

    /**
     * @brief Get pattern name
     */
    [[nodiscard]] const std::string& getName() const noexcept { return name; }

    /**
     * @brief Set pattern name
     */
    void setName(const std::string& newName) noexcept { name = newName; }

    /**
     * @brief Get number of steps
     */
    [[nodiscard]] int getNumSteps() const noexcept { return numSteps; }

    /**
     * @brief Set number of steps
     */
    void setNumSteps(int steps) noexcept { numSteps = steps; }

    /**
     * @brief Add MIDI note to pattern
     */
    void addNote(const MIDINote& note) noexcept;

    /**
     * @brief Remove note at index
     */
    void removeNote(size_t index) noexcept;

    /**
     * @brief Clear all notes
     */
    void clearNotes() noexcept;

    /**
     * @brief Get all notes
     */
    [[nodiscard]] const std::vector<MIDINote>& getNotes() const noexcept { return notes; }

    /**
     * @brief Get notes for a specific step
     */
    [[nodiscard]] std::vector<MIDINote> getNotesForStep(int step) const noexcept;

    /**
     * @brief Quantize all notes to grid
     * @param gridDivision Grid division (1/4, 1/8, 1/16, etc.)
     */
    void quantize(double gridDivision) noexcept;

    /**
     * @brief Get pattern length in beats
     */
    [[nodiscard]] double getLengthBeats() const noexcept;

private:
    std::string name;
    int numSteps;
    std::vector<MIDINote> notes;
};

} // namespace daw::project

