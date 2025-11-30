#include "EngineContext.h"
#include <juce_core/juce_core.h>

namespace daw::audio::engine
{

EngineContext::EngineContext()
    : engine(std::make_unique<DawEngine>())
{
}

EngineContext::~EngineContext()
{
    shutdown();
}

bool EngineContext::initialise()
{
    return engine->initialise();
}

void EngineContext::shutdown()
{
    if (engine != nullptr)
        engine->shutdown();
}

void EngineContext::play()
{
    engine->play();
}

void EngineContext::stop()
{
    engine->stop();
}

void EngineContext::setPositionInBeats(double positionBeats)
{
    engine->setPositionInBeats(positionBeats);
}

void EngineContext::setTempo(double bpm)
{
    engine->setTempo(bpm);
}

void EngineContext::setTimeSignature(int numerator, int denominator)
{
    engine->setTimeSignature(numerator, denominator);
}

void EngineContext::setMetronomeEnabled(bool enabled)
{
    engine->setMetronomeEnabled(enabled);
}

void EngineContext::setMetronomeVolume(float volume)
{
    engine->setMetronomeVolume(volume);
}

bool EngineContext::isMetronomeEnabled() const
{
    return engine->isMetronomeEnabled();
}

float EngineContext::getMetronomeVolume() const
{
    return engine->getMetronomeVolume();
}

void EngineContext::setLoopEnabled(bool enabled)
{
    engine->setLoopEnabled(enabled);
}

void EngineContext::setLoopRegion(double startBeats, double endBeats)
{
    engine->setLoopRegion(startBeats, endBeats);
}

bool EngineContext::isLoopEnabled() const
{
    return engine->isLoopEnabled();
}

double EngineContext::getLoopStart() const
{
    return engine->getLoopStart();
}

double EngineContext::getLoopEnd() const
{
    return engine->getLoopEnd();
}

bool EngineContext::isPlaying() const
{
    return engine->isPlaying();
}

double EngineContext::getPositionInBeats() const
{
    return engine->getPositionInBeats();
}

double EngineContext::getTempo() const
{
    return engine->getTempo();
}

int EngineContext::getTimeSignatureNumerator() const
{
    return engine->getTimeSignatureNumerator();
}

int EngineContext::getTimeSignatureDenominator() const
{
    return engine->getTimeSignatureDenominator();
}

int EngineContext::addTrack()
{
    return engine->addTrack();
}

void EngineContext::removeTrack(int trackIndex)
{
    engine->removeTrack(trackIndex);
}

void EngineContext::setTrackGain(int trackIndex, float gainDb)
{
    engine->setTrackGain(trackIndex, gainDb);
}

void EngineContext::setTrackPan(int trackIndex, float pan)
{
    engine->setTrackPan(trackIndex, pan);
}

void EngineContext::setTrackMute(int trackIndex, bool muted)
{
    engine->setTrackMute(trackIndex, muted);
}

void EngineContext::setTrackSolo(int trackIndex, bool soloed)
{
    engine->setTrackSolo(trackIndex, soloed);
}

int EngineContext::getNumTracks() const
{
    return engine->getNumTracks();
}

EngineContext::MeterData EngineContext::getTrackMeter(int trackIndex) const
{
    return engine->getTrackMeter(trackIndex);
}

EngineContext::MeterData EngineContext::getMasterMeter() const
{
    return engine->getMasterMeter();
}

void EngineContext::setMasterGain(float gainDb) noexcept
{
    engine->setMasterGain(gainDb);
}

float EngineContext::getMasterGain() const noexcept
{
    return engine->getMasterGain();
}

float EngineContext::getCpuLoad() const
{
    return engine->getCpuLoad();
}

uint64_t EngineContext::getXrunCount() const
{
    return engine->getXrunCount();
}

float EngineContext::getRamUsageMB() const
{
    // JUCE 7 no longer exposes per-process memory usage; return total RAM size.
    return static_cast<float> (juce::SystemStats::getMemorySizeInMegabytes());
}

double EngineContext::getSampleRate() const
{
    auto* device = engine->getDeviceManager().getCurrentAudioDevice();
    return device != nullptr ? device->getCurrentSampleRate() : 44100.0;
}

juce::AudioDeviceManager* EngineContext::getDeviceManager()
{
    return engine != nullptr ? &engine->getDeviceManager() : nullptr;
}

} // namespace daw::audio::engine
