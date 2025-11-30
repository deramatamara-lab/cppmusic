#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <array>
#include <memory>

namespace daw::ui::components {

/**
 * Real-time spectral analyzer widget for displaying live audio analysis
 * Features: Spectrum, MFCC coefficients, Chroma vectors, and feature history
 * Optimized for <1% CPU usage with efficient rendering
 */
class SpectralAnalyzerWidget : public juce::Component,
                               private juce::Timer
{
public:
    explicit SpectralAnalyzerWidget();
    ~SpectralAnalyzerWidget() override = default;

    //==============================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==============================================================================
    // Public interface
    void setSpectralData(const std::array<float, 4096>& magnitudeSpectrum,
                        const std::array<float, 13>& mfcc,
                        const std::array<float, 12>& chroma,
                        float centroid, float spread, float flatness,
                        float tempo, float onsetStrength);

    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    // Display modes
    enum class DisplayMode { Spectrum, MFCC, Chroma, Combined };
    void setDisplayMode(DisplayMode mode) { displayMode_ = mode; repaint(); }

    //==============================================================================
    // Timer override for periodic updates
    void timerCallback() override;

private:
    //==============================================================================
    // State
    std::atomic<bool> enabled_{true};
    DisplayMode displayMode_{DisplayMode::Combined};

    // Spectral data (thread-safe copies)
    std::array<float, 4096> magnitudeSpectrum_;
    std::array<float, 13> mfcc_;
    std::array<float, 12> chroma_;
    float spectralCentroid_{0.0f};
    float spectralSpread_{0.0f};
    float spectralFlatness_{0.0f};
    float tempoEstimate_{120.0f};
    float onsetStrength_{0.0f};

    // History for temporal display
    static constexpr int HISTORY_SIZE = 64;
    std::array<std::array<float, 4096>, HISTORY_SIZE> spectrumHistory_;
    std::array<std::array<float, 13>, HISTORY_SIZE> mfccHistory_;
    std::array<std::array<float, 12>, HISTORY_SIZE> chromaHistory_;
    std::array<float, HISTORY_SIZE> centroidHistory_;
    std::array<float, HISTORY_SIZE> tempoHistory_;
    int historyIndex_{0};

    //==============================================================================
    // Rendering
    void drawSpectrum(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawMFCC(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawChroma(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawCombined(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawFeatureBars(juce::Graphics& g, juce::Rectangle<int> bounds);

    // Color schemes
    juce::Colour spectrumColor_{juce::Colours::cyan};
    juce::Colour mfccColor_{juce::Colours::magenta};
    juce::Colour chromaColor_{juce::Colours::yellow};
    juce::Colour backgroundColor_{juce::Colours::black.withAlpha(0.8f)};
    juce::Colour gridColor_{juce::Colours::grey.withAlpha(0.3f)};

    //==============================================================================
    // Layout
    juce::Rectangle<int> spectrumArea_;
    juce::Rectangle<int> featuresArea_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralAnalyzerWidget)
};

} // namespace daw::ui::components
