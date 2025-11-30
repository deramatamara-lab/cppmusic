// AuroraReverb.cpp — Implementation for AuroraReverb audio processor

#include "AuroraReverb.h"
#include "ui/AuroraReverbEditor.h"

namespace cppmusic {
namespace audio {

//============================= Comb Implementation
void AuroraReverbAudioProcessor::Comb::setDamp(float hfHz, double sr)
{
    float alpha = std::exp(-2.0f * juce::MathConstants<float>::pi * juce::jlimit(1000.0f,20000.0f,hfHz) / (float)sr);
    damp1 = 1.0f - alpha;
    damp2 = alpha;
}

float AuroraReverbAudioProcessor::Comb::process(float x, float feedback, float /*hfHz*/, double /*sr*/, bool freeze)
{
    if (size<=0) return 0.0f;
    const float y = buf[(size_t)idx];
    filterStore = damp1*y + damp2*filterStore;
    const float fbIn = freeze ? y : x + feedback * filterStore;
    buf[(size_t)idx] = fbIn;
    idx++;
    if (idx>=size) idx=0;
    return y;
}

//============================= Allpass Implementation
float AuroraReverbAudioProcessor::Allpass::process(float x, float a)
{
    const float y = buf[(size_t)idx];
    const float z = y + (-a) * x;
    buf[(size_t)idx] = x + a * z;
    idx++;
    if (idx>=size) idx=0;
    return z;
}

//============================= Processor Methods
void AuroraReverbAudioProcessor::prepareToPlay(double sr, int maxBlock)
{
    sampleRate = sr;
    maxBlockSize = maxBlock;

    // Pre-delay buffer
    const int maxPre = (int) std::ceil(0.25 * sr) + 8;
    for (int ch=0; ch<2; ++ch) {
        predelayBuf[ch].assign((size_t)maxPre, 0.0f);
        preW[ch]=0;
        preLen=maxPre;
    }

    // Early reflections
    const int earlyMax = (int) std::ceil(0.050 * sr) + 8;
    for (int ch=0; ch<2; ++ch) {
        earlyBuf[ch].assign((size_t)earlyMax, 0.0f);
        earlyW[ch]=0;
        earlyLen=earlyMax;
    }

    // Tank setup
    tuneForSampleRate();

    // Damping filters
    for (int ch=0; ch<2; ++ch)
    {
        hp[ch].reset();
        lp[ch].reset();
        hp[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, 20.0f);
        lp[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, 20000.0f);
    }

    gateEnv = 0.0f;
    gateOpen = true;
    updateDuckingCoeffs();

    mixSm.reset(sr, 0.02);
    outSm.reset(sr, 0.02);
}

void AuroraReverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals guard;
    const int numSamples = buffer.getNumSamples();
    const int chs = juce::jmin(2, buffer.getNumChannels());

    const float mixT  = (mix != nullptr ? mix->load() : 20.0f) * 0.01f;
    mixSm.setTargetValue(mixT);
    const float outG  = juce::Decibels::decibelsToGain(outTrim != nullptr ? outTrim->load() : 0.0f);
    outSm.setTargetValue(outG);

    // precompute colour parameters
    const float cutHz  = cutLF  != nullptr ? cutLF->load()  : 120.0f;
    const float dampHz = dampHF != nullptr ? dampHF->load() : 9000.0f;

