#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include <juce_core/juce_core.h>

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
        float probability{1.0f};
        float microTiming{0.0f};
        int trigCondition{0};

        bool operator==(const MIDINote& other) const noexcept
        {
            return note == other.note &&
                   velocity == other.velocity &&
                   juce::approximatelyEqual(startBeat, other.startBeat) &&
                   juce::approximatelyEqual(lengthBeats, other.lengthBeats) &&
                   channel == other.channel &&
                   juce::approximatelyEqual(probability, other.probability) &&
                   juce::approximatelyEqual(microTiming, other.microTiming) &&
                   trigCondition == other.trigCondition;
        }
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
     * @brief Replace entire note list (keeps deterministic ordering)
     */
    void setNotes(const std::vector<MIDINote>& newNotes) noexcept;

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

    /**
     * @brief Get swing amount (0.0 = straight, 1.0 = maximum swing)
     */
    [[nodiscard]] float getSwing() const noexcept { return swing; }

    /**
     * @brief Set swing amount (0.0 = straight, 1.0 = maximum swing)
     */
    void setSwing(float swingAmount) noexcept { swing = juce::jlimit(0.0f, 1.0f, swingAmount); }

    /**
     * @brief Get pattern ID
     */
    [[nodiscard]] uint32_t getId() const noexcept { return id; }

private:
    uint32_t id;
    std::string name;
    int numSteps;
    std::vector<MIDINote> notes;
    float swing{0.0f}; // Swing amount: 0.0 = straight, 1.0 = maximum swing

    static uint32_t nextId;
    static uint32_t generateId();
};

} // namespace daw::project

