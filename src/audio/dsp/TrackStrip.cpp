#include "TrackStrip.h"
#include <algorithm>
#include <cmath>

namespace daw::audio::dsp
{

TrackStrip::TrackStrip() noexcept
    : gainLinear(1.0f)
    , pan(0.0f)
    , muted(false)
    , soloed(false)
    , peakLevel(0.0f)
    , rmsLevel(0.0f)
    , currentSampleRate(44100.0)
    , currentBlockSize(512)
{
}

void TrackStrip::prepareToPlay(double sampleRate, int maximumBlockSize) noexcept
{
    currentSampleRate = sampleRate;
    currentBlockSize = maximumBlockSize;
    reset();
}

void TrackStrip::releaseResources() noexcept
{
    resetMeters();
}

void TrackStrip::reset() noexcept
{
    resetMeters();
}

void TrackStrip::setGain(float gainDb) noexcept
{
    gainLinear.store(dbToLinear(gainDb), std::memory_order_release);
}

void TrackStrip::setPan(float newPan) noexcept
{
    newPan = std::clamp(newPan, -1.0f, 1.0f);
    pan.store(newPan, std::memory_order_release);
}

void TrackStrip::setMute(bool isMuted) noexcept
{
    muted.store(isMuted, std::memory_order_release);
}

void TrackStrip::setSolo(bool isSoloed) noexcept
{
    soloed.store(isSoloed, std::memory_order_release);
}

float TrackStrip::getGain() const noexcept
{
    return gainLinear.load(std::memory_order_acquire);
}

float TrackStrip::getPan() const noexcept
{
    return pan.load(std::memory_order_acquire);
}

bool TrackStrip::isMuted() const noexcept
{
    return muted.load(std::memory_order_acquire);
}

bool TrackStrip::isSoloed() const noexcept
{
    return soloed.load(std::memory_order_acquire);
}

void TrackStrip::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/) noexcept
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (numChannels == 0 || numSamples == 0)
        return;

    // Load parameters atomically
    const auto currentGain = gainLinear.load(std::memory_order_acquire);
    const auto currentPan = pan.load(std::memory_order_acquire);
    const auto currentMuted = muted.load(std::memory_order_acquire);

    // Apply mute
    if (currentMuted)
    {
        buffer.clear();
        updateMeters(buffer);
        return;
    }

    // Apply gain
    if (currentGain != 1.0f)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                data[i] *= currentGain;
            }
        }
    }

    // Apply pan using constant power panning (professional standard)
    // Constant power maintains perceived loudness across pan positions
    if (numChannels == 2 && std::abs(currentPan) > 0.001f)
    {
        // Convert linear pan (-1.0 = left, 0.0 = center, 1.0 = right) to constant power
        // Using sin/cos law for constant power panning
        const float panPosition = juce::jlimit(-1.0f, 1.0f, currentPan);
        const float panAngle = (panPosition + 1.0f) * juce::MathConstants<float>::pi / 4.0f; // 0 to pi/2

        const float leftGain = std::cos(panAngle);
        const float rightGain = std::sin(panAngle);

        auto* leftData = buffer.getWritePointer(0);
        auto* rightData = buffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i)
        {
            leftData[i] *= leftGain;
            rightData[i] *= rightGain;
        }
    }

    // Update meters
    updateMeters(buffer);
}

float TrackStrip::getPeakLevel() const noexcept
{
    return peakLevel.load(std::memory_order_acquire);
}

float TrackStrip::getRmsLevel() const noexcept
{
    return rmsLevel.load(std::memory_order_acquire);
}

void TrackStrip::resetMeters() noexcept
{
    peakLevel.store(0.0f, std::memory_order_release);
    rmsLevel.store(0.0f, std::memory_order_release);
}

float TrackStrip::dbToLinear(float db) noexcept
{
    return std::pow(10.0f, db / 20.0f);
}

void TrackStrip::updateMeters(const juce::AudioBuffer<float>& buffer) noexcept
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (numChannels == 0 || numSamples == 0)
        return;

    // Calculate peak
    float peak = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const auto absValue = std::abs(data[i]);
            if (absValue > peak)
                peak = absValue;
        }
    }

    // Calculate RMS
    float sumSquares = 0.0f;
    const auto totalSamples = numChannels * numSamples;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            sumSquares += data[i] * data[i];
        }
    }
    const auto rms = std::sqrt(sumSquares / static_cast<float>(totalSamples));

    // Store atomically
    peakLevel.store(peak, std::memory_order_release);
    rmsLevel.store(rms, std::memory_order_release);
}

} // namespace daw::audio::dsp

