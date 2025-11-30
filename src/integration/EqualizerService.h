#pragma once

#include "../audio/AnalogModeledEQ.h"
#include "../ui/AnalogEQEditor.h"
#include "../core/EngineContext.h"
#include "../core/RealtimeMessageQueue.h"

namespace cppmusic {
namespace integration {

/**
 * EqualizerService - Professional integration of AnalogModeledEQ with EngineContext
 *
 * Features:
 * - Real-time safe parameter updates via message queue
 * - Automatic UI synchronization with audio processor state
 * - Performance monitoring and statistics collection
 * - Preset management with A/B comparison
 * - Integration with DAW automation and MIDI control
 *
 * Architecture:
 * - UI thread sends parameter changes via RealtimeMessageQueue
 * - Audio thread processes messages during safe processing windows
 * - Statistics flow back to UI thread for real-time display
 * - Zero heap allocation in audio processing path
 */
class EqualizerService {
public:
    // Parameter update message types
    struct EQParameterUpdate {
        enum Type {
            BandEnabled, BandType, BandFrequency, BandGain, BandQ, BandDrive,
            BandSaturation, BandMix, BandSlope, BandSolo, BandBypass,
            InputGain, OutputGain, TransformerDrive, TubeWarmth,
            TapeSaturation, AnalogNoise, AnalogModel, BypassAll
        };

        Type type;
        int bandIndex; // -1 for global parameters
        float floatValue;
        int intValue;
        bool boolValue;

        // Factory methods for type safety
        static EQParameterUpdate bandEnabled(int band, bool enabled) {
            return {BandEnabled, band, 0.0f, 0, enabled};
        }

        static EQParameterUpdate bandFrequency(int band, float freq) {
            return {BandFrequency, band, freq, 0, false};
        }

        static EQParameterUpdate bandGain(int band, float gain) {
            return {BandGain, band, gain, 0, false};
        }

        static EQParameterUpdate analogModel(audio::AnalogModeledEQ::AnalogModel model) {
            return {AnalogModel, -1, 0.0f, static_cast<int>(model), false};
        }

        // ... Additional factory methods for all parameters
    };

    // Statistics update message (from audio to UI thread)
    struct EQStatisticsUpdate {
        std::array<float, audio::AnalogModeledEQ::NUM_BANDS> bandPeaks;
        float inputPeakL, inputPeakR;
        float outputPeakL, outputPeakR;
        float totalGainReduction;
        float analogHarmonics;
        float cpuUsage;
        int processedSamples;
        float totalHarmonicDistortion;
        // audio::AnalogModeledEQ::AnalysisData analysisData; // Too big for message queue
    };

    explicit EqualizerService(daw::core::EngineContext& context, daw::core::RTMemoryPool& pool);
    ~EqualizerService();

    // Lifecycle
    void initialize(const audio::AnalogModeledEQ::Config& config);
    void shutdown();

    // Audio thread processing (real-time safe)
    void processBlock(juce::AudioBuffer<float>& buffer);

    // UI thread interface
    std::unique_ptr<ui::AnalogEQEditor> createEditor();
    void sendParameterUpdate(const EQParameterUpdate& update);
    EQStatisticsUpdate getLatestStatistics();

    // Preset management
    void savePreset(const juce::String& name, int slot);
    bool loadPreset(const juce::String& name);
    juce::StringArray getAvailablePresets() const;

    // Automation interface
    void setAutomationParameter(int parameterId, float normalizedValue);
    float getAutomationParameter(int parameterId) const;
    juce::String getParameterName(int parameterId) const;
    juce::String getParameterText(int parameterId) const;
    float getParameterDefaultValue(int parameterId) const;

    // MIDI control interface
    void handleMidiControlChange(int controller, int value);
    void assignMidiController(int parameterId, int midiController);

    // Performance monitoring
    struct PerformanceMetrics {
        double averageProcessingTime = 0.0;
        double peakProcessingTime = 0.0;
        double cpuUsagePercent = 0.0;
        int droppedMessages = 0;
        int totalProcessedBlocks = 0;
    };

