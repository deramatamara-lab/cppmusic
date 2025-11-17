#include "AudioGraph.h"
#include "../dsp/TrackStrip.h"
#include <cmath>
#include <algorithm>

namespace daw::audio::engine
{

AudioGraph::AudioGraph()
    : masterGainLinear(1.0f)
{
}

void AudioGraph::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    for (auto& trackStrip : trackStrips)
    {
        trackStrip->prepareToPlay(sampleRate, samplesPerBlock);
    }
    
    masterBuffer.setSize(2, samplesPerBlock);
}

void AudioGraph::releaseResources()
{
    for (auto& trackStrip : trackStrips)
    {
        trackStrip->releaseResources();
    }
    
    masterBuffer.setSize(0, 0);
}

void AudioGraph::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0)
        return;
    
    // Clear master buffer
    masterBuffer.setSize(numChannels, numSamples, false, false, true);
    masterBuffer.clear();
    
    // Process each track and sum to master
    for (auto& trackStrip : trackStrips)
    {
        // Create a temporary buffer for this track
        juce::AudioBuffer<float> trackBuffer(numChannels, numSamples);
        trackBuffer.clear();
        
        // Process track
        trackStrip->processBlock(trackBuffer, midiMessages);
        
        // Sum to master
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const auto* trackData = trackBuffer.getReadPointer(ch);
            auto* masterData = masterBuffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                masterData[i] += trackData[i];
            }
        }
    }
    
    // Apply master gain
    const auto currentMasterGain = masterGainLinear.load(std::memory_order_acquire);
    if (currentMasterGain != 1.0f)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* masterData = masterBuffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                masterData[i] *= currentMasterGain;
            }
        }
    }
    
    // Copy to output buffer
    for (int ch = 0; ch < numChannels; ++ch)
    {
        buffer.copyFrom(ch, 0, masterBuffer, ch, 0, numSamples);
    }
}

bool AudioGraph::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

daw::audio::dsp::TrackStrip* AudioGraph::addTrack()
{
    auto trackStrip = std::make_unique<daw::audio::dsp::TrackStrip>();
    auto* trackStripPtr = trackStrip.get();
    trackStrips.push_back(std::move(trackStrip));
    return trackStripPtr;
}

void AudioGraph::removeTrack(int index)
{
    if (index >= 0 && index < static_cast<int>(trackStrips.size()))
    {
        trackStrips.erase(trackStrips.begin() + index);
    }
}

daw::audio::dsp::TrackStrip* AudioGraph::getTrack(int index)
{
    if (index >= 0 && index < static_cast<int>(trackStrips.size()))
        return trackStrips[index].get();
    return nullptr;
}

void AudioGraph::setMasterGain(float gainDb) noexcept
{
    masterGainLinear.store(dbToLinear(gainDb), std::memory_order_release);
}

float AudioGraph::getMasterGain() const noexcept
{
    return masterGainLinear.load(std::memory_order_acquire);
}

float AudioGraph::dbToLinear(float db) noexcept
{
    return std::pow(10.0f, db / 20.0f);
}

} // namespace daw::audio::engine

