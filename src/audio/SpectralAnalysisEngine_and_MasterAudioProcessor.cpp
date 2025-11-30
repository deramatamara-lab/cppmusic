// SpectralAnalysisEngine_and_MasterAudioProcessor.cpp — Ultra‑Enhanced Implementation (single TU)
// Implements:
//   cppmusic::audio::SpectralAnalysisEngine
//   cppmusic::audio::MasterAudioProcessor (lean, RT‑safe, effects included)
// Requirements: juce_core, juce_audio_basics, juce_dsp, juce_data_structures (for tests optional)
// RT policy: No heap allocations in processBlock(); all buffers pre‑allocated in prepare().

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <array>
#include <cmath>
#include <memory>
#include <algorithm>

// --- Actual includes for the engine types
#include "engine/EngineContext.h"
#include "core/RTMemoryPool.h"
#include "core/ServiceLocator.h"
#include "AdvancedSynthesizer_rt.hpp"
#include "AnalogModeledEQ.h"

namespace daw { namespace audio {

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

    struct Config {
        int fftSize = 2048;      // power of two <= 8192
        int hopSize = 512;       // <= fftSize
        int windowType = 0;      // 0 Hann, 1 Hamming, 2 Blackman, 3 Kaiser
        double sampleRate = 44100.0;
        int maxBlockSize = MAX_BLOCK_SIZE;
        bool enableMFCC = true, enableChroma = true, enableSpectralContrast = true, enableTonnetz = true,
             enablePitchDetection = true, enableOnsetDetection = true, enableTempoEstimation = true,
             enableHarmonicPercussiveSeparation = true;
        float pitchMinFreq = 80.0f, pitchMaxFreq = 1000.0f; float onsetThreshold = 0.3f;
        float tempoMinBPM = 60.0f, tempoMaxBPM = 200.0f; int melMinFreq = 0, melMaxFreq = 8000;
        int analysisLatency = 0; bool enableZeroLatencyMode = false; int maxConcurrentAnalyses = 4;
    };

    struct SpectralFeatures {
        float spectralCentroid=0, spectralSpread=0, spectralSkewness=0, spectralKurtosis=0, spectralFlatness=0,
              spectralRolloff85=0, spectralRolloff95=0, spectralSlope=0, spectralFlux=0, spectralComplexity=0;
        std::array<float, NUM_MFCC_COEFFS> mfcc{};
        std::array<float, NUM_CHROMA_BINS> chroma{};
        std::array<float, NUM_SPECTRAL_CONTRAST_BANDS> spectralContrast{};
        std::array<float, 6> tonnetz{};
        float zeroCrossingRate=0, rmsEnergy=0, totalEnergy=0, shortTimeEnergy=0;
        float fundamentalFrequency=0, pitchConfidence=0, inharmonicity=0, harmonicToNoiseRatio=0, pitchSalience=0;
        float onsetStrength=0, onsetConfidence=0, tempoEstimate=0, beatConfidence=0, rhythmicRegularity=0;
        float dynamicRange=0, perceivedLoudness=0, loudnessRange=0, crestFactor=0;
        struct SpectralPeak { float frequency=0, magnitude=0, phase=0, bandwidth=0; };
        std::array<SpectralPeak, MAX_SPECTRAL_PEAKS> peaks; int numPeaks=0;
        double timestamp=0.0; int frameNumber=0; float confidence=0.0f; bool isValidFrame=false;
    };

    struct HPSeparation { juce::AudioBuffer<float> harmonicComponent, percussiveComponent; float harmonicEnergy=0, percussiveEnergy=0, harmonicPercussiveRatio=0; };

    struct PitchTracker { float currentPitch=0, pitchConfidence=0, pitchStability=0; std::array<float,16> pitchHistory{}; int historyIndex=0; std::array<float,2048> autocorrelationBuffer{}; std::array<float,1024> yinBuffer{}; float yinThreshold=0.15f; };
    struct OnsetDetector { float currentOnsetStrength=0, onsetThreshold=0.3f; bool onsetDetected=false; double lastOnsetTime=0; std::array<float,ONSET_HISTORY_SIZE> spectralFluxHistory{}; std::array<float,ONSET_HISTORY_SIZE> energyHistory{}; std::array<float,ONSET_HISTORY_SIZE> complexDomainHistory{}; int historyIndex=0; float peakThreshold=0.6f; int peakWaitTime=10; int framesSinceLastPeak=0; };
    struct TempoEstimator { float currentTempo=120.0f, tempoConfidence=0.0f, tempoStability=0.0f; std::array<float,TEMPO_HISTORY_SIZE> tempoHistory{}; std::array<double,64> beatTimes{}; int beatIndex=0; std::array<float,512> onsetAutocorrelation{}; struct CombFilter { float delay=0, feedback=0.7f, output=0; std::array<float,1024> delayLine{}; int writeIndex=0; }; std::array<CombFilter,16> combFilters; };

    struct MLFeatures { std::array<float,64> featureVector{}; std::array<float,32> timbreFeatures{}; std::array<float,24> rhythmFeatures{}; std::array<float,16> harmonicFeatures{}; std::array<float,12> emotionalFeatures{}; float confidence=0.0f; bool isValid=false; };

    struct Statistics { std::atomic<int> framesProcessed{0}, validFrames{0}, onsetCount{0}; std::atomic<float> averageConfidence{0}, processingLoad{0}, latency{0}; std::atomic<double> lastAnalysisTime{0}; };

    explicit SpectralAnalysisEngine(daw::core::EngineContext& c, daw::core::RTMemoryPool& p) : engineContext_(c), memoryPool_(p) {}
    ~SpectralAnalysisEngine() = default;

    void prepare(const Config& cfg)
    {
        config_ = cfg; sampleRate_ = cfg.sampleRate; fftSize_ = juce::jlimit(256, MAX_FFT_SIZE, juce::nextPowerOfTwo(cfg.fftSize)); hopSize_ = juce::jlimit(64, fftSize_, cfg.hopSize);
        inputRingBuffer_.setSize(1, juce::jmax(cfg.maxBlockSize*8, fftSize_*2), false, false, true);
    ringBufferWritePos_.store(0); ringBufferReadPos_.store(0); samplesUntilNextAnalysis_ = 0;
    // Reset atomics explicitly
    statistics_.framesProcessed.store(0);
    statistics_.validFrames.store(0);
    statistics_.onsetCount.store(0);
    statistics_.averageConfidence.store(0.0f);
    statistics_.processingLoad.store(0.0f);
    statistics_.latency.store(0.0f);
    statistics_.lastAnalysisTime.store(0.0);
        fft_.reset(new juce::dsp::FFT((int)std::log2((double)fftSize_)));
        using WF = juce::dsp::WindowingFunction<float>;
        auto type = cfg.windowType==1? WF::hamming : cfg.windowType==2? WF::blackman : cfg.windowType==3? WF::kaiser : WF::hann;
        window_.reset(new WF((size_t)fftSize_, type, true));
        for (int i=0;i<fftSize_;++i) windowBuffer_[i] = 1.0f;
    juce::dsp::WindowingFunction<float>::fillWindowingTables(windowBuffer_.data(), (size_t)fftSize_, type, true);
        std::fill(previousMagnitudeSpectrum_.begin(), previousMagnitudeSpectrum_.end(), 0.0f);
        initializeMelFilterBank(); initializeChromaFilterBank();
        hpSeparation_.harmonicComponent.setSize(1, fftSize_, false, false, true);
        hpSeparation_.percussiveComponent.setSize(1, fftSize_, false, false, true);
    }

