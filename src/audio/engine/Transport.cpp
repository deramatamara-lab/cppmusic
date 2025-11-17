#include "Transport.h"
#include <algorithm>

namespace daw::audio::engine
{

Transport::Transport() noexcept
    : playing(false)
    , positionBeats(0.0)
    , positionSamples(0)
    , tempoBpm(120.0)
    , timeSigNumerator(4)
    , timeSigDenominator(4)
    , samplesPerBeat(0.0)
{
    updateSamplesPerBeat();
}

void Transport::play() noexcept
{
    playing.store(true, std::memory_order_release);
}

void Transport::stop() noexcept
{
    playing.store(false, std::memory_order_release);
}

void Transport::setPositionInBeats(double newPositionBeats) noexcept
{
    positionBeats.store(newPositionBeats, std::memory_order_release);
    
    // Update sample position based on current tempo
    const auto currentTempo = tempoBpm.load(std::memory_order_acquire);
    const auto currentSampleRate = 44100.0; // Will be updated when we have sample rate context
    const auto beatsPerSecond = currentTempo / 60.0;
    const auto samplesPerSecond = currentSampleRate;
    const auto newPositionSamples = static_cast<int64_t>((newPositionBeats / beatsPerSecond) * samplesPerSecond);
    positionSamples.store(newPositionSamples, std::memory_order_release);
}

void Transport::setTempo(double bpm) noexcept
{
    bpm = std::clamp(bpm, 20.0, 999.0);
    tempoBpm.store(bpm, std::memory_order_release);
    updateSamplesPerBeat();
}

void Transport::setTimeSignature(int numerator, int denominator) noexcept
{
    numerator = std::clamp(numerator, 1, 32);
    denominator = std::clamp(denominator, 1, 32);
    timeSigNumerator.store(numerator, std::memory_order_release);
    timeSigDenominator.store(denominator, std::memory_order_release);
}

bool Transport::isPlaying() const noexcept
{
    return playing.load(std::memory_order_acquire);
}

double Transport::getPositionInBeats() const noexcept
{
    return positionBeats.load(std::memory_order_acquire);
}

int64_t Transport::getPositionInSamples() const noexcept
{
    return positionSamples.load(std::memory_order_acquire);
}

double Transport::getTempo() const noexcept
{
    return tempoBpm.load(std::memory_order_acquire);
}

int Transport::getTimeSignatureNumerator() const noexcept
{
    return timeSigNumerator.load(std::memory_order_acquire);
}

int Transport::getTimeSignatureDenominator() const noexcept
{
    return timeSigDenominator.load(std::memory_order_acquire);
}

void Transport::updatePosition(int numSamplesProcessed, double sampleRate) noexcept
{
    if (!playing.load(std::memory_order_acquire))
        return;
    
    // Update sample position
    const auto currentSamples = positionSamples.load(std::memory_order_acquire);
    const auto newSamples = currentSamples + numSamplesProcessed;
    positionSamples.store(newSamples, std::memory_order_release);
    
    // Update beat position
    updateSamplesPerBeat();
    const auto beatsPerSample = 1.0 / (samplesPerBeat * sampleRate / 44100.0);
    const auto currentBeats = positionBeats.load(std::memory_order_acquire);
    const auto newBeats = currentBeats + (numSamplesProcessed * beatsPerSample);
    positionBeats.store(newBeats, std::memory_order_release);
}

void Transport::updateSamplesPerBeat() noexcept
{
    const auto currentTempo = tempoBpm.load(std::memory_order_acquire);
    const auto beatsPerSecond = currentTempo / 60.0;
    const auto sampleRate = 44100.0; // Default, will be updated when we have context
    samplesPerBeat = sampleRate / beatsPerSecond;
}

} // namespace daw::audio::engine

