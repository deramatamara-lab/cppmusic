#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/DesignSystem.h"
#include "../../audio/engine/EngineContext.h"
#include "../../project/ProjectModel.h"
#include "MixerStrip.h"
#include <memory>
#include <vector>

namespace daw::ui::views
{

/**
 * @brief Mixer view container
 *
 * Horizontal scrollable container of MixerStrip components.
 * Follows DAW_DEV_RULES: responsive layout, uses design system.
 */
class MixerView : public juce::Component
{
public:
    MixerView(std::shared_ptr<daw::audio::engine::EngineContext> engineContext,
               std::shared_ptr<daw::project::ProjectModel> projectModel);
    ~MixerView() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void refreshStrips();

private:
    std::shared_ptr<daw::audio::engine::EngineContext> engineContext;
    std::shared_ptr<daw::project::ProjectModel> projectModel;

    juce::Viewport viewport;
    juce::Component stripsContainer;
    std::vector<std::unique_ptr<MixerStrip>> strips;

    void rebuildStrips();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerView)
};

} // namespace daw::ui::views