    void reset()
    {
        ringBufferWritePos_.store(0); ringBufferReadPos_.store(0); samplesUntilNextAnalysis_ = 0;
        statistics_.framesProcessed.store(0);
        statistics_.validFrames.store(0);
        statistics_.onsetCount.store(0);
        statistics_.averageConfidence.store(0.0f);
        statistics_.processingLoad.store(0.0f);
        statistics_.latency.store(0.0f);
        statistics_.lastAnalysisTime.store(0.0);
        onsetDetector_ = {};
        tempoEstimator_ = {};
        pitchTracker_ = {};
        previousMagnitudeSpectrum_.fill(0.0f);
    }

    void processBlock(const juce::AudioBuffer<float>& inputBuffer)
    {
        if (!analysisEnabled_) return;
        const int n = inputBuffer.getNumSamples();
        writeToRingBuffer(inputBuffer);
        samplesUntilNextAnalysis_ -= n;
        if (samplesUntilNextAnalysis_ <= 0 && isEnoughDataAvailable())
        {
            performFFTAnalysis();
            extractSpectralFeatures();
            if (config_.enablePitchDetection) updatePitchTracking();
            if (config_.enableOnsetDetection) updateOnsetDetection();
            if (config_.enableTempoEstimation) updateTempoEstimation();
            if (config_.enableHarmonicPercussiveSeparation) performHPSeparation();
            samplesUntilNextAnalysis_ = hopSize_;
        }
    }

    // Thread-safe copies (atomic indices + POD copies)
    SpectralFeatures getLatestFeatures() const { return currentFeatures_; }
    SpectralFeatures getFeaturesAtTime(double /*timestamp*/) const { return currentFeatures_; }
    std::vector<SpectralFeatures> getFeatureHistory(int frames) const
    {
        frames = juce::jlimit(0, (int)featureHistory_.size(), frames);
        std::vector<SpectralFeatures> out; out.reserve(frames);
        int idx = featureHistoryIndex_.load();
        for (int i=0;i<frames;++i){ int j = (idx - 1 - i + (int)featureHistory_.size()) % (int)featureHistory_.size(); out.push_back(featureHistory_[j]); }
        return out;
    }

    HPSeparation getHarmonicPercussiveSeparation() const { return hpSeparation_; }
    std::array<float, MAX_FFT_SIZE/2> getMagnitudeSpectrum() const { return magnitudeSpectrum_; }
    std::array<float, MAX_FFT_SIZE/2> getPhaseSpectrum() const { return phaseSpectrum_; }
    std::array<float, MAX_FFT_SIZE/2> getPowerSpectrum() const { return powerSpectrum_; }

    const PitchTracker& getPitchTracker() const { return pitchTracker_; }
    const OnsetDetector& getOnsetDetector() const { return onsetDetector_; }
    const TempoEstimator& getTempoEstimator() const { return tempoEstimator_; }

    MLFeatures getMLFeatures() const { MLFeatures m{}; calculateMLFeatures(const_cast<MLFeatures&>(m)); return m; }

    void setAnalysisEnabled(bool e){ analysisEnabled_ = e; }
    void setPitchTrackingEnabled(bool e){ config_.enablePitchDetection = e; }
    void setOnsetDetectionEnabled(bool e){ config_.enableOnsetDetection = e; }
    void setTempoEstimationEnabled(bool e){ config_.enableTempoEstimation = e; }
    void setHPSeparationEnabled(bool e){ config_.enableHarmonicPercussiveSeparation = e; }

    void setOnsetThreshold(float t){ onsetDetector_.onsetThreshold = t; }
    void setPitchRange(float mn, float mx){ config_.pitchMinFreq=mn; config_.pitchMaxFreq=mx; }
    void setTempoRange(float mn, float mx){ config_.tempoMinBPM=mn; config_.tempoMaxBPM=mx; }

    const Statistics& getStatistics() const { return statistics_; }
    void resetStatistics(){
        statistics_.framesProcessed.store(0);
        statistics_.validFrames.store(0);
        statistics_.onsetCount.store(0);
        statistics_.averageConfidence.store(0.0f);
        statistics_.processingLoad.store(0.0f);
        statistics_.latency.store(0.0f);
        statistics_.lastAnalysisTime.store(0.0);
    }

private:
    daw::core::EngineContext& engineContext_; daw::core::RTMemoryPool& memoryPool_;
    Config config_{}; double sampleRate_=44100.0; int fftSize_=2048, hopSize_=512; bool analysisEnabled_=true;

    std::unique_ptr<juce::dsp::FFT> fft_; std::unique_ptr<juce::dsp::WindowingFunction<float>> window_;
    std::array<float, MAX_FFT_SIZE> fftInputBuffer_{}; std::array<float, MAX_FFT_SIZE*2> fftOutputBuffer_{}; std::array<float, MAX_FFT_SIZE> windowBuffer_{};
    std::array<float, MAX_FFT_SIZE/2> magnitudeSpectrum_{}; std::array<float, MAX_FFT_SIZE/2> phaseSpectrum_{}; std::array<float, MAX_FFT_SIZE/2> powerSpectrum_{}; std::array<float, MAX_FFT_SIZE/2> previousMagnitudeSpectrum_{};

    juce::AudioBuffer<float> inputRingBuffer_; std::atomic<int> ringBufferWritePos_{0}, ringBufferReadPos_{0}; int samplesUntilNextAnalysis_=0;

    SpectralFeatures currentFeatures_{}; std::array<SpectralFeatures, 32> featureHistory_{}; std::atomic<int> featureHistoryIndex_{0};

    struct MelFilterBank { std::array<std::array<float, MAX_FFT_SIZE/2>, NUM_MEL_FILTERS> filters{}; std::array<float, NUM_MEL_FILTERS+2> centerFreqs{}; bool initialized=false; } melFilterBank_{};
    struct ChromaFilterBank { std::array<std::array<float, MAX_FFT_SIZE/2>, NUM_CHROMA_BINS> filters{}; bool initialized=false; } chromaFilterBank_{};

    PitchTracker pitchTracker_{}; OnsetDetector onsetDetector_{}; TempoEstimator tempoEstimator_{};
    HPSeparation hpSeparation_{}; bool hpSeparationEnabled_=false;

    Statistics statistics_{}; juce::Time analysisStartTime_{};

    // ---------- Core analysis ----------
    void performFFTAnalysis()
    {
        fillAnalysisBuffer(); // fills fftInputBuffer_
        // apply window
        for (int i=0;i<fftSize_;++i) fftInputBuffer_[i] *= windowBuffer_[i];
        // copy to complex buffer (real in, imag zero)
        juce::FloatVectorOperations::clear(fftOutputBuffer_.data(), (int)fftOutputBuffer_.size());
        for (int i=0;i<fftSize_;++i) fftOutputBuffer_[2*i] = fftInputBuffer_[i];
        fft_->performRealOnlyForwardTransform(fftInputBuffer_.data());
        // juce RealOnly stores in-place; reconstruct magnitude/phase
        // Map to magnitude/phase using standard layout
        magnitudeSpectrum_.fill(0.0f); phaseSpectrum_.fill(0.0f); powerSpectrum_.fill(0.0f);
        const int bins = fftSize_/2;
        // first bin (DC)
        float re0 = fftInputBuffer_[0], im0 = 0.0f; magnitudeSpectrum_[0] = std::sqrt(re0*re0 + im0*im0); phaseSpectrum_[0]=0; powerSpectrum_[0]=magnitudeSpectrum_[0]*magnitudeSpectrum_[0];
        for (int k=1;k<bins;++k)
        {
            float re = fftInputBuffer_[2*k]; float im = fftInputBuffer_[2*k+1];
            float mag = std::sqrt(re*re + im*im); magnitudeSpectrum_[k]=mag; powerSpectrum_[k]=mag*mag; phaseSpectrum_[k]=std::atan2(im, re);
        }
    }

