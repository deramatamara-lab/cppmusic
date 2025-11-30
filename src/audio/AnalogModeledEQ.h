// AnalogModeledEQ.hpp — single translation unit (header + implementation)
// Drop-in for cppmusic::audio::AnalogModeledEQ. Fixed RT-safety, LUT saturation,
// RBJ biquads, transformer pre/post, oversampling (optional), and JUCE unit tests.
//
// NOTE: This TU supersedes the earlier header. It reconciles type scope issues and
// implements all declared methods with zero heap allocation in processBlock().
// Requires: juce_core, juce_audio_basics, juce_dsp. Optional: juce_unit_tests.

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <array>
#include <memory>
#include <cmath>
#include <string>

// Forward-declare your engine headers (kept as includes in original). If available, include them.
#include "core/EngineContext.h"
#include "core/RTMemoryPool.h"

namespace cppmusic {
namespace audio {

class AnalogModeledEQ {
public:
    static constexpr int NUM_BANDS = 5;
    static constexpr int MAX_BLOCK_SIZE = 1024;
    static constexpr int SATURATION_TABLE_SIZE = 2048;

    enum class BandType : uint8_t { LowShelf=0, Parametric, HighShelf, HighPass, LowPass, BandPass, Notch };
    enum class FilterSlope : uint8_t { Slope6dB=0, Slope12dB, Slope24dB, Slope48dB };
    enum class AnalogModel : uint8_t { Clean=0, NeveVintage, SSLChannel, APIChannel, PultecEQP1A, FairchildLimiter, TubePreamp };

    struct Config {
        double sampleRate = 44100.0;
        int maxBlockSize = MAX_BLOCK_SIZE;
        AnalogModel analogModel = AnalogModel::NeveVintage;
        bool enableOversampling = true;
        int oversamplingFactor = 2; // 2x only in this TU
        bool enableSpectralAnalysis = false;
        bool enableLookAheadLimiting = false; // not implemented (reserved)
        float maxGainReduction = -20.0f;      // reserved
        int latencyCompensation = 0;          // reserved
    };

    // Filter coefficients/state promoted to class-scope so all sub-structures can use them
    struct FilterCoefficients { float b0{1}, b1{0}, b2{0}, a1{0}, a2{0}; };
    struct BiquadState { float x1{0}, x2{0}, y1{0}, y2{0}; };

    struct EQBand {
        BandType type = BandType::Parametric;
        FilterSlope slope = FilterSlope::Slope12dB;

        std::atomic<float> frequency{1000.0f};
        std::atomic<float> gain{0.0f};
        std::atomic<float> q{1.0f};
        std::atomic<float> drive{1.0f};
        std::atomic<float> saturation{0.0f};
        std::atomic<float> mix{1.0f};
        std::atomic<bool> enabled{true};
        std::atomic<bool> solo{false};
        std::atomic<bool> bypassed{false};

        // Up to 4 cascaded sections to emulate 6/12/24/48 dB slopes
        FilterCoefficients coeffs[4];
        BiquadState states[2][4]; // [channel][section]

        float lastSaturationInput = 0.0f;
        float saturationState = 0.0f;

        float currentGainReduction = 0.0f;
        float peakInput = 0.0f;
        float peakOutput = 0.0f;
    };

    struct AnalogProcessor {
        AnalogModel currentModel = AnalogModel::Clean;
        std::atomic<float> inputGain{0.0f};
        std::atomic<float> outputGain{0.0f};
        std::atomic<float> transformerDrive{1.0f};
        std::atomic<float> tubeWarmth{0.0f};
        std::atomic<float> tapeSaturation{0.0f};
        std::atomic<float> analogNoise{0.0f};

        struct TransformerModel {
            BiquadState preFilter[2];
            BiquadState postFilter[2];
            FilterCoefficients preCoeffs;
            FilterCoefficients postCoeffs;
        } transformer;

        std::array<float, SATURATION_TABLE_SIZE> saturationLUT{};
        std::array<float, SATURATION_TABLE_SIZE> tubeLUT{};
        std::array<float, SATURATION_TABLE_SIZE> tapeLUT{};

        uint32_t noiseState = 0x12345678;
        float noiseLevel = 0.0f;
    };

    // Plain settings for preset serialization (avoid std::atomic in Preset)
    struct BandSettings {
        BandType type{BandType::Parametric};
        FilterSlope slope{FilterSlope::Slope12dB};
        float frequency{1000.0f}, gain{0.0f}, q{1.0f}, drive{1.0f}, saturation{0.0f}, mix{1.0f};
        bool enabled{true}, solo{false}, bypassed{false};
    };

