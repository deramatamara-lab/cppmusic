#include "Synthesizer.h"
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <cmath>
#include <random>

namespace daw::audio::synthesis
{

// Voice constructor
Synthesizer::Voice::Voice()
    : envelope(std::make_unique<dsp::Envelope>())
    , noteNumber(-1)
    , velocity(0.0f)
    , isActive(false)
{
    std::fill(std::begin(filterState), std::end(filterState), 0.0f);
}

Synthesizer::Synthesizer()
{
    // Voices are initialized with their constructors (envelopes already created)
    // Initialize parameters
    filterCutoff.store(1000.0f);
    filterResonance.store(0.5f);

    // Default oscillator waveforms and levels
    oscWaveforms[0].store(Waveform::Sawtooth, std::memory_order_relaxed);
    oscWaveforms[1].store(Waveform::Square,   std::memory_order_relaxed);
    oscWaveforms[2].store(Waveform::Sine,     std::memory_order_relaxed);

    oscLevels[0].store(1.0f, std::memory_order_relaxed);
    oscLevels[1].store(0.5f, std::memory_order_relaxed);
    oscLevels[2].store(0.0f, std::memory_order_relaxed);

    updateFilterCoefficients();
}

void Synthesizer::prepareToPlay(double sampleRate, int maximumBlockSize)
{
    AudioProcessorBase::prepareToPlay(sampleRate, maximumBlockSize);

    // Pre-allocate envelope buffer (real-time safety)
    envelopeBuffer.resize(static_cast<size_t>(maximumBlockSize), 0.0f);

    // Prepare all oscillators and envelopes
    for (auto& voice : voices)
    {
        for (auto& osc : voice.oscillators)
        {
            osc.setSampleRate(sampleRate);
        }

        if (voice.envelope != nullptr)
        {
            voice.envelope->prepareToPlay(sampleRate, maximumBlockSize);
        }
    }

    updateFilterCoefficients();
    reset();
}

void Synthesizer::processBlock(float* buffer, int numSamples) noexcept
{
    if (buffer == nullptr || numSamples <= 0)
        return;

    // Clear output buffer
    std::fill(buffer, buffer + numSamples, 0.0f);

    // Process each active voice
    for (auto& voice : voices)
    {
        if (!voice.isActive || voice.envelope == nullptr)
            continue;

        // Process envelope (using pre-allocated buffer - no allocation here)
        voice.envelope->processBlock(envelopeBuffer.data(), numSamples);

        // Process voice and mix into output
        for (int i = 0; i < numSamples; ++i)
        {
            float voiceOutput = 0.0f;

            // Sum all oscillators
            for (int oscIdx = 0; oscIdx < NUM_OSCILLATORS; ++oscIdx)
            {
                const auto waveform = oscWaveforms[oscIdx].load(std::memory_order_acquire);
                const auto level = oscLevels[oscIdx].load(std::memory_order_acquire);

                if (level > 0.0f)
                {
                    // Set waveform (thread-safe, only updates when changed)
                    voice.oscillators[oscIdx].setWaveform(waveform);
                    const float oscSample = voice.oscillators[oscIdx].getNextSample();
                    voiceOutput += oscSample * level;
                }
            }

            // Apply velocity
            voiceOutput *= voice.velocity;

            // Apply filter
            voiceOutput = processFilter(voiceOutput, voice);

            // Apply envelope
            voiceOutput *= envelopeBuffer[i];

            // Mix into output buffer
            buffer[i] += voiceOutput;

            // Prevent denormals
            buffer[i] += DENORMAL_PREVENTION;
        }

        // Check if voice should be deactivated
        if (voice.envelope->getCurrentStage() == dsp::Envelope::Stage::Idle)
        {
            voice.isActive = false;
            voice.noteNumber = -1;
        }
    }
}

void Synthesizer::reset()
{
    // Reset all voices
    for (auto& voice : voices)
    {
        for (auto& osc : voice.oscillators)
        {
            osc.setFrequency(0.0f);
        }

        if (voice.envelope != nullptr)
        {
            voice.envelope->reset();
        }

        std::fill(std::begin(voice.filterState), std::end(voice.filterState), 0.0f);
        voice.noteNumber = -1;
        voice.velocity = 0.0f;
        voice.isActive = false;
    }

    updateFilterCoefficients();
}

void Synthesizer::processMidi(const juce::MidiBuffer& midiBuffer) noexcept
{
    for (const auto metadata : midiBuffer)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            const int noteNumber = message.getNoteNumber();
            const float velocity = message.getVelocity() / 127.0f;

            // Find existing voice for this note (re-trigger) or find free voice
            auto* voice = findVoiceForNote(noteNumber);
            if (voice == nullptr)
                voice = findFreeVoice();

            if (voice != nullptr)
            {
                // Set up voice
                const float frequency = midiNoteToFrequency(static_cast<float>(noteNumber));
                for (auto& osc : voice->oscillators)
                {
                    osc.setFrequency(frequency);
                }

                voice->noteNumber = noteNumber;
                voice->velocity = velocity;
                voice->isActive = true;

                // Reset filter state
                std::fill(std::begin(voice->filterState), std::end(voice->filterState), 0.0f);

                // Trigger envelope
                if (voice->envelope != nullptr)
                {
                    voice->envelope->trigger();
                }
            }
        }
        else if (message.isNoteOff())
        {
            const int noteNumber = message.getNoteNumber();
            auto* voice = findVoiceForNote(noteNumber);

            if (voice != nullptr && voice->envelope != nullptr)
            {
                voice->envelope->release();
            }
        }
    }
}