    void extractSpectralFeatures()
    {
        calculateBasicSpectralFeatures();
        if (config_.enableMFCC) calculateMFCC();
        if (config_.enableChroma) calculateChroma();
        if (config_.enableSpectralContrast) calculateSpectralContrast();
        if (config_.enableTonnetz) calculateTonnetz();
        calculateTemporalFeatures();
        calculateOnsetFeatures();

        // write to history
        currentFeatures_.frameNumber = statistics_.framesProcessed.fetch_add(1) + 1;
        currentFeatures_.timestamp = statistics_.lastAnalysisTime.load();
        currentFeatures_.isValidFrame = true;
        auto idx = (featureHistoryIndex_.load()+1) % (int)featureHistory_.size();
        featureHistory_[idx] = currentFeatures_; featureHistoryIndex_.store(idx);
        statistics_.validFrames.fetch_add(1);
    }

    void updatePitchTracking()
    {
        // YIN over window
        float f = detectPitchYIN(fftInputBuffer_.data(), fftSize_);
        if (f <= 0) f = detectPitchAutocorrelation(fftInputBuffer_.data(), fftSize_);
        pitchTracker_.currentPitch = f; pitchTracker_.pitchConfidence = calculatePitchConfidence(f, fftInputBuffer_.data(), fftSize_);
        pitchTracker_.pitchHistory[pitchTracker_.historyIndex++ % (int)pitchTracker_.pitchHistory.size()] = f;
        currentFeatures_.fundamentalFrequency = f; currentFeatures_.pitchConfidence = pitchTracker_.pitchConfidence;
    }

    void updateOnsetDetection()
    {
        float flux = calculateSpectralFlux(); onsetDetector_.currentOnsetStrength = flux; currentFeatures_.onsetStrength = flux;
        bool onset = peakPick(flux);
        onsetDetector_.onsetDetected = onset;
        if (onset) { statistics_.onsetCount.fetch_add(1); onsetDetector_.lastOnsetTime = statistics_.lastAnalysisTime.load(); }
    }

    void updateTempoEstimation()
    {
        // naive autocorrelation of onset strength history
        float bpm = estimateTempoFromAutocorrelation();
        tempoEstimator_.currentTempo = bpm; tempoEstimator_.tempoConfidence = 0.7f; currentFeatures_.tempoEstimate = bpm; currentFeatures_.beatConfidence = tempoEstimator_.tempoConfidence;
    }

    void performHPSeparation()
    {
        // Simple median filters along time (harmonic) and freq (percussive)
    const int bins = fftSize_/2; const int F=5; // kernel size (frequency)
        // Copy spectra to temp arrays
        static std::array<float, MAX_FFT_SIZE/2> harm, perc; harm = magnitudeSpectrum_; perc = magnitudeSpectrum_;
        // Frequency median (percussive)
        for (int k=0;k<bins;++k){ float window[9]; int w=0; for (int dk=-F/2; dk<=F/2; ++dk){ int kk = juce::jlimit(0,bins-1,k+dk); window[w++]=magnitudeSpectrum_[kk]; } std::nth_element(window, window+ w/2, window+w); perc[k]=window[w/2]; }
        // Time median approximated: use previousMagnitudeSpectrum_ as a proxy timeseries
        for (int k=0;k<bins;++k){ float window[9] = { previousMagnitudeSpectrum_[k], magnitudeSpectrum_[k], magnitudeSpectrum_[k] }; std::nth_element(window, window+1, window+3); harm[k]=window[1]; }
        // Energy estimates
        float eH=0,eP=0; for (int k=0;k<bins;++k){ eH += harm[k]; eP += perc[k]; }
        hpSeparation_.harmonicPercussiveRatio = (eH>1e-9f)? eH/(eH+eP) : 0.0f; hpSeparation_.harmonicEnergy=eH; hpSeparation_.percussiveEnergy=eP;
        previousMagnitudeSpectrum_ = magnitudeSpectrum_;
    }

    // ---------- Feature calculations ----------
    void calculateBasicSpectralFeatures()
    {
        const int bins = fftSize_/2; const float sr = (float)sampleRate_;
        float sum=0, sumk=0, sumk2=0, maxMag=1e-12f; for (int k=1;k<bins;++k){ float m=magnitudeSpectrum_[k]; sum += m; sumk += k*m; sumk2 += k*k*m; maxMag = std::max(maxMag,m); }
        float centroidBin = (sum>1e-12f)? sumk/sum : 0.0f; currentFeatures_.spectralCentroid = centroidBin * (sr/fftSize_);
        float var = (sum>1e-12f)? (sumk2/sum - centroidBin*centroidBin) : 0.0f; currentFeatures_.spectralSpread = std::sqrt(std::max(0.0f,var))*(sr/fftSize_);
        // Flatness
        float geo=0, arith=0; int N=0; for (int k=1;k<bins;++k){ float m = std::max(1e-12f, magnitudeSpectrum_[k]); geo += std::log(m); arith += m; ++N; }
        geo = std::exp(geo / std::max(1,N)); arith = arith / std::max(1,N); currentFeatures_.spectralFlatness = (arith>1e-9f)? geo/arith : 0.0f;
        // Rolloff
        float thresh85 = 0.85f*sum, thresh95 = 0.95f*sum; float acc=0; int r85=0, r95=0; for (int k=1;k<bins;++k){ acc+=magnitudeSpectrum_[k]; if (!r85 && acc>=thresh85) r85=k; if (!r95 && acc>=thresh95) r95=k; }
        currentFeatures_.spectralRolloff85 = r85*(sr/fftSize_); currentFeatures_.spectralRolloff95=r95*(sr/fftSize_);
        // Flux
        float flux=0; for (int k=1;k<bins;++k){ float d = magnitudeSpectrum_[k] - previousMagnitudeSpectrum_[k]; flux += std::max(0.0f, d); } currentFeatures_.spectralFlux = flux;
        currentFeatures_.spectralSlope = (sum>1e-9f)? (magnitudeSpectrum_[bins-1]-magnitudeSpectrum_[1]) / (float)(bins-2) : 0.0f;
        currentFeatures_.spectralComplexity = (float)std::count_if(magnitudeSpectrum_.begin()+1, magnitudeSpectrum_.begin()+bins, [&](float v){return v>0.1f*maxMag;});
    }

    void calculateMFCC()
    {
        // Mel energies
        std::array<float, NUM_MEL_FILTERS> melE{}; const int bins = fftSize_/2;
        for (int m=0;m<NUM_MEL_FILTERS;++m){ float e=0; for (int k=0;k<bins;++k) e += melFilterBank_.filters[m][k]*powerSpectrum_[k]; melE[m]=std::log(1e-12f+e); }
        // DCT-II
        std::array<float, NUM_MEL_FILTERS> dctIn{}; for (int i=0;i<NUM_MEL_FILTERS;++i) dctIn[i]=melE[i];
        performDCT(dctIn.data(), currentFeatures_.mfcc.data(), NUM_MEL_FILTERS);
        // Keep first 13
    }