    // update colour filters
    for (int ch=0; ch<2; ++ch)
    {
    *hp[ch].coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, juce::jlimit(20.0f, 500.0f, cutHz));
    *lp[ch].coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, juce::jlimit(1000.0f, 20000.0f, dampHz));
    }

    const float pdMs   = predelay != nullptr ? predelay->load() : 0.0f;
    const float pdSamp = juce::jlimit(0.0f, 0.25f, pdMs * 0.001f) * (float)sampleRate;
    const float gWidth = width != nullptr ? width->load() : 1.0f;
    const bool doFreeze = (freeze != nullptr && freeze->load() > 0.5f);

    const float decaySeconds = decay != nullptr ? decay->load() : 5.0f;
    const float rt60 = juce::jlimit(0.1f, 30.0f, decaySeconds);
    const float tankFeedback = computeTankFeedback(rt60);
    const float diffusionAmt = diffusion != nullptr ? diffusion->load() : 0.7f;
    const float apBase = juce::jlimit(0.2f, 0.95f, 0.65f + diffusionAmt * 0.3f);
    const float modDepthVal = modDepth != nullptr ? modDepth->load() : 0.1f;
    const float modRateVal  = modRate  != nullptr ? modRate->load()  : 0.2f;
    const float apModDepth = juce::jlimit(0.0f, 0.5f, modDepthVal);
    const float apModRate  = juce::jlimit(0.05f, 2.0f, modRateVal);

    const float gateThVal = gateTh != nullptr ? gateTh->load() : -40.0f;
    const float gateThresh = juce::Decibels::decibelsToGain(gateThVal);
    const bool gateEnabled = (gateOn != nullptr && gateOn->load() > 0.5f);

    for (int n=0; n<numSamples; ++n)
    {
        const float inL = buffer.getSample(0, n);
        const float inR = chs>1 ? buffer.getSample(1, n) : inL;
        const float dryMono = 0.5f*(inL+inR);

        writePre(0, inL);
        writePre(1, inR);
        const float pdL = readPre(0, pdSamp);
        const float pdR = readPre(1, pdSamp);

        const float erL = earlyRef(0, pdL);
        const float erR = earlyRef(1, pdR);

    float tankInL = hp[0].processSample(erL);
    float tankInR = hp[1].processSample(erR);

        float combOutL = 0.0f, combOutR = 0.0f;
        for (int i=0;i<NUM_COMBS;++i) {
            combOutL += combL[i].process(tankInL, tankFeedback, dampHz, sampleRate, doFreeze);
            combOutR += combR[i].process(tankInR, tankFeedback, dampHz, sampleRate, doFreeze);
        }
        combOutL *= (1.0f/NUM_COMBS);
        combOutR *= (1.0f/NUM_COMBS);

        float apL = combOutL, apR = combOutR;
        lfoPhase += apModRate / sampleRate;
        if (lfoPhase >= 1.0) lfoPhase -= 1.0;
        const float lfo = std::sin(2.0f * juce::MathConstants<float>::pi * (float)lfoPhase);
        const float apCoeff = apBase + apModDepth * 0.2f * lfo;
        for (int i=0;i<NUM_AP;++i) {
            apL = allpassL[i].process(apL, apCoeff);
            apR = allpassR[i].process(apR, apCoeff);
        }

    float wetL = lp[0].processSample(apL);
    float wetR = lp[1].processSample(apR);

        if (gWidth < 0.999f) {
            float M = 0.5f*(wetL+wetR);
            float S = 0.5f*(wetL-wetR);
            S *= gWidth*2.0f;
            wetL = M+S;
            wetR = M-S;
        }

        updateDuck(dryMono);
    const float duckAmtVal = duckAmt != nullptr ? duckAmt->load() : 0.0f;
    const float duckGain = 1.0f - duckGR * duckAmtVal;
        wetL *= duckGain;
        wetR *= duckGain;

        if (gateEnabled) {
            const float e = std::max(std::abs(wetL), std::abs(wetR));
            gateEnv = 0.99f*gateEnv + 0.01f*e;
            gateOpen = gateEnv >= gateThresh || doFreeze;
            if (!gateOpen) { wetL = wetR = 0.0f; }
        }

        const float m = mixSm.getNextValue();
        const float o = outSm.getNextValue();
        const float outL = inL*(1.0f-m) + wetL*m;
        const float outR = inR*(1.0f-m) + wetR*m;

        buffer.setSample(0, n, outL*o);
        if (chs>1) buffer.setSample(1, n, outR*o);

        lastWetEnergy = 0.99f*lastWetEnergy + 0.01f*(std::abs(wetL)+std::abs(wetR))*0.5f;
    }
}

void AuroraReverbAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, dest);
}

void AuroraReverbAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorValueTreeState::ParameterLayout AuroraReverbAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::mix, "Mix", juce::NormalisableRange<float>(0.f,100.f,0.01f), 20.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::size, "Size", juce::NormalisableRange<float>(0.2f,1.5f,0.001f,0.35f), 1.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::decay, "Decay", juce::NormalisableRange<float>(0.1f,30.f,0.001f,0.3f), 5.5f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::predelay, "PreDelay", juce::NormalisableRange<float>(0.f,200.f,0.01f,0.35f), 12.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::dampHF, "HF Damp", juce::NormalisableRange<float>(1000.f,20000.f,1.f,0.35f), 9000.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::cutLF, "LF Cut", juce::NormalisableRange<float>(20.f,500.f,0.1f,0.35f), 120.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::diffusion, "Diffusion", juce::NormalisableRange<float>(0.f,1.f,0.0001f), 0.7f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::modRate, "Mod Rate", juce::NormalisableRange<float>(0.05f,2.0f,0.001f,0.3f), 0.2f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::modDepth, "Mod Depth", juce::NormalisableRange<float>(0.f,0.5f,0.0001f), 0.1f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::width, "Width", juce::NormalisableRange<float>(0.f,1.f,0.0001f), 0.9f));
    p.push_back(std::make_unique<juce::AudioParameterBool>(IDs::gateOn, "Gate", false));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::gateTh, "Gate Th", juce::NormalisableRange<float>(-60.f,-20.f,0.01f), -40.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::duckAmt, "Ducking", juce::NormalisableRange<float>(0.f,1.f,0.0001f), 0.35f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::duckAtk, "Duck Attack", juce::NormalisableRange<float>(1.f,200.f,0.01f,0.35f), 30.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::duckRel, "Duck Release", juce::NormalisableRange<float>(10.f,1000.f,0.01f,0.35f), 250.f));
    p.push_back(std::make_unique<juce::AudioParameterBool>(IDs::freeze, "Freeze", false));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(IDs::algo, "Algo", juce::StringArray{"Plate","Hall","Room"}, 1));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::outTrim, "Output", juce::NormalisableRange<float>(-24.f,12.f,0.01f), 0.f));
    return { p.begin(), p.end() };
}

