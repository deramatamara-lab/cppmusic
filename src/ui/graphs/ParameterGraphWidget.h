/**
 * @file ParameterGraphWidget.h
 * @brief Parameter modulation graph visualization header
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace daw::ui::graphs {

/**
 * @brief Visualization of parameter modulation graph
 *
 * Shows parameters as nodes and modulation connections as edges.
 * Supports interactive editing of the graph structure.
 */
class ParameterGraphWidget : public juce::Component {
public:
    ParameterGraphWidget();
    ~ParameterGraphWidget() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // View control
    void setZoomLevel(float zoom);
    [[nodiscard]] float getZoomLevel() const;
    void setPan(float x, float y);
    void resetView();

    // Display options
    void setShowValues(bool show);
    [[nodiscard]] bool isShowingValues() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterGraphWidget)
};

}  // namespace daw::ui::graphs
