#include "EqualizerService.h"
#include <juce_core/juce_core.h>

namespace cppmusic {
namespace integration {

//==============================================================================
EqualizerService::EqualizerService(daw::core::EngineContext& context, daw::core::RTMemoryPool& pool)
    : engineContext_(context)
    , memoryPool_(pool)
    , parameterQueue_()
    , statisticsQueue_()
{
    setupAutomationParameters();
    setupDefaultPresets();
}

EqualizerService::~EqualizerService()
{
    shutdown();
}

//==============================================================================
void EqualizerService::initialize(const audio::AnalogModeledEQ::Config& config)
{
    if (initialized_.load()) {
        shutdown();
    }

    // Create EQ instance with engine context
    equalizer_ = std::make_unique<audio::AnalogModeledEQ>(
        engineContext_,
        memoryPool_
    );

    // Prepare the EQ
    equalizer_->prepare(config);
    equalizer_->reset();

    initialized_.store(true);

    juce::Logger::writeToLog("EqualizerService initialized with sample rate: " +
                            juce::String(config.sampleRate) + " Hz");
}

void EqualizerService::shutdown()
{
    if (!initialized_.load()) return;

    initialized_.store(false);
    equalizer_.reset();

    // Clear message queues
    EQParameterUpdate paramUpdate;
    while (parameterQueue_.pop(paramUpdate)) {}

    EQStatisticsUpdate statsUpdate;
    while (statisticsQueue_.pop(statsUpdate)) {}

    juce::Logger::writeToLog("EqualizerService shutdown complete");
}

//==============================================================================
void EqualizerService::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (!initialized_.load() || !equalizer_) {
        return;
    }

    auto startTime = juce::Time::getHighResolutionTicks();

    // Process any pending parameter updates (real-time safe)
    processParameterUpdates();

    // Bypass check
    if (!bypassed_.load()) {
        // Process audio through EQ
        equalizer_->processBlock(buffer);
    }

    // Send statistics update (throttled to avoid overwhelming UI)
    static int statisticsCounter = 0;
    if (++statisticsCounter >= 32) { // Every ~32nd block (roughly 15-30 Hz update rate)
        sendStatisticsUpdate();
        statisticsCounter = 0;
    }