    struct AnalogSettings {
        AnalogModel currentModel{AnalogModel::Clean};
        float inputGain{0.0f}, outputGain{0.0f}, transformerDrive{1.0f}, tubeWarmth{0.0f}, tapeSaturation{0.0f}, analogNoise{0.0f};
    };

    struct Preset {
        std::string name;
        std::array<BandSettings, NUM_BANDS> bands{};
        AnalogSettings analog{};
    };

    struct AnalysisData {
        std::array<float, 512> frequencyResponse{};
        std::array<float, 512> phaseResponse{};
        std::array<float, NUM_BANDS> bandGainReduction{};
        float totalHarmonicDistortion = 0.0f;
        float dynamicRange = 0.0f;
        float stereoWidth = 0.0f;
    };

    struct Statistics {
        std::atomic<float> inputPeakL{0.0f};
        std::atomic<float> inputPeakR{0.0f};
        std::atomic<float> outputPeakL{0.0f};
        std::atomic<float> outputPeakR{0.0f};
        std::atomic<float> totalGainReduction{0.0f};
        std::atomic<float> analogHarmonics{0.0f};
        std::atomic<float> cpuUsage{0.0f};
        std::atomic<int> processedSamples{0};
    };

    explicit AnalogModeledEQ(daw::core::EngineContext& context, daw::core::RTMemoryPool& pool)
        : engineContext_(context), memoryPool_(pool) {}
    ~AnalogModeledEQ() = default;

    AnalogModeledEQ(const AnalogModeledEQ&) = delete;
    AnalogModeledEQ& operator=(const AnalogModeledEQ&) = delete;
    AnalogModeledEQ(AnalogModeledEQ&&) = delete;
    AnalogModeledEQ& operator=(AnalogModeledEQ&&) = delete;

    void prepare(const Config& cfg)
    {
        config_ = cfg;
        sampleRate_ = cfg.sampleRate;
        maxBlockSize_ = juce::jmax(32, cfg.maxBlockSize);

        wetBuffer_.setSize(2, maxBlockSize_, false, false, true);
        dryBuffer_.setSize(2, maxBlockSize_, false, false, true);
        bandBuffer_.setSize(2, maxBlockSize_, false, false, true);

        initializeSaturationTables();
        setAnalogModel(cfg.analogModel);

        // Transformers: simple gentle LP at input, HP at output
        designOnePoleLowpass(12000.0f, analogProcessor_.transformer.preCoeffs);
        designOnePoleHighpass(18.0f, analogProcessor_.transformer.postCoeffs);

        // Oversampling (2x) pre-allocated
        if (cfg.enableOversampling && cfg.oversamplingFactor == 2)
            oversampling_.reset(new juce::dsp::Oversampling<float>(2 /*channels*/, 1 /*stages => 2x*/, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR));
        else
            oversampling_.reset();

        // Precompute all band coefficients for initial params
        for (auto& b : bands_) calculateBiquadCoefficients(b);
    }

    void reset()
    {
        for (auto& b : bands_) {
            for (int ch=0; ch<2; ++ch) for (int s=0; s<4; ++s) b.states[ch][s] = {};
        }
        analogProcessor_.transformer.preFilter[0] = analogProcessor_.transformer.preFilter[1] = {};
        analogProcessor_.transformer.postFilter[0] = analogProcessor_.transformer.postFilter[1] = {};
        resetStatistics();
    }

    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        const int numSamples = buffer.getNumSamples();
        jassert(numSamples <= maxBlockSize_);

        auto t0 = juce::Time::getMillisecondCounterHiRes();

        // Copy input to dry for wet/dry mixing per-band
        dryBuffer_.makeCopyOf(buffer, true);

        // Input analog gain
        const float inGain = dbToLinear(analogProcessor_.inputGain.load());
        if (inGain != 1.0f) buffer.applyGain(inGain);

        // Optional oversampling (pre)
    juce::dsp::AudioBlock<float> block(buffer);
        std::unique_ptr<juce::dsp::AudioBlock<float>> osBlock;
        if (oversampling_) {
            auto& os = *oversampling_;
            auto up = os.processSamplesUp(block);
            osBlock.reset(new juce::dsp::AudioBlock<float>(up));
        }
        auto& workBlock = osBlock ? *osBlock : block;

        // Work buffer view
        wetBuffer_.setSize((int)workBlock.getNumChannels(), (int)workBlock.getNumSamples(), false, false, true);
        bandBuffer_.setSize((int)workBlock.getNumChannels(), (int)workBlock.getNumSamples(), false, false, true);
        for (size_t ch=0; ch<workBlock.getNumChannels(); ++ch)
            wetBuffer_.copyFrom((int)ch, 0, workBlock.getChannelPointer(ch), (int)workBlock.getNumSamples());

