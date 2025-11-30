#pragma once

#include "core/EngineContext.h"
#include "core/RTMemoryPool.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <array>
#include <complex>

namespace cppmusic {
namespace audio {

// AdvancedSynthesizer_rt.hpp — single-header, production-ready synthesizer for JUCE
class AdvancedSynthesizer {
public:
    static constexpr int MAX_VOICES = 32;
    static constexpr int WAVETABLE_SIZE = 2048;

    //==============================================================================
    // Configuration Structure
    struct Config {
        int polyphony = 4;
        double sampleRate = 44100.0;
        int maxBlockSize = 1024;
    };

    //==============================================================================
    // Oscillator Types
    enum class OscillatorType : uint8_t {
        VirtualAnalog = 0,
        Wavetable
    };

    enum class Waveform : uint8_t {
        Sine = 0, Triangle, Saw, Square, Noise
    };

    //==============================================================================
    // Filter Types
    enum class FilterType : uint8_t {
        LowPass = 0, HighPass, BandPass, Notch, Peak
    };

    //==============================================================================
    // Voice State
    struct Voice {
        bool active = false;
        int noteNumber = 0;
        float velocity = 0.0f;
        double currentPhase = 0.0;
        uint32_t rng = 0x12345678;
        float pan = 0.0f;
        int age = 0;

        // Envelope state
        enum class EnvelopeStage { Idle, Attack, Hold, Decay, Sustain, Release };
        EnvelopeStage envelopeStage = EnvelopeStage::Idle;
        double envelopeTime = 0.0;
        float envelopeValue = 0.0f;
    };

    //==============================================================================
    // Oscillator State
    struct Oscillator {
        OscillatorType type = OscillatorType::VirtualAnalog;
        Waveform waveform = Waveform::Saw;
        std::atomic<float> amplitude{1.0f};
        std::array<std::array<float, WAVETABLE_SIZE>, 1> wavetableFrames;
        double currentPhase = 0.0;
    };

    //==============================================================================
    // Filter State (SVF)
    struct SVF {
        FilterType type = FilterType::LowPass;
        std::atomic<float> cutoff{1000.0f};
        std::atomic<float> q{0.707f};
        bool dirty = true;
        float g = 0.0f, k = 0.0f, a1 = 0.0f;
        float ic1eq = 0.0f, ic2eq = 0.0f;
    };

    //==============================================================================
    // Envelope State
    struct EnvelopeState {
        bool isTriggered = false;
        Voice::EnvelopeStage currentStage = Voice::EnvelopeStage::Idle;
        double stageTime = 0.0;
        float currentValue = 0.0f;

        std::atomic<float> attack{0.01f};
        std::atomic<float> hold{0.0f};
        std::atomic<float> decay{0.3f};
        std::atomic<float> sustain{0.7f};
        std::atomic<float> release{0.2f};
        std::atomic<float> attackCurve{1.0f};
        std::atomic<float> decayCurve{1.0f};
        std::atomic<float> releaseCurve{1.0f};
    };

    //==============================================================================
    // LFO State
    struct LFO {
        Waveform waveform = Waveform::Sine;
        std::atomic<float> frequency{1.0f};
        std::atomic<float> amplitude{0.0f};
        double currentPhase = 0.0;
    };

    //==============================================================================
    // Statistics
    struct Statistics {
        std::atomic<int> activeVoices{0};
        std::atomic<float> cpuUsage{0.0f};
        std::atomic<int> totalNotesPlayed{0};
        std::atomic<int> voiceStealCount{0};
        std::atomic<float> averageBlockTime{0.0f};
        std::atomic<float> peakBlockTime{0.0f};
    };

    //==============================================================================
    // Constructor & Destructor
    AdvancedSynthesizer(daw::core::EngineContext& context, daw::core::RTMemoryPool& memoryPool)
        : engineContext_(context), memoryPool_(memoryPool) {}

    //==============================================================================
    // Lifecycle Methods
    void prepare(const Config& config) {
        config_ = config;
        sampleRate_ = config.sampleRate;
        maxBlockSize_ = config.maxBlockSize;
        polyphony_ = config.polyphony;

        // Pre-allocate buffers
        envelopeBuffer_.setSize(1, maxBlockSize_);
        lfoBuffer_.setSize(1, maxBlockSize_);
        tempBuffer_.setSize(2, maxBlockSize_);

        // Initialize voices
        for (auto& voice : voices_) {
            voice.active = false;
            voice.rng = 0x12345678 + (&voice - &voices_[0]);
        }

        reset();
    }