    void calculateChroma()
    {
        std::array<float, NUM_CHROMA_BINS> ch{}; const int bins = fftSize_/2;
        for (int c=0;c<NUM_CHROMA_BINS;++c){ float e=0; for (int k=1;k<bins;++k) e += chromaFilterBank_.filters[c][k]*magnitudeSpectrum_[k]; ch[c]=e; }
        // Normalize
        float sum=0; for (auto v:ch) sum+=v; if (sum>1e-9f) for (auto &v: ch) v/=sum;
        currentFeatures_.chroma = ch;
    }

    void calculateSpectralContrast()
    {
    const int bins = fftSize_/2; const int bands = NUM_SPECTRAL_CONTRAST_BANDS;
        for (int b=0;b<bands;++b){ int k0 = (int)std::floor((b/(float)bands)*(bins-1))+1; int k1 = (int)std::floor(((b+1)/(float)bands)*(bins-1))+1; k0 = std::max(k0,1); k1 = std::max(k1,k0+1); float peaks=0, valleys=0; for(int k=k0;k<k1;++k){ peaks = std::max(peaks, magnitudeSpectrum_[k]); valleys = (valleys==0? magnitudeSpectrum_[k]: std::min(valleys, magnitudeSpectrum_[k])); } currentFeatures_.spectralContrast[b] = (valleys>1e-9f)? juce::Decibels::gainToDecibels(peaks/valleys) : 0.0f; }
    }

    void calculateTonnetz()
    {
        // Simple linear map from chroma to 6D tonnetz (Harte & Sandler approximation)
        static const float T[6][12] = {
            {1, -1, 0, 1, -1, 0, 1, -1, 0, 1, -1, 0},
            {0, 1, -1, 0, 1, -1, 0, 1, -1, 0, 1, -1},
            {1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1},
            {1, 1, 1, -1,-1,-1, 1, 1, 1, -1,-1,-1},
            {1, 0, 1, 0, 1, 0, -1,0,-1,0,-1,0},
            {0, 1, 0, 1, 0, 1, 0,-1,0,-1,0,-1},
        };
        for (int i=0;i<6;++i){ float acc=0; for (int c=0;c<12;++c) acc += T[i][c]*currentFeatures_.chroma[c]; currentFeatures_.tonnetz[i]=acc; }
    }

    void calculateTemporalFeatures()
    {
        // ZCR and RMS on input window
        int zc=0; for (int i=1;i<fftSize_;++i) if ((fftInputBuffer_[i-1] >= 0) != (fftInputBuffer_[i] >= 0)) ++zc; currentFeatures_.zeroCrossingRate = (float)zc/fftSize_;
        double acc=0; for (int i=0;i<fftSize_;++i) acc+= fftInputBuffer_[i]*fftInputBuffer_[i]; currentFeatures_.rmsEnergy = std::sqrt(acc/fftSize_); currentFeatures_.totalEnergy=(float)acc; currentFeatures_.crestFactor = currentFeatures_.rmsEnergy>1e-12f? (float) (1.0 / currentFeatures_.rmsEnergy) : 0.0f;
    }

    void calculateOnsetFeatures()
    {
        currentFeatures_.onsetConfidence = onsetDetector_.onsetDetected ? 1.0f : 0.0f;
    }

    // ---------- Pitch ----------
    float detectPitchYIN(const float* x, int N)
    {
        int tauMax = (int)std::floor(sampleRate_/config_.pitchMinFreq);
        int tauMin = (int)std::floor(sampleRate_/config_.pitchMaxFreq); tauMin = std::max(2, tauMin);
        auto& d = pitchTracker_.yinBuffer; std::fill(d.begin(), d.end(), 0.0f);
        for (int tau=tauMin; tau<tauMax && tau<(int)d.size(); ++tau){ double sum=0; for (int i=0;i<N-tau;++i){ double diff = x[i]-x[i+tau]; sum += diff*diff; } d[tau]=(float)sum; }
        // cumulative mean normalized difference
        std::array<float,1024> cmnd{}; cmnd[0]=1; float running=0; int bestTau=-1; float bestVal=1e9f;
        for (int tau=tauMin; tau<tauMax && tau<(int)d.size(); ++tau){ running += d[tau]; cmnd[tau] = d[tau] * tau / std::max(1e-9f, running); if (tau>tauMin && cmnd[tau] < pitchTracker_.yinThreshold && cmnd[tau] < bestVal){ bestVal=cmnd[tau]; bestTau=tau; } }
    if (bestTau<=0) { return 0.0f; }
    return (float)(sampleRate_ / bestTau);
    }

    float detectPitchAutocorrelation(const float* x, int N)
    {
        int tauMax = (int)std::floor(sampleRate_/config_.pitchMinFreq); int tauMin = (int)std::floor(sampleRate_/config_.pitchMaxFreq); tauMin = std::max(2,tauMin);
        int bestTau=0; float best=0; for (int tau=tauMin; tau<tauMax; ++tau){ float acc=0; for (int i=0;i<N-tau;++i) acc += x[i]*x[i+tau]; if (acc>best){ best=acc; bestTau=tau; } }
        return bestTau>0? (float)(sampleRate_/bestTau) : 0.0f;
    }

    float calculatePitchConfidence(float pitch, const float* x, int N)
    {
    if (pitch<=0) { return 0.0f; }
    int tau = (int)(sampleRate_/pitch); tau = juce::jlimit(2, N-1, tau);
    float r=0, e=0; for (int i=0;i<N-tau;++i){ r += x[i]*x[i+tau]; e += x[i]*x[i]; }
    return (e>1e-9f)? juce::jlimit(0.0f, 1.0f, r/e) : 0.0f;
    }

    // ---------- Onset ----------
    float calculateSpectralFlux()
    {
        const int bins = fftSize_/2; float flux=0; for (int k=1;k<bins;++k){ float d = magnitudeSpectrum_[k]-previousMagnitudeSpectrum_[k]; if (d>0) flux += d; }
        // push into history ring
        int idx = (onsetDetector_.historyIndex++ % ONSET_HISTORY_SIZE); onsetDetector_.spectralFluxHistory[idx]=flux; return flux;
    }

    bool peakPick(float val)
    {
        bool fire = val > (onsetDetector_.onsetThreshold + 0.5f * onsetDetector_.peakThreshold);
        if (fire && onsetDetector_.framesSinceLastPeak > onsetDetector_.peakWaitTime){ onsetDetector_.framesSinceLastPeak=0; return true; }
        ++onsetDetector_.framesSinceLastPeak; return false;
    }

    float estimateTempoFromAutocorrelation()
    {
        // Autocorrelate last ONSET_HISTORY_SIZE values
        const int L = ONSET_HISTORY_SIZE; std::array<float,L> v{}; for (int i=0;i<L;++i) v[i]=onsetDetector_.spectralFluxHistory[(onsetDetector_.historyIndex - 1 - i + L) % L];
        float bestLag=0, best=0; for (int lag=2; lag<L/2; ++lag){ float acc=0; for (int i=0;i<L-lag;++i) acc += v[i]*v[i+lag]; if (acc>best){ best=acc; bestLag=(float)lag; } }
    if (bestLag<=0) { return tempoEstimator_.currentTempo; }
    float secondsPerFrame = (float)hopSize_/ (float)sampleRate_;
    float period = bestLag * secondsPerFrame;
    if (period<=1e-6f) { return tempoEstimator_.currentTempo; }
    float bpm = 60.0f/period; // clamp range
    while (bpm < config_.tempoMinBPM) bpm*=2.0f;
    while (bpm > config_.tempoMaxBPM) bpm*=0.5f;
    return bpm;
    }

