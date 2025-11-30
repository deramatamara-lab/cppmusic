#pragma once

#include <string>
#include <cstdint>
#include <juce_graphics/juce_graphics.h>

namespace daw::project
{

/**
 * @brief Track model
 *
 * Represents a track in the project with properties like name, color, visibility,
 * and audio parameters (gain, pan, mute, solo).
 */
class Track
{
public:
    Track() = default;
    Track(std::string name, juce::Colour color);
    ~Track() = default;

    // Non-copyable, movable
    Track(const Track&) = default;
    Track& operator=(const Track&) = default;
    Track(Track&&) noexcept = default;
    Track& operator=(Track&&) noexcept = default;

    // Properties
    [[nodiscard]] std::string getName() const { return name; }
    void setName(const std::string& newName) { name = newName; }

    [[nodiscard]] juce::Colour getColor() const { return color; }
    void setColor(juce::Colour newColor) { color = newColor; }

    [[nodiscard]] bool isVisible() const { return visible; }
    void setVisible(bool isVisible) { visible = isVisible; }

    [[nodiscard]] float getGainDb() const { return gainDb; }
    void setGainDb(float newGainDb) { gainDb = newGainDb; }

    [[nodiscard]] float getPan() const { return pan; }
    void setPan(float newPan) { pan = std::clamp(newPan, -1.0f, 1.0f); }

    [[nodiscard]] bool isMuted() const { return muted; }
    void setMuted(bool isMuted) { muted = isMuted; }

    [[nodiscard]] bool isSoloed() const { return soloed; }
    void setSoloed(bool isSoloed) { soloed = isSoloed; }

    [[nodiscard]] bool isRecordArmed() const { return recordArmed; }
    void setRecordArmed(bool isArmed) { recordArmed = isArmed; }

    [[nodiscard]] uint32_t getId() const { return id; }
    [[nodiscard]] int getIndex() const { return index; }
    void setIndex(int newIndex) { index = newIndex; }

private:
    uint32_t id;
    std::string name;
    juce::Colour color;
    bool visible = true;
    float gainDb = 0.0f;
    float pan = 0.0f;
    bool muted = false;
    bool soloed = false;
    bool recordArmed = false;
    int index = -1;

    static uint32_t nextId;
    static uint32_t generateId();
};

} // namespace daw::project

