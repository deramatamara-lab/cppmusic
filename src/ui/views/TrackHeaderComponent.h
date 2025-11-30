#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/DesignSystem.h"
#include "../lookandfeel/UltraDesignSystem.hpp"
#include "../../project/ProjectModel.h"
#include "../../audio/engine/EngineContext.h"
#include <memory>

namespace daw::ui::views
{

/**
 * @brief FL Studio-style track header component
 *
 * Provides comprehensive track control with mute/solo/record buttons,
 * volume fader, pan control, level meter, and editable track name.
 * Follows DAW_DEV_RULES: uses design system, responsive layout.
 */
class TrackHeaderComponent : public juce::Component
{
public:
    TrackHeaderComponent(std::shared_ptr<daw::audio::engine::EngineContext> engineContext,
                        daw::project::Track* track);
    ~TrackHeaderComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    // Track control callbacks
    std::function<void(int trackIndex, bool muted)> onMuteChanged;
    std::function<void(int trackIndex, bool soloed)> onSoloChanged;
    std::function<void(int trackIndex, bool armed)> onRecordArmChanged;
    std::function<void(int trackIndex, float gainDb)> onVolumeChanged;
    std::function<void(int trackIndex, float pan)> onPanChanged;
    std::function<void(int trackIndex, const juce::String& newName)> onNameChanged;

    // Update track state
    void setTrack(daw::project::Track* newTrack);
    void updateMeters();
    void refresh();

    // Layout constants
    static constexpr int kPreferredWidth = 200;
    static constexpr int kMinimumWidth = 120;

private:
    std::shared_ptr<daw::audio::engine::EngineContext> engineContext;
    daw::project::Track* track;

    // Control components
    ultra::PillToggle muteButton;
    ultra::PillToggle soloButton;
    ultra::PillToggle recordButton;

    juce::Slider volumeSlider;
    juce::Slider panSlider;
    juce::Label volumeLabel;
    juce::Label panLabel;

    juce::Label trackNameLabel;
    std::unique_ptr<juce::TextEditor> trackNameEditor;

    // Level meters
    float leftPeak{0.0f};
    float rightPeak{0.0f};
    float leftRms{0.0f};
    float rightRms{0.0f};

    // UI state
    bool isEditingName{false};
    bool showDetailedControls{true};

    void setupUI();
    void updateButtonStates();
    void startNameEdit();
    void finishNameEdit();
    void drawLevelMeter(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawTrackColourStrip(juce::Graphics& g, juce::Rectangle<float> bounds);

    // Button callbacks
    void muteButtonClicked();
    void soloButtonClicked();
    void recordButtonClicked();
    void volumeSliderChanged();
    void panSliderChanged();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackHeaderComponent)
};

} // namespace daw::ui::views