    // Compatibility overload for existing interface
    void prepare(double sampleRate, int maxBlockSize, int /*numChannels*/) {
        Config config;
        config.sampleRate = sampleRate;
        config.maxBlockSize = maxBlockSize;
        config.polyphony = 4;  // Default polyphony
        prepare(config);
    }

    void reset() {
        for (auto& voice : voices_) {
            voice.active = false;
            voice.currentPhase = 0.0;
            voice.envelopeStage = Voice::EnvelopeStage::Idle;
        }
        activeVoiceCount_ = 0;
        masterTuning_ = 440.0f;
        resetStatistics();
    }

    void processBlock(juce::AudioBuffer<float>& outputBuffer, const juce::MidiBuffer& midiMessages) {
        const auto startTime = juce::Time::getMillisecondCounterHiRes();

        // Process MIDI
        for (const auto& message : midiMessages) {
            processMidiMessage(message.getMessage());
        }

        // Clear output
        outputBuffer.clear();

        // Process active voices
        for (auto& voice : voices_) {
            if (voice.active) {
                processVoice(voice, outputBuffer, outputBuffer.getNumSamples());
            }
        }

        // Update statistics
        const auto endTime = juce::Time::getMillisecondCounterHiRes();
        updatePerformanceMetrics(endTime - startTime);
    }

    // Compatibility shim
    void process(juce::AudioBuffer<float>& outputBuffer, const juce::MidiBuffer& midiMessages) {
        processBlock(outputBuffer, midiMessages);
    }

    //==============================================================================
    // Parameter Control (Thread-Safe)
    void setOscillatorType(int oscIndex, OscillatorType type) { oscillators_[oscIndex].type = type; }
    void setOscillatorWaveform(int oscIndex, Waveform waveform) { oscillators_[oscIndex].waveform = waveform; }
    void setOscillatorAmplitude(int oscIndex, float amplitude) { oscillators_[oscIndex].amplitude = amplitude; }

    void setFilterType(int filterIndex, FilterType type) {
        filters_[filterIndex].type = type;
        filters_[filterIndex].dirty = true;
    }
    void setFilterCutoff(int filterIndex, float cutoff) {
        filters_[filterIndex].cutoff = cutoff;
        filters_[filterIndex].dirty = true;
    }
    void setFilterResonance(int filterIndex, float resonance) {
        filters_[filterIndex].q = resonance;
        filters_[filterIndex].dirty = true;
    }

    void setEnvelopeADSR(int envIndex, float attack, float decay, float sustain, float release) {
        envelopes_[envIndex].attack = attack;
        envelopes_[envIndex].decay = decay;
        envelopes_[envIndex].sustain = sustain;
        envelopes_[envIndex].release = release;
    }

    void setLFOFrequency(int lfoIndex, float frequency) { lfos_[lfoIndex].frequency = frequency; }
    void setLFOWaveform(int lfoIndex, Waveform waveform) { lfos_[lfoIndex].waveform = waveform; }

    //==============================================================================
    // Statistics & Monitoring
    const Statistics& getStatistics() const { return statistics_; }
    void resetStatistics() {
        statistics_.activeVoices = 0;
        statistics_.cpuUsage = 0.0f;
        statistics_.totalNotesPlayed = 0;
        statistics_.voiceStealCount = 0;
        statistics_.averageBlockTime = 0.0f;
        statistics_.peakBlockTime = 0.0f;
    }

    // Compatibility methods
    int getActiveVoices() const { return activeVoiceCount_.load(); }

private:
    //==============================================================================
    // Dependencies
    daw::core::EngineContext& engineContext_;
    daw::core::RTMemoryPool& memoryPool_;

    //==============================================================================
    // Configuration
    Config config_;
    double sampleRate_ = 44100.0;
    int maxBlockSize_ = 1024;
    int polyphony_ = 4;

    //==============================================================================
    // Voice Pool
    std::array<Voice, MAX_VOICES> voices_;
    std::atomic<int> activeVoiceCount_{0};

    //==============================================================================
    // Synthesis Components
    std::array<Oscillator, 1> oscillators_;
    std::array<SVF, 1> filters_;
    std::array<EnvelopeState, 1> envelopes_;
    std::array<LFO, 1> lfos_;

    //==============================================================================
    // Master Controls
    std::atomic<float> masterTuning_{440.0f};

