#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include "../lookandfeel/DesignSystem.h"
#include "../../project/ProjectModel.h"
#include "../../project/Track.h"
#include "../../project/Clip.h"
#include "../../project/ClipContainer.h"
#include "../../project/Pattern.h"
#include "../../audio/engine/EngineContext.h"
#include <memory>

namespace daw::ui::views
{

/**
 * @brief Inspector panel
 *
 * Displays and allows editing of selected track or clip properties.
 * Follows DAW_DEV_RULES: uses design system, updates model immediately.
 */
class InspectorPanel : public juce::Component
{
public:
    InspectorPanel(std::shared_ptr<daw::project::ProjectModel> projectModel,
                   std::shared_ptr<daw::audio::engine::EngineContext> engineContext);
    ~InspectorPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void refresh();

private:
    std::shared_ptr<daw::project::ProjectModel> projectModel;
    std::shared_ptr<daw::audio::engine::EngineContext> engineContext;

    juce::Label titleLabel;
    juce::Label nameLabel;
    juce::TextEditor nameEditor;
    juce::Label colorLabel;
    juce::TextButton colorButton;

    // Track-specific
    juce::Label gainLabel;
    juce::Slider gainSlider;
    juce::Label panLabel;
    juce::Slider panSlider;

    // Clip-specific
    juce::Label startLabel;
    juce::TextEditor startEditor;
    juce::Label lengthLabel;
    juce::TextEditor lengthEditor;

    // Pattern-specific
    juce::Label stepsLabel;
    juce::Slider stepsSlider;
    juce::Label swingLabel;
    juce::Slider swingSlider;

    // Container-specific
    juce::TextButton collapseButton;

    daw::project::Track* currentTrack;
    daw::project::Clip* currentClip;
    daw::project::ClipContainer* currentContainer;
    daw::project::Pattern* currentPattern;

    void setupUI();
    void updateFromSelection();
    void nameChanged();
    void colorChanged();
    void gainChanged();
    void panChanged();
    void startChanged();
    void lengthChanged();
    void stepsChanged();
    void swingChanged();
    void collapseToggled();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InspectorPanel)
};

} // namespace daw::ui::views