    // Update performance metrics
    auto endTime = juce::Time::getHighResolutionTicks();
    auto processingTime = juce::Time::highResolutionTicksToSeconds(endTime - startTime);
    updatePerformanceMetrics(processingTime);
}

//==============================================================================
std::unique_ptr<ui::AnalogEQEditor> EqualizerService::createEditor()
{
    if (!initialized_.load() || !equalizer_) {
        return nullptr;
    }

    auto editor = std::make_unique<ui::AnalogEQEditor>(*equalizer_, engineContext_);
    currentEditor_ = editor.get();
    return editor;
}

void EqualizerService::sendParameterUpdate(const EQParameterUpdate& update)
{
    if (!parameterQueue_.push(update)) {
        // Queue full - increment dropped messages counter
        std::lock_guard<std::mutex> lock(performanceMetricsMutex_);
        performanceMetrics_.droppedMessages++;

        juce::Logger::writeToLog("EqualizerService: Parameter queue full, dropping update");
    }
}

EqualizerService::EQStatisticsUpdate EqualizerService::getLatestStatistics()
{
    EQStatisticsUpdate latest;

    // Get the most recent statistics (non-blocking)
    EQStatisticsUpdate temp;
    while (statisticsQueue_.pop(temp)) {
        latest = temp;
    }

    return latest;
}

//==============================================================================
void EqualizerService::savePreset(const juce::String& name, int slot)
{
    if (!initialized_.load() || !equalizer_) return;

    std::lock_guard<std::mutex> lock(presetsMutex_);

    PresetData presetData;
    presetData.name = name;
    presetData.data = equalizer_->savePreset(name.toStdString());
    presetData.createdTime = juce::Time::getCurrentTime();

    // Expand presets vector if necessary
    if (slot >= static_cast<int>(storedPresets_.size())) {
        storedPresets_.resize(slot + 1);
    }

    storedPresets_[slot] = presetData;

    juce::Logger::writeToLog("Saved EQ preset: " + name + " to slot " + juce::String(slot));
}

bool EqualizerService::loadPreset(const juce::String& name)
{
    if (!initialized_.load() || !equalizer_) return false;

    std::lock_guard<std::mutex> lock(presetsMutex_);

    // Find preset by name
    for (const auto& preset : storedPresets_) {
        if (preset.name == name) {
            equalizer_->loadPreset(preset.data);

            // Update UI if connected
            if (currentEditor_) {
                currentEditor_->updateFromEQ();
            }

            juce::Logger::writeToLog("Loaded EQ preset: " + name);
            return true;
        }
    }

    juce::Logger::writeToLog("EQ preset not found: " + name);
    return false;
}

juce::StringArray EqualizerService::getAvailablePresets() const
{
    std::lock_guard<std::mutex> lock(presetsMutex_);

    juce::StringArray presetNames;
    for (const auto& preset : storedPresets_) {
        if (preset.name.isNotEmpty()) {
            presetNames.add(preset.name);
        }
    }

    return presetNames;
}

//==============================================================================
void EqualizerService::setAutomationParameter(int parameterId, float normalizedValue)
{
    if (parameterId < 0 || parameterId >= numAutomationParameters_) return;

    const auto& param = automationParameters_[parameterId];
    float actualValue = param.minValue + normalizedValue * (param.maxValue - param.minValue);

    if (param.setter) {
        param.setter(actualValue);
    }
}

float EqualizerService::getAutomationParameter(int parameterId) const
{
    if (parameterId < 0 || parameterId >= numAutomationParameters_) return 0.0f;

    const auto& param = automationParameters_[parameterId];
    if (param.getter) {
        float actualValue = param.getter();
        return (actualValue - param.minValue) / (param.maxValue - param.minValue);
    }

    return 0.0f;
}

juce::String EqualizerService::getParameterName(int parameterId) const
{
    if (parameterId < 0 || parameterId >= numAutomationParameters_) return {};
    return automationParameters_[parameterId].name;
}

juce::String EqualizerService::getParameterText(int parameterId) const
{
    if (parameterId < 0 || parameterId >= numAutomationParameters_) return {};

    const auto& param = automationParameters_[parameterId];
    if (param.getter) {
        float value = param.getter();
        return juce::String(value, 2) + param.unit;
    }

    return {};
}

float EqualizerService::getParameterDefaultValue(int parameterId) const
{
    if (parameterId < 0 || parameterId >= numAutomationParameters_) return 0.0f;

    const auto& param = automationParameters_[parameterId];
    return (param.defaultValue - param.minValue) / (param.maxValue - param.minValue);
}

//==============================================================================
void EqualizerService::handleMidiControlChange(int controller, int value)
{
    std::lock_guard<std::mutex> lock(midiMutex_);

    auto it = midiControllerMap_.find(controller);
    if (it != midiControllerMap_.end()) {
        int parameterId = it->second;
        float normalizedValue = value / 127.0f;
        setAutomationParameter(parameterId, normalizedValue);
    }
}

void EqualizerService::assignMidiController(int parameterId, int midiController)
{
    std::lock_guard<std::mutex> lock(midiMutex_);
    midiControllerMap_[midiController] = parameterId;

    juce::Logger::writeToLog("Assigned MIDI CC " + juce::String(midiController) +
                            " to parameter " + juce::String(parameterId));
}

//==============================================================================
EqualizerService::PerformanceMetrics EqualizerService::getPerformanceMetrics() const
{
    std::lock_guard<std::mutex> lock(performanceMetricsMutex_);
    return performanceMetrics_;
}

void EqualizerService::resetPerformanceMetrics()
{
    std::lock_guard<std::mutex> lock(performanceMetricsMutex_);
    performanceMetrics_ = {};
}

//==============================================================================
// Private methods (Audio thread - real-time safe)
//==============================================================================

void EqualizerService::processParameterUpdates()
{
    EQParameterUpdate update;
    int updatesProcessed = 0;
    const int maxUpdatesPerBlock = 16; // Limit updates per block for consistent performance

    while (parameterQueue_.pop(update) && updatesProcessed < maxUpdatesPerBlock) {
        switch (update.type) {
            case EQParameterUpdate::BandEnabled:
                equalizer_->setBandEnabled(update.bandIndex, update.boolValue);
                break;

            case EQParameterUpdate::BandFrequency:
                equalizer_->setBandFrequency(update.bandIndex, update.floatValue);
                break;

            case EQParameterUpdate::BandGain:
                equalizer_->setBandGain(update.bandIndex, update.floatValue);
                break;

            case EQParameterUpdate::BandQ:
                equalizer_->setBandQ(update.bandIndex, update.floatValue);
                break;

            case EQParameterUpdate::BandDrive:
                equalizer_->setBandDrive(update.bandIndex, update.floatValue);
                break;

            case EQParameterUpdate::BandSaturation:
                equalizer_->setBandSaturation(update.bandIndex, update.floatValue);
                break;

            case EQParameterUpdate::BandMix:
                equalizer_->setBandMix(update.bandIndex, update.floatValue);
                break;

            case EQParameterUpdate::BandSolo:
                equalizer_->soloBand(update.bandIndex, update.boolValue);
                break;

            case EQParameterUpdate::BandBypass:
                equalizer_->bypassBand(update.bandIndex, update.boolValue);
                break;

            case EQParameterUpdate::InputGain:
                equalizer_->setInputGain(update.floatValue);
                break;

            case EQParameterUpdate::OutputGain:
                equalizer_->setOutputGain(update.floatValue);
                break;

            case EQParameterUpdate::TransformerDrive:
                equalizer_->setTransformerDrive(update.floatValue);
                break;

            case EQParameterUpdate::TubeWarmth:
                equalizer_->setTubeWarmth(update.floatValue);
                break;

            case EQParameterUpdate::TapeSaturation:
                equalizer_->setTapeSaturation(update.floatValue);
                break;

            case EQParameterUpdate::AnalogNoise:
                equalizer_->setAnalogNoise(update.floatValue);
                break;

            case EQParameterUpdate::AnalogModel:
                equalizer_->setAnalogModel(static_cast<audio::AnalogModeledEQ::AnalogModel>(update.intValue));
                break;

            case EQParameterUpdate::BypassAll:
                equalizer_->bypassAll(update.boolValue);
                break;

            default:
                break;
        }

        ++updatesProcessed;
    }
}

void EqualizerService::sendStatisticsUpdate()
{
    if (!equalizer_) return;

    EQStatisticsUpdate stats;

    // Get statistics from EQ
    const auto& eqStats = equalizer_->getStatistics();
    stats.inputPeakL = eqStats.inputPeakL.load();
    stats.inputPeakR = eqStats.inputPeakR.load();
    stats.outputPeakL = eqStats.outputPeakL.load();
    stats.outputPeakR = eqStats.outputPeakR.load();
    stats.totalGainReduction = eqStats.totalGainReduction.load();
    stats.analogHarmonics = eqStats.analogHarmonics.load();
    stats.cpuUsage = eqStats.cpuUsage.load();
    stats.processedSamples = eqStats.processedSamples.load();

    // Get analysis data
    stats.totalHarmonicDistortion = equalizer_->getAnalysisData().totalHarmonicDistortion;

    // Send to UI thread (non-blocking)
    if (!statisticsQueue_.push(stats)) {
        // Queue full - not critical, just skip this update
    }
}

void EqualizerService::updatePerformanceMetrics(double processingTime)
{
    std::lock_guard<std::mutex> lock(performanceMetricsMutex_);

    performanceMetrics_.totalProcessedBlocks++;

    // Update average processing time (exponential moving average)
    const double alpha = 0.1; // Smoothing factor
    performanceMetrics_.averageProcessingTime =
        alpha * processingTime + (1.0 - alpha) * performanceMetrics_.averageProcessingTime;

    // Update peak processing time
    if (processingTime > performanceMetrics_.peakProcessingTime) {
        performanceMetrics_.peakProcessingTime = processingTime;
    }

    // Estimate CPU usage percentage (rough approximation)
    double sampleRate = 44100.0; // Would get from actual config
    double bufferSize = 512.0;   // Would get from actual config
    double blockDuration = bufferSize / sampleRate;
    performanceMetrics_.cpuUsagePercent = (processingTime / blockDuration) * 100.0;
}

//==============================================================================
// Parameter conversion helpers
//==============================================================================

float EqualizerService::normalizedToBandFrequency(float normalized) const
{
    // Logarithmic mapping from 20 Hz to 20 kHz
    return 20.0f * std::pow(1000.0f, normalized);
}

float EqualizerService::bandFrequencyToNormalized(float frequency) const
{
    return std::log10(frequency / 20.0f) / std::log10(1000.0f);
}

float EqualizerService::normalizedToBandGain(float normalized) const
{
    // Linear mapping from -24 dB to +24 dB
    return (normalized * 48.0f) - 24.0f;
}

float EqualizerService::bandGainToNormalized(float gain) const
{
    return (gain + 24.0f) / 48.0f;
}

float EqualizerService::normalizedToBandQ(float normalized) const
{
    // Logarithmic mapping from 0.1 to 40
    return 0.1f * std::pow(400.0f, normalized);
}

float EqualizerService::bandQToNormalized(float q) const
{
    return std::log10(q / 0.1f) / std::log10(400.0f);
}

//==============================================================================
// Initialization helpers
//==============================================================================

void EqualizerService::setupAutomationParameters()
{
    numAutomationParameters_ = 0;

    // Add band parameters
    for (int band = 0; band < audio::AnalogModeledEQ::NUM_BANDS; ++band) {
        // Frequency
        automationParameters_[numAutomationParameters_++] = {
            "Band " + juce::String(band + 1) + " Frequency",
            20.0f, 20000.0f, 1000.0f,
            [this, band](float value) {
                sendParameterUpdate(EQParameterUpdate::bandFrequency(band, value));
            },
            nullptr, // Getter would need access to current EQ state
            " Hz"
        };

        // Gain
        automationParameters_[numAutomationParameters_++] = {
            "Band " + juce::String(band + 1) + " Gain",
            -24.0f, 24.0f, 0.0f,
            [this, band](float value) {
                sendParameterUpdate(EQParameterUpdate::bandGain(band, value));
            },
            nullptr,
            " dB"
        };

        // Q
        automationParameters_[numAutomationParameters_++] = {
            "Band " + juce::String(band + 1) + " Q",
            0.1f, 40.0f, 1.0f,
            [this, band](float value) {
                sendParameterUpdate({EQParameterUpdate::BandQ, band, value, 0, false});
            },
            nullptr,
            ""
        };
    }

    // Add global parameters
    automationParameters_[numAutomationParameters_++] = {
        "Input Gain", -24.0f, 24.0f, 0.0f,
        [this](float value) {
            sendParameterUpdate({EQParameterUpdate::InputGain, -1, value, 0, false});
        },
        nullptr, " dB"
    };

    automationParameters_[numAutomationParameters_++] = {
        "Output Gain", -24.0f, 24.0f, 0.0f,
        [this](float value) {
            sendParameterUpdate({EQParameterUpdate::OutputGain, -1, value, 0, false});
        },
        nullptr, " dB"
    };

    // ... Add remaining global parameters
}

void EqualizerService::setupDefaultPresets()
{
    // This would set up factory presets like "Studio EQ", "Mastering", etc.
    // Implementation would create presets and store them in storedPresets_
}

//==============================================================================
// JUCE AudioProcessor Implementation
//==============================================================================

EqualizerProcessor::EqualizerProcessor()
    : AudioProcessor(BusesProperties()
                    .withInput("Input", juce::AudioChannelSet::stereo(), true)
                    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // Initialize engine context and memory pool
    engineContext_ = std::make_unique<daw::core::EngineContext>();

    daw::core::RTMemoryPool::PoolConfig poolConfig;
    poolConfig.maxPoolSize = 1024 * 1024;
    memoryPool_ = std::make_unique<daw::core::RTMemoryPool>(poolConfig); // 1MB pool

    // Create equalizer service
    equalizerService_ = std::make_unique<EqualizerService>(*engineContext_, *memoryPool_);
}

EqualizerProcessor::~EqualizerProcessor()
{
}

void EqualizerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    audio::AnalogModeledEQ::Config config;
    config.sampleRate = sampleRate;
    config.maxBlockSize = samplesPerBlock;
    config.enableOversampling = true;
    config.analogModel = audio::AnalogModeledEQ::AnalogModel::NeveVintage;

