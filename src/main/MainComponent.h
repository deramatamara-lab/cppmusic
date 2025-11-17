#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../ui/lookandfeel/MainLookAndFeel.h"
#include "../ui/lookandfeel/DesignTokens.h"
#include "../ui/components/WaveformViewer.h"
#include "../ui/components/ControlPanel.h"
#include "../ui/components/FlagshipDevicePanel.h"
#include "../ui/components/PatternSequencerPanel.h"
#include "../ui/components/SessionLauncherView.h"

namespace daw
{

/**
 * @brief Main window component for the DAW
 * 
 * Follows DAW_DEV_RULES: responsive, dockable, professional UX
 */
class MainComponent : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    std::unique_ptr<daw::ui::lookandfeel::MainLookAndFeel> lookAndFeel;
    const daw::ui::lookandfeel::DesignTokens* tokens { nullptr };
    
    // UI Components
    daw::ui::components::WaveformViewer waveformViewer;
    daw::ui::components::ControlPanel controlPanel;
    daw::ui::components::FlagshipDevicePanel flagshipPanel;
    daw::ui::components::PatternSequencerPanel patternSequencer;
    daw::ui::components::SessionLauncherView sessionLauncher;
    
    // Header
    juce::Label titleLabel;
    
    void setupUI();
    void applyDesignSystem();
};

} // namespace daw