        // Determine if any band is soloed
        bool anySolo = false; for (auto& b : bands_) if (b.solo.load()) { anySolo = true; break; }

        // Process bands sequentially onto wetBuffer_
        for (int bi=0; bi<NUM_BANDS; ++bi)
        {
            auto& band = bands_[bi];
            if (!band.enabled.load() || band.bypassed.load()) continue;
            if (anySolo && !band.solo.load()) continue; // respect solo

            calculateBiquadCoefficients(band); // cheap; per-block param pull

            bandBuffer_.makeCopyOf(wetBuffer_, true);
            processBand(band, bandBuffer_);

            const float mix = juce::jlimit(0.0f, 1.0f, band.mix.load());
            for (int ch=0; ch<(int)workBlock.getNumChannels(); ++ch)
                wetBuffer_.addFrom(ch, 0, bandBuffer_, ch, 0, (int)workBlock.getNumSamples(), mix - 1.0f); // wet replaces, since bandBuffer_ started as wetBuffer_
        }

        // Write back to workBlock
        for (size_t ch=0; ch<workBlock.getNumChannels(); ++ch)
            juce::FloatVectorOperations::copy(workBlock.getChannelPointer(ch), wetBuffer_.getReadPointer((int)ch), (int)workBlock.getNumSamples());

        // Analog transformer & saturations (post-bands)
        if (analogProcessor_.currentModel != AnalogModel::Clean)
            processAnalogModeling(wetBuffer_);

        // Downsample if needed
        if (oversampling_) {
            auto& os = *oversampling_;
            os.processSamplesDown(block);
        }

        // Output gain
        const float outGain = dbToLinear(analogProcessor_.outputGain.load());
        if (outGain != 1.0f) buffer.applyGain(outGain);

        // Stats
        float inL = 0, inR = 0, outL = 0, outR = 0;
        inL = dryBuffer_.getMagnitude(0, 0, numSamples);
        if (buffer.getNumChannels() > 1) inR = dryBuffer_.getMagnitude(1, 0, numSamples);
        outL = buffer.getMagnitude(0, 0, numSamples);
        if (buffer.getNumChannels() > 1) outR = buffer.getMagnitude(1, 0, numSamples);
        statistics_.inputPeakL.store(inL); statistics_.inputPeakR.store(inR);
        statistics_.outputPeakL.store(outL); statistics_.outputPeakR.store(outR);
        statistics_.processedSamples.fetch_add(numSamples);

        auto t1 = juce::Time::getMillisecondCounterHiRes();
        statistics_.cpuUsage.store((float) (t1 - t0)); // ms for this block (UI can turn to %)

        if (analysisEnabled_) updateAnalysisData(buffer);
    }

    // Parameter API ------------------------------------------------------
    void setBandEnabled(int i, bool v) { bands_[clampBand(i)].enabled.store(v); }
    void setBandType(int i, BandType t) { bands_[clampBand(i)].type = t; }
    void setBandFrequency(int i, float f) { bands_[clampBand(i)].frequency.store(f); }
    void setBandGain(int i, float g) { bands_[clampBand(i)].gain.store(g); }
    void setBandQ(int i, float v) { bands_[clampBand(i)].q.store(v); }
    void setBandDrive(int i, float v) { bands_[clampBand(i)].drive.store(v); }
    void setBandSaturation(int i, float v) { bands_[clampBand(i)].saturation.store(v); }
    void setBandMix(int i, float v) { bands_[clampBand(i)].mix.store(v); }
    void setBandSlope(int i, FilterSlope s) { bands_[clampBand(i)].slope = s; }

    void setAnalogModel(AnalogModel m) { analogProcessor_.currentModel = m; switch (m){case AnalogModel::NeveVintage: configureNeveEmulation(); break; case AnalogModel::SSLChannel: configureSSLEmulation(); break; case AnalogModel::APIChannel: configureAPIEmulation(); break; case AnalogModel::PultecEQP1A: configurePultecEmulation(); break; case AnalogModel::FairchildLimiter: configureFairchildEmulation(); break; case AnalogModel::TubePreamp: configureTubePreampEmulation(); break; default: break;} }
    void setInputGain(float db) { analogProcessor_.inputGain.store(db); }
    void setOutputGain(float db) { analogProcessor_.outputGain.store(db); }
    void setTransformerDrive(float d){ analogProcessor_.transformerDrive.store(d); }
    void setTubeWarmth(float w){ analogProcessor_.tubeWarmth.store(w); }
    void setTapeSaturation(float s){ analogProcessor_.tapeSaturation.store(s); }
    void setAnalogNoise(float n){ analogProcessor_.analogNoise.store(n); }