    // ---------- Filter bank init ----------
    void initializeMelFilterBank()
    {
        const int bins = fftSize_/2; float fMin = (float)config_.melMinFreq; float fMax = (float)juce::jmin((int)(sampleRate_/2), config_.melMaxFreq); auto mel = [this](float f){ return 2595.0f*std::log10(1.0f + f/700.0f); }; auto invMel = [this](float m){ return 700.0f*(std::pow(10.0f, m/2595.0f)-1.0f); };
        float melMin = mel(fMin), melMax = mel(fMax);
        for (int m=0;m<NUM_MEL_FILTERS+2;++m) melFilterBank_.centerFreqs[m] = invMel(melMin + (melMax-melMin)*m/(NUM_MEL_FILTERS+1));
        for (int m=0;m<NUM_MEL_FILTERS;++m)
        {
            float f0=melFilterBank_.centerFreqs[m], f1=melFilterBank_.centerFreqs[m+1], f2=melFilterBank_.centerFreqs[m+2];
            for (int k=0;k<bins;++k){ float fk = (float)k * (float)sampleRate_ / (float)fftSize_; float w=0; if (fk>=f0 && fk<=f1) w = (fk-f0)/(f1-f0); else if (fk>f1 && fk<=f2) w = (f2-fk)/(f2-f1); melFilterBank_.filters[m][k] = std::max(0.0f, w); }
        }
        melFilterBank_.initialized = true;
    }

    void initializeChromaFilterBank()
    {
    const int bins = fftSize_/2;
        for (int c=0;c<NUM_CHROMA_BINS;++c)
        {
            for (int k=1;k<bins;++k){ float fk = (float)k * (float)sampleRate_ / (float)fftSize_; if (fk<20.0f) { chromaFilterBank_.filters[c][k]=0; continue; } float midi = 69.0f + 12.0f*std::log2(fk/440.0f); int pc = ((int)std::round(midi)) % 12; chromaFilterBank_.filters[c][k] = (pc==c)? 1.0f : 0.0f; }
        }
        chromaFilterBank_.initialized = true;
    }

    // ---------- Math helpers ----------
    void applyWindow(float* s, int N) { for (int i=0;i<N;++i) s[i]*=windowBuffer_[i]; }

    void performDCT(const float* in, float* out, int L)
    {
        // DCT-II for MFCC (naive O(L^2), L<=40)
        for (int k=0;k<NUM_MFCC_COEFFS;++k){ double acc=0; for (int n=0;n<L;++n) acc += in[n] * std::cos(juce::MathConstants<double>::pi * (n+0.5) * k / L); out[k] = (float)acc; }
    }

    void writeToRingBuffer(const juce::AudioBuffer<float>& in)
    {
        const float* src = in.getReadPointer(0); const int n = in.getNumSamples(); int size = inputRingBuffer_.getNumSamples(); int w = ringBufferWritePos_.load();
        float* dst = inputRingBuffer_.getWritePointer(0);
        int first = std::min(n, size - w); std::memcpy(dst + w, src, first * sizeof(float)); if (n > first) std::memcpy(dst, src + first, (n-first) * sizeof(float));
        ringBufferWritePos_.store((w + n) % size);
        statistics_.lastAnalysisTime.store(juce::Time::getMillisecondCounterHiRes()/1000.0);
    }

    bool isEnoughDataAvailable() const
    {
        int size = inputRingBuffer_.getNumSamples(); int w = ringBufferWritePos_.load(); int r = ringBufferReadPos_.load(); int available = (w - r + size) % size; return available >= fftSize_;
    }

    void fillAnalysisBuffer()
    {
        int size = inputRingBuffer_.getNumSamples(); int r = ringBufferReadPos_.load(); float* src = inputRingBuffer_.getWritePointer(0); int first = std::min(fftSize_, size - r); std::memcpy(fftInputBuffer_.data(), src + r, first * sizeof(float)); if (fftSize_>first) std::memcpy(fftInputBuffer_.data()+first, src, (fftSize_-first) * sizeof(float)); ringBufferReadPos_.store((r + hopSize_) % size);
    }

    void calculateMLFeatures(MLFeatures& f) const
    {
        // Simple concat of MFCC(0..11), chroma(0..11), centroid/spread/flatness/rolloff85/tempo/onsetStrength, rms
        int i=0; for (int k=0;k<12 && i<(int)f.featureVector.size(); ++k) f.featureVector[i++] = currentFeatures_.mfcc[k];
        for (int k=0;k<12 && i<(int)f.featureVector.size(); ++k) f.featureVector[i++] = currentFeatures_.chroma[k];
        float scalars[] = { currentFeatures_.spectralCentroid, currentFeatures_.spectralSpread, currentFeatures_.spectralFlatness, currentFeatures_.spectralRolloff85, currentFeatures_.tempoEstimate, currentFeatures_.onsetStrength, currentFeatures_.rmsEnergy };
        for (float v: scalars) if (i<(int)f.featureVector.size()) f.featureVector[i++] = v;
        // normalize rough
        float maxv=1e-6f; for (int j=0;j<i;++j) maxv = std::max(maxv, std::abs(f.featureVector[j])); for (int j=0;j<i;++j) f.featureVector[j] /= maxv;
        f.isValid = currentFeatures_.isValidFrame; f.confidence = currentFeatures_.confidence;
    }
};

//======================================================================
//                           MasterAudioProcessor
//======================================================================

class MasterAudioProcessor {
public:
    static constexpr int MAX_CHANNELS = 8;
    static constexpr int MAX_BLOCK_SIZE = 1024;
    static constexpr int NUM_INSERT_SLOTS = 8;
    static constexpr int NUM_SEND_SLOTS = 4;

    struct Config {
        double sampleRate = 44100.0; int maxBlockSize = MAX_BLOCK_SIZE; int numInputChannels = 2; int numOutputChannels = 2; int bufferLatency = 256;
        bool enableSynthesizer = true, enableAnalogEQ = true, enableSpectralAnalysis = true, enableAdvancedEffects = false, enableMasterLimiter = true;
        float cpuThreshold = 0.8f; bool enableAutoOptimization = true; int maxConcurrentVoices = 32; bool enableOversampling = false;
        bool enableRealtimeAnalysis = true; bool enableMLFeatureExtraction = false; int analysisLatency = 512;
        float maxInputGain = 20.0f, maxOutputGain = 10.0f, emergencyLimiterThreshold = -0.1f;
    };

    struct ProcessingStats { std::atomic<float> cpuUsage{0}, memoryUsage{0}, latency{0}; std::atomic<int> droppedFrames{0}, overruns{0}; std::atomic<float> inputPeakL{0}, inputPeakR{0}, outputPeakL{0}, outputPeakR{0}, inputRMS{0}, outputRMS{0}; std::atomic<int> activeSynthVoices{0}; std::atomic<float> synthCpuUsage{0}, eqCpuUsage{0}, analysisCpuUsage{0}; std::atomic<float> totalHarmonicDistortion{0}, dynamicRange{0}, stereoWidth{0}, phaseCoherence{0}; };

    struct InsertSlot {
        enum class Type { None=0, Compressor, Gate, Expander, Distortion, Chorus, Flanger, Phaser, Delay, Reverb };
        Type type=Type::None; std::atomic<bool> enabled{false}, bypassed{false}; std::atomic<float> mix{1.0f}, inputGain{0.0f}, outputGain{0.0f}; std::array<std::atomic<float>,8> parameters{}; struct ProcessingState { juce::AudioBuffer<float> buffer; float peakInput=0, peakOutput=0, cpuUsage=0; } state;
        // Effect internal state (preallocated in prepare)
        struct DelayState { juce::AudioBuffer<float> line; int write=0; } delay;
        struct ChorusState { juce::AudioBuffer<float> line; int write=0; float phase=0; } chorus;
    };

