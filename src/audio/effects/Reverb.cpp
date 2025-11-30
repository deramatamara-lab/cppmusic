#include "Reverb.h"
#include <algorithm>
#include <cmath>

namespace daw::audio::effects
{

Reverb::Reverb()
{
    // Initialize with default values
    roomSize.store(0.5f);
    damping.store(0.5f);
    mixAmount.store(0.3f);
}

void Reverb::prepareToPlay(double sampleRate, int maximumBlockSize)
{
    AudioProcessorBase::prepareToPlay(sampleRate, maximumBlockSize);

    // Calculate scaling for delay lines
    const double sampleRateScale = sampleRate / 44100.0;

    // Initialize comb filters
    for (size_t i = 0; i < combFilters.size(); ++i)
    {
        const int delaySamples = static_cast<int>(COMB_DELAYS[i] * sampleRateScale);
        combFilters[i].bufferSize = delaySamples;
        combFilters[i].buffer.resize(static_cast<size_t>(delaySamples), 0.0f);
        combFilters[i].bufferIndex = 0;
        combFilters[i].filterStore = 0.0f;
    }

    // Initialize allpass filters
    for (size_t i = 0; i < allpassFilters.size(); ++i)
    {
        const int delaySamples = static_cast<int>(ALLPASS_DELAYS[i] * sampleRateScale);
        allpassFilters[i].bufferSize = delaySamples;
        allpassFilters[i].buffer.resize(static_cast<size_t>(delaySamples), 0.0f);
        allpassFilters[i].bufferIndex = 0;
    }

    updateParameters();
    reset();
}

void Reverb::processBlock(float* buffer, int numSamples) noexcept
{
    if (buffer == nullptr || numSamples <= 0)
        return;

    // Load parameters atomically
    const float mix = mixAmount.load(std::memory_order_acquire);
    const float dry = 1.0f - mix;
    const float wet = mix * SCALE_WET;

    // Update filter parameters (smoothly interpolated)
    updateParameters();

    for (int i = 0; i < numSamples; ++i)
    {
        const float input = buffer[i];

        // Sum of all comb filter outputs
        float combSum = 0.0f;

        // Process all comb filters in parallel
        for (auto& comb : combFilters)
        {
            float combOutput = 0.0f;
            processCombFilter(comb, input, combOutput);
            combSum += combOutput;
        }

        // Process allpass filters in series
        float allpassOutput = combSum;
        for (auto& allpass : allpassFilters)
        {
            processAllpassFilter(allpass, allpassOutput, allpassOutput);
        }

        // Mix dry and wet signals
        buffer[i] = (input * dry * SCALE_DRY) + (allpassOutput * wet);

        // Prevent denormals
        buffer[i] += DENORMAL_PREVENTION;
    }
}

void Reverb::reset()
{
    // Clear all comb filter buffers
    for (auto& comb : combFilters)
    {
        std::fill(comb.buffer.begin(), comb.buffer.end(), 0.0f);
        comb.bufferIndex = 0;
        comb.filterStore = 0.0f;
    }

    // Clear all allpass filter buffers
    for (auto& allpass : allpassFilters)
    {
        std::fill(allpass.buffer.begin(), allpass.buffer.end(), 0.0f);
        allpass.bufferIndex = 0;
    }

    updateParameters();
}

void Reverb::setRoomSize(float roomSize) noexcept
{
    const float clamped = juce::jlimit(0.0f, 1.0f, roomSize);
    this->roomSize.store(clamped, std::memory_order_release);
    updateParameters();
}

float Reverb::getRoomSize() const noexcept
{
    return roomSize.load(std::memory_order_acquire);
}

void Reverb::setDamping(float damping) noexcept
{
    const float clamped = juce::jlimit(0.0f, 1.0f, damping);
    this->damping.store(clamped, std::memory_order_release);
    updateParameters();
}

float Reverb::getDamping() const noexcept
{
    return damping.load(std::memory_order_acquire);
}

void Reverb::setMix(float mix) noexcept
{
    const float clamped = juce::jlimit(0.0f, 1.0f, mix);
    mixAmount.store(clamped, std::memory_order_release);
}

float Reverb::getMix() const noexcept
{
    return mixAmount.load(std::memory_order_acquire);
}

void Reverb::updateParameters() noexcept
{
    const float room = roomSize.load(std::memory_order_acquire);
    const float damp = damping.load(std::memory_order_acquire);

    // Calculate feedback and damping for each comb filter
    for (auto& comb : combFilters)
    {
        comb.feedback = (room * SCALE_ROOM) + OFFSET_ROOM;
        comb.damp1 = damp * SCALE_DAMPING;
        comb.damp2 = 1.0f - comb.damp1;
    }
}

void Reverb::processCombFilter(CombFilter& filter, float input, float& output) noexcept
{
    if (filter.bufferSize == 0)
    {
        output = 0.0f;
        return;
    }

    // Read from delay buffer
    const float delayed = filter.buffer[static_cast<size_t>(filter.bufferIndex)];

    // Apply damping (low-pass filter)
    filter.filterStore = (delayed * filter.damp2) + (filter.filterStore * filter.damp1);

    // Calculate output with feedback
    output = delayed;

    // Write input + feedback to delay buffer
    filter.buffer[static_cast<size_t>(filter.bufferIndex)] = input + (filter.filterStore * filter.feedback);

    // Prevent denormals
    filter.buffer[static_cast<size_t>(filter.bufferIndex)] += DENORMAL_PREVENTION;

    // Advance buffer index (circular buffer)
    filter.bufferIndex = (filter.bufferIndex + 1) % filter.bufferSize;
}

void Reverb::processAllpassFilter(AllpassFilter& filter, float input, float& output) noexcept
{
    if (filter.bufferSize == 0)
    {
        output = input;
        return;
    }

    // Read from delay buffer
    const float delayed = filter.buffer[static_cast<size_t>(filter.bufferIndex)];

    // Allpass filter: output = input + delayed, buffer = delayed - input
    output = delayed + input;
    filter.buffer[static_cast<size_t>(filter.bufferIndex)] = delayed - input;

    // Prevent denormals
    filter.buffer[static_cast<size_t>(filter.bufferIndex)] += DENORMAL_PREVENTION;

    // Advance buffer index (circular buffer)
    filter.bufferIndex = (filter.bufferIndex + 1) % filter.bufferSize;
}

} // namespace daw::audio::effects

