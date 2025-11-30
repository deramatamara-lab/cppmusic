#pragma once

#include "src/core/EngineContext.h"
#include "src/core/RTMemoryPool.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <array>
#include <complex>

namespace cppmusic {
namespace audio {

/**
 * Ultra-Advanced Real-Time Spectral Analysis Engine
 *
 * REAL-TIME SAFETY GUARANTEES:
 * - Zero heap allocations in processBlock()
 * - Lock-free communication via atomic ring buffers
 * - Fixed-size analysis windows and feature arrays
 * - Pre-allocated FFT buffers and filter banks
 *
 * Key Features:
 * - Real-time FFT analysis with configurable window sizes
 * - Advanced spectral features (MFCC, Chroma, Spectral Contrast, Tonnetz)
 * - Pitch detection with confidence estimation
 * - Onset detection and tempo estimation
 * - Harmonic/percussive source separation
 * - Spectral peak tracking and fundamental frequency estimation
 * - Audio-rate spectral processing capabilities
 * - Machine learning feature extraction for AI models
 */
class SpectralAnalysisEngine {
public:
    static constexpr int MAX_FFT_SIZE = 8192;
    static constexpr int MAX_BLOCK_SIZE = 1024;
    static constexpr int NUM_MEL_FILTERS = 40;
    static constexpr int NUM_MFCC_COEFFS = 13;
    static constexpr int NUM_CHROMA_BINS = 12;
    static constexpr int NUM_SPECTRAL_CONTRAST_BANDS = 7;
    static constexpr int MAX_SPECTRAL_PEAKS = 32;
    static constexpr int ONSET_HISTORY_SIZE = 64;
    static constexpr int TEMPO_HISTORY_SIZE = 32;

    //==============================================================================
    // Analysis Configuration
    struct Config {
        int fftSize = 2048;
        int hopSize = 512;
        int windowType = 0; // 0=Hann, 1=Hamming, 2=Blackman, 3=Kaiser
        double sampleRate = 44100.0;
        int maxBlockSize = MAX_BLOCK_SIZE;

        // Feature extraction flags
        bool enableMFCC = true;
        bool enableChroma = true;
        bool enableSpectralContrast = true;
        bool enableTonnetz = true;
        bool enablePitchDetection = true;
        bool enableOnsetDetection = true;
        bool enableTempoEstimation = true;
        bool enableHarmonicPercussiveSeparation = true;

        // Analysis parameters
        float pitchMinFreq = 80.0f;
        float pitchMaxFreq = 1000.0f;
        float onsetThreshold = 0.3f;
        float tempoMinBPM = 60.0f;
        float tempoMaxBPM = 200.0f;
        int melMinFreq = 0;
        int melMaxFreq = 8000;

        // Real-time constraints
        int analysisLatency = 0; // Samples of analysis latency
        bool enableZeroLatencyMode = false;
        int maxConcurrentAnalyses = 4;
    };

    //==============================================================================
    // Spectral Features Structure
    struct SpectralFeatures {
        // Basic spectral statistics
        float spectralCentroid = 0.0f;
        float spectralSpread = 0.0f;
        float spectralSkewness = 0.0f;
        float spectralKurtosis = 0.0f;
        float spectralFlatness = 0.0f;
        float spectralRolloff85 = 0.0f;
        float spectralRolloff95 = 0.0f;
        float spectralSlope = 0.0f;
        float spectralFlux = 0.0f;
        float spectralComplexity = 0.0f;

        // Perceptual features
        std::array<float, NUM_MFCC_COEFFS> mfcc{};
        std::array<float, NUM_CHROMA_BINS> chroma{};
        std::array<float, NUM_SPECTRAL_CONTRAST_BANDS> spectralContrast{};
        std::array<float, 6> tonnetz{}; // Tonal centroid features

        // Temporal features
        float zeroCrossingRate = 0.0f;
        float rmsEnergy = 0.0f;
        float totalEnergy = 0.0f;
        float shortTimeEnergy = 0.0f;

