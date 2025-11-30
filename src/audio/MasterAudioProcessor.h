#pragma once

#include "AdvancedSynthesizer_rt.hpp"
#include "AnalogModeledEQ.h"
#include "SpectralAnalysisEngine.h"
#include "src/core/EngineContext.h"
#include "src/core/RTMemoryPool.h"
#include "src/core/ServiceLocator.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <array>
#include <atomic>

namespace cppmusic {
namespace audio {

/**
 * Master Audio Processor - Integrates all advanced DSP components
 *
 * REAL-TIME SAFETY GUARANTEES:
 * - Zero heap allocations in processBlock()
 * - Lock-free inter-component communication
 * - Fixed-size processing chains and buffers
 * - RAII-based resource management
 *
 * Architecture:
 * - Input Stage: Gain, Filtering, Analysis
 * - Synthesis Stage: Advanced Synthesizer with multiple engines
 * - Processing Stage: EQ, Dynamics, Effects chain
 * - Analysis Stage: Real-time spectral analysis and feature extraction
 * - Output Stage: Limiting, Metering, Final gain
 *
 * Key Features:
 * - Ultra-low latency processing (< 10ms at 44.1kHz)
 * - Real-time spectrum analysis with ML feature extraction
 * - Advanced synthesis with quantum/neural/fractal modes
 * - Vintage analog modeling throughout the chain
 * - Comprehensive performance monitoring and optimization
 * - Automatic CPU load balancing and quality scaling
 */
class MasterAudioProcessor {
public:
    static constexpr int MAX_CHANNELS = 8;
    static constexpr int MAX_BLOCK_SIZE = 1024;
    static constexpr int NUM_INSERT_SLOTS = 8;
    static constexpr int NUM_SEND_SLOTS = 4;

    //==============================================================================
    // Processing Chain Configuration
    struct Config {
        double sampleRate = 44100.0;
        int maxBlockSize = MAX_BLOCK_SIZE;
        int numInputChannels = 2;
        int numOutputChannels = 2;
        int bufferLatency = 256; // Target latency in samples

        // Component enables
        bool enableSynthesizer = true;
        bool enableAnalogEQ = true;
        bool enableSpectralAnalysis = true;
        bool enableAdvancedEffects = false;
        bool enableMasterLimiter = true;

        // Performance settings
        float cpuThreshold = 0.8f;         // CPU threshold for quality scaling
        bool enableAutoOptimization = true; // Automatic performance optimization
        int maxConcurrentVoices = 32;      // Maximum polyphony
        bool enableOversampling = false;    // 2x oversampling for critical sections

        // Analysis settings
        bool enableRealtimeAnalysis = true;
        bool enableMLFeatureExtraction = false;
        int analysisLatency = 512;          // Analysis latency allowance

        // Safety limits
        float maxInputGain = 20.0f;         // dB
        float maxOutputGain = 10.0f;        // dB
        float emergencyLimiterThreshold = -0.1f; // dB
    };

    //==============================================================================
    // Processing Statistics
    struct ProcessingStats {
        // Performance metrics
        std::atomic<float> cpuUsage{0.0f};
        std::atomic<float> memoryUsage{0.0f};
        std::atomic<float> latency{0.0f};
        std::atomic<int> droppedFrames{0};
        std::atomic<int> overruns{0};

        // Audio metrics
        std::atomic<float> inputPeakL{0.0f};
        std::atomic<float> inputPeakR{0.0f};
        std::atomic<float> outputPeakL{0.0f};
        std::atomic<float> outputPeakR{0.0f};
        std::atomic<float> inputRMS{0.0f};
        std::atomic<float> outputRMS{0.0f};

        // Component metrics
        std::atomic<int> activeSynthVoices{0};
        std::atomic<float> synthCpuUsage{0.0f};
        std::atomic<float> eqCpuUsage{0.0f};
        std::atomic<float> analysisCpuUsage{0.0f};

        // Quality metrics
        std::atomic<float> totalHarmonicDistortion{0.0f};
        std::atomic<float> dynamicRange{0.0f};
        std::atomic<float> stereoWidth{0.0f};
        std::atomic<float> phaseCoherence{0.0f};
    };

