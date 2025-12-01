/**
 * @file Transport.cpp
 * @brief Implementation of sample-accurate transport control.
 */

#include "Transport.hpp"
#include <algorithm>
#include <cmath>

namespace cppmusic::engine {

Transport::Transport() noexcept = default;

// =============================================================================
// Control Methods
// =============================================================================

void Transport::play() noexcept {
    state_.store(State::Playing, std::memory_order_release);
}

void Transport::stop() noexcept {
    state_.store(State::Stopped, std::memory_order_release);
    positionBeats_.store(0.0, std::memory_order_release);
    positionSamples_.store(0, std::memory_order_release);
}

void Transport::pause() noexcept {
    state_.store(State::Paused, std::memory_order_release);
}

void Transport::setPositionBeats(double beats) noexcept {
    beats = std::max(0.0, beats);
    positionBeats_.store(beats, std::memory_order_release);
    positionSamples_.store(beatsToSamples(beats), std::memory_order_release);
}

void Transport::setPositionSamples(std::int64_t samples) noexcept {
    samples = std::max(static_cast<std::int64_t>(0), samples);
    positionSamples_.store(samples, std::memory_order_release);
    positionBeats_.store(samplesToBeats(samples), std::memory_order_release);
}

void Transport::setTempo(double bpm) noexcept {
    bpm = std::clamp(bpm, 20.0, 999.0);
    tempoBpm_.store(bpm, std::memory_order_release);
}

void Transport::setTimeSignature(int numerator, int denominator) noexcept {
    numerator = std::clamp(numerator, 1, 32);
    denominator = std::clamp(denominator, 1, 32);
    timeSigNumerator_.store(numerator, std::memory_order_release);
    timeSigDenominator_.store(denominator, std::memory_order_release);
}

void Transport::setSampleRate(double rate) noexcept {
    rate = std::max(1.0, rate);
    sampleRate_.store(rate, std::memory_order_release);
}

// =============================================================================
// State Queries
// =============================================================================

Transport::State Transport::getState() const noexcept {
    return state_.load(std::memory_order_acquire);
}

bool Transport::isPlaying() const noexcept {
    return state_.load(std::memory_order_acquire) == State::Playing;
}

double Transport::getPositionBeats() const noexcept {
    return positionBeats_.load(std::memory_order_acquire);
}

std::int64_t Transport::getPositionSamples() const noexcept {
    return positionSamples_.load(std::memory_order_acquire);
}

double Transport::getTempo() const noexcept {
    return tempoBpm_.load(std::memory_order_acquire);
}

int Transport::getTimeSignatureNumerator() const noexcept {
    return timeSigNumerator_.load(std::memory_order_acquire);
}

int Transport::getTimeSignatureDenominator() const noexcept {
    return timeSigDenominator_.load(std::memory_order_acquire);
}

double Transport::getSampleRate() const noexcept {
    return sampleRate_.load(std::memory_order_acquire);
}

// =============================================================================
// Audio Thread Methods
// =============================================================================

void Transport::advancePosition(int numSamples) noexcept {
    if (state_.load(std::memory_order_acquire) != State::Playing) {
        return;
    }

    // Advance sample position
    const auto currentSamples = positionSamples_.load(std::memory_order_acquire);
    const auto newSamples = currentSamples + numSamples;
    positionSamples_.store(newSamples, std::memory_order_release);

    // Update beat position (sample-accurate)
    const double newBeats = samplesToBeats(newSamples);
    positionBeats_.store(newBeats, std::memory_order_release);
}

// =============================================================================
// Utility Methods
// =============================================================================

std::int64_t Transport::beatsToSamples(double beats) const noexcept {
    const double samplesPerBeat = getSamplesPerBeat();
    return static_cast<std::int64_t>(std::round(beats * samplesPerBeat));
}

double Transport::samplesToBeats(std::int64_t samples) const noexcept {
    const double samplesPerBeat = getSamplesPerBeat();
    if (samplesPerBeat <= 0.0) {
        return 0.0;
    }
    return static_cast<double>(samples) / samplesPerBeat;
}

double Transport::getSamplesPerBeat() const noexcept {
    const double tempo = tempoBpm_.load(std::memory_order_acquire);
    const double sr = sampleRate_.load(std::memory_order_acquire);
    
    if (tempo <= 0.0 || sr <= 0.0) {
        return 0.0;
    }
    
    // samples_per_beat = sample_rate * 60 / tempo
    return sr * 60.0 / tempo;
}

} // namespace cppmusic::engine