    equalizerService_->initialize(config);
}

void EqualizerProcessor::releaseResources()
{
    equalizerService_->shutdown();
}

void EqualizerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Process MIDI messages
    for (const auto metadata : midiMessages) {
        const auto midiMessage = metadata.getMessage();
        if (midiMessage.isController()) {
            equalizerService_->handleMidiControlChange(
                midiMessage.getControllerNumber(),
                midiMessage.getControllerValue()
            );
        }
    }

    // Process audio
    equalizerService_->processBlock(buffer);
}

juce::AudioProcessorEditor* EqualizerProcessor::createEditor()
{
    return new EqualizerProcessorEditor(*this);
}

void EqualizerProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ignoreUnused(destData);
    // Serialize current EQ state
    // Implementation would save all parameter values and presets
}

void EqualizerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
    // Deserialize EQ state
    // Implementation would restore all parameter values and presets
}

int EqualizerProcessor::getNumParameters()
{
    return 64; // Max automation parameters
}

float EqualizerProcessor::getParameter(int index)
{
    return equalizerService_->getAutomationParameter(index);
}

void EqualizerProcessor::setParameter(int index, float newValue)
{
    equalizerService_->setAutomationParameter(index, newValue);
}

const juce::String EqualizerProcessor::getParameterName(int index)
{
    return equalizerService_->getParameterName(index);
}

