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

private:
    uint32_t id;
    uint32_t trackId;
    double startBeats;
    double lengthBeats;
    std::string label;
    
    static uint32_t nextId;
    static uint32_t generateId();
};

} // namespace daw::project

