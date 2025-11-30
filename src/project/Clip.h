#pragma once

#include <cstdint>
#include <string>

namespace daw::project
{

/**
 * @brief Clip model
 *
 * Represents a clip on the timeline with start time, length, and track reference.
 */
class Clip
{
public:
    Clip() = default;
    Clip(uint32_t trackId, double startBeats, double lengthBeats, std::string label);
    ~Clip() = default;

    // Non-copyable, movable
    Clip(const Clip&) = default;
    Clip& operator=(const Clip&) = default;
    Clip(Clip&&) noexcept = default;
    Clip& operator=(Clip&&) noexcept = default;

    [[nodiscard]] uint32_t getId() const { return id; }
    [[nodiscard]] uint32_t getTrackId() const { return trackId; }
    void setTrackId(uint32_t newTrackId) { trackId = newTrackId; }

    [[nodiscard]] double getStartBeats() const { return startBeats; }
    void setStartBeats(double newStartBeats) { startBeats = newStartBeats; }

    [[nodiscard]] double getLengthBeats() const { return lengthBeats; }
    void setLengthBeats(double newLengthBeats) { lengthBeats = newLengthBeats; }

    [[nodiscard]] double getEndBeats() const { return startBeats + lengthBeats; }

    [[nodiscard]] std::string getLabel() const { return label; }
    void setLabel(const std::string& newLabel) { label = newLabel; }

    // Fade controls
    [[nodiscard]] double getFadeInBeats() const { return fadeInBeats; }
    void setFadeInBeats(double newFadeInBeats) { fadeInBeats = newFadeInBeats; }

    [[nodiscard]] double getFadeOutBeats() const { return fadeOutBeats; }
    void setFadeOutBeats(double newFadeOutBeats) { fadeOutBeats = newFadeOutBeats; }

    // Pattern reference (for pattern clips)
    [[nodiscard]] uint32_t getPatternId() const { return patternId; }
    void setPatternId(uint32_t newPatternId) { patternId = newPatternId; }
    [[nodiscard]] bool hasPattern() const { return patternId != 0; }

    // Clip color for visual distinction
    [[nodiscard]] int getColorIndex() const { return colorIndex; }
    void setColorIndex(int newColorIndex) { colorIndex = newColorIndex; }

private:
    uint32_t id;
    uint32_t trackId;
    double startBeats;
    double lengthBeats;
    std::string label;
    uint32_t patternId = 0; // 0 means no pattern
    double fadeInBeats = 0.0;
    double fadeOutBeats = 0.0;
    int colorIndex = 0; // Default color index

    static uint32_t nextId;
    static uint32_t generateId();
};

} // namespace daw::project

