#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "StepSequencer.h"
#include "../lookandfeel/DesignTokens.h"

namespace daw::ui::components
{

class PatternSequencerPanel : public juce::Component
{
public:
    PatternSequencerPanel();
    ~PatternSequencerPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setTempo(double bpm);
    void setIsPlaying(bool isPlaying);

private:
    const daw::ui::lookandfeel::DesignTokens* tokens { nullptr };
    StepSequencer stepSequencer;
    juce::Label headerLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternSequencerPanel)
};

} // namespace daw::ui::components
