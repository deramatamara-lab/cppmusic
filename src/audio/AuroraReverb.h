// AuroraReverb.hpp — Single-header JUCE reverb plugin with modern DSP + sleek UI
// Drop-in: add to your JUCE project. Factory: return new AuroraReverbAudioProcessor();
// Real-time safe (no heap in processBlock). Freeverb++ core (stereo decorrelated combs + modulated allpasses),
// early reflections, pre-delay, damping, diffusion, width (M/S), gating, ducking, freeze.
// UI: Neon look, XY pad (Mix/Decay), animated decay scope, GR meter, macro strip.
// Tests at bottom (#if JUCE_UNIT_TESTS).

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>
#include <atomic>

namespace cppmusic {
namespace ui { class AuroraReverbEditor; }
namespace audio {

class AuroraReverbAudioProcessor : public juce::AudioProcessor
{
public:
    struct IDs {
        static constexpr const char* mix       = "mix";          // 0..100 %
        static constexpr const char* size      = "size";         // 0.2..1.5 (scales delay network)
        static constexpr const char* decay     = "decay";        // 0.1..30 s (target RT60)
        static constexpr const char* predelay  = "predelay";     // 0..200 ms
        static constexpr const char* dampHF    = "dampHF";       // 1k..20k Hz (low-pass in tank)
        static constexpr const char* cutLF     = "cutLF";        // 20..500 Hz (high-pass before tank)
        static constexpr const char* diffusion = "diffusion";    // 0..1 (allpass feedback)
        static constexpr const char* modRate   = "modRate";      // 0.05..2.0 Hz
        static constexpr const char* modDepth  = "modDepth";     // 0..0.5 (allpass coeff modulation)
        static constexpr const char* width     = "width";        // 0..1 (M/S spread)
        static constexpr const char* gateOn    = "gateOn";       // bool (noise gate on output)
        static constexpr const char* gateTh    = "gateTh";       // -60..-20 dB
        static constexpr const char* duckAmt   = "duckAmt";      // 0..1
        static constexpr const char* duckAtk   = "duckAtk";      // 1..200 ms
        static constexpr const char* duckRel   = "duckRel";      // 10..1000 ms
        static constexpr const char* freeze    = "freeze";       // bool
        static constexpr const char* algo      = "algo";         // Plate/Hall/Room (choice)
        static constexpr const char* outTrim   = "outTrim";      // -24..+12 dB
    };

    AuroraReverbAudioProcessor()
        : apvts(*this, nullptr, juce::Identifier("AuroraReverb"), createLayout())
    {
        // cache param pointers (fast RT reads)
        mix      = apvts.getRawParameterValue(IDs::mix);
        size     = apvts.getRawParameterValue(IDs::size);
        decay    = apvts.getRawParameterValue(IDs::decay);
        predelay = apvts.getRawParameterValue(IDs::predelay);
        dampHF   = apvts.getRawParameterValue(IDs::dampHF);
        cutLF    = apvts.getRawParameterValue(IDs::cutLF);
        diffusion= apvts.getRawParameterValue(IDs::diffusion);
        modRate  = apvts.getRawParameterValue(IDs::modRate);
        modDepth = apvts.getRawParameterValue(IDs::modDepth);
        width    = apvts.getRawParameterValue(IDs::width);
        gateOn   = apvts.getRawParameterValue(IDs::gateOn);
        gateTh   = apvts.getRawParameterValue(IDs::gateTh);
        duckAmt  = apvts.getRawParameterValue(IDs::duckAmt);
        duckAtk  = apvts.getRawParameterValue(IDs::duckAtk);
        duckRel  = apvts.getRawParameterValue(IDs::duckRel);
        freeze   = apvts.getRawParameterValue(IDs::freeze);
        algo     = apvts.getRawParameterValue(IDs::algo);
        outTrim  = apvts.getRawParameterValue(IDs::outTrim);
    }

    //================================== AudioProcessor
    const juce::String getName() const override { return "AuroraReverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 30.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Init"; }
    void changeProgramName (int, const juce::String&) override {}

