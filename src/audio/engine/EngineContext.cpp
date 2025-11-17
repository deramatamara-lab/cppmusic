#include "EngineContext.h"

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

float EngineContext::getCpuLoad() const
{
    return engine->getCpuLoad();
}

} // namespace daw::audio::engine