    //==============================================================================
    // Insert Effects Chain
    struct InsertSlot {
        enum class Type {
            None = 0,
            Compressor,
            Gate,
            Expander,
            Distortion,
            Chorus,
            Flanger,
            Phaser,
            Delay,
            Reverb
        };

        Type type = Type::None;
        std::atomic<bool> enabled{false};
        std::atomic<bool> bypassed{false};
        std::atomic<float> mix{1.0f};
        std::atomic<float> inputGain{0.0f};
        std::atomic<float> outputGain{0.0f};

        // Generic parameter controls
        std::array<std::atomic<float>, 8> parameters;

        // Processing state
        struct ProcessingState {
            juce::AudioBuffer<float> buffer;
            float peakInput = 0.0f;
            float peakOutput = 0.0f;
            float cpuUsage = 0.0f;
        } state;
    };

    //==============================================================================
    // Send Effects
    struct SendSlot {
        std::atomic<bool> enabled{false};
        std::atomic<float> sendLevel{0.0f};     // dB
        std::atomic<float> returnLevel{0.0f};   // dB
        std::atomic<float> preFaderSend{0.0f};  // 0.0-1.0
        std::atomic<bool> mute{false};

        InsertSlot::Type effectType = InsertSlot::Type::Reverb;
        juce::AudioBuffer<float> sendBuffer;
        juce::AudioBuffer<float> returnBuffer;
    };

    //==============================================================================
    // Constructor & Destructor
    explicit MasterAudioProcessor(core::EngineContext& context,
                                core::RTMemoryPool& memoryPool,
                                core::ServiceLocator& serviceLocator);
    ~MasterAudioProcessor();

    // Non-copyable, non-movable
    MasterAudioProcessor(const MasterAudioProcessor&) = delete;
    MasterAudioProcessor& operator=(const MasterAudioProcessor&) = delete;
    MasterAudioProcessor(MasterAudioProcessor&&) = delete;
    MasterAudioProcessor& operator=(MasterAudioProcessor&&) = delete;

    //==============================================================================
    // Lifecycle Methods
    void prepare(const Config& config);
    void reset();
    void processBlock(juce::AudioBuffer<float>& inputBuffer,
                     juce::AudioBuffer<float>& outputBuffer,
                     const juce::MidiBuffer& midiMessages);

    //==============================================================================
    // Component Access
    AdvancedSynthesizer& getSynthesizer() { return *synthesizer_; }
    const AdvancedSynthesizer& getSynthesizer() const { return *synthesizer_; }

    AnalogModeledEQ& getEQ() { return *analogEQ_; }
    const AnalogModeledEQ& getEQ() const { return *analogEQ_; }

    SpectralAnalysisEngine& getAnalysis() { return *spectralAnalysis_; }
    const SpectralAnalysisEngine& getAnalysis() const { return *spectralAnalysis_; }

    //==============================================================================
    // Insert Effects Management
    void setInsertEffect(int slotIndex, InsertSlot::Type type);
    void enableInsertSlot(int slotIndex, bool enabled);
    void bypassInsertSlot(int slotIndex, bool bypassed);
    void setInsertParameter(int slotIndex, int paramIndex, float value);
    void setInsertMix(int slotIndex, float mix);

    //==============================================================================
    // Send Effects Management
    void enableSendSlot(int slotIndex, bool enabled);
    void setSendLevel(int slotIndex, float levelDB);
    void setReturnLevel(int slotIndex, float levelDB);
    void setSendEffect(int slotIndex, InsertSlot::Type type);

    //==============================================================================
    // Master Controls
    void setInputGain(float gainDB);
    void setOutputGain(float gainDB);
    void setMasterMute(bool muted);
    void setMasterSolo(bool soloed);
    void setMasterBypass(bool bypassed);

    //==============================================================================
    // Performance Control
    void setCPUThreshold(float threshold);
    void setAutoOptimizationEnabled(bool enabled);
    void setMaxPolyphony(int maxVoices);
    void setOversamplingEnabled(bool enabled);
    void triggerEmergencyOptimization();

    //==============================================================================
    // Analysis Control
    void setRealtimeAnalysisEnabled(bool enabled);
    void setMLFeatureExtractionEnabled(bool enabled);
    void setAnalysisLatency(int samples);