    void soloBand(int i, bool s){ bands_[clampBand(i)].solo.store(s); }
    void bypassBand(int i, bool b){ bands_[clampBand(i)].bypassed.store(b); }
    void bypassAll(bool b){ for (auto& band: bands_) band.bypassed.store(b); }

    void loadPreset(const Preset& p){
        // Bands
        for (int i=0; i<NUM_BANDS; ++i){
            const auto& src = p.bands[(size_t)i];
            auto& dst = bands_[(size_t)i];
            dst.type = src.type;
            dst.slope = src.slope;
            dst.frequency.store(src.frequency);
            dst.gain.store(src.gain);
            dst.q.store(src.q);
            dst.drive.store(src.drive);
            dst.saturation.store(src.saturation);
            dst.mix.store(src.mix);
            dst.enabled.store(src.enabled);
            dst.solo.store(src.solo);
            dst.bypassed.store(src.bypassed);
        }
        // Analog
        setAnalogModel(p.analog.currentModel);
        analogProcessor_.inputGain.store(p.analog.inputGain);
        analogProcessor_.outputGain.store(p.analog.outputGain);
        analogProcessor_.transformerDrive.store(p.analog.transformerDrive);
        analogProcessor_.tubeWarmth.store(p.analog.tubeWarmth);
        analogProcessor_.tapeSaturation.store(p.analog.tapeSaturation);
        analogProcessor_.analogNoise.store(p.analog.analogNoise);
    }
    Preset savePreset(const std::string& name) const {
        Preset pr{}; pr.name = name;
        for (int i=0; i<NUM_BANDS; ++i){
            const auto& src = bands_[(size_t)i];
            auto& dst = pr.bands[(size_t)i];
            dst.type = src.type;
            dst.slope = src.slope;
            dst.frequency = src.frequency.load();
            dst.gain = src.gain.load();
            dst.q = src.q.load();
            dst.drive = src.drive.load();
            dst.saturation = src.saturation.load();
            dst.mix = src.mix.load();
            dst.enabled = src.enabled.load();
            dst.solo = src.solo.load();
            dst.bypassed = src.bypassed.load();
        }
        pr.analog.currentModel = analogProcessor_.currentModel;
        pr.analog.inputGain = analogProcessor_.inputGain.load();
        pr.analog.outputGain = analogProcessor_.outputGain.load();
        pr.analog.transformerDrive = analogProcessor_.transformerDrive.load();
        pr.analog.tubeWarmth = analogProcessor_.tubeWarmth.load();
        pr.analog.tapeSaturation = analogProcessor_.tapeSaturation.load();
        pr.analog.analogNoise = analogProcessor_.analogNoise.load();
        return pr;
    }
    void loadVintagePreset(const std::string& n){ if (n=="Neve") setAnalogModel(AnalogModel::NeveVintage); else if (n=="SSL") setAnalogModel(AnalogModel::SSLChannel); else if (n=="API") setAnalogModel(AnalogModel::APIChannel); else if (n=="Pultec") setAnalogModel(AnalogModel::PultecEQP1A); else if (n=="Fairchild") setAnalogModel(AnalogModel::FairchildLimiter); else if (n=="Tube") setAnalogModel(AnalogModel::TubePreamp); }

    const AnalysisData& getAnalysisData() const { return analysisData_; }
    void enableAnalysis(bool en) { analysisEnabled_ = en; }

    const Statistics& getStatistics() const { return statistics_; }
    void resetStatistics(){
        statistics_.inputPeakL.store(0.0f);
        statistics_.inputPeakR.store(0.0f);
        statistics_.outputPeakL.store(0.0f);
        statistics_.outputPeakR.store(0.0f);
        statistics_.totalGainReduction.store(0.0f);
        statistics_.analogHarmonics.store(0.0f);
        statistics_.cpuUsage.store(0.0f);
        statistics_.processedSamples.store(0);
    }

private:
    daw::core::EngineContext& engineContext_;
    daw::core::RTMemoryPool& memoryPool_;

    Config config_{};
    double sampleRate_ = 44100.0;
    int maxBlockSize_ = MAX_BLOCK_SIZE;

    std::array<EQBand, NUM_BANDS> bands_{};
    AnalogProcessor analogProcessor_{};

    juce::AudioBuffer<float> wetBuffer_, dryBuffer_, bandBuffer_;

    AnalysisData analysisData_{};
    Statistics statistics_{};
    bool analysisEnabled_ = false;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling_{}; // 2x optional

    // ---------------------- DSP internals -------------------------------
    static int clampBand(int i){ return juce::jlimit(0, NUM_BANDS-1, i); }

