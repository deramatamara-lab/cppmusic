#include "WaveformViewer.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace daw::ui::components
{

using daw::ui::lookandfeel::getDesignTokens;

namespace
{
constexpr float kMinPeakDisplay = 0.01f;
constexpr int kDefaultFftOrder = 11; // 2^11 = 2048
}

WaveformViewer::WaveformViewer()
{
    tokens = &getDesignTokens();

    waveformColor  = tokens->colours.accentPrimary;
    spectrumColor  = tokens->colours.accentSecondary;
    backgroundColor = tokens->colours.panelBackground;
    gridColor      = tokens->colours.panelBorder.withAlpha(0.4f);

    fftProcessor = std::make_unique<juce::dsp::FFT>(kDefaultFftOrder);
    fftData.resize(static_cast<size_t>(fftProcessor->getSize()) * 2u);

    windowFunction.resize(static_cast<size_t>(fftProcessor->getSize()));
    for (size_t i = 0; i < windowFunction.size(); ++i)
    {
        windowFunction[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * static_cast<float>(i)
                               / static_cast<float>(windowFunction.size() - 1)));
    }

    spectrumHistory.resize(static_cast<size_t>(numHistoryBuffers));
    for (auto& spectrum : spectrumHistory)
        spectrum.resize(fftConfig.numBins, 0.0f);

    startTimerHz(updateRateHz.load());
    setOpaque(true);
}

WaveformViewer::~WaveformViewer()
{
    stopTimer();
}

void WaveformViewer::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds();
    g.fillAll(backgroundColor);

    if (showGrid)
        renderGrid(g, bounds);

    switch (currentMode)
    {
        case VisualizationMode::Waveform:    renderWaveform(g, bounds); break;
        case VisualizationMode::Spectrum:    renderSpectrum(g, bounds); break;
        case VisualizationMode::Spectrogram: renderSpectrogram(g, bounds); break;
        case VisualizationMode::Combined:    renderCombined(g, bounds); break;
        default: break;
    }

    if (showPeaks)
        renderPeaks(g, bounds);
}

void WaveformViewer::resized()
{
    // Reserved for future layout
}

void WaveformViewer::timerCallback()
{
    if (needsRepaint.exchange(false))
        repaint();
}

void WaveformViewer::pushAudioData(const float* data, int numSamples, int numChannels)
{
    if (data == nullptr || numSamples <= 0)
        return;

    juce::ScopedLock lock(dataLock);
    audioBuffer.setSize(std::max(1, numChannels), numSamples, false, false, true);
    for (int channel = 0; channel < audioBuffer.getNumChannels(); ++channel)
    {
        const float* src = data + juce::jmin(channel, numChannels - 1) * numSamples;
        juce::FloatVectorOperations::copy(audioBuffer.getWritePointer(channel), src, numSamples);
    }

    currentWaveform.resize(static_cast<size_t>(numSamples));
    const float* mono = audioBuffer.getReadPointer(0);
    std::copy(mono, mono + numSamples, currentWaveform.begin());

    performFFT();
    updatePeakLevels();
    needsRepaint = true;
}

void WaveformViewer::pushFFTData(const std::vector<float>& newFftData)
{
    if (newFftData.empty())
        return;

    juce::ScopedLock lock(dataLock);
    currentSpectrum = newFftData;
    spectrumHistory.push_back(currentSpectrum);
    while (static_cast<int>(spectrumHistory.size()) > numHistoryBuffers)
        spectrumHistory.pop_front();

    needsRepaint = true;
}

void WaveformViewer::setAudioBuffer(const juce::AudioBuffer<float>& buffer)
{
    juce::ScopedLock lock(dataLock);
    audioBuffer = buffer;

    if (buffer.getNumSamples() == 0)
        return;

    currentWaveform.resize(static_cast<size_t>(buffer.getNumSamples()));
    const float* mono = buffer.getReadPointer(0);
    std::copy(mono, mono + buffer.getNumSamples(), currentWaveform.begin());

    performFFT();
    updatePeakLevels();
    needsRepaint = true;
}

void WaveformViewer::clearData()
{
    juce::ScopedLock lock(dataLock);
    audioBuffer.clear();
    currentSpectrum.clear();
    currentWaveform.clear();
    peakLevels.clear();

    for (auto& spectrum : spectrumHistory)
        std::fill(spectrum.begin(), spectrum.end(), 0.0f);

    needsRepaint = true;
}

