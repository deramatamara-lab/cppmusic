#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <juce_graphics/juce_graphics.h>

namespace daw::project
{

/**
 * @brief Clip container model
 *
 * Groups multiple clips together for group operations (move, color, collapse/expand).
 * Follows DAW_DEV_RULES: clear model, no UI dependencies.
 */
class ClipContainer
{
public:
    ClipContainer();
    explicit ClipContainer(const std::string& name, juce::Colour color);
    ~ClipContainer() = default;

    // Non-copyable, movable
    ClipContainer(const ClipContainer&) = default;
    ClipContainer& operator=(const ClipContainer&) = default;
    ClipContainer(ClipContainer&&) noexcept = default;
    ClipContainer& operator=(ClipContainer&&) noexcept = default;

    [[nodiscard]] uint32_t getId() const { return id; }

    [[nodiscard]] std::string getName() const { return name; }
    void setName(const std::string& newName) { name = newName; }

    [[nodiscard]] juce::Colour getColor() const { return color; }
    void setColor(juce::Colour newColor) { color = newColor; }

    [[nodiscard]] bool isCollapsed() const { return collapsed; }
    void setCollapsed(bool newCollapsed) { collapsed = newCollapsed; }

    // Clip management
    void addClip(uint32_t clipId);
    void removeClip(uint32_t clipId);
    [[nodiscard]] const std::vector<uint32_t>& getClips() const { return clipIds; }
    [[nodiscard]] bool containsClip(uint32_t clipId) const;

private:
    uint32_t id;
    std::string name;
    juce::Colour color;
    bool collapsed = false;
    std::vector<uint32_t> clipIds;

    static uint32_t nextId;
    static uint32_t generateId();
};

} // namespace daw::project