void Synthesizer::setFilterCutoff(float cutoffHz) noexcept
{
    const float clamped = juce::jlimit(20.0f, 20000.0f, cutoffHz);
    filterCutoff.store(clamped, std::memory_order_release);
    updateFilterCoefficients();
}

float Synthesizer::getFilterCutoff() const noexcept
{
    return filterCutoff.load(std::memory_order_acquire);
}

void Synthesizer::setFilterResonance(float resonance) noexcept
{
    const float clamped = juce::jlimit(0.0f, 1.0f, resonance);
    filterResonance.store(clamped, std::memory_order_release);
    updateFilterCoefficients();
}

float Synthesizer::getFilterResonance() const noexcept
{
    return filterResonance.load(std::memory_order_acquire);
}

void Synthesizer::setOscillatorWaveform(int oscIndex, Waveform waveform) noexcept
{
    if (oscIndex >= 0 && oscIndex < NUM_OSCILLATORS)
    {
        oscWaveforms[oscIndex].store(waveform, std::memory_order_release);
    }
}

void Synthesizer::setOscillatorLevel(int oscIndex, float level) noexcept
{
    if (oscIndex >= 0 && oscIndex < NUM_OSCILLATORS)
    {
        const float clamped = juce::jlimit(0.0f, 1.0f, level);
        oscLevels[oscIndex].store(clamped, std::memory_order_release);
    }
}

void Synthesizer::setAttackTime(float attackSeconds) noexcept
{
    attackTime.store(juce::jlimit(0.0f, 10.0f, attackSeconds), std::memory_order_release);
    for (auto& voice : voices)
    {
        if (voice.envelope != nullptr)
        {
            voice.envelope->setAttackTime(attackSeconds);
        }
    }
}

void Synthesizer::setDecayTime(float decaySeconds) noexcept
{
    decayTime.store(juce::jlimit(0.0f, 10.0f, decaySeconds), std::memory_order_release);
    for (auto& voice : voices)
    {
        if (voice.envelope != nullptr)
        {
            voice.envelope->setDecayTime(decaySeconds);
        }
    }
}

void Synthesizer::setSustainLevel(float sustainLevel) noexcept
{
    this->sustainLevel.store(juce::jlimit(0.0f, 1.0f, sustainLevel), std::memory_order_release);
    for (auto& voice : voices)
    {
        if (voice.envelope != nullptr)
        {
            voice.envelope->setSustainLevel(sustainLevel);
        }
    }
}

void Synthesizer::setReleaseTime(float releaseSeconds) noexcept
{
    releaseTime.store(juce::jlimit(0.0f, 10.0f, releaseSeconds), std::memory_order_release);
    for (auto& voice : voices)
    {
        if (voice.envelope != nullptr)
        {
            voice.envelope->setReleaseTime(releaseSeconds);
        }
    }
}

float Synthesizer::midiNoteToFrequency(float noteNumber) noexcept
{
    return A4_FREQUENCY * std::pow(2.0f, (noteNumber - MIDI_NOTE_A4) / 12.0f);
}

Synthesizer::Voice* Synthesizer::findFreeVoice() noexcept
{
    for (auto& voice : voices)
    {
        if (!voice.isActive)
            return &voice;
    }

    // Voice stealing: find oldest voice in release
    Voice* oldestVoice = nullptr;
    for (auto& voice : voices)
    {
        if (voice.envelope != nullptr &&
            voice.envelope->getCurrentStage() == dsp::Envelope::Stage::Release)
        {
            if (oldestVoice == nullptr)
                oldestVoice = &voice;
        }
    }

    // If no voice in release, steal first active voice
    if (oldestVoice == nullptr && !voices.empty())
    {
        oldestVoice = &voices[0];
    }

    return oldestVoice;
}

Synthesizer::Voice* Synthesizer::findVoiceForNote(int noteNumber) noexcept
{
    for (auto& voice : voices)
    {
        if (voice.isActive && voice.noteNumber == noteNumber)
            return &voice;
    }
    return nullptr;
}

void Synthesizer::processVoice(Voice& voice, float* output, int numSamples) noexcept
{
    // This is handled in processBlock for efficiency
    juce::ignoreUnused(voice, output, numSamples);
}

float Synthesizer::processFilter(float input, Voice& voice) noexcept
{
    // Moog-style ladder filter (simplified 4-pole)
    // This is a simplified version for real-time safety

    const float cutoff = filterCutoff.load(std::memory_order_acquire);
    const float resonance = filterResonance.load(std::memory_order_acquire);

    // Calculate filter coefficient
    const float w = 2.0f * static_cast<float>(juce::MathConstants<float>::pi) * cutoff / static_cast<float>(currentSampleRate);
    const float g = 0.9892f * w - 0.4342f * w * w + 0.1381f * w * w * w - 0.0202f * w * w * w * w;
    const float resonanceGain = 1.0f + (resonance * 3.0f);

    // Process through 4 stages
    float stage0 = input - (voice.filterState[3] * resonanceGain);
    stage0 = juce::jlimit(-1.5f, 1.5f, stage0);

    voice.filterState[0] += g * (stage0 - voice.filterState[0]);
    voice.filterState[1] += g * (voice.filterState[0] - voice.filterState[1]);
    voice.filterState[2] += g * (voice.filterState[1] - voice.filterState[2]);
    voice.filterState[3] += g * (voice.filterState[2] - voice.filterState[3]);

    // Prevent denormals
    for (float& state : voice.filterState)
    {
        state += DENORMAL_PREVENTION;
    }

    return voice.filterState[3];
}


void Synthesizer::updateFilterCoefficients() noexcept
{
    // Filter coefficients are calculated per-sample in processFilter
    // This function is reserved for future optimizations
}

} // namespace daw::audio::synthesis
