#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../audio/engine/EngineContext.h"
#include "../lookandfeel/DesignSystem.h"
#include <memory>

namespace daw::ui::views
{

/**
 * @brief Transport control bar
 * 
 * Provides play/stop/record buttons, tempo/time signature controls,
 * position display, and CPU meter.
 * Follows DAW_DEV_RULES: uses design system, real-time updates.
 */
class TransportBar : public juce::Component, public juce::Timer
{
public:
    explicit TransportBar(std::shared_ptr<daw::audio::engine::EngineContext> engineContext);
    ~TransportBar() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    std::shared_ptr<daw::audio::engine::EngineContext> engineContext;
    
    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::TextButton recordButton;
    
    juce::Label tempoLabel;
    juce::Slider tempoSlider;
    juce::Label tempoValueLabel;
    
    juce::Label timeSigLabel;
    juce::Label timeSigValueLabel;
    
    juce::Label positionLabel;
    juce::Label positionValueLabel;
    
    juce::Label cpuLabel;
    juce::Label cpuValueLabel;
    
    void setupUI();
    void updatePositionDisplay();
    void updateCpuDisplay();
    
    void playButtonClicked();
    void stopButtonClicked();
    void recordButtonClicked();
    void tempoChanged();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};

} // namespace daw::ui::views