        // Pitch and harmony
        float fundamentalFrequency = 0.0f;
        float pitchConfidence = 0.0f;
        float inharmonicity = 0.0f;
        float harmonicToNoiseRatio = 0.0f;
        float pitchSalience = 0.0f;

        // Onset and rhythm
        float onsetStrength = 0.0f;
        float onsetConfidence = 0.0f;
        float tempoEstimate = 0.0f;
        float beatConfidence = 0.0f;
        float rhythmicRegularity = 0.0f;

        // Dynamic and loudness
        float dynamicRange = 0.0f;
        float perceivedLoudness = 0.0f;
        float loudnessRange = 0.0f;
        float crestFactor = 0.0f;

        // Spectral peaks
        struct SpectralPeak {
            float frequency = 0.0f;
            float magnitude = 0.0f;
            float phase = 0.0f;
            float bandwidth = 0.0f;
        };
        std::array<SpectralPeak, MAX_SPECTRAL_PEAKS> peaks;
        int numPeaks = 0;

        // Analysis metadata
        double timestamp = 0.0;
        int frameNumber = 0;
        float confidence = 0.0f;
        bool isValidFrame = false;
    };

    //==============================================================================
    // Harmonic/Percussive Separation
    struct HPSeparation {
        juce::AudioBuffer<float> harmonicComponent;
        juce::AudioBuffer<float> percussiveComponent;
        float harmonicEnergy = 0.0f;
        float percussiveEnergy = 0.0f;
        float harmonicPercussiveRatio = 0.0f;
    };

    //==============================================================================
    // Constructor & Destructor
    explicit SpectralAnalysisEngine(core::EngineContext& context,
                                  core::RTMemoryPool& memoryPool);
    ~SpectralAnalysisEngine();

    // Non-copyable, non-movable
    SpectralAnalysisEngine(const SpectralAnalysisEngine&) = delete;
    SpectralAnalysisEngine& operator=(const SpectralAnalysisEngine&) = delete;
    SpectralAnalysisEngine(SpectralAnalysisEngine&&) = delete;
    SpectralAnalysisEngine& operator=(SpectralAnalysisEngine&&) = delete;

    //==============================================================================
    // Lifecycle Methods
    void prepare(const Config& config);
    void reset();
    void processBlock(const juce::AudioBuffer<float>& inputBuffer);

    //==============================================================================
    // Feature Extraction (Thread-Safe Access)
    SpectralFeatures getLatestFeatures() const;
    SpectralFeatures getFeaturesAtTime(double timestamp) const;
    std::vector<SpectralFeatures> getFeatureHistory(int numFrames) const;

    //==============================================================================
    // Specialized Analysis
    HPSeparation getHarmonicPercussiveSeparation() const;
    std::array<float, MAX_FFT_SIZE/2> getMagnitudeSpectrum() const;
    std::array<float, MAX_FFT_SIZE/2> getPhaseSpectrum() const;
    std::array<float, MAX_FFT_SIZE/2> getPowerSpectrum() const;

    //==============================================================================
    // Real-time Pitch Tracking
    struct PitchTracker {
        float currentPitch = 0.0f;
        float pitchConfidence = 0.0f;
        float pitchStability = 0.0f;
        std::array<float, 16> pitchHistory{};
        int historyIndex = 0;

        // Autocorrelation-based tracking
        std::array<float, 2048> autocorrelationBuffer{};

        // YIN algorithm state
        std::array<float, 1024> yinBuffer{};
        float yinThreshold = 0.15f;
    };

    const PitchTracker& getPitchTracker() const { return pitchTracker_; }

    //==============================================================================
    // Real-time Onset Detection
    struct OnsetDetector {
        float currentOnsetStrength = 0.0f;
        float onsetThreshold = 0.3f;
        bool onsetDetected = false;
        double lastOnsetTime = 0.0;

        // Onset detection functions
        std::array<float, ONSET_HISTORY_SIZE> spectralFluxHistory{};
        std::array<float, ONSET_HISTORY_SIZE> energyHistory{};
        std::array<float, ONSET_HISTORY_SIZE> complexDomainHistory{};
        int historyIndex = 0;

