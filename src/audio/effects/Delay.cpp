/**
 * @file Delay.cpp
 * @brief Real-time safe delay effect implementation.
 */

#include "Delay.h"

#include <algorithm>
#include <juce_core/juce_core.h>

namespace daw::audio::effects
{

namespace
{
constexpr float kFeedbackSafety = 0.995f;
constexpr float kMixMin = 0.0f;
constexpr float kMixMax = 1.0f;
} // namespace

Delay::Delay()
{
    updateDelaySamples();
}

void Delay::prepareToPlay(double sampleRate, int maximumBlockSize)
{
    processors::AudioProcessorBase::prepareToPlay(sampleRate, maximumBlockSize);

    const int maxDelaySamples = static_cast<int>((MAX_DELAY_MS * 0.001f * sampleRate) + maximumBlockSize + 1);
    bufferSize = std::max(1, maxDelaySamples);
    delayBuffer.assign(static_cast<size_t>(bufferSize), 0.0f);
    writePosition = 0;

    updateDelaySamples();
    currentDelaySamples = targetDelaySamples;
}

void Delay::reset()
{
    std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0f);
    writePosition = 0;
    updateDelaySamples();
    currentDelaySamples = targetDelaySamples;
}

void Delay::processBlock(float* buffer, int numSamples) noexcept
{
    if (buffer == nullptr || numSamples <= 0 || delayBuffer.empty())
        return;

    const float feedback = juce::jlimit(0.0f, kFeedbackSafety, feedbackAmount.load(std::memory_order_acquire));
    const float mix = juce::jlimit(kMixMin, kMixMax, mixAmount.load(std::memory_order_acquire));
    const float dry = 1.0f - mix;

    for (int i = 0; i < numSamples; ++i)
    {
        const float input = buffer[i];

        updateDelaySamples();
        currentDelaySamples += (targetDelaySamples - currentDelaySamples) * INTERPOLATION_RATE;

        const float delayed = readDelay(std::max(currentDelaySamples, 0.0f));

        buffer[i] = (input * dry) + (delayed * mix);

        const float writeSample = input + (delayed * feedback);
        delayBuffer[static_cast<size_t>(writePosition)] = writeSample + DENORMAL_PREVENTION;

        writePosition = (writePosition + 1) % bufferSize;
    }
}

void Delay::setDelayTime(float delayMs) noexcept
{
    const float clamped = juce::jlimit(MIN_DELAY_MS, MAX_DELAY_MS, delayMs);
    delayTimeMs.store(clamped, std::memory_order_release);
    updateDelaySamples();
}

float Delay::getDelayTime() const noexcept
{
    return delayTimeMs.load(std::memory_order_acquire);
}

void Delay::setFeedback(float feedback) noexcept
{
    const float clamped = juce::jlimit(0.0f, kFeedbackSafety, feedback);
    feedbackAmount.store(clamped, std::memory_order_release);
}

float Delay::getFeedback() const noexcept
{
    return feedbackAmount.load(std::memory_order_acquire);
}

void Delay::setMix(float mix) noexcept
{
    const float clamped = juce::jlimit(kMixMin, kMixMax, mix);
    mixAmount.store(clamped, std::memory_order_release);
}

float Delay::getMix() const noexcept
{
    return mixAmount.load(std::memory_order_acquire);
}

void Delay::updateDelaySamples() noexcept
{
    if (currentSampleRate <= 0.0)
        return;

    const float delayMs = juce::jlimit(MIN_DELAY_MS, MAX_DELAY_MS, delayTimeMs.load(std::memory_order_acquire));
    targetDelaySamples = delayMs * 0.001f * static_cast<float>(currentSampleRate);
    targetDelaySamples = juce::jlimit(0.0f, static_cast<float>(std::max(1, bufferSize - 1)), targetDelaySamples);
}

float Delay::readDelay(float delaySamples) const noexcept
{
    if (delayBuffer.empty())
        return 0.0f;

    float readPos = static_cast<float>(writePosition) - delaySamples;
    while (readPos < 0.0f)
        readPos += static_cast<float>(bufferSize);

    const int index0 = static_cast<int>(readPos) % bufferSize;
    const int index1 = (index0 + 1) % bufferSize;
    const float frac = readPos - static_cast<float>(index0);

    const float sample0 = delayBuffer[static_cast<size_t>(index0)];
    const float sample1 = delayBuffer[static_cast<size_t>(index1)];
    return sample0 + frac * (sample1 - sample0);
}

} // namespace daw::audio::effects