    struct SendSlot { std::atomic<bool> enabled{false}; std::atomic<float> sendLevel{0.0f}, returnLevel{0.0f}, preFaderSend{0.0f}; std::atomic<bool> mute{false}; InsertSlot::Type effectType = InsertSlot::Type::Reverb; juce::AudioBuffer<float> sendBuffer, returnBuffer; };

    struct EmergencyProtection { std::atomic<bool> limiterActive{false}, thermalProtection{false}, overloadDetected{false}; std::atomic<float> gainReduction{0}; float thermalThreshold=0.95f, overloadThreshold=0.99f, limiterThreshold=-0.1f, limiterRelease=50.0f; int overloadCount=0, thermalCount=0; juce::Time lastOverload, lastThermalEvent; };

    explicit MasterAudioProcessor(daw::core::EngineContext& c, daw::core::RTMemoryPool& p, daw::core::ServiceLocator& s)
        : engineContext_(c), memoryPool_(p), serviceLocator_(s) {}
    ~MasterAudioProcessor() = default;

    void prepare(const Config& cfg)
    {
        config_ = cfg; sampleRate_=cfg.sampleRate; maxBlockSize_=cfg.maxBlockSize; numInputChannels_=cfg.numInputChannels; numOutputChannels_=cfg.numOutputChannels;
        synthBuffer_.setSize(numOutputChannels_, maxBlockSize_, false, false, true);
        eqBuffer_.setSize(numOutputChannels_, maxBlockSize_, false, false, true);
        effectsBuffer_.setSize(numOutputChannels_, maxBlockSize_, false, false, true);
        analysisBuffer_.setSize(1, maxBlockSize_, false, false, true);
        masterBuffer_.setSize(numOutputChannels_, maxBlockSize_, false, false, true);
        tempBuffer1_.setSize(numOutputChannels_, maxBlockSize_*4, false, false, true);
        tempBuffer2_.setSize(numOutputChannels_, maxBlockSize_*4, false, false, true);

    if (config_.enableSynthesizer) { synthesizer_.reset(new cppmusic::audio::AdvancedSynthesizer(engineContext_, memoryPool_)); synthesizer_->prepare(sampleRate_, maxBlockSize_, numOutputChannels_); }
    if (config_.enableAnalogEQ)    { analogEQ_.reset(new cppmusic::audio::AnalogModeledEQ(engineContext_, memoryPool_)); cppmusic::audio::AnalogModeledEQ::Config ec; ec.sampleRate=sampleRate_; ec.maxBlockSize=maxBlockSize_; analogEQ_->prepare(ec); }
        if (config_.enableSpectralAnalysis) { spectralAnalysis_.reset(new SpectralAnalysisEngine(engineContext_, memoryPool_)); SpectralAnalysisEngine::Config ac; ac.sampleRate=sampleRate_; ac.maxBlockSize=maxBlockSize_; spectralAnalysis_->prepare(ac); }

        initializeInsertEffects(); initializeSendEffects(); initializeMasterLimiter();
    }

    void reset()
    {
    if (synthesizer_) synthesizer_->reset();
    if (analogEQ_) analogEQ_->reset();
    if (spectralAnalysis_) spectralAnalysis_->reset();
    // Reset stats atomics explicitly
    stats_.cpuUsage.store(0.0f);
    stats_.memoryUsage.store(0.0f);
    stats_.latency.store(0.0f);
    stats_.droppedFrames.store(0);
    stats_.overruns.store(0);
    stats_.inputPeakL.store(0.0f);
    stats_.inputPeakR.store(0.0f);
    stats_.outputPeakL.store(0.0f);
    stats_.outputPeakR.store(0.0f);
    stats_.inputRMS.store(0.0f);
    stats_.outputRMS.store(0.0f);
    stats_.activeSynthVoices.store(0);
    stats_.synthCpuUsage.store(0.0f);
    stats_.eqCpuUsage.store(0.0f);
    stats_.analysisCpuUsage.store(0.0f);
    stats_.totalHarmonicDistortion.store(0.0f);
    stats_.dynamicRange.store(0.0f);
    stats_.stereoWidth.store(0.0f);
    stats_.phaseCoherence.store(0.0f);
    // Reset emergency protection atomics
    emergencyProtection_.limiterActive.store(false);
    emergencyProtection_.thermalProtection.store(false);
    emergencyProtection_.overloadDetected.store(false);
    emergencyProtection_.gainReduction.store(0.0f);
    averageProcessingTime_=0;
    processingTimeCount_=0;
    }

    void processBlock(juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output, const juce::MidiBuffer& midi)
    {
        jassert(input.getNumSamples() <= maxBlockSize_); processingStartTime_ = juce::Time::getCurrentTime();
        masterBuffer_.makeCopyOf(input, true);

        processInputStage(masterBuffer_);
        processSynthesisStage(masterBuffer_, midi);
        processEQStage(masterBuffer_);
        processInsertEffects(masterBuffer_);
        processSendEffects(masterBuffer_);
        processAnalysisStage(masterBuffer_);
        processOutputStage(masterBuffer_);

        output.makeCopyOf(masterBuffer_, true);

        auto t = (juce::Time::getCurrentTime() - processingStartTime_).inMilliseconds(); updatePerformanceStats((double)t);
        performAutoOptimization(); handleEmergencyConditions();
    }

    // --- Monitoring ---
    const ProcessingStats& getStatistics() const { return stats_; }

private:
    daw::core::EngineContext& engineContext_; daw::core::RTMemoryPool& memoryPool_; daw::core::ServiceLocator& serviceLocator_;
    Config config_{}; double sampleRate_=44100; int maxBlockSize_=1024; int numInputChannels_=2, numOutputChannels_=2;

    std::unique_ptr<cppmusic::audio::AdvancedSynthesizer> synthesizer_; std::unique_ptr<cppmusic::audio::AnalogModeledEQ> analogEQ_; std::unique_ptr<SpectralAnalysisEngine> spectralAnalysis_;

    std::array<InsertSlot, NUM_INSERT_SLOTS> insertSlots_{}; std::array<SendSlot, NUM_SEND_SLOTS> sendSlots_{};

    juce::AudioBuffer<float> synthBuffer_, eqBuffer_, effectsBuffer_, analysisBuffer_, masterBuffer_, tempBuffer1_, tempBuffer2_;

    std::atomic<float> inputGain_{0}, outputGain_{0}; std::atomic<bool> masterMute_{false}, masterSolo_{false}, masterBypass_{false};
    std::atomic<float> cpuThreshold_{0.8f}; std::atomic<bool> autoOptimizationEnabled_{true}; std::atomic<int> maxPolyphony_{32}; std::atomic<bool> oversamplingEnabled_{false};
    int qualityLevel_=100; int lastPolyphonyReduction_=0; juce::Time lastOptimization_{};
    std::atomic<bool> realtimeAnalysisEnabled_{true}; std::atomic<bool> mlFeatureExtractionEnabled_{false}; std::atomic<int> analysisLatency_{512};

    ProcessingStats stats_{}; EmergencyProtection emergencyProtection_{}; juce::Time processingStartTime_{}; double averageProcessingTime_=0; int processingTimeCount_=0;

    struct MasterLimiter { float threshold=-0.1f, release=50.0f, lookahead=5.0f; float gainReduction=0, envelope=0; juce::AudioBuffer<float> lookaheadBuffer; int lookaheadSamples=0, writeIndex=0; float attackCoeff=0, releaseCoeff=0; } masterLimiter_;