        // Peak picking
        float peakThreshold = 0.6f;
        int peakWaitTime = 10; // frames
        int framesSinceLastPeak = 0;
    };

    const OnsetDetector& getOnsetDetector() const { return onsetDetector_; }

    //==============================================================================
    // Real-time Tempo Estimation
    struct TempoEstimator {
        float currentTempo = 120.0f;
        float tempoConfidence = 0.0f;
        float tempoStability = 0.0f;

        // Beat tracking
        std::array<float, TEMPO_HISTORY_SIZE> tempoHistory{};
        std::array<double, 64> beatTimes{};
        int beatIndex = 0;

        // Autocorrelation of onset function
        std::array<float, 512> onsetAutocorrelation{};

        // Comb filter bank for tempo detection
        struct CombFilter {
            float delay = 0.0f;
            float feedback = 0.7f;
            float output = 0.0f;
            std::array<float, 1024> delayLine{};
            int writeIndex = 0;
        };
        std::array<CombFilter, 16> combFilters;
    };

    const TempoEstimator& getTempoEstimator() const { return tempoEstimator_; }

    //==============================================================================
    // Machine Learning Feature Vectors
    struct MLFeatures {
        // Compact feature vector for ML models
        std::array<float, 64> featureVector{};

        // Specialized feature sets
        std::array<float, 32> timbreFeatures{};      // For instrument classification
        std::array<float, 24> rhythmFeatures{};      // For rhythm analysis
        std::array<float, 16> harmonicFeatures{};    // For chord recognition
        std::array<float, 12> emotionalFeatures{};   // For mood detection

        float confidence = 0.0f;
        bool isValid = false;
    };

    MLFeatures getMLFeatures() const;

    //==============================================================================
    // Configuration & Control
    void setAnalysisEnabled(bool enabled);
    void setPitchTrackingEnabled(bool enabled);
    void setOnsetDetectionEnabled(bool enabled);
    void setTempoEstimationEnabled(bool enabled);
    void setHPSeparationEnabled(bool enabled);

    void setOnsetThreshold(float threshold);
    void setPitchRange(float minFreq, float maxFreq);
    void setTempoRange(float minBPM, float maxBPM);

    //==============================================================================
    // Statistics & Monitoring
    struct Statistics {
        std::atomic<int> framesProcessed{0};
        std::atomic<int> validFrames{0};
        std::atomic<int> onsetCount{0};
        std::atomic<float> averageConfidence{0.0f};
        std::atomic<float> processingLoad{0.0f};
        std::atomic<float> latency{0.0f};
        std::atomic<double> lastAnalysisTime{0.0};
    };

    const Statistics& getStatistics() const { return statistics_; }
    void resetStatistics();

private:
    //==============================================================================
    // Dependencies
    core::EngineContext& engineContext_;
    core::RTMemoryPool& memoryPool_;

    //==============================================================================
    // Configuration
    Config config_;
    double sampleRate_ = 44100.0;
    int fftSize_ = 2048;
    int hopSize_ = 512;
    bool analysisEnabled_ = true;

    //==============================================================================
    // FFT Processing
    std::unique_ptr<juce::dsp::FFT> fft_;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window_;

    // FFT Buffers (Pre-allocated)
    std::array<float, MAX_FFT_SIZE> fftInputBuffer_{};
    std::array<float, MAX_FFT_SIZE * 2> fftOutputBuffer_{}; // Complex data
    std::array<float, MAX_FFT_SIZE> windowBuffer_{};

    // Spectrum buffers
    std::array<float, MAX_FFT_SIZE/2> magnitudeSpectrum_{};
    std::array<float, MAX_FFT_SIZE/2> phaseSpectrum_{};
    std::array<float, MAX_FFT_SIZE/2> powerSpectrum_{};
    std::array<float, MAX_FFT_SIZE/2> previousMagnitudeSpectrum_{};

    //==============================================================================
    // Input Buffer Management
    juce::AudioBuffer<float> inputRingBuffer_;
    std::atomic<int> ringBufferWritePos_{0};
    std::atomic<int> ringBufferReadPos_{0};
    int samplesUntilNextAnalysis_ = 0;

