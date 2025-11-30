#include "EngineContext.h"
#include <juce_events/juce_events.h>
#include <chrono>

namespace daw::core {

EngineContext::EngineContext()
    : transportQueue(std::make_unique<TransportQueue>())
    , parameterQueue(std::make_unique<ParameterQueue>())
    , aiResultQueue(std::make_unique<AIResultQueue>())
    , meterQueue(std::make_unique<MeterQueue>())
{
    // Initialize parameter cache
    for (auto& param : parameterCache)
    {
        param.store(0.0f, std::memory_order_relaxed);
    }

    // Initialize meter cache
    Messages::MeterUpdate defaultMeter{};
    for (auto& meter : meterCache)
    {
        meter.store(defaultMeter, std::memory_order_relaxed);
    }
}

EngineContext::~EngineContext()
{
    shutdown();
}

//==============================================================================
// Transport Control
//==============================================================================

void EngineContext::play()
{
    Messages::TransportCommand cmd{};
    cmd.command = Messages::TransportCommand::Play;
    cmd.positionSeconds = engineState.currentPosition.load();
    cmd.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    (void)transportQueue->push(cmd);
}

void EngineContext::stop()
{
    Messages::TransportCommand cmd{};
    cmd.command = Messages::TransportCommand::Stop;
    cmd.positionSeconds = 0.0;
    cmd.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    (void)transportQueue->push(cmd);
}

void EngineContext::pause()
{
    Messages::TransportCommand cmd{};
    cmd.command = Messages::TransportCommand::Pause;
    cmd.positionSeconds = engineState.currentPosition.load();
    cmd.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    (void)transportQueue->push(cmd);
}

void EngineContext::record()
{
    Messages::TransportCommand cmd{};
    cmd.command = Messages::TransportCommand::Record;
    cmd.positionSeconds = engineState.currentPosition.load();
    cmd.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    (void)transportQueue->push(cmd);
}

void EngineContext::setPosition(double positionSeconds)
{
    Messages::TransportCommand cmd{};
    cmd.command = Messages::TransportCommand::SetPosition;
    cmd.positionSeconds = positionSeconds;
    cmd.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    (void)transportQueue->push(cmd);
}

//==============================================================================
// Parameter Control
//==============================================================================

bool EngineContext::setParameter(uint32_t parameterId, float value)
{
    if (parameterId >= parameterCache.size())
        return false;

    Messages::ParameterChange param{};
    param.parameterId = parameterId;
    param.value = value;
    param.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    // Update cache immediately for UI queries
    parameterCache[parameterId].store(value, std::memory_order_relaxed);

    return parameterQueue->push(param);
}

float EngineContext::getParameter(uint32_t parameterId) const
{
    if (parameterId >= parameterCache.size())
        return 0.0f;

    return parameterCache[parameterId].load(std::memory_order_relaxed);
}

//==============================================================================
// AI Integration
//==============================================================================

bool EngineContext::submitAIResult(const Messages::AIResult& result)
{
    return aiResultQueue->push(result);
}

void EngineContext::setAIResultCallback(std::function<void(const Messages::AIResult&)> callback)
{
    aiResultCallback = std::move(callback);
}

//==============================================================================
// Metering & Monitoring
//==============================================================================

Messages::MeterUpdate EngineContext::getMeterData(uint32_t channelId) const
{
    if (channelId >= meterCache.size())
        return Messages::MeterUpdate{};

    return meterCache[channelId].load(std::memory_order_acquire);
}

void EngineContext::setMeterCallback(std::function<void(const Messages::MeterUpdate&)> callback)
{
    meterCallback = std::move(callback);
}

//==============================================================================
// Internal Audio Thread Interface
//==============================================================================

void EngineContext::processAudioThreadMessages() noexcept
{
    processTransportMessages();
    processParameterMessages();
    processAIMessages();
}

void EngineContext::updatePerformanceMetrics(float cpuLoad, int samplesProcessed) noexcept
{
    performanceMetrics.averageLoad.store(cpuLoad, std::memory_order_relaxed);

    // Update peak load with exponential decay
    const float currentPeak = performanceMetrics.peakLoad.load(std::memory_order_relaxed);
    const float newPeak = std::max(cpuLoad, currentPeak * 0.95f);
    performanceMetrics.peakLoad.store(newPeak, std::memory_order_relaxed);

    performanceMetrics.samplesProcessed.fetch_add(samplesProcessed, std::memory_order_relaxed);
    performanceMetrics.callbackCount.fetch_add(1, std::memory_order_relaxed);

    const auto now = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    performanceMetrics.lastCallbackTime.store(now, std::memory_order_relaxed);

    // Update engine state
    engineState.cpuUsage.store(cpuLoad, std::memory_order_relaxed);
}

void EngineContext::submitMeterData(const Messages::MeterUpdate& meterData) noexcept
{
    (void)meterQueue->push(meterData);
}

//==============================================================================
// Lifecycle
//==============================================================================

void EngineContext::initialize()
{
    transportQueue->clear();
    parameterQueue->clear();
    aiResultQueue->clear();
    meterQueue->clear();

    initialized.store(true, std::memory_order_release);
}

void EngineContext::shutdown()
{
    initialized.store(false, std::memory_order_release);

    // Clear callbacks to prevent dangling references
    aiResultCallback = nullptr;
    meterCallback = nullptr;
}

//==============================================================================
// Private Message Processing
//==============================================================================

void EngineContext::processTransportMessages() noexcept
{
    Messages::TransportCommand cmd;
    while (transportQueue->pop(cmd))
    {
        switch (cmd.command)
        {
            case Messages::TransportCommand::Play:
                engineState.isPlaying.store(true, std::memory_order_relaxed);
                engineState.isRecording.store(false, std::memory_order_relaxed);
                break;

            case Messages::TransportCommand::Stop:
                engineState.isPlaying.store(false, std::memory_order_relaxed);
                engineState.isRecording.store(false, std::memory_order_relaxed);
                engineState.currentPosition.store(0.0, std::memory_order_relaxed);
                break;

            case Messages::TransportCommand::Pause:
                engineState.isPlaying.store(false, std::memory_order_relaxed);
                break;

            case Messages::TransportCommand::Record:
                engineState.isPlaying.store(true, std::memory_order_relaxed);
                engineState.isRecording.store(true, std::memory_order_relaxed);
                break;

            case Messages::TransportCommand::SetPosition:
                engineState.currentPosition.store(cmd.positionSeconds, std::memory_order_relaxed);
                break;
        }
    }
}

void EngineContext::processParameterMessages() noexcept
{
    Messages::ParameterChange param;
    while (parameterQueue->pop(param))
    {
        if (param.parameterId < parameterCache.size())
        {
            // Parameter cache is already updated by setParameter(),
            // but audio thread can process the actual parameter change here
            // (e.g., update DSP nodes, filters, etc.)
        }
    }
}

void EngineContext::processAIMessages() noexcept
{
    Messages::AIResult result;
    while (aiResultQueue->pop(result))
    {
        // Audio thread received AI result
        // This could trigger audio processing changes,
        // or queue the result for UI callback processing
        if (aiResultCallback)
        {
            // Schedule callback on message thread (not audio thread)
            juce::MessageManager::callAsync([this, result]()
            {
                if (aiResultCallback)
                    aiResultCallback(result);
            });
        }
    }
}

void EngineContext::processMeterMessages()
{
    Messages::MeterUpdate meter;
    while (meterQueue->pop(meter))
    {
        // Update meter cache
        if (meter.channelId < meterCache.size())
        {
            meterCache[meter.channelId].store(meter, std::memory_order_release);
        }

        // Trigger UI callback if registered
        if (meterCallback)
        {
            meterCallback(meter);
        }
    }
}

} // namespace daw::core
