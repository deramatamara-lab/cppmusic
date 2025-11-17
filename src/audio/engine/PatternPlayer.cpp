#include "PatternPlayer.h"
#include <algorithm>
#include <cmath>

namespace daw::audio::engine
{

PatternPlayer::PatternPlayer() noexcept
    : pattern(nullptr)
    , quantization(1.0 / 16.0)
    , currentSampleRate(44100.0)
    , currentBlockSize(128)
    , lastBeatPosition(0.0)
{
}

void PatternPlayer::prepareToPlay(double sampleRate, int maxBlockSize) noexcept
{
    currentSampleRate = sampleRate;
    currentBlockSize = maxBlockSize;
    reset();
}

void PatternPlayer::releaseResources() noexcept
{
}

void PatternPlayer::reset() noexcept
{
    lastBeatPosition = 0.0;
    pendingNotes.clear();
}

void PatternPlayer::setPattern(const daw::project::Pattern* pattern) noexcept
{
    this->pattern = pattern;
    reset();
}

void PatternPlayer::setQuantization(double gridDivision) noexcept
{
    quantization = gridDivision;
}

void PatternPlayer::processBlock(juce::MidiBuffer& buffer, int numSamples, double currentBeat, double tempoBpm) noexcept
{
    if (pattern == nullptr)
        return;
    
    const auto patternLength = pattern->getLengthBeats();
    if (patternLength <= 0.0)
        return;
    
    // Calculate beat range for this block
    const auto samplesPerBeat = beatsToSamples(1.0, tempoBpm);
    const auto beatIncrement = numSamples / samplesPerBeat;
    const auto startBeat = currentBeat;
    const auto endBeat = currentBeat + beatIncrement;
    
    // Handle pattern looping
    const auto loopedStartBeat = std::fmod(startBeat, patternLength);
    const auto loopedEndBeat = std::fmod(endBeat, patternLength);
    
    // Schedule notes in this beat range
    scheduleNotes(loopedStartBeat, loopedEndBeat, tempoBpm, numSamples, buffer);
    
    lastBeatPosition = currentBeat;
}

void PatternPlayer::scheduleNotes(double startBeat, double endBeat, double tempoBpm, int blockSamples, juce::MidiBuffer& buffer) noexcept
{
    if (pattern == nullptr)
        return;
    
    const auto& notes = pattern->getNotes();
    const auto samplesPerBeat = beatsToSamples(1.0, tempoBpm);
    
    for (const auto& note : notes)
    {
        // Check if note is in the beat range (handle wrapping)
        bool inRange = false;
        if (startBeat <= endBeat)
        {
            inRange = (note.startBeat >= startBeat && note.startBeat < endBeat);
        }
        else
        {
            // Wrapped range
            inRange = (note.startBeat >= startBeat || note.startBeat < endBeat);
        }
        
        if (inRange)
        {
            // Quantize note start time
            const auto quantizedBeat = std::round(note.startBeat / quantization) * quantization;
            const auto sampleOffset = static_cast<int>((quantizedBeat - startBeat) * samplesPerBeat);
            
            if (sampleOffset >= 0 && sampleOffset < blockSamples)
            {
                juce::MidiMessage midiNoteOn = juce::MidiMessage::noteOn(note.channel + 1, note.note, juce::uint8(note.velocity));
                buffer.addEvent(midiNoteOn, sampleOffset);
            }
        }
    }
}

double PatternPlayer::beatsToSamples(double beats, double tempoBpm) const noexcept
{
    const auto beatsPerSecond = tempoBpm / 60.0;
    const auto samplesPerBeat = currentSampleRate / beatsPerSecond;
    return beats * samplesPerBeat;
}

} // namespace daw::audio::engine