    //==============================================================================
    // Feature Extraction State
    SpectralFeatures currentFeatures_;
    std::array<SpectralFeatures, 32> featureHistory_; // Ring buffer of recent features
    std::atomic<int> featureHistoryIndex_{0};

    //==============================================================================
    // Mel Filter Bank (Pre-computed)
    struct MelFilterBank {
        std::array<std::array<float, MAX_FFT_SIZE/2>, NUM_MEL_FILTERS> filters;
        std::array<float, NUM_MEL_FILTERS + 2> centerFreqs;
        bool initialized = false;
    } melFilterBank_;

    //==============================================================================
    // Chroma Filter Bank (Pre-computed)
    struct ChromaFilterBank {
        std::array<std::array<float, MAX_FFT_SIZE/2>, NUM_CHROMA_BINS> filters;
        bool initialized = false;
    } chromaFilterBank_;

    //==============================================================================
    // Analysis Components
    PitchTracker pitchTracker_;
    OnsetDetector onsetDetector_;
    TempoEstimator tempoEstimator_;

    //==============================================================================
    // Harmonic/Percussive Separation
    HPSeparation hpSeparation_;
    bool hpSeparationEnabled_ = false;

    //==============================================================================
    // Performance Monitoring
    Statistics statistics_;
    juce::Time analysisStartTime_;

    //==============================================================================
    // Core Analysis Methods (Real-time safe)
    void performFFTAnalysis();
    void extractSpectralFeatures();
    void updatePitchTracking();
    void updateOnsetDetection();
    void updateTempoEstimation();
    void performHPSeparation();

    //==============================================================================
    // Feature Extraction Methods
    void calculateBasicSpectralFeatures();
    void calculateMFCC();
    void calculateChroma();
    void calculateSpectralContrast();
    void calculateTonnetz();
    void calculateTemporalFeatures();
    void calculatePitchFeatures();
    void calculateOnsetFeatures();

    //==============================================================================
    // Pitch Detection Algorithms
    float detectPitchYIN(const float* samples, int numSamples);
    float detectPitchAutocorrelation(const float* samples, int numSamples);
    float detectPitchHPS(const std::array<float, MAX_FFT_SIZE/2>& spectrum);
    float calculatePitchConfidence(float pitch, const float* samples, int numSamples);

    //==============================================================================
    // Onset Detection Algorithms
    float calculateSpectralFlux();
    float calculateEnergyOnset();
    float calculateComplexDomain();
    bool peakPick(float onsetStrength);

    //==============================================================================
    // Tempo Estimation Algorithms
    float estimateTempoFromOnsets();
    float estimateTempoFromAutocorrelation();
    void updateBeatTracker(double currentTime);

    //==============================================================================
    // Harmonic/Percussive Separation
    void separateHarmonicPercussive(const std::array<float, MAX_FFT_SIZE/2>& spectrum);
    void medianFilter2D(float* data, int width, int height, int kernelSize);

    //==============================================================================
    // Filter Bank Initialization
    void initializeMelFilterBank();
    void initializeChromaFilterBank();
    float melScale(float frequency) const;
    float inverseMelScale(float mel) const;

    //==============================================================================
    // Utility Functions
    void applyWindow(float* samples, int numSamples);
    void calculateMagnitudeSpectrum();
    void calculatePhaseSpectrum();
    float interpolateSpectralPeak(int peakBin);
    std::vector<int> findSpectralPeaks(float threshold);

    float autocorrelate(const float* signal, int length, int lag);
    void performDCT(const float* input, float* output, int length);

    //==============================================================================
    // Ring Buffer Management
    void writeToRingBuffer(const juce::AudioBuffer<float>& buffer);
    bool isEnoughDataAvailable() const;
    void fillAnalysisBuffer();

    //==============================================================================
    // Performance Tracking
    void updatePerformanceMetrics(double processingTime);

    //==============================================================================
    // Machine Learning Features
    void calculateMLFeatures(MLFeatures& features);
    void normalizeFeatureVector(std::array<float, 64>& features);
};

} // namespace audio
} // namespace cppmusic