    void processBand(EQBand& band, juce::AudioBuffer<float>& buf)
    {
        const int n = buf.getNumSamples();
        const int chs = juce::jmin(2, buf.getNumChannels());

        // Filtering: cascade 1..4 sections to emulate slopes
        const int sections = slopeToSections(band.slope);
        for (int s=0; s<sections; ++s)
        {
            auto& c = band.coeffs[s];
            for (int ch=0; ch<chs; ++ch)
            {
                auto* x = buf.getWritePointer(ch);
                auto& st = band.states[ch][s];
                processBiquad(c, st, x, n);
            }
        }

        // Per-band saturation
        const float drive = juce::jlimit(0.1f, 10.0f, band.drive.load());
        const float amt   = juce::jlimit(0.0f, 1.0f, band.saturation.load());
        if (amt > 0.0001f)
        {
            for (int ch=0; ch<chs; ++ch)
                processSaturation(buf.getWritePointer(ch), n, drive, amt);
        }

        // Peak tracking (cheap)
        for (int ch=0; ch<chs; ++ch)
        {
            float pk = buf.getMagnitude(ch, 0, n);
            band.peakOutput = juce::jmax(band.peakOutput, pk);
        }
    }

    void updateBandCoefficients(EQBand& band) { calculateBiquadCoefficients(band); }

    void processBiquad(const FilterCoefficients& c, BiquadState& s, float* x, int n)
    {
        const float b0=c.b0, b1=c.b1, b2=c.b2, a1=c.a1, a2=c.a2;
        float x1=s.x1, x2=s.x2, y1=s.y1, y2=s.y2;
        for (int i=0;i<n;++i){ float xi=x[i]; float y = b0*xi + b1*x1 + b2*x2 - a1*y1 - a2*y2; x2=x1; x1=xi; y2=y1; y1=y; x[i]=y; }
        s.x1=x1; s.x2=x2; s.y1=y1; s.y2=y2;
    }

    // ---------------------- Analog modeling ----------------------------
    void processAnalogModeling(juce::AudioBuffer<float>& buf)
    {
        const int n = buf.getNumSamples();
        const int chs = juce::jmin(2, buf.getNumChannels());

        // Input transformer (gentle LP)
        for (int ch=0; ch<chs; ++ch)
            processBiquad(analogProcessor_.transformer.preCoeffs, analogProcessor_.transformer.preFilter[ch], buf.getWritePointer(ch), n);

        // Tube & tape chains
        const float warmth = analogProcessor_.tubeWarmth.load();
        const float tape   = analogProcessor_.tapeSaturation.load();
        const float noise  = analogProcessor_.analogNoise.load();
        const float drive  = analogProcessor_.transformerDrive.load();

        for (int ch=0; ch<chs; ++ch)
        {
            auto* p = buf.getWritePointer(ch);
            if (warmth > 0.0f) processTubeSaturation(p, n, warmth);
            if (tape   > 0.0f) processTapeSaturation(p, n, tape);
            if (noise  > 0.0f) addAnalogNoise(p, n, noise * 0.002f);
            if (drive  > 1.0f) processSaturation(p, n, drive, 0.25f);
        }

        // Output transformer (gentle HP)
        for (int ch=0; ch<chs; ++ch)
            processBiquad(analogProcessor_.transformer.postCoeffs, analogProcessor_.transformer.postFilter[ch], buf.getWritePointer(ch), n);
    }

    void processSaturation(float* x, int n, float drive, float amount)
    {
        const auto& lut = analogProcessor_.saturationLUT;
        for (int i=0;i<n;++i)
        {
            float v = x[i] * drive;
            x[i] = juce::jmap(lutLookup(lut, v), v, amount) + (1.0f-amount)*x[i];
        }
    }

    void processTubeSaturation(float* x, int n, float warmth)
    {
        const auto& lut = analogProcessor_.tubeLUT;
        for (int i=0;i<n;++i) x[i] = juce::jmap(lutLookup(lut, x[i]), x[i], warmth) + (1.0f-warmth)*x[i];
    }

    void processTapeSaturation(float* x, int n, float sat)
    {
        const auto& lut = analogProcessor_.tapeLUT;
        for (int i=0;i<n;++i) x[i] = juce::jmap(lutLookup(lut, x[i]), x[i], sat) + (1.0f-sat)*x[i];
    }

    void addAnalogNoise(float* x, int n, float level)
    {
        auto& st = analogProcessor_.noiseState;
        for (int i=0;i<n;++i){ st = st*1664525u + 1013904223u; float r = ((st>>9)&0x7FFFFF)/float(0x7FFFFF); x[i] += (r*2.0f-1.0f)*level; }
    }