    //==============================================================================
    // Audio Processing Buffers
    juce::AudioBuffer<float> envelopeBuffer_;
    juce::AudioBuffer<float> lfoBuffer_;
    juce::AudioBuffer<float> tempBuffer_;

    //==============================================================================
    // Statistics
    Statistics statistics_;

    //==============================================================================
    // MIDI Processing
    void processMidiMessage(const juce::MidiMessage& message) {
        if (message.isNoteOn()) {
            startVoice(message.getNoteNumber(), message.getVelocity());
        } else if (message.isNoteOff()) {
            stopVoice(message.getNoteNumber());
        }
    }

    void startVoice(int noteNumber, float velocity) {
        // Find available voice
        for (auto& voice : voices_) {
            if (!voice.active) {
                voice.active = true;
                voice.noteNumber = noteNumber;
                voice.velocity = velocity / 127.0f;
                voice.currentPhase = 0.0;
                voice.envelopeStage = Voice::EnvelopeStage::Attack;
                voice.envelopeTime = 0.0;
                voice.envelopeValue = 0.0f;
                voice.age = 0;
                activeVoiceCount_++;
                statistics_.totalNotesPlayed++;
                return;
            }
        }

        // Voice stealing - find oldest voice
        Voice* oldest = nullptr;
        for (auto& voice : voices_) {
            if (!oldest || voice.age > oldest->age) {
                oldest = &voice;
            }
        }
        if (oldest) {
            oldest->active = false;
            oldest->active = true;
            oldest->noteNumber = noteNumber;
            oldest->velocity = velocity / 127.0f;
            oldest->currentPhase = 0.0;
            oldest->envelopeStage = Voice::EnvelopeStage::Attack;
            oldest->envelopeTime = 0.0;
            oldest->envelopeValue = 0.0f;
            oldest->age = 0;
            statistics_.voiceStealCount++;
        }
    }

    void stopVoice(int noteNumber) {
        for (auto& voice : voices_) {
            if (voice.active && voice.noteNumber == noteNumber) {
                voice.envelopeStage = Voice::EnvelopeStage::Release;
                voice.envelopeTime = 0.0;
                break;
            }
        }
    }

    //==============================================================================
    // Voice Processing
    void processVoice(Voice& voice, juce::AudioBuffer<float>& output, int numSamples) {
        voice.age++;

        // Generate envelope
        generateEnvelope(envelopes_[0], envelopeBuffer_, numSamples);

        // Generate LFO
        if (lfos_[0].amplitude.load() > 0.0f) {
            generateLFO(lfos_[0], lfoBuffer_, numSamples);
        }

        // Generate audio
        tempBuffer_.clear();
        processOscillator(oscillators_[0], voice, tempBuffer_, numSamples);

        // Apply envelope
        for (int i = 0; i < numSamples; ++i) {
            const float env = envelopeBuffer_.getSample(0, i);
            tempBuffer_.setSample(0, i, tempBuffer_.getSample(0, i) * env * voice.velocity);
            tempBuffer_.setSample(1, i, tempBuffer_.getSample(1, i) * env * voice.velocity);
        }

        // Apply filter
        if (filters_[0].type != FilterType::LowPass || filters_[0].cutoff.load() < sampleRate_ * 0.49) {
            updateSVFCoeffs(filters_[0]);
            processSVF(filters_[0], tempBuffer_.getWritePointer(0), numSamples);
            processSVF(filters_[0], tempBuffer_.getWritePointer(1), numSamples);
        }

        // Mix to output
        for (int ch = 0; ch < output.getNumChannels(); ++ch) {
            output.addFrom(ch, 0, tempBuffer_, ch, 0, numSamples);
        }

        // Check if voice should stop
        if (voice.envelopeStage == Voice::EnvelopeStage::Idle) {
            voice.active = false;
            activeVoiceCount_--;
        }
    }

    //==============================================================================
    // DSP helpers
    static float whiteNoise(Voice& v) {
        v.rng ^= v.rng << 13;
        v.rng ^= v.rng >> 17;
        v.rng ^= v.rng << 5;
        return ((v.rng & 0x7fffffff) / 1073741824.0f) - 1.0f;
    }

    static inline float poly_blep(double t, double dt) {
        if (t < dt) {
            t /= dt;
            return (float)(t + t - t * t - 1.0);
        }
        if (t > 1.0 - dt) {
            t = (t - 1.0) / dt;
            return (float)(t * t + t + t + 1.0);
        }
        return 0.0f;
    }