//============================= Helper Methods
void AuroraReverbAudioProcessor::tuneForSampleRate()
{
    const float srScale = (float)sampleRate / 44100.0f;
    const float s = juce::jlimit(0.2f,1.5f, size? size->load():1.0f);

    for (int i=0;i<NUM_COMBS;++i)
    {
        combL[i].setSize((int) std::round(combTuningL[i] * srScale * s));
        combR[i].setSize((int) std::round(combTuningR[i] * srScale * s));
        combL[i].setDamp(dampHF?dampHF->load():9000.0f, sampleRate);
        combR[i].setDamp(dampHF?dampHF->load():9000.0f, sampleRate);
    }
    for (int i=0;i<NUM_AP;++i)
    {
        const int n = (int) std::round(apTuning[i] * srScale * s);
        allpassL[i].setSize(n);
        allpassR[i].setSize(n);
    }
}

float AuroraReverbAudioProcessor::computeTankFeedback(float rt60) const noexcept
{
    float meanLen = 0.0f;
    for (int i=0;i<NUM_COMBS;++i)
        meanLen += (combL[i].size + combR[i].size)*0.5f;
    meanLen /= (float)NUM_COMBS;
    const float g = std::pow(10.0f, (-3.0f * meanLen) / (rt60 * (float)sampleRate));
    return juce::jlimit(0.0f, 0.99f, g);
}

inline void AuroraReverbAudioProcessor::writePre(int ch, float s) noexcept
{
    auto& b = predelayBuf[(size_t)ch];
    int& w = preW[(size_t)ch];
    b[(size_t)w] = s;
    w = (w+1) % preLen;
}

inline float AuroraReverbAudioProcessor::readPre(int ch, float dSamples) const noexcept
{
    const auto& b = predelayBuf[(size_t)ch];
    const int w = preW[(size_t)ch];
    float r = (float)w - dSamples;
    while (r < 0.f) r += (float)preLen;
    int i0 = ((int)r) % preLen;
    int i1 = (i0+1)%preLen;
    float f = r - (int)r;
    return b[(size_t)i0]*(1.0f-f) + b[(size_t)i1]*f;
}

float AuroraReverbAudioProcessor::earlyRef(int ch, float x)
{
    static const float tapMs[8] = { 3.1f, 7.2f, 11.7f, 15.3f, 17.9f, 22.6f, 27.4f, 33.0f };
    static const float tapGain[8] = { 0.7f, 0.6f, 0.5f, 0.45f, 0.4f, 0.35f, 0.3f, 0.25f };
    auto& buf = earlyBuf[(size_t)ch];
    int& w = earlyW[(size_t)ch];
    buf[(size_t)w] = x;
    float y=0.0f;
    const float s = juce::jlimit(0.2f,1.5f, size?size->load():1.0f);
    for (int i=0;i<8;++i) {
        float d = (tapMs[i]*0.001f * (float)sampleRate) * s * (1.0f + 0.02f*i);
        float r = (float)w - d;
        while (r<0) r += (float)earlyLen;
        int i0=(int)r % earlyLen;
        int i1=(i0+1)%earlyLen;
        float f=r-(int)r;
        float v = buf[(size_t)i0]*(1.0f-f) + buf[(size_t)i1]*f;
        y += tapGain[i] * v;
    }
    w = (w+1) % earlyLen;
    return x*0.2f + y*0.8f;
}

void AuroraReverbAudioProcessor::updateDuckingCoeffs()
{
    const float aMs = duckAtk?juce::jlimit(1.0f,200.0f,duckAtk->load()):30.0f;
    const float rMs = duckRel?juce::jlimit(10.0f,1000.0f,duckRel->load()):250.0f;
    duckAtkC = std::exp(-1.0f / (0.001f*aMs*(float)sampleRate));
    duckRelC = std::exp(-1.0f / (0.001f*rMs*(float)sampleRate));
}

inline void AuroraReverbAudioProcessor::updateDuck(float dryMono)
{
    const float x = std::abs(dryMono);
    duckEnv = (x>duckEnv) ? x + duckAtkC*(duckEnv-x) : x + duckRelC*(duckEnv-x);
    const float th = 0.1f;
    const float over = juce::jlimit(0.0f,1.0f,(duckEnv - th)*5.0f);
    duckGR = 0.6f * over;
}

juce::AudioProcessorEditor* AuroraReverbAudioProcessor::createEditor()
{
    return new cppmusic::ui::AuroraReverbEditor(*this);
}

} // namespace audio
} // namespace cppmusic
