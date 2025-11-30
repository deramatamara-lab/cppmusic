#include "PatternPlaybackEngine.h"

#include <juce_core/juce_core.h>

namespace daw::audio::engine
{

PatternPlaybackEngine::PatternPlaybackEngine() noexcept = default;

void PatternPlaybackEngine::prepare(double sampleRate, int maxBlockSize) noexcept
{
    patternPlayer.prepareToPlay(sampleRate, maxBlockSize);
    patternPlayer.setQuantization(quantizationDivision.load(std::memory_order_acquire));
    patternDirty.store(true, std::memory_order_release);
}

void PatternPlaybackEngine::reset() noexcept
{
    patternPlayer.reset();
    patternDirty.store(true, std::memory_order_release);
}

void PatternPlaybackEngine::setPattern(const daw::project::Pattern* pattern) noexcept
{
    pendingPattern.store(pattern, std::memory_order_release);
    patternDirty.store(true, std::memory_order_release);
}

void PatternPlaybackEngine::clearPattern() noexcept
{
    pendingPattern.store(nullptr, std::memory_order_release);
    patternDirty.store(true, std::memory_order_release);
}

void PatternPlaybackEngine::setQuantization(double gridDivision) noexcept
{
    const double safeDivision = (gridDivision <= 0.0) ? (1.0 / 16.0) : gridDivision;
    quantizationDivision.store(safeDivision, std::memory_order_release);
    patternDirty.store(true, std::memory_order_release);
}

void PatternPlaybackEngine::processBlock(juce::MidiBuffer& buffer,
                                         int numSamples,
                                         double startBeat,
                                         double tempoBpm) noexcept
{
    syncPatternIfNeeded();

    const auto* pattern = activePattern.load(std::memory_order_acquire);
    if (pattern == nullptr)
        return;

    if (tempoBpm <= 0.0)
        return;

    patternPlayer.processBlock(buffer, numSamples, startBeat, tempoBpm);
}

bool PatternPlaybackEngine::hasActivePattern() const noexcept
{
    return activePattern.load(std::memory_order_acquire) != nullptr;
}

void PatternPlaybackEngine::syncPatternIfNeeded() noexcept
{
    const bool dirty = patternDirty.load(std::memory_order_acquire);
    const auto* desiredPattern = pendingPattern.load(std::memory_order_acquire);
    const auto* currentPattern = activePattern.load(std::memory_order_acquire);

    if (!dirty && desiredPattern == currentPattern)
        return;

    activePattern.store(desiredPattern, std::memory_order_release);
    patternDirty.store(false, std::memory_order_release);

    if (desiredPattern != nullptr)
    {
        patternPlayer.setPattern(desiredPattern);
        patternPlayer.setQuantization(quantizationDivision.load(std::memory_order_acquire));
        daw::core::utilities::Logger::info("PatternPlaybackEngine: pattern updated (id=" +
                                           std::to_string(desiredPattern->getId()) + ")");
    }
    else
    {
        patternPlayer.setPattern(nullptr);
        daw::core::utilities::Logger::info("PatternPlaybackEngine: pattern cleared");
    }
}

} // namespace daw::audio::engine