void WaveformViewer::setVisualizationMode(VisualizationMode mode)
{
    if (currentMode == mode)
        return;

    currentMode = mode;
    needsRepaint = true;
}

void WaveformViewer::setFFTConfig(const FFTConfig& config)
{
    juce::ScopedLock lock(dataLock);
    fftConfig = config;

    int desiredOrder = 0;
    int size = fftConfig.fftSize;
    while (size > 1)
    {
        size >>= 1;
        ++desiredOrder;
    }

    if (fftProcessor == nullptr || fftProcessor->getSize() != fftConfig.fftSize)
    {
        fftProcessor = std::make_unique<juce::dsp::FFT>(desiredOrder);
        fftData.resize(static_cast<size_t>(fftProcessor->getSize()) * 2u);
        windowFunction.resize(static_cast<size_t>(fftProcessor->getSize()));
        for (size_t i = 0; i < windowFunction.size(); ++i)
        {
            windowFunction[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * static_cast<float>(i)
                                   / static_cast<float>(windowFunction.size() - 1)));
        }
    }

    needsRepaint = true;
}

void WaveformViewer::setWaveformColor(juce::Colour color)
{
    waveformColor = color;
    needsRepaint = true;
}

void WaveformViewer::setSpectrumColor(juce::Colour color)
{
    spectrumColor = color;
    needsRepaint = true;
}

void WaveformViewer::setBackgroundColor(juce::Colour color)
{
    backgroundColor = color;
    needsRepaint = true;
}

void WaveformViewer::setGridColor(juce::Colour color)
{
    gridColor = color;
    needsRepaint = true;
}

void WaveformViewer::setShowGrid(bool show)
{
    showGrid = show;
    needsRepaint = true;
}

void WaveformViewer::setShowPeaks(bool show)
{
    showPeaks = show;
    needsRepaint = true;
}

void WaveformViewer::setSmoothScrolling(bool smooth)
{
    smoothScrolling = smooth;
}

void WaveformViewer::setUpdateRateHz(int hertz)
{
    updateRateHz = juce::jlimit(1, 120, hertz);
    startTimerHz(updateRateHz.load());
}

void WaveformViewer::setBufferSize(int size)
{
    bufferSize = juce::jmax(256, size);
}

void WaveformViewer::setNumHistoryBuffers(int numBuffers)
{
    juce::ScopedLock lock(dataLock);
    numHistoryBuffers = juce::jmax(1, numBuffers);
    while (static_cast<int>(spectrumHistory.size()) > numHistoryBuffers)
        spectrumHistory.pop_front();
    while (static_cast<int>(spectrumHistory.size()) < numHistoryBuffers)
        spectrumHistory.emplace_back(fftConfig.numBins, 0.0f);
}

float WaveformViewer::getRMSLevel() const
{
    if (currentWaveform.empty())
        return 0.0f;

    const float sum = std::accumulate(currentWaveform.begin(), currentWaveform.end(), 0.0f,
                                      [](float acc, float sample) { return acc + sample * sample; });
    return std::sqrt(sum / static_cast<float>(currentWaveform.size()));
}

float WaveformViewer::getPeakLevel() const
{
    if (currentWaveform.empty())
        return 0.0f;

    return *std::max_element(currentWaveform.begin(), currentWaveform.end(),
                             [](float a, float b) { return std::abs(a) < std::abs(b); });
}

float WaveformViewer::getFrequencyForBin(int binIndex) const
{
    if (binIndex < 0 || binIndex >= fftConfig.numBins)
        return 0.0f;

    const float ratio = static_cast<float>(binIndex) / static_cast<float>(fftConfig.numBins);
    return fftConfig.minFrequency * std::pow(fftConfig.maxFrequency / fftConfig.minFrequency, ratio);
}

int WaveformViewer::getBinForFrequency(float frequency) const
{
    if (frequency < fftConfig.minFrequency || frequency > fftConfig.maxFrequency)
        return -1;

    const float ratio = std::log(frequency / fftConfig.minFrequency)
                       / std::log(fftConfig.maxFrequency / fftConfig.minFrequency);
    return juce::jlimit(0, fftConfig.numBins - 1, static_cast<int>(ratio * fftConfig.numBins));
}