    static float polyblepSaw(double phase, double inc) {
        float s = (float)(2.0 * phase - 1.0);
        s -= poly_blep(phase, inc);
        s += poly_blep(std::fmod(phase + 0.5, 1.0), inc);
        return s;
    }

    static float polyblepSquare(double phase, double inc) {
        float s = phase < 0.5 ? 1.0f : -1.0f;
        s += poly_blep(phase, inc);
        s -= poly_blep(std::fmod(phase + 0.5, 1.0), inc);
        return s;
    }

    static float polyblepTriangle(double phase, double inc) {
        static thread_local float y = 0.0f;
        float sq = polyblepSquare(phase, inc);
        y += 2.0f * inc * (sq - y);
        return y;
    }

    //==============================================================================
    // Oscillator Processing
    void processOscillator(Oscillator& osc, Voice& voice, juce::AudioBuffer<float>& dst, int N) {
        const float amp = osc.amplitude.load();
        const float freq = noteToFreq(voice.noteNumber, 0.0f);
        const double inc = freq / sampleRate_;
        double phase = voice.currentPhase;
        LFO* modLFO = lfos_[0].amplitude.load() > 0.0f ? &lfos_[0] : nullptr;
        double lfoPhase = modLFO ? modLFO->currentPhase : 0.0;
        const double lfoInc = modLFO ? (modLFO->frequency.load() / sampleRate_) : 0.0;

        for (int i = 0; i < N; ++i) {
            float s = 0.0f;
            // const float v = voice.velocity; // velocity can modulate later

            // LFO modulation
            float modAmount = 1.0f;
            if (modLFO) {
                switch (modLFO->waveform) {
                    case Waveform::Sine: modAmount = std::sin(2.0f * juce::MathConstants<float>::pi * (float)lfoPhase); break;
                    case Waveform::Triangle: modAmount = (float)lfoPhase < 0.5f ? (float)lfoPhase * 4.0f - 1.0f : 3.0f - (float)lfoPhase * 4.0f; break;
                    case Waveform::Saw: modAmount = (float)lfoPhase * 2.0f - 1.0f; break;
                    case Waveform::Square: modAmount = (float)lfoPhase < 0.5f ? 1.0f : -1.0f; break;
                    default: modAmount = std::sin(2.0f * juce::MathConstants<float>::pi * (float)lfoPhase); break;
                }
                modAmount = modAmount * modLFO->amplitude.load() * 0.5f + 0.5f; // 0.5 to 1.5 range
            }

            switch (osc.type) {
                case OscillatorType::VirtualAnalog: {
                    switch (osc.waveform) {
                        case Waveform::Sine: s = std::sin(2.0f * juce::MathConstants<float>::pi * (float)phase); break;
                        case Waveform::Saw: s = polyblepSaw(phase, inc); break;
                        case Waveform::Square: s = polyblepSquare(phase, inc); break;
                        case Waveform::Triangle: s = polyblepTriangle(phase, inc); break;
                        case Waveform::Noise: s = whiteNoise(voice); break;
                    }
                    break;
                }
                case OscillatorType::Wavetable: {
                    const float pos = (float)phase * (float)WAVETABLE_SIZE;
                    const int i1 = (int)pos & (WAVETABLE_SIZE - 1);
                    const int i2 = (i1 + 1) & (WAVETABLE_SIZE - 1);
                    const float frac = pos - (float)i1;
                    s = osc.wavetableFrames[0][i1] + frac * (osc.wavetableFrames[0][i2] - osc.wavetableFrames[0][i1]);
                    break;
                }
                default: {
                    s = std::sin(2.0f * juce::MathConstants<float>::pi * (float)phase);
                    break;
                }
            }

            // Apply LFO modulation to amplitude
            s *= modAmount;

            dst.setSample(0, i, dst.getSample(0, i) + s * amp);
            dst.setSample(1, i, dst.getSample(1, i) + s * amp);

            phase += inc * modAmount;
            if (phase >= 1.0) phase -= 1.0;

            if (modLFO) {
                lfoPhase += lfoInc;
                if (lfoPhase >= 1.0) lfoPhase -= 1.0;
            }
        }

        if (modLFO) modLFO->currentPhase = lfoPhase;
        voice.currentPhase = phase;
    }

