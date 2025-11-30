#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "audio/AnalogModeledEQ.h"
#include "core/EngineContext.h"

namespace cppmusic {
namespace ui {

/**
 * AnalogEQEditor - Professional studio-ready UI for AnalogModeledEQ
 *
 * Features:
 * - Per-band parameter knobs with real-time visual feedback
 * - Live frequency response plot with individual band overlays
 * - A/B preset comparison with morphing
 * - Vintage hardware model selection
 * - Real-time spectrum analyzer overlay
 * - Professional studio styling with analog-inspired design
 *
 * Real-time safe parameter updates via EngineContext messaging
 */
class AnalogEQEditor : public juce::Component,
                       public juce::Timer,
                       public juce::Slider::Listener,
                       public juce::Button::Listener,
                       public juce::ComboBox::Listener
{
public:
    explicit AnalogEQEditor(audio::AnalogModeledEQ& eq, daw::core::EngineContext& context);
    ~AnalogEQEditor() override;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    // Listener implementations
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    void timerCallback() override;

    // Preset management
    void savePresetA();
    void savePresetB();
    void loadPresetA();
    void loadPresetB();
    void morphPresets(float amount); // 0.0 = A, 1.0 = B

    // UI State
    void updateFromEQ();
    void setAnalyzerEnabled(bool enabled);

private:
    // Core references
    audio::AnalogModeledEQ& eq_;
    daw::core::EngineContext& engineContext_;

    // Band parameter controls (5 bands)
    struct BandControls {
        std::unique_ptr<juce::Slider> frequencyKnob;
        std::unique_ptr<juce::Slider> gainKnob;
        std::unique_ptr<juce::Slider> qKnob;
        std::unique_ptr<juce::Slider> driveKnob;
        std::unique_ptr<juce::Slider> saturationKnob;
        std::unique_ptr<juce::Slider> mixKnob;
        std::unique_ptr<juce::ComboBox> typeCombo;
        std::unique_ptr<juce::ComboBox> slopeCombo;
        std::unique_ptr<juce::ToggleButton> enableButton;
        std::unique_ptr<juce::ToggleButton> soloButton;
        std::unique_ptr<juce::ToggleButton> bypassButton;

        juce::Rectangle<int> bounds;
        juce::Colour bandColour;
        bool isMouseOver = false;
    };
    std::array<BandControls, audio::AnalogModeledEQ::NUM_BANDS> bandControls_;

    // Global controls
    std::unique_ptr<juce::Slider> inputGainKnob_;
    std::unique_ptr<juce::Slider> outputGainKnob_;
    std::unique_ptr<juce::Slider> transformerDriveKnob_;
    std::unique_ptr<juce::Slider> tubeWarmthKnob_;
    std::unique_ptr<juce::Slider> tapeSaturationKnob_;
    std::unique_ptr<juce::Slider> analogNoiseKnob_;
    std::unique_ptr<juce::ComboBox> analogModelCombo_;

    // Preset controls
    std::unique_ptr<juce::TextButton> presetAButton_;
    std::unique_ptr<juce::TextButton> presetBButton_;
    std::unique_ptr<juce::TextButton> saveAButton_;
    std::unique_ptr<juce::TextButton> saveBButton_;
    std::unique_ptr<juce::Slider> morphSlider_;
    std::unique_ptr<juce::ToggleButton> analyzerButton_;

    // Visual components
    juce::Rectangle<int> responseArea_;
    juce::Rectangle<int> analyzerArea_;

    // Response plot data
    std::array<float, 512> frequencyResponse_;
    std::array<float, 512> phaseResponse_;
    std::array<std::array<float, 512>, audio::AnalogModeledEQ::NUM_BANDS> bandResponses_;

    // Analyzer data
    std::array<float, 512> spectrumData_;
    std::array<float, 512> peakHoldData_;
    std::array<int, 512> peakHoldTime_;
    bool analyzerEnabled_ = false;

    // Preset storage
    audio::AnalogModeledEQ::Preset presetA_;
    audio::AnalogModeledEQ::Preset presetB_;
    bool hasPresetA_ = false;
    bool hasPresetB_ = false;

    // Visual state
    int hoveredBand_ = -1;
    float morphAmount_ = 0.0f;

    // Update management
    bool needsUpdate_ = true;
    std::atomic<bool> parametersChanged_{false};

    // UI Creation helpers
    void createBandControls();
    void createGlobalControls();
    void createPresetControls();
    void createAnalyzerControls();

    // Knob styling
    void styleKnob(juce::Slider& knob, const juce::String& suffix = "");
    void styleButton(juce::Button& button);
    void styleComboBox(juce::ComboBox& combo);

    // Parameter mapping
    void updateEQParameter(int bandIndex, const juce::String& parameter, float value);
    void updateGlobalParameter(const juce::String& parameter, float value);

    // Visual updates
    void updateFrequencyResponse();
    void updateBandResponse(int bandIndex);
    void updateSpectrumAnalyzer();

    // Drawing helpers
    void drawFrequencyResponse(juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawSpectrumAnalyzer(juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawBandHandle(juce::Graphics& g, int bandIndex, const juce::Rectangle<int>& area);
    void drawGrid(juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawFrequencyLabels(juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawGainLabels(juce::Graphics& g, const juce::Rectangle<int>& area);

    // Color scheme
    juce::Colour getBackgroundColour() const;
    juce::Colour getPanelColour() const;
    juce::Colour getTextColour() const;
    juce::Colour getHighlightColour() const;
    juce::Colour getBandColour(int bandIndex) const;
    juce::Colour getGridColour() const;

    // Utility
    float frequencyToX(float frequency, const juce::Rectangle<int>& area) const;
    float gainToY(float gainDB, const juce::Rectangle<int>& area) const;
    float xToFrequency(float x, const juce::Rectangle<int>& area) const;
    float yToGain(float y, const juce::Rectangle<int>& area) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalogEQEditor)
};

/**
 * Custom LookAndFeel for professional analog-style EQ interface
 */
class AnalogEQLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AnalogEQLookAndFeel();
    ~AnalogEQLookAndFeel() override = default;

    // Slider customization
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                         juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float minSliderPos, float maxSliderPos,
                         const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    // Button customization
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                             const juce::Colour& backgroundColour,
                             bool shouldDrawButtonAsHighlighted,
                             bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                       bool shouldDrawButtonAsHighlighted,
                       bool shouldDrawButtonAsDown) override;

    // ComboBox customization
    void drawComboBox(juce::Graphics& g, int width, int height,
                     bool isButtonDown, int buttonX, int buttonY,
                     int buttonW, int buttonH, juce::ComboBox& box) override;

    // Label customization
    void drawLabel(juce::Graphics& g, juce::Label& label) override;

    // Popup menu customization
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;
    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                          bool isSeparator, bool isActive, bool isHighlighted,
                          bool isTicked, bool hasSubMenu, const juce::String& text,
                          const juce::String& shortcutKeyText,
                          const juce::Drawable* icon, const juce::Colour* textColour) override;

private:
    // Color palette
    juce::Colour analogWarmth_ = juce::Colour(0xff2a1810);  // Warm brown
    juce::Colour analogMetal_ = juce::Colour(0xff404040);   // Dark metal
    juce::Colour analogGold_ = juce::Colour(0xffd4af37);    // Gold accents
    juce::Colour analogGreen_ = juce::Colour(0xff00ff41);   // VU green
    juce::Colour analogRed_ = juce::Colour(0xffff4444);     // Warning red
    juce::Colour analogCream_ = juce::Colour(0xfffff8dc);   // Cream text

    // Gradients and effects
    juce::ColourGradient createMetalGradient(const juce::Rectangle<float>& area) const;
    juce::ColourGradient createKnobGradient(const juce::Rectangle<float>& area) const;
    void addInnerShadow(juce::Graphics& g, const juce::Rectangle<float>& area) const;
    void addOuterGlow(juce::Graphics& g, const juce::Rectangle<float>& area, juce::Colour glowColour) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalogEQLookAndFeel)
};

} // namespace ui
} // namespace cppmusic
