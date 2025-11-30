// AuroraReverbEditor.h — UI for AuroraReverb with neon styling

#pragma once
#include "AuroraReverb.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <functional>

namespace cppmusic {
namespace ui {

//=========================== Custom Look & Feel ======================
class AuroraLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AuroraLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                         float pos, float sa, float ea, juce::Slider& s) override;
};

//=========================== Visual Components =======================
class DecayScope : public juce::Component, private juce::Timer
{
public:
    explicit DecayScope(std::function<float()> f);
    void paint(juce::Graphics& g) override;

private:
    std::function<float()> feed;
    void timerCallback() override { repaint(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DecayScope)
};

class GRMeter : public juce::Component, private juce::Timer
{
public:
    explicit GRMeter(std::function<float()> f);
    void paint(juce::Graphics& g) override;

private:
    std::function<float()> feed;
    void timerCallback() override { repaint(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GRMeter)
};

class XYPadRV : public juce::Component
{
public:
    explicit XYPadRV(std::function<void(float,float)> cb);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void setPosition(float newX, float newY);

private:
    float x=0.2f, y=0.7f;
    std::function<void(float,float)> onChange;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYPadRV)
};

//=========================== Main Editor =============================
class AuroraReverbEditor : public juce::AudioProcessorEditor
{
public:
    explicit AuroraReverbEditor(audio::AuroraReverbAudioProcessor& p);
    ~AuroraReverbEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    audio::AuroraReverbAudioProcessor& proc;
    AuroraLookAndFeel lnf;

    // Visual components
    std::unique_ptr<XYPadRV> xy;
    std::unique_ptr<DecayScope> scope;
    std::unique_ptr<GRMeter> gr;

    // Controls
    juce::Slider mix, size, decay, predelay, damp, cut, diff, mrate, mdepth, width, out;
    juce::Slider duckAmt, duckAtk, duckRel;
    juce::ToggleButton gate, freeze;
    juce::ComboBox algo;
    juce::Label duckLabel;

    // Attachments
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SA> mixA, sizeA, decayA, preA, dampA, cutA, diffA, mrA, mdA, widthA, outA;
    std::unique_ptr<SA> duckAmtA, duckAtkA, duckRelA;
    std::unique_ptr<BA> gateA, freezeA;
    std::unique_ptr<CA> algoA;

    void updateXYFromSliders();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuroraReverbEditor)
};

} // namespace ui
} // namespace cppmusic
