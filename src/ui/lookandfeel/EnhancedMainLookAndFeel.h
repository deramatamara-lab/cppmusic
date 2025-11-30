#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "MainLookAndFeel.h"
#include "../effects/GlassMorphismRenderer.h"

namespace daw::ui::lookandfeel
{

/**
 * EnhancedMainLookAndFeel - Extends MainLookAndFeel with UltraLookAndFeel techniques
 *
 * Features:
 * - Professional panel backgrounds with glassmorphism
 * - Enhanced meters with gradients
 * - Professional waveform rendering
 * - Smooth animations
 * - Glow effects
 * - Advanced typography system
 * - Professional spacing system
 */
class EnhancedMainLookAndFeel final : public MainLookAndFeel
{
public:
    explicit EnhancedMainLookAndFeel(Theme theme = Theme::Dark);
    ~EnhancedMainLookAndFeel() override = default;

    // Enhanced panel rendering
    void drawEnhancedPanel(juce::Graphics& g, juce::Rectangle<float> bounds,
                          bool isHighlighted = false, bool useGlassmorphism = true);

    // Enhanced meter rendering
    void drawProfessionalMeter(juce::Graphics& g, juce::Rectangle<float> bounds,
                              float level, float peakLevel, bool isVertical = true);

    // Professional waveform rendering
    void drawProfessionalWaveform(juce::Graphics& g, juce::Rectangle<float> bounds,
                                 const juce::AudioBuffer<float>& audioData,
                                 juce::Colour waveformColor = juce::Colour(DesignSystem::Colors::accent));

    // Enhanced button with glow
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

private:
    void drawGlowEffect(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                       juce::Colour glowColor, float intensity = 0.5f);
    void drawGradientMeter(juce::Graphics& g, juce::Rectangle<float> bounds,
                          float level, bool isVertical);
};

} // namespace daw::ui::lookandfeel