const juce::String EqualizerProcessor::getParameterText(int index)
{
    return equalizerService_->getParameterText(index);
}

int EqualizerProcessor::getNumPrograms() { return 1; }
int EqualizerProcessor::getCurrentProgram() { return currentProgram_; }
void EqualizerProcessor::setCurrentProgram(int index) { currentProgram_ = index; }
const juce::String EqualizerProcessor::getProgramName(int index) { juce::ignoreUnused(index); return "Default"; }
void EqualizerProcessor::changeProgramName(int index, const juce::String& newName) { juce::ignoreUnused(index, newName); }

//==============================================================================
EqualizerProcessorEditor::EqualizerProcessorEditor(EqualizerProcessor& processor)
    : AudioProcessorEditor(&processor), processor_(processor)
{
    lookAndFeel_ = std::make_unique<ui::AnalogEQLookAndFeel>();
    setLookAndFeel(lookAndFeel_.get());

    // Create the EQ editor from the service
    eqEditor_ = processor_.equalizerService_->createEditor();
    if (eqEditor_) {
        addAndMakeVisible(*eqEditor_);
        setSize(eqEditor_->getWidth(), eqEditor_->getHeight());
    } else {
        setSize(400, 300);
    }
}

EqualizerProcessorEditor::~EqualizerProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void EqualizerProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));
}

void EqualizerProcessorEditor::resized()
{
    if (eqEditor_) {
        eqEditor_->setBounds(getLocalBounds());
    }
}

} // namespace integration
} // namespace cppmusic
