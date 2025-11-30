#pragma once

#include <array>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/DesignTokens.h"

namespace daw::ui::components
{

class FlagshipDevicePanel final : public juce::Component,
                                  private juce::Timer
{
public:
    FlagshipDevicePanel();
    ~FlagshipDevicePanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setTitle(const juce::String& newTitle);
    void setBackgroundImage(juce::Image newImage);

    /** Returns a macro slider for parameter attachments */
    juce::Slider& getMacroSlider(std::size_t index);

    /** Set macro label text */
    void setMacroLabel(std::size_t index, const juce::String& label);

private:
    void timerCallback() override;

    juce::String title;
    juce::Image backgroundImage;

    std::array<std::unique_ptr<juce::Slider>, 4> macroSliders;
    std::array<std::unique_ptr<juce::Label>, 4> macroLabels;

    float animationPhase = 0.0f;
};

} // namespace daw::ui::components