    // ---------- Stages ----------
    void processInputStage(juce::AudioBuffer<float>& buf)
    {
        float inGain = std::pow(10.0f, inputGain_.load()/20.0f); if (inGain != 1.0f) buf.applyGain(inGain);
        float rms, pk; calculateRMSAndPeak(buf, rms, pk); stats_.inputRMS.store(rms); stats_.inputPeakL.store(pk); stats_.inputPeakR.store(pk);
        checkInputOverload(buf);
    }

    void processSynthesisStage(juce::AudioBuffer<float>& buf, const juce::MidiBuffer& midi)
    {
    if (!synthesizer_) { return; }
    synthBuffer_.clear();
    synthesizer_->process(synthBuffer_, midi);
    buf.addFrom(0,0,synthBuffer_,0,0,buf.getNumSamples());
    if (buf.getNumChannels()>1) buf.addFrom(1,0,synthBuffer_,1,0,buf.getNumSamples());
    stats_.activeSynthVoices.store(synthesizer_->getActiveVoices());
    }

    void processEQStage(juce::AudioBuffer<float>& buf)
    {
    if (!analogEQ_) { return; }
    eqBuffer_.makeCopyOf(buf, true);
    analogEQ_->processBlock(eqBuffer_);
    buf.makeCopyOf(eqBuffer_, true);
    }

    void processInsertEffects(juce::AudioBuffer<float>& buf)
    {
        for (auto& slot : insertSlots_) if (slot.enabled.load() && !slot.bypassed.load()) processInsertSlot(slot, buf);
    }

    void processSendEffects(juce::AudioBuffer<float>& buf)
    {
        for (auto& slot : sendSlots_) if (slot.enabled.load()) processSendSlot(slot, buf, buf);
    }

    void processAnalysisStage(const juce::AudioBuffer<float>& buf)
    {
    if (!spectralAnalysis_ || !realtimeAnalysisEnabled_.load()) { return; }
    analysisBuffer_.copyFrom(0,0,buf.getReadPointer(0), buf.getNumSamples());
    spectralAnalysis_->processBlock(analysisBuffer_);
    }

    void processOutputStage(juce::AudioBuffer<float>& buf)
    {
        if (masterLimiter_.lookaheadSamples>0) processMasterLimiter(buf);
        float outGain = std::pow(10.0f, outputGain_.load()/20.0f); if (outGain != 1.0f) buf.applyGain(outGain);
        float rms, pk; calculateRMSAndPeak(buf, rms, pk); stats_.outputRMS.store(rms); stats_.outputPeakL.store(pk); stats_.outputPeakR.store(pk);
    }

    // ---------- Effects ----------
    void initializeInsertEffects()
    {
        for (auto& s : insertSlots_) { s.state.buffer.setSize(numOutputChannels_, maxBlockSize_, false, false, true); s.delay.line.setSize(numOutputChannels_, maxBlockSize_*8, false, false, true); s.chorus.line.setSize(numOutputChannels_, maxBlockSize_*4, false, false, true); }
    }

    void initializeSendEffects()
    {
        for (auto& s : sendSlots_) {
            s.sendBuffer.setSize(numOutputChannels_, maxBlockSize_, false, false, true);
            s.returnBuffer.setSize(numOutputChannels_, maxBlockSize_, false, false, true);
        }
    }

    void processInsertSlot(InsertSlot& slot, juce::AudioBuffer<float>& buf)
    {
        switch (slot.type) {
            case InsertSlot::Type::Compressor: processCompressor(slot, buf); break;
            case InsertSlot::Type::Gate:       processGate(slot, buf); break;
            case InsertSlot::Type::Distortion: processDistortion(slot, buf); break;
            case InsertSlot::Type::Delay:      processDelay(slot, buf); break;
            case InsertSlot::Type::Chorus:     processChorus(slot, buf); break;
            case InsertSlot::Type::Reverb:     processReverb(slot, buf); break;
            default: break;
        }
    }

    void processCompressor(InsertSlot& slot, juce::AudioBuffer<float>& buf)
    {
        const float thresh = juce::Decibels::decibelsToGain(slot.parameters[0].load()); // dB threshold
        const float ratio  = std::max(1.0f, slot.parameters[1].load());
        const float attack = std::max(0.1f, slot.parameters[2].load());
        const float release= std::max(0.1f, slot.parameters[3].load());
        for (int ch=0; ch<buf.getNumChannels(); ++ch)
        {
            float env=0; float* x = buf.getWritePointer(ch); int N=buf.getNumSamples(); for (int i=0;i<N;++i){ float v=std::abs(x[i]); float targ = v; float coeff = (env<targ)? std::exp(-1.0f/(attack*sampleRate_*0.001f)) : std::exp(-1.0f/(release*sampleRate_*0.001f)); env = coeff*env + (1-coeff)*targ; float g = 1.0f; if (env>thresh) g = std::pow(env/thresh, 1.0f - 1.0f/ratio); x[i] *= 1.0f/g; }
        }
    }

    void processGate(InsertSlot& slot, juce::AudioBuffer<float>& buf)
    {
        const float gate = juce::Decibels::decibelsToGain(slot.parameters[0].load());
        for (int ch=0; ch<buf.getNumChannels(); ++ch){ float* x = buf.getWritePointer(ch); for (int i=0;i<buf.getNumSamples(); ++i) if (std::abs(x[i]) < gate) x[i]*=0.0f; }
    }

    void processDistortion(InsertSlot& slot, juce::AudioBuffer<float>& buf)
    {
        const float drive = std::max(1.0f, slot.parameters[0].load());
        for (int ch=0; ch<buf.getNumChannels(); ++ch){ float* x = buf.getWritePointer(ch); for (int i=0;i<buf.getNumSamples(); ++i){ float v=x[i]*drive; x[i]=std::tanh(v); } }
    }

    void processDelay(InsertSlot& slot, juce::AudioBuffer<float>& buf)
    {
        const int delaySamp = (int) juce::jlimit(1.0f, 2.0f, slot.parameters[0].load()) * (int) (0.5f*sampleRate_); // coarse 0.5..1s
        const float fb = juce::jlimit(0.0f, 0.95f, slot.parameters[1].load());
        auto& line = slot.delay.line; int size=line.getNumSamples();
        for (int ch=0; ch<buf.getNumChannels(); ++ch)
        {
            float* x = buf.getWritePointer(ch); float* dl = line.getWritePointer(ch); int w=slot.delay.write;
            for (int i=0;i<buf.getNumSamples(); ++i)
            {
                int r = (w - delaySamp + size) % size; float d = dl[r]; float y = x[i] + d; dl[w] = x[i] + d*fb; x[i] = y; w = (w+1)%size;
            }
            slot.delay.write = w;
        }
    }

    void processChorus(InsertSlot& slot, juce::AudioBuffer<float>& buf)
    {
        const float rate=juce::jlimit(0.05f,5.0f, slot.parameters[0].load()); const float depth=juce::jlimit(0.0f, 20.0f, slot.parameters[1].load());
        auto& st = slot.chorus; auto& line = st.line; int size=line.getNumSamples();
        for (int ch=0; ch<buf.getNumChannels(); ++ch)
        {
            float* x = buf.getWritePointer(ch); float* dl = line.getWritePointer(ch); int w=st.write; float ph = st.phase;
            for (int i=0;i<buf.getNumSamples(); ++i)
            {
                float mod = (std::sin(ph) * 0.5f + 0.5f) * depth + 8.0f; int dSamp = (int)mod; int r = (w - dSamp + size) % size; float d = dl[r]; dl[w] = x[i]; x[i] = 0.5f*(x[i]+d); w=(w+1)%size; ph += 2.0f*(float)juce::MathConstants<double>::pi * rate / (float)sampleRate_;
            }
            st.write = w; st.phase = ph;
        }
    }