juce::Image WaveformViewer::createSnapshot(int width, int height)
{
    juce::Image snapshot(juce::Image::RGB, width, height, true);
    juce::Graphics g(snapshot);

    const auto originalBounds = getBounds();
    setBounds(0, 0, width, height);
    paint(g);
    setBounds(originalBounds);

    return snapshot;
}

std::vector<float> WaveformViewer::getCurrentSpectrum() const
{
    juce::ScopedLock lock(dataLock);
    return currentSpectrum;
}

std::vector<float> WaveformViewer::getCurrentWaveform() const
{
    juce::ScopedLock lock(dataLock);
    return currentWaveform;
}

void WaveformViewer::renderWaveform(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    juce::ScopedLock lock(dataLock);
    if (currentWaveform.empty())
        return;

    const auto area = bounds.reduced(12);
    auto path = createWaveformPath(currentWaveform, area);
    g.setColour(waveformColor);
    g.strokePath(path, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void WaveformViewer::renderSpectrum(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    juce::ScopedLock lock(dataLock);
    if (currentSpectrum.empty())
        return;

    const auto area = bounds.reduced(12);
    auto path = createSpectrumPath(currentSpectrum, area);
    g.setColour(spectrumColor);
    g.strokePath(path, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void WaveformViewer::renderSpectrogram(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    juce::ScopedLock lock(dataLock);
    if (spectrumHistory.empty())
        return;

    const int historySize = static_cast<int>(spectrumHistory.size());
    const float binHeight = bounds.getHeight() / static_cast<float>(fftConfig.numBins);

    for (int historyIndex = 0; historyIndex < historySize; ++historyIndex)
    {
        const auto& spectrum = spectrumHistory[historyIndex];
        if (spectrum.empty())
            continue;

        const float x = bounds.getX() + (bounds.getWidth() * historyIndex) / static_cast<float>(historySize);
        const float columnWidth = bounds.getWidth() / static_cast<float>(historySize);

        for (int binIndex = 0; binIndex < fftConfig.numBins; ++binIndex)
        {
            const float magnitude = spectrum[binIndex];
            const float y = bounds.getY() + (fftConfig.numBins - binIndex - 1) * binHeight;
            juce::Colour binColour = spectrumColor.withAlpha(juce::jlimit(0.05f, 0.9f, magnitude * 0.8f));
            g.setColour(binColour);
            g.fillRect(x, y, columnWidth, binHeight);
        }
    }
}

void WaveformViewer::renderCombined(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    auto area = bounds;
    auto waveformBounds = area.removeFromTop(area.getHeight() / 2);
    renderWaveform(g, waveformBounds);
    renderSpectrum(g, area);
}

void WaveformViewer::renderGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.setColour(gridColor);
    for (int i = 1; i < 4; ++i)
    {
        const float y = bounds.getY() + (bounds.getHeight() * i) / 4.0f;
        g.drawLine((float)bounds.getX(), y, (float)bounds.getRight(), y, 1.0f);
    }
    for (int i = 1; i < 8; ++i)
    {
        const float x = bounds.getX() + (bounds.getWidth() * i) / 8.0f;
        g.drawLine(x, (float)bounds.getY(), x, (float)bounds.getBottom(), 1.0f);
    }
    g.setColour(gridColor.withAlpha(0.6f));
    g.drawLine((float)bounds.getX(), (float)bounds.getCentreY(), (float)bounds.getRight(), (float)bounds.getCentreY(), 1.2f);
}

void WaveformViewer::renderPeaks(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    juce::ScopedLock lock(dataLock);
    if (peakLevels.empty())
        return;

    g.setColour(tokens->colours.accentWarning);
    const size_t maxPeaks = std::min<size_t>(10, peakLevels.size());
    for (size_t i = 0; i < maxPeaks; ++i)
    {
        const float peakValue = peakLevels[i];
        if (peakValue < kMinPeakDisplay)
            continue;

        const float x = bounds.getX() + (bounds.getWidth() * static_cast<float>(i) / static_cast<float>(maxPeaks));
        const float y = bounds.getCentreY() - peakValue * bounds.getHeight() * 0.4f;
        g.drawLine(x - 2.0f, y, x + 2.0f, y, 2.0f);
    }
}

void WaveformViewer::performFFT()
{
    if (fftProcessor == nullptr)
        return;

    const int fftSize = fftProcessor->getSize();
    if (audioBuffer.getNumSamples() < fftSize)
        return;

    const float* audioData = audioBuffer.getReadPointer(0);
    for (int i = 0; i < fftSize; ++i)
        fftData[i] = audioData[i];

    if (fftConfig.useWindowing)
        applyWindowing();

    fftProcessor->performFrequencyOnlyForwardTransform(fftData.data());

    currentSpectrum.resize(static_cast<size_t>(fftConfig.numBins));
    for (int i = 0; i < fftConfig.numBins; ++i)
    {
        const int fftIndex = juce::jlimit(0, fftSize / 2 - 1, i * (fftSize / 2) / fftConfig.numBins);
        currentSpectrum[i] = std::abs(fftData[fftIndex]);
    }

    normalizeSpectrum(currentSpectrum);
    for (auto& value : currentSpectrum)
        value = linearToDecibels(value + 1e-12f);

    spectrumHistory.push_back(currentSpectrum);
    while (static_cast<int>(spectrumHistory.size()) > numHistoryBuffers)
        spectrumHistory.pop_front();
}

void WaveformViewer::updatePeakLevels()
{
    if (currentWaveform.empty())
        return;

    peakLevels.clear();
    constexpr int numPeaks = 10;
    const int samplesPerPeak = juce::jmax(1, static_cast<int>(currentWaveform.size()) / numPeaks);

    for (int i = 0; i < numPeaks; ++i)
    {
        const int startSample = i * samplesPerPeak;
        const int endSample = juce::jmin(startSample + samplesPerPeak, static_cast<int>(currentWaveform.size()));
        float peak = 0.0f;
        for (int sample = startSample; sample < endSample; ++sample)
            peak = juce::jmax(peak, std::abs(currentWaveform[static_cast<size_t>(sample)]));
        peakLevels.push_back(peak);
    }
}

void WaveformViewer::applyWindowing()
{
    const int fftSize = fftProcessor->getSize();
    for (int i = 0; i < fftSize; ++i)
        fftData[i] *= windowFunction[static_cast<size_t>(i)];
}

void WaveformViewer::normalizeSpectrum(std::vector<float>& spectrum) const
{
    if (spectrum.empty())
        return;

    const float maxValue = *std::max_element(spectrum.begin(), spectrum.end());
    if (maxValue <= 0.0f)
        return;

    for (auto& value : spectrum)
        value /= maxValue;
}

juce::Path WaveformViewer::createWaveformPath(const std::vector<float>& data, const juce::Rectangle<int>& bounds) const
{
    juce::Path path;
    if (data.empty())
        return path;

    const float centerY = bounds.getCentreY();
    const float scaleY = bounds.getHeight() * 0.45f;
    const float stepX = bounds.getWidth() / static_cast<float>(data.size());

    for (size_t i = 0; i < data.size(); ++i)
    {
        const float x = bounds.getX() + static_cast<float>(i) * stepX;
        const float y = centerY - data[i] * scaleY;
        if (i == 0)
            path.startNewSubPath(x, y);
        else
            path.lineTo(x, y);
    }
    return path;
}

juce::Path WaveformViewer::createSpectrumPath(const std::vector<float>& data, const juce::Rectangle<int>& bounds) const
{
    juce::Path path;
    if (data.empty())
        return path;

    const float stepX = bounds.getWidth() / static_cast<float>(data.size());
    const float scaleY = bounds.getHeight() / 60.0f; // assume -60..0 dB

    for (size_t i = 0; i < data.size(); ++i)
    {
        const float x = bounds.getX() + static_cast<float>(i) * stepX;
        const float y = bounds.getBottom() - juce::jmax(0.0f, (data[i] + 60.0f) * scaleY);
        if (i == 0)
            path.startNewSubPath(x, y);
        else
            path.lineTo(x, y);
    }
    return path;
}

float WaveformViewer::linearToDecibels(float linear) const
{
    return 20.0f * std::log10(std::max(linear, 1e-12f));
}

float WaveformViewer::decibelsToLinear(float dB) const
{
    return std::pow(10.0f, dB / 20.0f);
}

} // namespace daw::ui::components

