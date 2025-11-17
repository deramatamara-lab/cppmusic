#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/DesignSystem.h"
#include "../../project/ProjectModel.h"
#include "../../audio/engine/EngineContext.h"
#include <memory>

namespace daw::ui::views
{

/**
 * @brief Arrangement/timeline view
 * 
 * Displays timeline with ruler, grid, track rows, and clips.
 * Supports clip selection and dragging.
 * Follows DAW_DEV_RULES: uses design system, responsive layout.
 */
class ArrangeView : public juce::Component
{
public:
    ArrangeView(std::shared_ptr<daw::project::ProjectModel> projectModel,
                std::shared_ptr<daw::audio::engine::EngineContext> engineContext);
    ~ArrangeView() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    
    void refresh();

private:
    std::shared_ptr<daw::project::ProjectModel> projectModel;
    std::shared_ptr<daw::audio::engine::EngineContext> engineContext;
    
    double pixelsPerBeat;
    int trackHeight;
    int rulerHeight;
    
    bool isDragging;
    uint32_t draggedClipId;
    double dragStartBeats;
    juce::Point<int> dragStartPos;
    
    void drawRuler(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawGrid(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawTracks(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawClips(juce::Graphics& g, juce::Rectangle<int> bounds);
    
    juce::Rectangle<int> getClipBounds(const daw::project::Clip* clip) const;
    daw::project::Clip* getClipAtPosition(juce::Point<int> pos) const;
    double beatsFromX(int x) const;
    int xFromBeats(double beats) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangeView)
};

} // namespace daw::ui::views

