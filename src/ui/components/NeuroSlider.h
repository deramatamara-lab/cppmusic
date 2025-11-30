#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/DesignSystem.h"
#include <functional>
#include <memory>

namespace daw::ui
{
namespace animation { class AdaptiveAnimationService; }

namespace components
{

/**
 * NeuroSlider - Ultra-professional slider with advanced feedback
 *
 * Features:
 * - Multiple groove styles (horizontal, vertical, circular, arc)
 * - Real-time value popup with smooth positioning
 * - Audio-reactive visual feedback
 * - Professional bezier curve value mapping
 * - Gesture recognition (double-click reset, right-click context)
 * - Accessibility-compliant (keyboard navigation, screen reader support)
 * - 60fps smooth animation and interpolation
 */
class NeuroSlider : public juce::Slider
{
public:
    //==============================================================================
    // SLIDER STYLES - Professional appearance options
    enum class Style {
        Linear,         // Traditional horizontal/vertical slider
        Circular,       // Rotary knob style
        Arc,           // Partial circle (like vintage hardware)
        Waveform,      // Audio waveform style with peaks
        Spectrum       // Frequency spectrum style
    };

    enum class Orientation {
        Horizontal,
        Vertical,
        Radial
    };

    //==============================================================================
    // VALUE MAPPING - Professional parameter control
    struct ValueMapping {
        double minValue = 0.0;
        double maxValue = 1.0;
        double defaultValue = 0.5;
        double interval = 0.0;          // 0.0 = continuous
        bool logarithmic = false;       // Exponential scaling
        double skewFactor = 1.0;        // Bias towards low/high values
        juce::String suffix;            // "Hz", "dB", "%", etc.
        int decimalPlaces = 2;

        ValueMapping() = default;
        ValueMapping(double min, double max, double def = -1.0)
            : minValue(min), maxValue(max), defaultValue(def < 0 ? min : def) {}
    };

    //==============================================================================
    NeuroSlider(Style style = Style::Linear);
    ~NeuroSlider() override;

    //==============================================================================
    // VALUE MANAGEMENT

    /** Set value mapping parameters */
    void setValueMapping(const ValueMapping& mapping);
    const ValueMapping& getValueMapping() const { return valueMapping; }

    //==============================================================================
    // APPEARANCE CONFIGURATION

    /** Set slider style */
    void setStyle(Style newStyle);
    Style getStyle() const { return sliderStyle; }

    /** Set orientation */
    void setOrientation(Orientation orientation);
    Orientation getOrientation() const { return sliderOrientation; }

    //==============================================================================
    // INTERACTION CALLBACKS

    /** Called when value changes */
    std::function<void(double)> onValueChange;

    /** Called when dragging starts */
    std::function<void()> onDragStart;

    /** Called when dragging ends */
    std::function<void()> onDragEnd;

    //==============================================================================
    // AUDIO REACTIVITY

    /** Enable audio-reactive visual feedback */
    void setAudioReactive(bool enabled, float sensitivity = 1.0f);
    void updateAudioLevel(float level);

    //==============================================================================
    // JUCE Overrides
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

private:
    Style sliderStyle = Style::Linear;
    Orientation sliderOrientation = Orientation::Horizontal;
    ValueMapping valueMapping;

    bool isDragging = false;
    bool isMouseOver = false;
    bool audioReactive = false;
    float audioSensitivity = 1.0f;
    float audioLevel = 0.0f;

    std::weak_ptr<animation::AdaptiveAnimationService> animationService;
    float hoverAmount = 0.0f;
    float glowAmount = 0.0f;
    uint32_t hoverAnimationId = 0;
    uint32_t glowAnimationId = 0;

    //==============================================================================
    // RENDERING METHODS

    /** Paint linear slider style */
    void paintLinearSlider(juce::Graphics& g, juce::Rectangle<float> bounds);

    /** Paint circular slider style */
    void paintCircularSlider(juce::Graphics& g, juce::Rectangle<float> bounds);

    /** Paint arc slider style */
    void paintArcSlider(juce::Graphics& g, juce::Rectangle<float> bounds);

    /** Paint waveform slider style */
    void paintWaveformSlider(juce::Graphics& g, juce::Rectangle<float> bounds);

    /** Paint spectrum slider style */
    void paintSpectrumSlider(juce::Graphics& g, juce::Rectangle<float> bounds);

    void animateState(float target, float durationMs, float& storage, uint32_t& handle);
    void cancelAnimation(uint32_t& handle);
    [[nodiscard]] float getNormalizedValue() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeuroSlider)
};

} // namespace components
} // namespace daw::ui