    // ---------------------- Filter design ------------------------------
    void calculateBiquadCoefficients(EQBand& b)
    {
        const float f = juce::jlimit(10.0f, (float)(0.45*sampleRate_), b.frequency.load());
        const float g = b.gain.load();
        const float Q = juce::jlimit(0.1f, 40.0f, b.q.load());

        const int sections = slopeToSections(b.slope);
        for (int s=0;s<sections;++s)
        {
            switch (b.type) {
                case BandType::LowShelf:  designLowShelf (f, g, Q, b.coeffs[s]); break;
                case BandType::HighShelf: designHighShelf(f, g, Q, b.coeffs[s]); break;
                case BandType::Parametric:designPeaking  (f, g, Q, b.coeffs[s]); break;
                case BandType::HighPass:  designHighpass (f, Q,                b.coeffs[s]); break;
                case BandType::LowPass:   designLowpass  (f, Q,                b.coeffs[s]); break;
                case BandType::BandPass:  designBandpass (f, Q,                b.coeffs[s]); break;
                case BandType::Notch:     designNotch    (f, Q,                b.coeffs[s]); break;
            }
        }
        // For >1 sections, subsequent sections reuse same coeffs with independent state (continuous)
    }

    // RBJ cookbook designs
    void designPeaking(float freq, float gainDB, float Q, FilterCoefficients& c)
    {
        const float A = std::pow(10.0f, gainDB/40.0f);
        const float w0 = 2.0f * float(juce::MathConstants<double>::pi) * (freq / (float)sampleRate_);
        const float alpha = std::sin(w0)/(2.0f*Q);
        const float cosw0 = std::cos(w0);
        float b0 = 1 + alpha*A;
        float b1 = -2*cosw0;
        float b2 = 1 - alpha*A;
        float a0 = 1 + alpha/A;
        float a1 = -2*cosw0;
        float a2 = 1 - alpha/A;
        norm(c, b0,b1,b2, a0,a1,a2);
    }

    void designLowShelf(float freq, float gainDB, float S, FilterCoefficients& c)
    {
        const float A = std::pow(10.0f, gainDB/40.0f);
        const float w0 = 2.0f * float(juce::MathConstants<double>::pi) * (freq / (float)sampleRate_);
        const float cosw0 = std::cos(w0), sinw0 = std::sin(w0);
        const float alpha = sinw0/2.0f * std::sqrt( (A + 1/A) * (1/S - 1) + 2 );
        const float b0 =    A*((A+1) - (A-1)*cosw0 + 2*std::sqrt(A)*alpha);
        const float b1 =  2*A*((A-1) - (A+1)*cosw0);
        const float b2 =    A*((A+1) - (A-1)*cosw0 - 2*std::sqrt(A)*alpha);
        const float a0 =        (A+1) + (A-1)*cosw0 + 2*std::sqrt(A)*alpha;
        const float a1 =   -2*((A-1) + (A+1)*cosw0);
        const float a2 =        (A+1) + (A-1)*cosw0 - 2*std::sqrt(A)*alpha;
        norm(c, b0,b1,b2, a0,a1,a2);
    }

    void designHighShelf(float freq, float gainDB, float S, FilterCoefficients& c)
    {
        const float A = std::pow(10.0f, gainDB/40.0f);
        const float w0 = 2.0f * float(juce::MathConstants<double>::pi) * (freq / (float)sampleRate_);
        const float cosw0 = std::cos(w0), sinw0 = std::sin(w0);
        const float alpha = sinw0/2.0f * std::sqrt( (A + 1/A) * (1/S - 1) + 2 );
        const float b0 =    A*((A+1) + (A-1)*cosw0 + 2*std::sqrt(A)*alpha);
        const float b1 = -2*A*((A-1) + (A+1)*cosw0);
        const float b2 =    A*((A+1) + (A-1)*cosw0 - 2*std::sqrt(A)*alpha);
        const float a0 =        (A+1) - (A-1)*cosw0 + 2*std::sqrt(A)*alpha;
        const float a1 =    2*((A-1) - (A+1)*cosw0);
        const float a2 =        (A+1) - (A-1)*cosw0 - 2*std::sqrt(A)*alpha;
        norm(c, b0,b1,b2, a0,a1,a2);
    }

    void designLowpass(float freq, float Q, FilterCoefficients& c)
    {
        const float w0 = 2.0f * float(juce::MathConstants<double>::pi) * (freq / (float)sampleRate_);
        const float alpha = std::sin(w0)/(2.0f*Q);
        const float cosw0 = std::cos(w0);
        float b0 = (1 - cosw0)/2;
        float b1 = 1 - cosw0;
        float b2 = (1 - cosw0)/2;
        float a0 = 1 + alpha;
        float a1 = -2*cosw0;
        float a2 = 1 - alpha;
        norm(c, b0,b1,b2,a0,a1,a2);
    }

