#include "DawEngine.h"
#include "../dsp/TrackStrip.h"
#include <algorithm>
#include <chrono>

namespace daw::audio::engine
{

// Audio callback wrapper class
class DawEngine::AudioCallback : public juce::AudioIODeviceCallback
{
public:
    explicit AudioCallback(DawEngine& engine) : engine(engine) {}

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels,
                                          float* const* outputChannelData, int numOutputChannels,
                                          int numSamples, const juce::AudioIODeviceCallbackContext& context) override
    {
        juce::ignoreUnused(inputChannelData, numInputChannels, context);
        
        if (outputChannelData == nullptr || numOutputChannels == 0 || numSamples == 0)
            return;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        juce::AudioBuffer<float> buffer(const_cast<float* const*>(outputChannelData), numOutputChannels, numSamples);
        juce::MidiBuffer midiMessages;
        
        engine.processBlock(buffer, midiMessages);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto processTime = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
        
        auto* device = engine.deviceManager.getCurrentAudioDevice();
        const auto sampleRate = device != nullptr ? device->getCurrentSampleRate() : 44100.0;
        engine.updateCpuLoad(processTime, numSamples, sampleRate);
    }

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override
    {
        if (device != nullptr)
        {
            engine.prepareToPlay(device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples());
        }
    }

    void audioDeviceStopped() override
    {
        engine.releaseResources();
    }

    void audioDeviceError(const juce::String& errorMessage) override
    {
        juce::ignoreUnused(errorMessage);
    }

private:
    DawEngine& engine;
};

DawEngine::DawEngine()
    : cpuLoad(0.0f)
    , processBlockCount(0)
    , audioCallback(std::make_unique<AudioCallback>(*this))
{
    transport = std::make_unique<Transport>();
    audioGraph = std::make_unique<AudioGraph>();
}

DawEngine::~DawEngine()
{
    shutdown();
}

bool DawEngine::initialise()
{
    juce::String error = deviceManager.initialiseWithDefaultDevices(0, 2);
    if (error.isNotEmpty())
        return false;
    
    deviceManager.addAudioCallback(audioCallback.get());
    return true;
}

void DawEngine::shutdown()
{
    deviceManager.removeAudioCallback(audioCallback.get());
    deviceManager.closeAudioDevice();
}

void DawEngine::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    audioGraph->prepareToPlay(sampleRate, samplesPerBlock);
}

void DawEngine::releaseResources()
{
    audioGraph->releaseResources();
}

void DawEngine::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) noexcept
{
    const auto numSamples = buffer.getNumSamples();
    
    // Update transport position
    if (transport->isPlaying())
    {
        auto* device = deviceManager.getCurrentAudioDevice();
        if (device != nullptr)
        {
            transport->updatePosition(numSamples, device->getCurrentSampleRate());
        }
    }
    
    // Process audio graph
    audioGraph->processBlock(buffer, midiMessages);
}

void DawEngine::play()
{
    transport->play();
}

void DawEngine::stop()
{
    transport->stop();
}

void DawEngine::setPositionInBeats(double positionBeats)
{
    transport->setPositionInBeats(positionBeats);
}

void DawEngine::setTempo(double bpm)
{
    transport->setTempo(bpm);
}

void DawEngine::setTimeSignature(int numerator, int denominator)
{
    transport->setTimeSignature(numerator, denominator);
}

bool DawEngine::isPlaying() const
{
    return transport->isPlaying();
}

double DawEngine::getPositionInBeats() const
{
    return transport->getPositionInBeats();
}

double DawEngine::getTempo() const
{
    return transport->getTempo();
}

int DawEngine::getTimeSignatureNumerator() const
{
    return transport->getTimeSignatureNumerator();
}

int DawEngine::getTimeSignatureDenominator() const
{
    return transport->getTimeSignatureDenominator();
}

int DawEngine::addTrack()
{
    auto* trackStrip = audioGraph->addTrack();
    if (trackStrip != nullptr)
    {
        auto* device = deviceManager.getCurrentAudioDevice();
        if (device != nullptr)
        {
            trackStrip->prepareToPlay(device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples());
        }
        return audioGraph->getNumTracks() - 1;
    }
    return -1;
}