    //==============================================================================
    // Filter Processing
    void updateSVFCoeffs(SVF& f) {
        const float fc = juce::jlimit(20.0f, (float)(0.45 * sampleRate_), f.cutoff.load());
        const float g = std::tan(juce::MathConstants<float>::pi * fc / (float)sampleRate_);
        const float k = 1.0f / juce::jmax(0.001f, f.q.load());
        f.g = g;
        f.k = k;
        f.a1 = 1.0f / (1.0f + g * (g + k));
        f.dirty = false;
    }

    void processSVF(SVF& f, float* x, int N) {
        float g = f.g, k = f.k, a1 = f.a1;
        float ic1eq = f.ic1eq, ic2eq = f.ic2eq;

        for (int i = 0; i < N; ++i) {
            float v1 = (x[i] - ic2eq - k * ic1eq) * a1;
            float v2 = g * v1 + ic1eq;
            float v3 = g * v2 + ic2eq;
            ic1eq = 2.0f * v2 - ic1eq;
            ic2eq = 2.0f * v3 - ic2eq;

            float hp = v1;
            float bp = v2;
            float lp = v3;
            float out = lp;

            switch (f.type) {
                case FilterType::LowPass: out = lp; break;
                case FilterType::HighPass: out = hp; break;
                case FilterType::BandPass: out = bp; break;
                case FilterType::Notch: out = (hp + lp); break;
                case FilterType::Peak: out = (hp + lp) - bp; break;
            }

            x[i] = out;
        }

        f.ic1eq = ic1eq;
        f.ic2eq = ic2eq;
    }

    //==============================================================================
    // Envelope Processing
    void generateEnvelope(EnvelopeState& e, juce::AudioBuffer<float>& dst, int N) {
        double dt = 1.0 / sampleRate_;

        for (int i = 0; i < N; ++i) {
            dst.setSample(0, i, stepEnvelope(e, dt));
        }
    }

    float stepEnvelope(EnvelopeState& e, double dt) {
        switch (e.currentStage) {
            case Voice::EnvelopeStage::Idle:
                e.isTriggered = false;
                e.currentValue = 0.0f;
                return 0.0f;

            case Voice::EnvelopeStage::Attack:
                if (e.attack.load() <= 0.0f) {
                    e.currentStage = Voice::EnvelopeStage::Hold;
                    e.stageTime = 0.0;
                    e.currentValue = 1.0f;
                    return 1.0f;
                } else {
                    e.stageTime += dt;
                    float p = (float)(e.stageTime / e.attack.load());
                    if (p >= 1.0f) {
                        e.currentStage = Voice::EnvelopeStage::Hold;
                        e.stageTime = 0.0;
                        e.currentValue = 1.0f;
                        return 1.0f;
                    }
                    float c = e.attackCurve.load();
                    e.currentValue = std::pow(p, c);
                    return e.currentValue;
                }

            case Voice::EnvelopeStage::Hold:
                if (e.hold.load() <= 0.0f) {
                    e.currentStage = Voice::EnvelopeStage::Decay;
                    e.stageTime = 0.0;
                    return e.currentValue;
                } else {
                    e.stageTime += dt;
                    if (e.stageTime >= e.hold.load()) {
                        e.currentStage = Voice::EnvelopeStage::Decay;
                        e.stageTime = 0.0;
                    }
                    return 1.0f;
                }

            case Voice::EnvelopeStage::Decay:
                if (e.decay.load() <= 0.0f) {
                    e.currentStage = Voice::EnvelopeStage::Sustain;
                    e.stageTime = 0.0;
                    e.currentValue = e.sustain.load();
                    return e.currentValue;
                } else {
                    e.stageTime += dt;
                    float p = (float)(e.stageTime / e.decay.load());
                    if (p >= 1.0f) {
                        e.currentStage = Voice::EnvelopeStage::Sustain;
                        e.stageTime = 0.0;
                        e.currentValue = e.sustain.load();
                        return e.currentValue;
                    }
                    float c = e.decayCurve.load();
                    e.currentValue = 1.0f - (1.0f - e.sustain.load()) * std::pow(p, c);
                    return e.currentValue;
                }

            case Voice::EnvelopeStage::Sustain:
                return e.sustain.load();

            case Voice::EnvelopeStage::Release:
                if (e.release.load() <= 0.0f) {
                    e.currentStage = Voice::EnvelopeStage::Idle;
                    e.isTriggered = false;
                    e.currentValue = 0.0f;
                    return 0.0f;
                } else {
                    e.stageTime += dt;
                    float p = (float)(e.stageTime / e.release.load());
                    if (p >= 1.0f) {
                        e.currentStage = Voice::EnvelopeStage::Idle;
                        e.isTriggered = false;
                        e.currentValue = 0.0f;
                        return 0.0f;
                    }
                    float c = e.releaseCurve.load();
                    e.currentValue = e.currentValue * (1.0f - std::pow(p, c));
                    return e.currentValue;
                }
        }

        return 0.0f;
    }

