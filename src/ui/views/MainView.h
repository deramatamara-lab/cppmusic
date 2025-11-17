#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/DesignSystem.h"
#include "../../audio/engine/EngineContext.h"
#include "../../project/ProjectModel.h"
#include "TransportBar.h"
#include "BrowserPanel.h"
#include "ArrangeView.h"
#include "InspectorPanel.h"
#include "MixerView.h"
#include "../components/FlagshipDevicePanel.h"
#include "../components/PatternSequencerPanel.h"
#include "../components/SessionLauncherView.h"
#include <memory>

namespace daw::ui::views
{

/**
 * @brief Main DAW view
 * 
 * Root component that arranges all main UI areas:
 * - Top: TransportBar
 * - Left: BrowserPanel
 * - Center: ArrangeView
 * - Right: InspectorPanel
 * - Bottom: MixerView
 * 
 * Follows DAW_DEV_RULES: responsive layout, uses design system.
 */
class MainView : public juce::Component
{
public:
    MainView(std::shared_ptr<daw::audio::engine::EngineContext> engineContext);
    ~MainView() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void refreshViews();

private:
    std::shared_ptr<daw::audio::engine::EngineContext> engineContext;
    std::shared_ptr<daw::project::ProjectModel> projectModel;
    
    TransportBar transportBar;
    BrowserPanel browserPanel;
    ArrangeView arrangeView;
    InspectorPanel inspectorPanel;
    MixerView mixerView;
    daw::ui::components::FlagshipDevicePanel flagshipPanel;
    daw::ui::components::PatternSequencerPanel patternSequencer;
    daw::ui::components::SessionLauncherView sessionLauncher;
    
    void setupUI();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainView)
};

} // namespace daw::ui::views

