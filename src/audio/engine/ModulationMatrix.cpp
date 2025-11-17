#include "ModulationMatrix.h"
#include "../dsp/Modulator.h"
#include <algorithm>
#include <cmath>

namespace daw::audio::engine
{

ModulationMatrix::ModulationMatrix() noexcept
    : currentSampleRate(44100.0)
    , currentBlockSize(128)
{
    slots.reserve(maxSlots);
    modulationBuffer.reserve(2048);
}

void ModulationMatrix::prepareToPlay(double sampleRate, int maxBlockSize) noexcept
{
    currentSampleRate = sampleRate;
    currentBlockSize = maxBlockSize;
    
    modulationBuffer.resize(maxBlockSize);
    
    // Prepare all modulators
    for (auto& slot : slots)
    {
        if (slot.modulator != nullptr)
        {
            slot.modulator->prepareToPlay(sampleRate, maxBlockSize);
        }
    }
}

void ModulationMatrix::releaseResources() noexcept
{
    for (auto& slot : slots)
    {
        if (slot.modulator != nullptr)
        {
            slot.modulator->releaseResources();
        }
    }
    
    modulationBuffer.clear();
}

int ModulationMatrix::addSlot(daw::audio::dsp::Modulator* modulator, float* targetParameter, float depth) noexcept
{
    if (modulator == nullptr || targetParameter == nullptr)
        return -1;
    
    if (static_cast<int>(slots.size()) >= maxSlots)
        return -1;
    
    ModulationSlot slot;
    slot.modulator = modulator;
    slot.targetParameter = targetParameter;
    slot.depth = std::clamp(depth, 0.0f, 1.0f);
    slot.enabled = true;
    
    slots.push_back(slot);
    return static_cast<int>(slots.size() - 1);
}

void ModulationMatrix::removeSlot(int slotIndex) noexcept
{
    if (slotIndex >= 0 && slotIndex < static_cast<int>(slots.size()))
    {
        slots.erase(slots.begin() + slotIndex);
    }
}

void ModulationMatrix::setSlotEnabled(int slotIndex, bool enabled) noexcept
{
    if (slotIndex >= 0 && slotIndex < static_cast<int>(slots.size()))
    {
        slots[slotIndex].enabled = enabled;
    }
}

void ModulationMatrix::setSlotDepth(int slotIndex, float depth) noexcept
{
    if (slotIndex >= 0 && slotIndex < static_cast<int>(slots.size()))
    {
        slots[slotIndex].depth = std::clamp(depth, 0.0f, 1.0f);
    }
}

void ModulationMatrix::processBlock(int numSamples, float* baseValues) noexcept
{
    if (numSamples == 0 || baseValues == nullptr)
        return;
    
    // Ensure modulation buffer is large enough
    if (static_cast<int>(modulationBuffer.size()) < numSamples)
    {
        modulationBuffer.resize(numSamples);
    }
    
    // Process each active slot
    for (const auto& slot : slots)
    {
        if (!slot.enabled || slot.modulator == nullptr || slot.targetParameter == nullptr)
            continue;
        
        processSlot(slot, numSamples, baseValues);
    }
}

void ModulationMatrix::processSlot(const ModulationSlot& slot, int numSamples, float* baseValues) noexcept
{
    // Generate modulation signal
    if (!slot.modulator->processBlock(modulationBuffer.data(), numSamples))
        return;
    
    // Apply modulation to target parameter
    const auto depth = slot.depth;
    const auto baseValue = *slot.targetParameter;
    
    for (int i = 0; i < numSamples; ++i)
    {
        const auto modValue = modulationBuffer[i] * depth;
        baseValues[i] += modValue * baseValue; // Scale by base value for relative modulation
    }
}

void ModulationMatrix::clear() noexcept
{
    slots.clear();
    modulationBuffer.clear();
}

} // namespace daw::audio::engine

