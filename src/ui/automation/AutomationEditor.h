/**
 * @file AutomationEditor.h
 * @brief Automation lanes editor header
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace daw::ui::automation {

/**
 * @brief Automation lanes editor with hierarchical layer support
 *
 * Features:
 * - Multiple automation lanes
 * - Base/Override/Macro layer types
 * - Grid snapping
 * - Multiple curve types
 */
class AutomationEditor : public juce::Component {
public:
    AutomationEditor();
    ~AutomationEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // View control
    void setViewRange(double startTime, double endTime);
    void setZoomLevel(float zoom);
    [[nodiscard]] float getZoomLevel() const;

    // Grid
    void setShowGrid(bool show);
    [[nodiscard]] bool isGridVisible() const;
    void setSnapToGrid(bool snap);
    [[nodiscard]] bool isSnapToGridEnabled() const;
    void setGridSize(double beats);
    [[nodiscard]] double getGridSize() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationEditor)
};

}  // namespace daw::ui::automation
