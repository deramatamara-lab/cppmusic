#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../audio/engine/EngineContext.h"
#include "../lookandfeel/DesignSystem.h"
#include "../lookandfeel/UltraDesignSystem.hpp"
#include "../core/AnimationHelper.h"
#include "../../core/ServiceLocator.h"
#include "../animation/AdaptiveAnimationService.h"
#include <memory>

namespace daw::ui::views
{

class TransportIconButton : public juce::Button
{
public:
    TransportIconButton(const juce::String& buttonName, bool isToggle = false);

    void setIcons(const juce::Path& defaultIcon, const juce::Path& toggledIcon = {});
    void setColourScheme(juce::Colour normalFill,
                         juce::Colour hoverFill,
                         juce::Colour downFill,
                         juce::Colour toggledFill,
                         juce::Colour normalIconColour,
                         juce::Colour toggledIconColour);

    void paintButton(juce::Graphics&, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    juce::Path iconOff;
    juce::Path iconOn;

    juce::Colour fillNormal;
    juce::Colour fillHover;
    juce::Colour fillDown;
    juce::Colour fillToggled;
    juce::Colour iconColourNormal;
    juce::Colour iconColourToggled;

    std::weak_ptr<daw::ui::animation::AdaptiveAnimationService> animationService;
    float hoverAmount { 0.0f };
    float pressAmount { 0.0f };
    uint32_t hoverAnimationId { 0 };
    uint32_t pressAnimationId { 0 };

    void animateState(float target,
                      float durationMs,
                      float& storage,
                      uint32_t& handle);
    void cancelAnimation(uint32_t& handle);
};

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
    daw::ui::utils::AnimationHelper animationHelper;

    TransportIconButton playButton;
    TransportIconButton stopButton;
    TransportIconButton recordButton;
    ultra::TabBarPro patternSongToggle;
    ultra::PillToggle metronomeToggle;
    juce::Slider metronomeVolumeSlider;
    ultra::PillToggle loopToggle;

    // Snap controls
    ultra::PillToggle snapToggle;
    juce::ComboBox snapDivisionCombo;

    juce::Label tempoLabel;
    juce::Slider tempoSlider;
    juce::Label tempoValueLabel;

    juce::Label timeSigLabel;
    juce::Label timeSigValueLabel;

    juce::Label positionLabel;
    juce::Label positionValueLabel;

    juce::Label cpuLabel;
    juce::Label cpuValueLabel;

    bool isRecording { false };
    bool metronomeOn { false };
    bool loopOn { false };
    float cpuSmoothed { 0.0f };

    enum class PlayMode { Pattern, Song };
    PlayMode currentPlayMode { PlayMode::Pattern };

    bool snapEnabled { true };
    double snapDivision { 0.25 }; // 1/16th note

    void setupUI();
    void updatePositionDisplay();
    void updateCpuDisplay();

    void playButtonClicked();
    void stopButtonClicked();
    void recordButtonClicked();
    void metronomeButtonClicked();
    void loopButtonClicked();
    void patternSongModeChanged(int selectedTab);
    void snapToggleClicked();
    void snapDivisionChanged();
    void tempoChanged();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};

} // namespace daw::ui::views
