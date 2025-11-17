#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <atomic>
#include <deque>
#include <memory>
#include <vector>

#include "../lookandfeel/DesignTokens.h"

namespace daw::ui::components
{

/**
 * @brief Premium waveform + spectrum visualizer migrated from the legacy "oldbutgold" suite.
 */
class WaveformViewer : public juce::Component,
                       private juce::Timer
{
public:
    enum class VisualizationMode
    {
        Waveform = 0,
        Spectrum,
        Spectrogram,
        Combined
    };

    struct FFTConfig
    {
        int fftSize = 2048;
        int numBins = 512;
        float sampleRate = 44100.0f;
        float minFrequency = 20.0f;
        float maxFrequency = 20000.0f;
        bool useWindowing = true;
    };

    WaveformViewer();
    ~WaveformViewer() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void pushAudioData(const float* data, int numSamples, int numChannels = 2);
    void pushFFTData(const std::vector<float>& fftData);
    void setAudioBuffer(const juce::AudioBuffer<float>& buffer);
    void clearData();

    void setVisualizationMode(VisualizationMode mode);
    [[nodiscard]] VisualizationMode getVisualizationMode() const noexcept { return currentMode; }

    void setFFTConfig(const FFTConfig& config);
    [[nodiscard]] FFTConfig getFFTConfig() const noexcept { return fftConfig; }

    void setWaveformColor(juce::Colour color);
    void setSpectrumColor(juce::Colour color);
    void setBackgroundColor(juce::Colour color);
    void setGridColor(juce::Colour color);
    void setShowGrid(bool show);
    void setShowPeaks(bool show);
    void setSmoothScrolling(bool smooth);
    void setUpdateRateHz(int hertz);
    void setBufferSize(int size);
    void setNumHistoryBuffers(int numBuffers);

    [[nodiscard]] float getRMSLevel() const;
    [[nodiscard]] float getPeakLevel() const;
    [[nodiscard]] float getFrequencyForBin(int binIndex) const;
    [[nodiscard]] int   getBinForFrequency(float frequency) const;

    juce::Image createSnapshot(int width, int height);
    [[nodiscard]] std::vector<float> getCurrentSpectrum() const;
    [[nodiscard]] std::vector<float> getCurrentWaveform() const;

private:
    void timerCallback() override;

    void renderWaveform(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void renderSpectrum(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void renderSpectrogram(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void renderCombined(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void renderGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void renderPeaks(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    void performFFT();
    void updatePeakLevels();
    void applyWindowing();
    void normalizeSpectrum(std::vector<float>& spectrum) const;
    juce::Path createWaveformPath(const std::vector<float>& data, const juce::Rectangle<int>& bounds) const;
    juce::Path createSpectrumPath(const std::vector<float>& data, const juce::Rectangle<int>& bounds) const;

    [[nodiscard]] float linearToDecibels(float linear) const;
    [[nodiscard]] float decibelsToLinear(float dB) const;

    const daw::ui::lookandfeel::DesignTokens* tokens { nullptr };
    VisualizationMode currentMode { VisualizationMode::Waveform };
    FFTConfig fftConfig;

    juce::AudioBuffer<float> audioBuffer;
    std::vector<float> currentSpectrum;
    std::deque<std::vector<float>> spectrumHistory;
    std::vector<float> currentWaveform;
    std::vector<float> peakLevels;

    std::atomic<bool> needsRepaint { true };
    std::atomic<int> updateRateHz { 60 };
    std::atomic<int> bufferSize { 2048 };
    std::atomic<int> numHistoryBuffers { 32 };

    juce::Colour waveformColor;
    juce::Colour spectrumColor;
    juce::Colour backgroundColor;
    juce::Colour gridColor;

    std::atomic<bool> showGrid { true };
    std::atomic<bool> showPeaks { true };
    std::atomic<bool> smoothScrolling { true };

    std::unique_ptr<juce::dsp::FFT> fftProcessor;
    std::vector<float> fftData;
    std::vector<float> windowFunction;

    juce::CriticalSection dataLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformViewer)
};

} // namespace daw::ui::components