    //==============================================================================
    // Preset Management
    struct Preset {
        std::string name;
        std::string description;
        std::string category;

        // Component presets
        AdvancedSynthesizer::Config synthConfig;
        AnalogModeledEQ::Preset eqPreset;
        SpectralAnalysisEngine::Config analysisConfig;

        // Insert effects
        std::array<InsertSlot, NUM_INSERT_SLOTS> insertSlots;
        std::array<SendSlot, NUM_SEND_SLOTS> sendSlots;

        // Master settings
        float inputGain = 0.0f;
        float outputGain = 0.0f;
        bool masterMute = false;
        bool masterSolo = false;
        bool masterBypass = false;
    };

    void loadPreset(const Preset& preset);
    Preset savePreset(const std::string& name) const;
    void loadFactoryPreset(const std::string& presetName);

    //==============================================================================
    // Real-time Monitoring
    const ProcessingStats& getStatistics() const { return stats_; }
    void resetStatistics();

    // Real-time safe getters for UI
    SpectralAnalysisEngine::SpectralFeatures getLatestSpectralFeatures() const;
    SpectralAnalysisEngine::MLFeatures getLatestMLFeatures() const;
    float getCurrentCPUUsage() const { return stats_.cpuUsage.load(); }
    float getCurrentLatency() const { return stats_.latency.load(); }
    int getActiveVoiceCount() const { return stats_.activeSynthVoices.load(); }

    //==============================================================================
    // Emergency Protection
    struct EmergencyProtection {
        std::atomic<bool> limiterActive{false};
        std::atomic<bool> thermalProtection{false};
        std::atomic<bool> overloadDetected{false};
        std::atomic<float> gainReduction{0.0f};

        float thermalThreshold = 0.95f;    // CPU threshold for thermal protection
        float overloadThreshold = 0.99f;   // Input overload threshold
        float limiterThreshold = -0.1f;    // dB
        float limiterRelease = 50.0f;      // ms

        // Emergency state
        int overloadCount = 0;
        int thermalCount = 0;
        juce::Time lastOverload;
        juce::Time lastThermalEvent;
    };

    const EmergencyProtection& getEmergencyProtection() const { return emergencyProtection_; }

private:
    //==============================================================================
    // Dependencies
    core::EngineContext& engineContext_;
    core::RTMemoryPool& memoryPool_;
    core::ServiceLocator& serviceLocator_;

    //==============================================================================
    // Configuration
    Config config_;
    double sampleRate_ = 44100.0;
    int maxBlockSize_ = MAX_BLOCK_SIZE;
    int numInputChannels_ = 2;
    int numOutputChannels_ = 2;

    //==============================================================================
    // Core DSP Components
    std::unique_ptr<AdvancedSynthesizer> synthesizer_;
    std::unique_ptr<AnalogModeledEQ> analogEQ_;
    std::unique_ptr<SpectralAnalysisEngine> spectralAnalysis_;

    //==============================================================================
    // Effects Processing
    std::array<InsertSlot, NUM_INSERT_SLOTS> insertSlots_;
    std::array<SendSlot, NUM_SEND_SLOTS> sendSlots_;

    //==============================================================================
    // Processing Buffers (Pre-allocated for real-time safety)
    juce::AudioBuffer<float> synthBuffer_;
    juce::AudioBuffer<float> eqBuffer_;
    juce::AudioBuffer<float> effectsBuffer_;
    juce::AudioBuffer<float> analysisBuffer_;
    juce::AudioBuffer<float> masterBuffer_;

    // Temporary buffers for intermediate processing
    juce::AudioBuffer<float> tempBuffer1_;
    juce::AudioBuffer<float> tempBuffer2_;

    //==============================================================================
    // Master Controls
    std::atomic<float> inputGain_{0.0f};    // dB
    std::atomic<float> outputGain_{0.0f};   // dB
    std::atomic<bool> masterMute_{false};
    std::atomic<bool> masterSolo_{false};
    std::atomic<bool> masterBypass_{false};

    //==============================================================================
    // Performance Management
    std::atomic<float> cpuThreshold_{0.8f};
    std::atomic<bool> autoOptimizationEnabled_{true};
    std::atomic<int> maxPolyphony_{32};
    std::atomic<bool> oversamplingEnabled_{false};

