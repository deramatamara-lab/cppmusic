#include "LFO.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>

namespace daw::audio::dsp
{

LFO::LFO() noexcept
    : phase(0.0f)
    , lastNoiseValue(0.0f)
    , lastSampleHoldValue(0.0f)
    , sampleHoldCounter(0)
{
}

void LFO::prepareToPlay(double sampleRate, int maxBlockSize) noexcept
{
    currentSampleRate = sampleRate;
    currentBlockSize = maxBlockSize;
    reset();
}

void LFO::releaseResources() noexcept
{
}

void LFO::reset() noexcept
{
    phase = phaseOffset.load(std::memory_order_acquire);
    lastNoiseValue = 0.0f;
    lastSampleHoldValue = 0.0f;
    sampleHoldCounter = 0;
}

bool LFO::processBlock(float* output, int numSamples) noexcept
{
    if (!isEnabled())
    {
        std::fill(output, output + numSamples, 0.0f);
        return false;
    }

    const auto wf = waveform.load(std::memory_order_acquire);
    const auto phaseInc = getPhaseIncrement();
    const auto depthValue = depth.load(std::memory_order_acquire);

    for (int i = 0; i < numSamples; ++i)
    {
        output[i] = generateSample(phase, wf) * depthValue;
        phase += phaseInc;

        // Wrap phase to [0, 1)
        if (phase >= 1.0f)
            phase -= 1.0f;
    }

    return true;
}

float LFO::getCurrentValue() const noexcept
{
    if (!isEnabled())
        return 0.0f;

    const auto wf = waveform.load(std::memory_order_acquire);
    const auto depthValue = depth.load(std::memory_order_acquire);
    return generateSample(phase, wf) * depthValue;
}

void LFO::setFrequency(float frequencyHz) noexcept
{
    this->frequencyHz.store(std::clamp(frequencyHz, 0.001f, 20000.0f), std::memory_order_release);
}

float LFO::getFrequency() const noexcept
{
    return frequencyHz.load(std::memory_order_acquire);
}

void LFO::setWaveform(Waveform waveform) noexcept
{
    this->waveform.store(waveform, std::memory_order_release);
}

LFO::Waveform LFO::getWaveform() const noexcept
{
    return waveform.load(std::memory_order_acquire);
}

void LFO::setPhaseOffset(float phaseOffset) noexcept
{
    const auto clamped = std::clamp(phaseOffset, 0.0f, 1.0f);
    this->phaseOffset.store(clamped, std::memory_order_release);
    phase = clamped;
}

float LFO::getPhaseOffset() const noexcept
{
    return phaseOffset.load(std::memory_order_acquire);
}

void LFO::setPulseWidth(float pulseWidth) noexcept
{
    this->pulseWidth.store(std::clamp(pulseWidth, 0.0f, 1.0f), std::memory_order_release);
}

float LFO::getPulseWidth() const noexcept
{
    return pulseWidth.load(std::memory_order_acquire);
}

void LFO::setSyncToTempo(bool sync, float tempoBpm, float beatDivision) noexcept
{
    syncToTempo.store(sync, std::memory_order_release);
    syncTempoBpm.store(tempoBpm, std::memory_order_release);
    syncBeatDivision.store(beatDivision, std::memory_order_release);
}

float LFO::generateSample(float phaseValue, Waveform wf) const noexcept
{
    switch (wf)
    {
        case Waveform::Sine:
            return std::sin(phaseValue * 2.0f * std::numbers::pi_v<float>);

        case Waveform::Triangle:
            if (phaseValue < 0.5f)
                return 4.0f * phaseValue - 1.0f;
            else
                return 3.0f - 4.0f * phaseValue;

        case Waveform::Sawtooth:
            return 2.0f * phaseValue - 1.0f;

        case Waveform::SawtoothInverse:
            return 1.0f - 2.0f * phaseValue;

        case Waveform::Square:
            return phaseValue < 0.5f ? -1.0f : 1.0f;

        case Waveform::Pulse:
        {
            const auto pw = pulseWidth.load(std::memory_order_acquire);
            return phaseValue < pw ? -1.0f : 1.0f;
        }

        case Waveform::Noise:
        {
            // High-quality pseudo-random using improved LCG (real-time safe, deterministic)
            // Uses better constants for improved statistical properties
            static thread_local uint32_t noiseSeed = 12345;
            noiseSeed = noiseSeed * 1664525u + 1013904223u; // Better LCG constants
            const auto normalized = static_cast<float>(noiseSeed & 0x7FFFFFFFu) / 2147483647.0f;

            // Apply slight filtering to reduce aliasing in noise
            lastNoiseValue = lastNoiseValue * 0.1f + normalized * 0.9f;
            return (lastNoiseValue * 2.0f - 1.0f);
        }

        case Waveform::SampleAndHold:
        {
            // Update every N samples (roughly at frequency rate)
            const auto freq = frequencyHz.load(std::memory_order_acquire);
            const auto updateRate = static_cast<int>(currentSampleRate / freq);

            if (sampleHoldCounter >= updateRate)
            {
                sampleHoldCounter = 0;
                // High-quality LCG for S&H (real-time safe, deterministic)
                static thread_local uint32_t shSeed = 67890;
                shSeed = shSeed * 1664525u + 1013904223u; // Better LCG constants
                const auto normalized = static_cast<float>(shSeed & 0x7FFFFFFFu) / 2147483647.0f;
                lastSampleHoldValue = normalized * 2.0f - 1.0f;
            }
            else
            {
                ++sampleHoldCounter;
            }

            return lastSampleHoldValue;
        }

        default:
            return 0.0f;
    }
}

float LFO::getPhaseIncrement() const noexcept
{
    if (syncToTempo.load(std::memory_order_acquire))
    {
        const auto tempo = syncTempoBpm.load(std::memory_order_acquire);
        const auto division = syncBeatDivision.load(std::memory_order_acquire);
        const auto beatsPerSecond = tempo / 60.0f;
        const auto frequency = beatsPerSecond * division;
        return static_cast<float>(frequency / currentSampleRate);
    }
    else
    {
        const auto freq = frequencyHz.load(std::memory_order_acquire);
        return static_cast<float>(freq / currentSampleRate);
    }
}

} // namespace daw::audio::dsp