    PerformanceMetrics getPerformanceMetrics() const;
    void resetPerformanceMetrics();

private:
    // Core references
    daw::core::EngineContext& engineContext_;
    daw::core::RTMemoryPool& memoryPool_;
    std::unique_ptr<audio::AnalogModeledEQ> equalizer_;

    // Message queues (lock-free)
    daw::core::RealtimeMessageQueue<EQParameterUpdate> parameterQueue_;
    daw::core::RealtimeMessageQueue<EQStatisticsUpdate> statisticsQueue_;

    // State management
    std::atomic<bool> initialized_{false};
    std::atomic<bool> bypassed_{false};

    // Performance tracking
    mutable std::mutex performanceMetricsMutex_;
    PerformanceMetrics performanceMetrics_;
    juce::Time lastProcessingTime_;

    // Preset storage
    struct PresetData {
        juce::String name;
        audio::AnalogModeledEQ::Preset data;
        juce::Time createdTime;
    };
    std::vector<PresetData> storedPresets_;
    mutable std::mutex presetsMutex_;

    // Automation mapping
    struct AutomationParameter {
        juce::String name;
        float minValue, maxValue, defaultValue;
        std::function<void(float)> setter;
        std::function<float()> getter;
        juce::String unit;
    };
    std::array<AutomationParameter, 64> automationParameters_; // Max 64 parameters
    int numAutomationParameters_ = 0;

    // MIDI control mapping
    std::map<int, int> midiControllerMap_; // MIDI CC -> Parameter ID
    mutable std::mutex midiMutex_;

    // UI editor reference (weak)
    ui::AnalogEQEditor* currentEditor_ = nullptr;

    // Audio thread methods (real-time safe)
    void processParameterUpdates();
    void sendStatisticsUpdate();
    void updatePerformanceMetrics(double processingTime);

    // Parameter conversion helpers
    float normalizedToBandFrequency(float normalized) const;
    float bandFrequencyToNormalized(float frequency) const;
    float normalizedToBandGain(float normalized) const;
    float bandGainToNormalized(float gain) const;
    float normalizedToBandQ(float normalized) const;
    float bandQToNormalized(float q) const;

    // Initialization helpers
    void setupAutomationParameters();
    void setupDefaultPresets();

    // Thread safety
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EqualizerService)
};

/**
 * EqualizerProcessor - JUCE AudioProcessor wrapper for EqualizerService
 *
 * Enables the EQ to be used as a VST/AU plugin or in a JUCE host application
 */
class EqualizerProcessor : public juce::AudioProcessor {
public:
    EqualizerProcessor();
    ~EqualizerProcessor() override;

    // AudioProcessor implementation
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    // Editor
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    // State management
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Plugin info
    const juce::String getName() const override { return "Analog Modeled EQ"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Program management
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    // Parameters
    int getNumParameters() override;
    float getParameter(int index) override;
    void setParameter(int index, float newValue) override;
    const juce::String getParameterName(int index) override;
    const juce::String getParameterText(int index) override;

    friend class EqualizerProcessorEditor;

private:
    // Core service
    std::unique_ptr<daw::core::EngineContext> engineContext_;
    std::unique_ptr<daw::core::RTMemoryPool> memoryPool_;
    std::unique_ptr<EqualizerService> equalizerService_;

    // Plugin state
    int currentProgram_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EqualizerProcessor)
};

/**
 * Custom AudioProcessorEditor for the EQ
 */
class EqualizerProcessorEditor : public juce::AudioProcessorEditor {
public:
    explicit EqualizerProcessorEditor(EqualizerProcessor& processor);
    ~EqualizerProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    EqualizerProcessor& processor_;
    std::unique_ptr<ui::AnalogEQEditor> eqEditor_;
    std::unique_ptr<ui::AnalogEQLookAndFeel> lookAndFeel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EqualizerProcessorEditor)
};

} // namespace integration
} // namespace cppmusic
