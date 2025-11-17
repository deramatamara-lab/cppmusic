#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "ClipLauncherSystem.h"
#include "../lookandfeel/DesignTokens.h"

namespace daw::ui::components
{

class SessionLauncherView : public juce::Component
{
public:
    SessionLauncherView();
    ~SessionLauncherView() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setTempo(double bpm);
    void setIsPlaying(bool isPlaying);
    void setLooping(bool shouldLoop);

private:
    const daw::ui::lookandfeel::DesignTokens* tokens { nullptr };
    ClipLauncherSystem clipLauncher;
    juce::Label headerLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionLauncherView)
};

} // namespace daw::ui::components
