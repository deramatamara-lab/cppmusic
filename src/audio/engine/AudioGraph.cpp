#include "AudioGraph.h"
#include "../dsp/TrackStrip.h"
#include "../synthesis/Oscillator.h"
#include <cmath>
#include <algorithm>

namespace daw::audio::engine
{

AudioGraph::AudioGraph()
    : masterGainLinear(1.0f)
    , testOscillator(std::make_unique<daw::audio::synthesis::Oscillator>())
{
    // Configure test oscillator for 440Hz sine wave
    testOscillator->setFrequency(440.0f);
    testOscillator->setWaveform(daw::audio::synthesis::Oscillator::Waveform::Sine);
}

AudioGraph::~AudioGraph() = default;

void AudioGraph::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Initialize test oscillator
    if (testOscillator)
    {
        testOscillator->setSampleRate(sampleRate);
    }

    for (auto& trackStrip : trackStrips)
    {
        trackStrip->prepareToPlay(sampleRate, samplesPerBlock);
    }

    // Pre-allocate buffers (no allocations in processBlock)
    masterBuffer.setSize(2, samplesPerBlock);

    // Pre-allocate track buffers (resize if needed, but this is called from non-audio thread)
    if (trackBuffers.size() < trackStrips.size())
    {
        trackBuffers.resize(trackStrips.size());
    }
    for (size_t i = 0; i < trackStrips.size(); ++i)
    {
        if (i < trackBuffers.size())
        {
            trackBuffers[i].setSize(2, samplesPerBlock);
        }
    }
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

    // Ensure buffers are sized (should already be from prepareToPlay, but safety check)
    if (masterBuffer.getNumSamples() < numSamples)
        return; // Not prepared properly

    // Clear master buffer (no allocation - buffer already exists)
    masterBuffer.clear();

    // Generate test tone if no tracks (temporary for audio verification)
    if (trackStrips.empty() && testOscillator)
    {
        constexpr float testVolume = 0.1f; // Low volume test tone
        for (int s = 0; s < numSamples; ++s)
        {
            const float sample = testOscillator->getNextSample() * testVolume;
            for (int ch = 0; ch < numChannels && ch < masterBuffer.getNumChannels(); ++ch)
            {
                masterBuffer.getWritePointer(ch)[s] = sample;
            }
        }
    }

    // Process each track and sum to master
    for (size_t i = 0; i < trackStrips.size(); ++i)
    {
        auto& trackStrip = trackStrips[i];

        // Use pre-allocated track buffer (no allocation)
        if (i < trackBuffers.size())
        {
            auto& trackBuffer = trackBuffers[i];
            if (trackBuffer.getNumSamples() < numSamples)
                continue; // Not prepared properly

            trackBuffer.clear();

            // Process track into pre-allocated buffer
            trackStrip->processBlock(trackBuffer, midiMessages);

            // Sum to master (no allocation)
            for (int ch = 0; ch < numChannels && ch < trackBuffer.getNumChannels(); ++ch)
            {
                const auto* trackData = trackBuffer.getReadPointer(ch);
                auto* masterData = masterBuffer.getWritePointer(ch);
                for (int s = 0; s < numSamples; ++s)
                {
                    masterData[s] += trackData[s];
                }
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
            for (int s = 0; s < numSamples; ++s)
            {
                masterData[s] *= currentMasterGain;
            }
        }
    }

    // Update master meters (lock-free, real-time safe)
    updateMasterMeters(masterBuffer);

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

    // Ensure track buffer exists (resize if needed - this is called from non-audio thread)
    if (trackBuffers.size() < trackStrips.size())
    {
        trackBuffers.resize(trackStrips.size());
        // Size will be set in prepareToPlay
    }

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

AudioGraph::MeterData AudioGraph::getMasterMeter() const noexcept
{
    return { masterPeakLevel.load(std::memory_order_acquire),
             masterRmsLevel.load(std::memory_order_acquire) };
}

void AudioGraph::updateMasterMeters(const juce::AudioBuffer<float>& buffer) noexcept
{
    // Real-time safe meter update (lock-free)
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (numChannels == 0 || numSamples == 0)
        return;

    float peak = 0.0f;
    float sumSquared = 0.0f;

    // Calculate peak and RMS across all channels
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* channelData = buffer.getReadPointer(ch);
        for (int s = 0; s < numSamples; ++s)
        {
            const float absValue = std::abs(channelData[s]);
            peak = std::max(peak, absValue);
            sumSquared += channelData[s] * channelData[s];
        }
    }

    // Calculate RMS
    const float rms = std::sqrt(sumSquared / static_cast<float>(numChannels * numSamples));

    // Update atomically (lock-free)
    masterPeakLevel.store(peak, std::memory_order_release);
    masterRmsLevel.store(rms, std::memory_order_release);
}

} // namespace daw::audio::engine