    // Quality scaling state
    int qualityLevel_ = 100; // 0-100%
    int lastPolyphonyReduction_ = 0;
    juce::Time lastOptimization_;

    //==============================================================================
    // Analysis State
    std::atomic<bool> realtimeAnalysisEnabled_{true};
    std::atomic<bool> mlFeatureExtractionEnabled_{false};
    std::atomic<int> analysisLatency_{512};

    //==============================================================================
    // Statistics & Monitoring
    ProcessingStats stats_;
    EmergencyProtection emergencyProtection_;

    // Performance timing
    juce::Time processingStartTime_;
    double averageProcessingTime_ = 0.0;
    int processingTimeCount_ = 0;

    //==============================================================================
    // Master Limiter/Safety
    struct MasterLimiter {
        float threshold = -0.1f;      // dB
        float release = 50.0f;        // ms
        float lookahead = 5.0f;       // ms

        // State
        float gainReduction = 0.0f;
        float envelope = 0.0f;
        juce::AudioBuffer<float> lookaheadBuffer;
        int lookaheadSamples = 0;
        int writeIndex = 0;

        // Coefficients
        float attackCoeff = 0.0f;
        float releaseCoeff = 0.0f;
    } masterLimiter_;

    //==============================================================================
    // Processing Chain Methods
    void processInputStage(juce::AudioBuffer<float>& buffer);
    void processSynthesisStage(juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midiMessages);
    void processEQStage(juce::AudioBuffer<float>& buffer);
    void processInsertEffects(juce::AudioBuffer<float>& buffer);
    void processSendEffects(juce::AudioBuffer<float>& buffer);
    void processAnalysisStage(const juce::AudioBuffer<float>& buffer);
    void processOutputStage(juce::AudioBuffer<float>& buffer);

    //==============================================================================
    // Effects Processing
    void processInsertSlot(InsertSlot& slot, juce::AudioBuffer<float>& buffer);
    void processSendSlot(SendSlot& slot, const juce::AudioBuffer<float>& inputBuffer,
                        juce::AudioBuffer<float>& outputBuffer);

    // Individual effect processors
    void processCompressor(InsertSlot& slot, juce::AudioBuffer<float>& buffer);
    void processGate(InsertSlot& slot, juce::AudioBuffer<float>& buffer);
    void processDistortion(InsertSlot& slot, juce::AudioBuffer<float>& buffer);
    void processChorus(InsertSlot& slot, juce::AudioBuffer<float>& buffer);
    void processDelay(InsertSlot& slot, juce::AudioBuffer<float>& buffer);
    void processReverb(InsertSlot& slot, juce::AudioBuffer<float>& buffer);

    //==============================================================================
    // Master Processing
    void processMasterLimiter(juce::AudioBuffer<float>& buffer);
    void updateMasterLimiterCoefficients();

    //==============================================================================
    // Performance Management
    void updatePerformanceStats(double processingTime);
    void performAutoOptimization();
    void scaleQualityForPerformance();
    void handleEmergencyConditions();

    //==============================================================================
    // Safety & Protection
    void checkInputOverload(const juce::AudioBuffer<float>& buffer);
    void checkThermalConditions();
    void applyEmergencyLimiting(juce::AudioBuffer<float>& buffer);

    //==============================================================================
    // Utility Methods
    float dbToLinear(float db) const;
    float linearToDb(float linear) const;
    void calculateRMSAndPeak(const juce::AudioBuffer<float>& buffer,
                            float& rms, float& peak) const;
    void clearBuffer(juce::AudioBuffer<float>& buffer);
    void copyBuffer(const juce::AudioBuffer<float>& source,
                   juce::AudioBuffer<float>& destination);
    void mixBuffers(const juce::AudioBuffer<float>& source,
                   juce::AudioBuffer<float>& destination, float gain);

    //==============================================================================
    // Component Initialization
    void initializeSynthesizer();
    void initializeAnalogEQ();
    void initializeSpectralAnalysis();
    void initializeInsertEffects();
    void initializeSendEffects();
    void initializeMasterLimiter();

    //==============================================================================
    // Preset System
    void initializeFactoryPresets();
    std::map<std::string, Preset> factoryPresets_;
};

} // namespace audio
} // namespace cppmusic
