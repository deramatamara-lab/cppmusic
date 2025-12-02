/**
 * @file PlaybackController.cpp
 * @brief Implementation of PlaybackController.
 */

#include "PlaybackController.hpp"
#include <algorithm>

namespace daw::ui::core
{

PlaybackController::PlaybackController(
    std::shared_ptr<daw::audio::engine::EngineContext> engineContext,
    std::shared_ptr<daw::project::ProjectModel> projectModel)
    : engineContext_(std::move(engineContext))
    , projectModel_(std::move(projectModel))
{
    // Initialize state from engine if available
    if (engineContext_)
    {
        updateStateFromEngine();
    }
}

PlaybackController::~PlaybackController()
{
    listeners_.clear();
}

// =========================================================================
// Transport Control
// =========================================================================

void PlaybackController::play()
{
    if (engineContext_)
    {
        engineContext_->play();
        playing_.store(true, std::memory_order_release);
        notifyTransportStateChanged(true);
    }
}

void PlaybackController::stop(bool resetPosition)
{
    if (engineContext_)
    {
        engineContext_->stop();
        playing_.store(false, std::memory_order_release);
        
        if (resetPosition)
        {
            engineContext_->setPositionInBeats(0.0);
            positionBeats_.store(0.0, std::memory_order_release);
            notifyPositionChanged(0.0);
        }
        
        notifyTransportStateChanged(false);
    }
}

void PlaybackController::togglePlayStop()
{
    if (isPlaying())
    {
        stop();
    }
    else
    {
        play();
    }
}

void PlaybackController::toggleLoop()
{
    bool newState = !loopEnabled_.load(std::memory_order_acquire);
    setLoopEnabled(newState);
}

void PlaybackController::seekToBeats(double beats)
{
    if (engineContext_)
    {
        engineContext_->setPositionInBeats(beats);
        positionBeats_.store(beats, std::memory_order_release);
        notifyPositionChanged(beats);
    }
}

void PlaybackController::setTempo(double bpm)
{
    if (engineContext_)
    {
        engineContext_->setTempo(bpm);
        tempo_.store(bpm, std::memory_order_release);
        notifyTempoChanged(bpm);
    }
}

void PlaybackController::setTimeSignature(int numerator, int denominator)
{
    if (engineContext_)
    {
        engineContext_->setTimeSignature(numerator, denominator);
    }
}

void PlaybackController::setLoopRegion(double startBeats, double endBeats)
{
    if (engineContext_)
    {
        engineContext_->setLoopRegion(startBeats, endBeats);
        loopStartBeats_.store(startBeats, std::memory_order_release);
        loopEndBeats_.store(endBeats, std::memory_order_release);
        notifyLoopChanged(loopEnabled_.load(std::memory_order_acquire), startBeats, endBeats);
    }
}

void PlaybackController::setLoopEnabled(bool enabled)
{
    if (engineContext_)
    {
        engineContext_->setLoopEnabled(enabled);
        loopEnabled_.store(enabled, std::memory_order_release);
        notifyLoopChanged(enabled, 
                          loopStartBeats_.load(std::memory_order_acquire),
                          loopEndBeats_.load(std::memory_order_acquire));
    }
}

void PlaybackController::setMetronomeEnabled(bool enabled)
{
    if (engineContext_)
    {
        engineContext_->setMetronomeEnabled(enabled);
        metronomeEnabled_.store(enabled, std::memory_order_release);
    }
}

void PlaybackController::setMetronomeVolume(float volume)
{
    if (engineContext_)
    {
        engineContext_->setMetronomeVolume(volume);
    }
}

// =========================================================================
// State Queries
// =========================================================================

PlaybackState PlaybackController::getCurrentState() const
{
    PlaybackState state;
    
    if (engineContext_)
    {
        state.positionBeats = engineContext_->getPositionInBeats();
        state.tempo = engineContext_->getTempo();
        state.timeSignatureNumerator = engineContext_->getTimeSignatureNumerator();
        state.timeSignatureDenominator = engineContext_->getTimeSignatureDenominator();
        state.playing = engineContext_->isPlaying();
        state.loopEnabled = engineContext_->isLoopEnabled();
        state.loopStartBeats = engineContext_->getLoopStart();
        state.loopEndBeats = engineContext_->getLoopEnd();
        state.metronomeEnabled = engineContext_->isMetronomeEnabled();
        state.cpuLoad = engineContext_->getCpuLoad();
        
        auto masterMeter = engineContext_->getMasterMeter();
        state.masterPeak = masterMeter.peak;
        state.masterRms = masterMeter.rms;
    }
    else
    {
        // Return cached atomic values if no engine
        state.positionBeats = positionBeats_.load(std::memory_order_acquire);
        state.tempo = tempo_.load(std::memory_order_acquire);
        state.playing = playing_.load(std::memory_order_acquire);
        state.loopEnabled = loopEnabled_.load(std::memory_order_acquire);
        state.loopStartBeats = loopStartBeats_.load(std::memory_order_acquire);
        state.loopEndBeats = loopEndBeats_.load(std::memory_order_acquire);
        state.metronomeEnabled = metronomeEnabled_.load(std::memory_order_acquire);
    }
    
    return state;
}

bool PlaybackController::isPlaying() const
{
    if (engineContext_)
    {
        return engineContext_->isPlaying();
    }
    return playing_.load(std::memory_order_acquire);
}

double PlaybackController::getPositionBeats() const
{
    if (engineContext_)
    {
        return engineContext_->getPositionInBeats();
    }
    return positionBeats_.load(std::memory_order_acquire);
}

double PlaybackController::getTempo() const
{
    if (engineContext_)
    {
        return engineContext_->getTempo();
    }
    return tempo_.load(std::memory_order_acquire);
}

bool PlaybackController::isLoopEnabled() const
{
    if (engineContext_)
    {
        return engineContext_->isLoopEnabled();
    }
    return loopEnabled_.load(std::memory_order_acquire);
}

// =========================================================================
// Listener Management
// =========================================================================

void PlaybackController::addListener(PlaybackListener* listener)
{
    if (listener != nullptr)
    {
        auto it = std::find(listeners_.begin(), listeners_.end(), listener);
        if (it == listeners_.end())
        {
            listeners_.push_back(listener);
        }
    }
}

void PlaybackController::removeListener(PlaybackListener* listener)
{
    auto it = std::find(listeners_.begin(), listeners_.end(), listener);
    if (it != listeners_.end())
    {
        listeners_.erase(it);
    }
}

// =========================================================================
// Update Method
// =========================================================================

void PlaybackController::updateStateFromEngine()
{
    if (engineContext_)
    {
        double newPosition = engineContext_->getPositionInBeats();
        bool newPlaying = engineContext_->isPlaying();
        double newTempo = engineContext_->getTempo();
        bool newLoopEnabled = engineContext_->isLoopEnabled();
        double newLoopStart = engineContext_->getLoopStart();
        double newLoopEnd = engineContext_->getLoopEnd();
        bool newMetronome = engineContext_->isMetronomeEnabled();
        
        // Update atomic state
        positionBeats_.store(newPosition, std::memory_order_release);
        playing_.store(newPlaying, std::memory_order_release);
        tempo_.store(newTempo, std::memory_order_release);
        loopEnabled_.store(newLoopEnabled, std::memory_order_release);
        loopStartBeats_.store(newLoopStart, std::memory_order_release);
        loopEndBeats_.store(newLoopEnd, std::memory_order_release);
        metronomeEnabled_.store(newMetronome, std::memory_order_release);
    }
}

// =========================================================================
// Listener Notification Helpers
// =========================================================================

void PlaybackController::notifyTransportStateChanged(bool playing)
{
    for (auto* listener : listeners_)
    {
        listener->onTransportStateChanged(playing);
    }
}

void PlaybackController::notifyPositionChanged(double positionBeats)
{
    for (auto* listener : listeners_)
    {
        listener->onPositionChanged(positionBeats);
    }
}

void PlaybackController::notifyTempoChanged(double bpm)
{
    for (auto* listener : listeners_)
    {
        listener->onTempoChanged(bpm);
    }
}

void PlaybackController::notifyLoopChanged(bool enabled, double startBeats, double endBeats)
{
    for (auto* listener : listeners_)
    {
        listener->onLoopChanged(enabled, startBeats, endBeats);
    }
}

} // namespace daw::ui::core