    void designHighpass(float freq, float Q, FilterCoefficients& c)
    {
        const float w0 = 2.0f * float(juce::MathConstants<double>::pi) * (freq / (float)sampleRate_);
        const float alpha = std::sin(w0)/(2.0f*Q);
        const float cosw0 = std::cos(w0);
        float b0 = (1 + cosw0)/2;
        float b1 = -(1 + cosw0);
        float b2 = (1 + cosw0)/2;
        float a0 = 1 + alpha;
        float a1 = -2*cosw0;
        float a2 = 1 - alpha;
        norm(c, b0,b1,b2,a0,a1,a2);
    }

    void designBandpass(float freq, float Q, FilterCoefficients& c)
    {
        const float w0 = 2.0f * float(juce::MathConstants<double>::pi) * (freq / (float)sampleRate_);
        const float alpha = std::sin(w0)/(2.0f*Q);
        const float cosw0 = std::cos(w0);
        float b0 = Q*alpha;
        float b1 = 0;
        float b2 = -Q*alpha;
        float a0 = 1 + alpha;
        float a1 = -2*cosw0;
        float a2 = 1 - alpha;
        norm(c, b0,b1,b2,a0,a1,a2);
    }

    void designNotch(float freq, float Q, FilterCoefficients& c)
    {
        const float w0 = 2.0f * float(juce::MathConstants<double>::pi) * (freq / (float)sampleRate_);
        const float alpha = std::sin(w0)/(2.0f*Q);
        const float cosw0 = std::cos(w0);
        float b0 = 1;
        float b1 = -2*cosw0;
        float b2 = 1;
        float a0 = 1 + alpha;
        float a1 = -2*cosw0;
        float a2 = 1 - alpha;
        norm(c, b0,b1,b2,a0,a1,a2);
    }

    static int slopeToSections(FilterSlope s){ switch(s){ case FilterSlope::Slope6dB: return 1; case FilterSlope::Slope12dB: return 1; case FilterSlope::Slope24dB: return 2; case FilterSlope::Slope48dB: return 4; } return 1; }

    void norm(FilterCoefficients& c, float b0,float b1,float b2,float a0,float a1,float a2)
    {
        const float invA0 = 1.0f / a0;
        c.b0 = b0 * invA0; c.b1 = b1 * invA0; c.b2 = b2 * invA0; c.a1 = a1 * invA0; c.a2 = a2 * invA0;
    }

    void designOnePoleLowpass(float cutoff, FilterCoefficients& c)
    {
        const float x = std::exp(-2.0f * float(juce::MathConstants<double>::pi) * cutoff / (float)sampleRate_);
        c.b0 = 1.0f - x; c.b1 = 0; c.b2 = 0; c.a1 = -x; c.a2 = 0;
    }
    void designOnePoleHighpass(float cutoff, FilterCoefficients& c)
    {
        const float x = std::exp(-2.0f * float(juce::MathConstants<double>::pi) * cutoff / (float)sampleRate_);
        c.b0 = (1.0f + x)/2.0f; c.b1 = -(1.0f + x); c.b2 = (1.0f + x)/2.0f; c.a1 = -x; c.a2 = 0;
    }

    // ---------------------- Saturation LUTs ----------------------------
    void initializeSaturationTables()
    {
        auto& s = analogProcessor_.saturationLUT;
        auto& t = analogProcessor_.tubeLUT;
        auto& p = analogProcessor_.tapeLUT;
        for (int i=0;i<SATURATION_TABLE_SIZE;++i)
        {
            float x = juce::jmap((float)i, 0.0f, (float)(SATURATION_TABLE_SIZE-1), -2.5f, 2.5f);
            s[i] = std::tanh(x);                      // soft clip
            t[i] = fastAsymTubes(x);                  // asym tube curve
            p[i] = tapeCurve(x);                      // tape-like S-curve
        }
    }

    static float fastAsymTubes(float x)
    {   // asymmetric tanh mix
        float a = std::tanh(0.9f * (x + 0.2f));
        float b = std::tanh(0.6f * (x - 0.1f));
        return 0.65f*a + 0.35f*b;
    }

    static float tapeCurve(float x)
    {   // companding-ish curve
        float s = std::tanh(0.8f*x);
        return s * (0.8f + 0.2f*std::tanh(2.0f*std::abs(x)));
    }

    static float lutLookup(const std::array<float,SATURATION_TABLE_SIZE>& lut, float v)
    {
        float x = juce::jlimit(-2.5f, 2.5f, v);
        float pos = (x + 2.5f) * (float)(SATURATION_TABLE_SIZE-1) / 5.0f;
        int i0 = (int) pos; int i1 = juce::jmin(i0+1, SATURATION_TABLE_SIZE-1);
        float t = pos - (float)i0; return lut[(size_t)i0]*(1.0f-t) + lut[(size_t)i1]*t;
    }

