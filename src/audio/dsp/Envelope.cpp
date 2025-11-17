#include "Envelope.h"
#include <juce_core/juce_core.h>
#include <algorithm>

namespace daw::audio::dsp
{

Envelope::Envelope() noexcept
    : currentStage(Stage::Idle)
    , currentValue(0.0f)
    , stageProgress(0.0f)
    , triggered(false)
    , released(false)
    , attackIncrement(0.0f)
    , decayIncrement(0.0f)
    , releaseIncrement(0.0f)
{
}

void Envelope::prepareToPlay(double sampleRate, int maxBlockSize) noexcept
{
    currentSampleRate = sampleRate;
    currentBlockSize = maxBlockSize;
    updateIncrements();
    reset();
}

void Envelope::releaseResources() noexcept
{
}

void Envelope::reset() noexcept
{
    currentStage = Stage::Idle;
    currentValue = 0.0f;
    stageProgress = 0.0f;
    triggered = false;
    released = false;
}

bool Envelope::processBlock(float* output, int numSamples) noexcept
{
    if (!isEnabled())
    {
        std::fill(output, output + numSamples, 0.0f);
        return false;
    }
    
    const auto depthValue = depth.load(std::memory_order_acquire);
    
    for (int i = 0; i < numSamples; ++i)
    {
        switch (currentStage)
        {
            case Stage::Idle:
                currentValue = 0.0f;
                break;
                
            case Stage::Attack:
            {
                const auto attack = attackTime.load(std::memory_order_acquire);
                const auto attackSamples = static_cast<float>(attack * currentSampleRate);
                
                if (attackSamples > 0.0f)
                {
                    stageProgress += 1.0f / attackSamples;
                    if (stageProgress >= 1.0f)
                    {
                        currentValue = 1.0f;
                        currentStage = Stage::Decay;
                        stageProgress = 0.0f;
                    }
                    else
                    {
                        const auto curve = attackCurve.load(std::memory_order_acquire);
                        currentValue = applyCurve(stageProgress, curve);
                    }
                }
                else
                {
                    currentValue = 1.0f;
                    currentStage = Stage::Decay;
                    stageProgress = 0.0f;
                }
                break;
            }
            
            case Stage::Decay:
            {
                const auto decay = decayTime.load(std::memory_order_acquire);
                const auto decaySamples = static_cast<float>(decay * currentSampleRate);
                const auto sustain = sustainLevel.load(std::memory_order_acquire);
                
                if (decaySamples > 0.0f)
                {
                    stageProgress += 1.0f / decaySamples;
                    if (stageProgress >= 1.0f)
                    {
                        currentValue = sustain;
                        currentStage = Stage::Sustain;
                        stageProgress = 0.0f;
                    }
                    else
                    {
                        const auto curve = decayCurve.load(std::memory_order_acquire);
                        const auto progress = applyCurve(stageProgress, curve);
                        currentValue = 1.0f - progress * (1.0f - sustain);
                    }
                }
                else
                {
                    currentValue = sustain;
                    currentStage = Stage::Sustain;
                    stageProgress = 0.0f;
                }
                break;
            }
            
            case Stage::Sustain:
            {
                const auto sustain = sustainLevel.load(std::memory_order_acquire);
                currentValue = sustain;
                
                if (released)
                {
                    currentStage = Stage::Release;
                    stageProgress = 0.0f;
                }
                break;
            }
            
            case Stage::Release:
            {
                const auto release = releaseTime.load(std::memory_order_acquire);
                const auto releaseSamples = static_cast<float>(release * currentSampleRate);
                
                if (releaseSamples > 0.0f)
                {
                    stageProgress += 1.0f / releaseSamples;
                    if (stageProgress >= 1.0f)
                    {
                        currentValue = 0.0f;
                        currentStage = Stage::Idle;
                        stageProgress = 0.0f;
                    }
                    else
                    {
                        const auto curve = decayCurve.load(std::memory_order_acquire);
                        const auto progress = applyCurve(stageProgress, curve);
                        const auto sustain = sustainLevel.load(std::memory_order_acquire);
                        currentValue = sustain * (1.0f - progress);
                    }
                }
                else
                {
                    currentValue = 0.0f;
                    currentStage = Stage::Idle;
                    stageProgress = 0.0f;
                }
                break;
            }
        }
        
        output[i] = currentValue * depthValue;
    }
    
    return currentStage != Stage::Idle;
}

float Envelope::getCurrentValue() const noexcept
{
    if (!isEnabled())
        return 0.0f;
    
    return currentValue * depth.load(std::memory_order_acquire);
}

void Envelope::trigger() noexcept
{
    currentStage = Stage::Attack;
    currentValue = 0.0f;
    stageProgress = 0.0f;
    triggered = true;
    released = false;
}

void Envelope::release() noexcept
{
    if (currentStage == Stage::Sustain || currentStage == Stage::Decay)
    {
        currentStage = Stage::Release;
        stageProgress = 0.0f;
    }
    released = true;
}

bool Envelope::isActive() const noexcept
{
    return currentStage != Stage::Idle;
}

void Envelope::setAttackTime(float attackSeconds) noexcept
{
    attackTime.store(juce::jlimit(0.0f, 10.0f, attackSeconds), std::memory_order_release);
    updateIncrements();
}

void Envelope::setDecayTime(float decaySeconds) noexcept
{
    decayTime.store(juce::jlimit(0.0f, 10.0f, decaySeconds), std::memory_order_release);
    updateIncrements();
}

void Envelope::setSustainLevel(float sustainLevel) noexcept
{
    this->sustainLevel.store(juce::jlimit(0.0f, 1.0f, sustainLevel), std::memory_order_release);
}

void Envelope::setReleaseTime(float releaseSeconds) noexcept
{
    releaseTime.store(juce::jlimit(0.0f, 10.0f, releaseSeconds), std::memory_order_release);
    updateIncrements();
}

void Envelope::setAttackCurve(float curve) noexcept
{
    attackCurve.store(juce::jlimit(0.1f, 10.0f, curve), std::memory_order_release);
}

void Envelope::setDecayCurve(float curve) noexcept
{
    decayCurve.store(juce::jlimit(0.1f, 10.0f, curve), std::memory_order_release);
}

void Envelope::updateIncrements() noexcept
{
    // Pre-calculate increments for efficiency
    const auto attack = attackTime.load(std::memory_order_acquire);
    const auto decay = decayTime.load(std::memory_order_acquire);
    const auto release = releaseTime.load(std::memory_order_acquire);
    
    if (currentSampleRate > 0.0)
    {
        attackIncrement = attack > 0.0f ? static_cast<float>(1.0 / (attack * currentSampleRate)) : 1.0f;
        decayIncrement = decay > 0.0f ? static_cast<float>(1.0 / (decay * currentSampleRate)) : 1.0f;
        releaseIncrement = release > 0.0f ? static_cast<float>(1.0 / (release * currentSampleRate)) : 1.0f;
    }
}

float Envelope::applyCurve(float value, float curve) const noexcept
{
    if (curve == 1.0f)
        return value;
    
    if (curve < 1.0f)
    {
        // Exponential curve (faster at start)
        return std::pow(value, 1.0f / curve);
    }
    else
    {
        // Logarithmic curve (faster at end)
        return 1.0f - std::pow(1.0f - value, curve);
    }
}

} // namespace daw::audio::dsp

