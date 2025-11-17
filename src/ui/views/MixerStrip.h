#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/DesignSystem.h"
#include "../../audio/engine/EngineContext.h"
#include "../../project/Track.h"
#include <memory>

namespace daw::ui::views
{

/**
 * @brief Individual mixer strip
 * 
 * Displays fader, pan, mute/solo buttons, and level meters for a track.
 * Follows DAW_DEV_RULES: uses design system, real-time meter updates.
 */
class MixerStrip : public juce::Component, public juce::Timer
{
public:
    MixerStrip(std::shared_ptr<daw::audio::engine::EngineContext> engineContext,
               daw::project::Track* track, int trackIndex);
    ~MixerStrip() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    std::shared_ptr<daw::audio::engine::EngineContext> engineContext;
    daw::project::Track* track;
    int trackIndex;
    
    juce::Label trackNameLabel;
    juce::Slider fader;
    juce::Slider panSlider;
    juce::TextButton muteButton;
    juce::TextButton soloButton;
    
    float peakLevel;
    float rmsLevel;
    
    void setupUI();
    void updateMeters();
    void faderChanged();
    void panChanged();
    void muteButtonClicked();
    void soloButtonClicked();
    
    void drawMeter(juce::Graphics& g, juce::Rectangle<int> bounds, float level);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerStrip)
};

} // namespace daw::ui::views