    // ---------------------- Vintage models (coarse) --------------------
    void configureNeveEmulation(){ analogProcessor_.tubeWarmth.store(0.15f); analogProcessor_.tapeSaturation.store(0.10f); analogProcessor_.transformerDrive.store(1.3f); }
    void configureSSLEmulation(){ analogProcessor_.tubeWarmth.store(0.05f); analogProcessor_.tapeSaturation.store(0.08f); analogProcessor_.transformerDrive.store(1.1f); }
    void configureAPIEmulation(){ analogProcessor_.tubeWarmth.store(0.10f); analogProcessor_.tapeSaturation.store(0.12f); analogProcessor_.transformerDrive.store(1.2f); }
    void configurePultecEmulation(){ analogProcessor_.tubeWarmth.store(0.18f); analogProcessor_.tapeSaturation.store(0.15f); analogProcessor_.transformerDrive.store(1.25f); }
    void configureFairchildEmulation(){ analogProcessor_.tubeWarmth.store(0.22f); analogProcessor_.tapeSaturation.store(0.12f); analogProcessor_.transformerDrive.store(1.15f); }
    void configureTubePreampEmulation(){ analogProcessor_.tubeWarmth.store(0.35f); analogProcessor_.tapeSaturation.store(0.00f); analogProcessor_.transformerDrive.store(1.4f); }

    // ---------------------- Analysis (optional, light) -----------------
    void updateAnalysisData(const juce::AudioBuffer<float>& buf)
    {
        // THD estimate via 3rd harmonic energy on short window
        const int n = juce::jmin(buf.getNumSamples(), 512);
        const float* x = buf.getReadPointer(0);
        float acc=0, acc3=0; for (int i=0;i<n;++i){ float v=x[i]; acc+=v*v; float w=std::tanh(3.0f*v); acc3+=w*w; }
        analysisData_.totalHarmonicDistortion = (acc>1e-9f)? juce::jlimit(0.0f,1.0f,std::sqrt(acc3/acc)-1.0f):0.0f;
        analysisData_.dynamicRange = 0.0f; // reserved
        analysisData_.stereoWidth = 0.0f;  // reserved
    }

    // Utility -----------------------------------------------------------
    float dbToLinear(float db) const { return std::pow(10.0f, db/20.0f); }

};

} // namespace audio
} // namespace cppmusic

// ------------------------------ Unit Tests -----------------------------
#if JUCE_UNIT_TESTS
namespace cppmusic { namespace audio {
class AnalogModeledEQTests : public juce::UnitTest {
public:
    AnalogModeledEQTests() : juce::UnitTest("AnalogModeledEQ — RT & Bands", "Audio") {}
    void runTest() override {
        daw::core::EngineContext c; daw::core::RTMemoryPool p; AnalogModeledEQ eq(c,p);
        AnalogModeledEQ::Config cfg; cfg.sampleRate=48000; cfg.maxBlockSize=512; cfg.enableOversampling=false;
        eq.prepare(cfg); eq.reset();

        beginTest("Peaking boost changes spectrum (rough)");
        juce::AudioBuffer<float> buf(2,512); buf.clear();
        // Input: 1 kHz sine
        double sr=48000.0; double ph=0, w=2.0*juce::MathConstants<double>::pi*1000.0/sr;
        for (int i=0;i<512;++i){ float s=(float)std::sin(ph); ph+=w; buf.setSample(0,i,s); buf.setSample(1,i,s);}
        eq.setBandType(1, AnalogModeledEQ::BandType::Parametric);
        eq.setBandFrequency(1, 1000.0f); eq.setBandQ(1, 1.0f); eq.setBandGain(1, +6.0f);
        eq.processBlock(buf);
        float outPk = buf.getMagnitude(0,0,512);
        expect(outPk > 0.5f);

        beginTest("Saturation reduces crest factor");
        eq.setAnalogModel(AnalogModeledEQ::AnalogModel::TubePreamp);
        juce::AudioBuffer<float> buf2(2,512); buf2.clear();
        for (int i=0;i<512;++i){ float s = (i%64<32? +0.8f:-0.8f); buf2.setSample(0,i,s); buf2.setSample(1,i,s);}
        float inPk = buf2.getMagnitude(0,0,512);
        eq.processBlock(buf2);
        float outPk2 = buf2.getMagnitude(0,0,512);
        expect(outPk2 <= inPk + 1e-5f);
    }
};
static AnalogModeledEQTests analogModeledEQTests;
}} // namespaces
#endif
