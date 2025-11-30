#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace daw::ui::utils
{

/**
 * @brief Helper for efficient dirty-rect repainting
 * 
 * Accumulates dirty regions and repaints only those areas.
 * Follows DAW_DEV_RULES: 60fps target, dirty-rect repainting.
 */
class RepaintHelper
{
public:
    RepaintHelper() = default;
    ~RepaintHelper() = default;

    /**
     * @brief Mark a region as dirty
     */
    void markDirty(juce::Rectangle<int> region) noexcept;

    /**
     * @brief Mark entire component as dirty
     */
    void markAllDirty() noexcept { allDirty = true; }

    /**
     * @brief Repaint accumulated dirty regions on a component
     */
    void repaintDirtyRegions(juce::Component& component) noexcept;

    /**
     * @brief Clear all dirty regions
     */
    void clear() noexcept;

    /**
     * @brief Check if there are dirty regions
     */
    [[nodiscard]] bool hasDirtyRegions() const noexcept { return allDirty || !dirtyRegions.empty(); }

private:
    std::vector<juce::Rectangle<int>> dirtyRegions;
    bool allDirty{false};
    static constexpr size_t maxRegions = 32; // Limit to prevent excessive repaints
};

/**
 * @brief Utility function for targeted repaint
 * 
 * Repaints only the specified region, or entire component if region is empty.
 */
inline void repaintRegion(juce::Component& component, const juce::Rectangle<int>& region) noexcept
{
    if (region.isEmpty())
        component.repaint();
    else
        component.repaint(region);
}

} // namespace daw::ui::utils