void DawEngine::removeTrack(int trackIndex)
{
    audioGraph->removeTrack(trackIndex);
}

void DawEngine::setTrackGain(int trackIndex, float gainDb)
{
    auto* trackStrip = audioGraph->getTrack(trackIndex);
    if (trackStrip != nullptr)
        trackStrip->setGain(gainDb);
}

void DawEngine::setTrackPan(int trackIndex, float pan)
{
    auto* trackStrip = audioGraph->getTrack(trackIndex);
    if (trackStrip != nullptr)
        trackStrip->setPan(pan);
}

void DawEngine::setTrackMute(int trackIndex, bool muted)
{
    auto* trackStrip = audioGraph->getTrack(trackIndex);
    if (trackStrip != nullptr)
        trackStrip->setMute(muted);
}

void DawEngine::setTrackSolo(int trackIndex, bool soloed)
{
    auto* trackStrip = audioGraph->getTrack(trackIndex);
    if (trackStrip != nullptr)
        trackStrip->setSolo(soloed);
}

int DawEngine::getNumTracks() const
{
    return audioGraph->getNumTracks();
}

DawEngine::MeterData DawEngine::getTrackMeter(int trackIndex) const
{
    const auto* trackStrip = audioGraph->getTrack(trackIndex);
    if (trackStrip != nullptr)
    {
        return { trackStrip->getPeakLevel(), trackStrip->getRmsLevel() };
    }
    return { 0.0f, 0.0f };
}

DawEngine::MeterData DawEngine::getMasterMeter() const
{
    // For now, return combined meter from all tracks
    // In a full implementation, this would be a separate master meter
    MeterData master = { 0.0f, 0.0f };
    const auto numTracks = audioGraph->getNumTracks();
    for (int i = 0; i < numTracks; ++i)
    {
        const auto trackMeter = getTrackMeter(i);
        master.peak = std::max(master.peak, trackMeter.peak);
        master.rms = std::max(master.rms, trackMeter.rms);
    }
    return master;
}

float DawEngine::getCpuLoad() const
{
    return cpuLoad.load(std::memory_order_acquire);
}

float DawEngine::getCpuLoadPercent() const
{
    return performanceMonitor.getCpuLoadPercent();
}

uint64_t DawEngine::getXrunCount() const
{
    return performanceMonitor.getXrunCount();
}

void DawEngine::resetXrunCount()
{
    performanceMonitor.resetXrunCount();
}

std::chrono::nanoseconds DawEngine::getP50ProcessTime() const
{
    return performanceMonitor.getP50ProcessTime();
}

std::chrono::nanoseconds DawEngine::getP95ProcessTime() const
{
    return performanceMonitor.getP95ProcessTime();
}

std::chrono::nanoseconds DawEngine::getP99ProcessTime() const
{
    return performanceMonitor.getP99ProcessTime();
}

void DawEngine::updateCpuLoad(std::chrono::high_resolution_clock::duration processTime, int numSamples, double sampleRate)
{
    // Update performance monitor (provides P50/P95/P99 and xrun detection)
    performanceMonitor.recordProcessTime(
        std::chrono::duration_cast<std::chrono::nanoseconds>(processTime),
        numSamples,
        sampleRate
    );
    
    // Legacy CPU load calculation (kept for compatibility)
    ++processBlockCount;
    accumulatedProcessTime += processTime;
    
    // Update every 10 blocks
    if (processBlockCount >= 10)
    {
        const auto totalTime = std::chrono::duration_cast<std::chrono::microseconds>(accumulatedProcessTime).count();
        const auto bufferTime = (numSamples / sampleRate) * 1e6 * processBlockCount;
        const auto load = bufferTime > 0.0 ? (static_cast<float>(totalTime) / static_cast<float>(bufferTime)) * 100.0f : 0.0f;
        
        cpuLoad.store(juce::jlimit(0.0f, 100.0f, load), std::memory_order_release);
        
        processBlockCount = 0;
        accumulatedProcessTime = std::chrono::high_resolution_clock::duration::zero();
    }
}

} // namespace daw::audio::engine