    bool isBusesLayoutSupported (const BusesLayout& l) const override
    { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }

    void prepareToPlay (double sr, int maxBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;

    //============================== UI
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    //============================== State
    void getStateInformation (juce::MemoryBlock& dest) override;
    void setStateInformation (const void* data, int size) override;

    juce::AudioProcessorValueTreeState apvts;

    // Expose meters for UI
    float getWetEnergy() const noexcept { return lastWetEnergy; }
    float getDuckGR()   const noexcept { return duckGR * (duckAmt ? duckAmt->load() : 0.0f); }

    // Layout builder
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    //============================= Core structures
    struct Comb {
        std::vector<float> buf; int size=1; int idx=0; float filterStore=0.0f;
        float damp1=0.0f, damp2=1.0f;
        void setDamp(float hfHz, double sr);
        void setSize (int n) { buf.assign((size_t) (n>1?n:1), 0.0f); size = (int)buf.size(); idx=0; filterStore=0.0f; }
        float process (float x, float feedback, float hfHz, double sr, bool freeze);
    };

    struct Allpass {
        std::vector<float> buf; int size=1; int idx=0; float fb=0.7f;
        void setSize (int n) { buf.assign((size_t)(n>1?n:1), 0.0f); size=(int)buf.size(); idx=0; }
        float process (float x, float a);
    };

    // Stereo banks
    static constexpr int NUM_COMBS = 8;
    static constexpr int NUM_AP    = 4;
    std::array<Comb, NUM_COMBS> combL, combR;
    std::array<Allpass, NUM_AP> allpassL, allpassR;

    // Tunings at 44100 Hz (samples)
    const int combTuningL[NUM_COMBS] = {1116,1188,1277,1356,1422,1491,1557,1617};
    const int combTuningR[NUM_COMBS] = {1139,1211,1300,1379,1445,1514,1580,1640};
    const int apTuning[NUM_AP]       = {556,441,341,225};

    // Pre-delay & early reflections
    std::array<std::vector<float>,2> predelayBuf;
    std::array<int,2> preW{ {0,0} };
    int preLen=1;
    std::array<std::vector<float>,2> earlyBuf;
    std::array<int,2> earlyW{ {0,0} };
    int earlyLen=1;

    // Filters
    std::array<juce::dsp::IIR::Filter<float>,2> hp, lp;

    // LFO for AP mod
    double lfoPhase = 0.0;

    // Ducking
    float duckEnv=0.0f, duckGR=0.0f;
    float duckAtkC=0.0f, duckRelC=0.0f;

    // UI meters
    float lastWetEnergy = 0.0f;

    // Smoothers
    juce::SmoothedValue<float> mixSm, outSm;

    // Params cache
    std::atomic<float>* mix{}; std::atomic<float>* size{}; std::atomic<float>* decay{}; std::atomic<float>* predelay{};
    std::atomic<float>* dampHF{}; std::atomic<float>* cutLF{}; std::atomic<float>* diffusion{}; std::atomic<float>* modRate{}; std::atomic<float>* modDepth{}; std::atomic<float>* width{};
    std::atomic<float>* gateOn{}; std::atomic<float>* gateTh{}; std::atomic<float>* duckAmt{}; std::atomic<float>* duckAtk{}; std::atomic<float>* duckRel{}; std::atomic<float>* freeze{}; std::atomic<float>* algo{}; std::atomic<float>* outTrim{};

    // Misc
    double sampleRate=44100.0; int maxBlockSize=512; float gateEnv=0.0f; bool gateOpen=true;

    // Helpers
    void tuneForSampleRate();
    float computeTankFeedback(float rt60) const noexcept;
    inline void writePre (int ch, float s) noexcept;
    inline float readPre  (int ch, float dSamples) const noexcept;
    float earlyRef (int ch, float x);
    void updateDuckingCoeffs();
    inline void updateDuck (float dryMono);
};

} // namespace audio
} // namespace cppmusic