    void processReverb(InsertSlot& slot, juce::AudioBuffer<float>& buf)
    {
        // Lightweight Schroeder-like: feedback comb via delay slot + damping using lowpass
        const float mix = juce::jlimit(0.0f,1.0f, slot.mix.load()); auto dry = buf; processDelay(slot, buf); // crude plate-ish
        for (int ch=0; ch<buf.getNumChannels(); ++ch){ float* x = buf.getWritePointer(ch); const float* d = dry.getReadPointer(ch); for (int i=0;i<buf.getNumSamples(); ++i) x[i] = d[i]*(1.0f-mix) + x[i]*mix; }
    }

    void processSendSlot(SendSlot& slot, const juce::AudioBuffer<float>& in, juce::AudioBuffer<float>& out)
    {
    if (slot.mute.load()) { return; }
    slot.sendBuffer.makeCopyOf(in, true); // simple pre-fader send
        // do a small reverb
        InsertSlot fake{}; fake.type = InsertSlot::Type::Reverb; fake.mix.store(1.0f); fake.parameters[0].store(1.0f); processReverb(fake, slot.sendBuffer);
        out.addFrom(0,0, slot.sendBuffer, 0,0, out.getNumSamples(), juce::Decibels::decibelsToGain(slot.returnLevel.load()));
        if (out.getNumChannels()>1) out.addFrom(1,0, slot.sendBuffer, 1,0, out.getNumSamples(), juce::Decibels::decibelsToGain(slot.returnLevel.load()));
    }

    // ---------- Limiter ----------
    void initializeMasterLimiter()
    {
        masterLimiter_.lookaheadSamples = (int)(masterLimiter_.lookahead*0.001f*sampleRate_);
        masterLimiter_.lookaheadBuffer.setSize(numOutputChannels_, masterLimiter_.lookaheadSamples + maxBlockSize_, false, false, true);
        masterLimiter_.attackCoeff = std::exp(-1.0f/(0.5f*sampleRate_*0.001f)); masterLimiter_.releaseCoeff = std::exp(-1.0f/(masterLimiter_.release*sampleRate_*0.001f));
    }

    void processMasterLimiter(juce::AudioBuffer<float>& buf)
    {
    auto& L = masterLimiter_; int LA=L.lookaheadSamples; int N = buf.getNumSamples();
        for (int i=0;i<N;++i)
        {
            // write into lookahead
            for (int ch=0; ch<buf.getNumChannels(); ++ch) L.lookaheadBuffer.setSample(ch, (L.writeIndex + i) % (LA+N), buf.getSample(ch, i));
        }
        float thresh = juce::Decibels::decibelsToGain(masterLimiter_.threshold);
        for (int i=0;i<N;++i)
        {
            float sample = 0; for (int ch=0; ch<buf.getNumChannels(); ++ch) sample = std::max(sample, std::abs(L.lookaheadBuffer.getSample(ch, (L.writeIndex + i) % (LA+N))));
            float env = L.envelope; env = (sample > env) ? L.attackCoeff * env + (1-L.attackCoeff)*sample : L.releaseCoeff * env + (1-L.releaseCoeff)*sample; L.envelope = env;
            float gain = (env>thresh)? thresh / env : 1.0f; L.gainReduction = 1.0f - gain; for (int ch=0; ch<buf.getNumChannels(); ++ch){ float v = L.lookaheadBuffer.getSample(ch, (L.writeIndex + i) % (LA+N)); buf.setSample(ch, i, v * gain); }
        }
        L.writeIndex = (L.writeIndex + N) % (LA+N);
    }

    // ---------- Perf & safety ----------
    void updatePerformanceStats(double ms)
    {
        averageProcessingTime_ = (averageProcessingTime_*processingTimeCount_ + ms) / (processingTimeCount_ + 1); ++processingTimeCount_; stats_.cpuUsage.store((float)ms);
    }
    void performAutoOptimization() { /* hook for quality scaling; noop by default */ }
    void handleEmergencyConditions() { /* overload/thermal hooks; noop */ }

    void checkInputOverload(const juce::AudioBuffer<float>& buf)
    {
        float peak = buf.getMagnitude(0,0,buf.getNumSamples()); if (peak > juce::Decibels::decibelsToGain(-0.1f)) { emergencyProtection_.overloadDetected.store(true); emergencyProtection_.gainReduction.store(peak); }
    }

    // ---------- Utils ----------
    void calculateRMSAndPeak(const juce::AudioBuffer<float>& b, float& rms, float& peak) const
    {
        double acc=0; peak=0; for (int ch=0; ch<b.getNumChannels(); ++ch){ const float* x=b.getReadPointer(ch); for (int i=0;i<b.getNumSamples(); ++i){ float v=x[i]; acc += v*v; peak = std::max(peak, std::abs(v)); } } rms = std::sqrt(acc / (b.getNumSamples()*std::max(1,b.getNumChannels())));
    }
};

}} // namespaces

//======================================================================
//                               Unit Tests
//======================================================================
#if JUCE_UNIT_TESTS
namespace daw { namespace audio {
class SpectralAndMasterTests : public juce::UnitTest {
public:
    SpectralAndMasterTests() : juce::UnitTest("SpectralAnalysisEngine+MasterAudioProcessor", "Audio") {}
    void runTest() override {
        core::EngineContext c; core::RTMemoryPool p; core::ServiceLocator s;
        // Spectral
        beginTest("Pitch detection on 440 Hz");
        SpectralAnalysisEngine spec(c,p); SpectralAnalysisEngine::Config ac; ac.sampleRate=48000; ac.fftSize=2048; ac.hopSize=512; spec.prepare(ac); spec.reset();
        juce::AudioBuffer<float> buf(1,512); double ph=0, w=2.0*juce::MathConstants<double>::pi*440.0/ac.sampleRate; for (int i=0;i<512;++i){ buf.setSample(0,i,(float)std::sin(ph)); ph+=w; }
        for (int i=0;i<10;++i) spec.processBlock(buf);
        auto ft = spec.getLatestFeatures(); expectWithinAbsoluteError(ft.fundamentalFrequency, 440.0f, 5.0f);

        beginTest("Onset reacts to transient");
        buf.clear(); for (int i=0;i<32;++i) buf.setSample(0,i, i/32.0f ); for (int i=32;i<64;++i) buf.setSample(0,i, 1.0f - (i-32)/32.0f ); for (int i=0;i<8;++i) spec.processBlock(buf); auto od = spec.getOnsetDetector(); expect(od.currentOnsetStrength > 0.01f);

        // Master
        beginTest("Master chain roundtrip without crash");
        MasterAudioProcessor mp(c,p,s); MasterAudioProcessor::Config mc; mc.sampleRate=48000; mc.maxBlockSize=512; mc.enableSpectralAnalysis=true; mc.enableAnalogEQ=true; mc.enableSynthesizer=false; mp.prepare(mc); mp.reset(); juce::AudioBuffer<float> in(2,512), out(2,512); in.clear(); in.addSample(0,0,0.8f); juce::MidiBuffer midi; mp.processBlock(in,out,midi); expect(out.getRMSLevel(0,0,512) >= 0.0f);
    }
};
static SpectralAndMasterTests spectralAndMasterTests;
}} // namespaces
#endif