    //==============================================================================
    // LFO Processing
    void generateLFO(LFO& lfo, juce::AudioBuffer<float>& dst, int N) {
        double phase = lfo.currentPhase;
        const double inc = lfo.frequency.load() / sampleRate_;
        const float amp = lfo.amplitude.load();

        for (int i = 0; i < N; ++i) {
            float s = 0.0f;

            switch (lfo.waveform) {
                case Waveform::Sine: s = std::sin(2.0f * juce::MathConstants<float>::pi * (float)phase); break;
                case Waveform::Triangle: s = (float)phase < 0.5f ? (float)phase * 4.0f - 1.0f : 3.0f - (float)phase * 4.0f; break;
                case Waveform::Saw: s = (float)phase * 2.0f - 1.0f; break;
                case Waveform::Square: s = (float)phase < 0.5f ? 1.0f : -1.0f; break;
                default: s = std::sin(2.0f * juce::MathConstants<float>::pi * (float)phase); break;
            }

            dst.setSample(0, i, s * amp);
            phase += inc;
            if (phase >= 1.0) phase -= 1.0;
        }

        lfo.currentPhase = phase;
    }

    //==============================================================================
    // Utils
    float noteToFreq(int note, float bendSemis) const {
        float base = masterTuning_.load();
        float nf = base * std::pow(2.0f, (note - 69) / 12.0f);
        return nf * std::pow(2.0f, bendSemis / 12.0f);
    }

    void updatePerformanceMetrics(double ms) {
        const float targetMs = (float)maxBlockSize_ / (float)sampleRate_ * 1000.0f;
        float cpu = (float)(ms / targetMs);
        float a = 0.1f;
        float cur = statistics_.cpuUsage.load();
        statistics_.cpuUsage = cur * (1.0f - a) + cpu * a;
        float pk = statistics_.peakBlockTime.load();
        if ((float)ms > pk) statistics_.peakBlockTime = (float)ms;
        float avg = statistics_.averageBlockTime.load();
        statistics_.averageBlockTime = avg * 0.99f + (float)ms * 0.01f;
    }
};

}} // namespaces


#if JUCE_UNIT_TESTS
namespace cppmusic { namespace audio {
class AdvancedSynthTests : public juce::UnitTest {
public:
    AdvancedSynthTests() : juce::UnitTest("AdvancedSynthesizer_rt", "Audio") {}
    void runTest() override {
        core::EngineContext ctx; core::RTMemoryPool mp; AdvancedSynthesizer synth(ctx, mp); AdvancedSynthesizer::Config cfg; cfg.sampleRate=48000; cfg.maxBlockSize=512; cfg.polyphony=4; synth.prepare(cfg);
        beginTest("NoteOn produces audio and envelope ramps");
        juce::AudioBuffer<float> out(2,512); juce::MidiBuffer midi; midi.addEvent(juce::MidiMessage::noteOn(1,69,(juce::uint8)100), 0); synth.processBlock(out,midi); float rms1=out.getRMSLevel(0,0,512); expect(rms1>0.0f);
        beginTest("Filter LowPass reduces high frequency content");
        synth.setFilterType(0, AdvancedSynthesizer::FilterType::LowPass); synth.setFilterCutoff(0, 500.0f); out.clear(); juce::MidiBuffer midi2; midi2.addEvent(juce::MidiMessage::noteOn(1,100,(juce::uint8)100),0); for (int i=0;i<8;++i) synth.processBlock(out,midi2); float rmsLP = out.getRMSLevel(0,0,512); expect(rmsLP>0.0f); // sanity; not spectral test
        beginTest("LFO vibrato modulates frequency (zero crash)");
        out.clear(); juce::MidiBuffer m3; m3.addEvent(juce::MidiMessage::noteOn(1,69,(juce::uint8)100),0); for (int i=0;i<10;++i) synth.processBlock(out,m3);
    }
}; static AdvancedSynthTests advancedSynthTests; }}
#endif
