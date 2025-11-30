#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>
#include <random>
#include <atomic>
#include <memory>
#include <functional>

namespace daw::ui::components
{
// ------------------------------ Constants ------------------------------
static constexpr int kDefaultSteps = 16;   // per pattern
static constexpr int kMaxSteps     = 64;
static constexpr int kNumInstruments = 8;  // Kick, Snare, Clap, Hat, Tom, Perc, Ride, Crash

// ------------------------------ Data Model -----------------------------
struct Step
{
    bool  active   { false };
    uint8_t velocity{ 100 };   // 1..127
    uint8_t prob    { 100 };   // 0..100 (%)
    uint8_t ratchet { 1 };     // 1..8
};

struct Pattern
{
    int steps = kDefaultSteps;          // 16..64
    float swing = 0.0f;                 // 0..1 (applied on odd steps)
    float accent = 0.0f;                // 0..1 (adds to velocity on accented beats)
    int accentEvery = 4;                // every N steps (grid‑aligned)
    Step grid[kNumInstruments][kMaxSteps] {};
};

struct Transport
{
    bool playing = false;
    double bpm = 128.0;
    double beatPos = 0.0; // current song position in beats (if externally driven)
};

// ------------------------------ Serialization --------------------------
// Internal helpers implemented in cpp
juce::var toVar(const Pattern& p);
Pattern fromVar(const juce::var& v);

// ------------------------------ Simple Synth Voices --------------------
struct Hit { int instrument; int samplesLeft; float gain; float tone; float noise; float pitchEnv; };

class DrumSynth
{
public:
    void setSampleRate(double sr);

    void trigger(int instrument, float velocity);

    void process(const juce::AudioSourceChannelInfo& info);

private:
    juce::Array<Hit> hits;
    double sampleRate { 44100.0 };
    float envKick = 1.0f, envSnare=1.0f, envHat=1.0f; // per-sample state (minor coloration)
    float limiterGain = 1.0f;

    int secondsToSamples(double sec) const;

    float sine(float& phase, float freq);

    float envelope(float x, float a, float d);

    float voiceSample(Hit& h);

    float noise();

    float rand01();

    float highpass(float x, float c);
    float bandpass(float x, float a, float c);

    float limiter(float x);

    float hpState=0, hpPrev=0, bpState=0;
    uint32_t rng { 1u };
};

// ------------------------------ Scheduler ------------------------------
class Scheduler
{
public:
    void setSampleRate(double sr);
    void setBPM(double b);
    void setSwing(float s);
    void setExternal(bool e);
    void setRandomSeed(uint32_t s);

    // Advance and emit hits into callback: cb(instrumentIndex, velocity)
    template <typename Fn>
    void process(int numSamples, const Pattern& pat, bool playing, Fn&& cb);

    void reset();

private:
    double sampleRate { 44100.0 };
    double bpm { 128.0 };
    float swing { 0.0f };
    bool external { false };

    // phase tracking
    int prevStep { -1 };
    int subIndex { 0 }; // ratchet sub‑step
    double sampleCursor { 0.0 };
    double accumBeat { 0.0 };

    uint32_t rng { 1u };

    float rand01();

    template <typename Fn>
    void stepAndEmit(double beat, const Pattern& pat, Fn&& cb);
};

// ------------------------------ UI Controls ---------------------------
class Knob : public juce::Component
{
public:
    Knob(juce::String name, float min, float max, float value, std::function<void(float)> onChange);

    void paint(juce::Graphics& g) override;

    void mouseDrag(const juce::MouseEvent& e) override;

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& w) override;

    void setValue(float v);
    float getValue() const;
    void setOnChange(std::function<void(float)> cb);

private:
    juce::String label;
    float rangeMin, rangeMax;
    float val;
    std::function<void(float)> onChanged;
};

class StepGrid : public juce::Component
{
public:
    std::function<void(int lane, int step, bool shift, bool alt)> onToggle;
    std::function<void(int lane, int step, int vel)> onVelocity;
    std::function<void(int lane, int step, int prob)> onProbability;
    std::function<void(int lane, int step, int ratchet)> onRatchet;

    void setPattern(const Pattern* p);

    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

private:
    const Pattern* pat { nullptr };

    void handle(const juce::MouseEvent& e);
};

// ------------------------------ DrumMachine Component ------------------
class DrumMachine : public juce::Component, private juce::Timer, private juce::AudioIODeviceCallback
{
public:
    DrumMachine();

    // External integration (optional)
    void setExternalClock(bool external, double bpm, bool playing, double beatPos);

    void setNoteCallback(std::function<void(int,float)> cb);

    // Internal audio if desired
    void attachToDeviceManager(juce::AudioDeviceManager& dm);

    void detachFromDeviceManager();

    [[nodiscard]] bool isAttachedToDeviceManager() const noexcept { return deviceManager != nullptr; }

    // Persistence
    juce::String toJson() const;
    void fromJson(const juce::String& s);

    // Component
    void paint(juce::Graphics& g) override;

    void resized() override;

private:
    // UI
    Knob knobTempo { "BPM", 40.0f, 220.0f, 128.0f, nullptr };
    Knob knobSwing { "Swing", 0.0f, 1.0f, 0.0f, nullptr };
    Knob knobAccent{ "Accent",0.0f, 0.5f, 0.0f, nullptr };
    juce::ComboBox lenBox;
    juce::TextButton playButton, randButton, clearButton;
    StepGrid grid;

    // State
    Pattern pattern;
    Transport transport;
    Scheduler scheduler;
    DrumSynth synth;
    juce::AudioDeviceManager* deviceManager { nullptr };
    std::function<void(int,float)> noteCallback;

    // Helpers
    static int lenFromIndex(int idx);

    void toggleStep(int lane, int step, bool shift, bool alt);

    void clearPattern();

    void randomize();

    // Timer: UI refresh if needed (could draw playhead etc.)
    void timerCallback() override;

    // Audio callback (internal mode)
    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData, int numInputChannels,
                                           float* const* outputChannelData, int numOutputChannels,
                                           int numSamples,
                                           const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;

    void audioDeviceStopped() override;

    void onHit(int inst, float vel);
};

} // namespace daw::ui::components

// ------------------------------ Unit Tests -----------------------------
#if JUCE_UNIT_TESTS
namespace daw::ui::components {
class DrumMachineTests : public juce::UnitTest {
public:
    DrumMachineTests();

    void runTest() override;
};
static DrumMachineTests drumMachineTests;
} // namespace daw::ui::components
#endif

// ------------------------------ Usage Example --------------------------
/*
// In your MainView or a test harness:
class DemoComponent : public juce::Component {
public:
    DemoComponent() { addAndMakeVisible(dm); addAndMakeVisible(start); start.onClick = [this]{ toggleAudio(); }; }
    void resized() override { auto r = getLocalBounds(); start.setBounds(r.removeFromTop(30).removeFromLeft(120).reduced(4)); dm.setBounds(r); }
private:
    dm::DrumMachine dm; juce::TextButton start{"Enable Audio"}; juce::AudioDeviceManager dev;
    void toggleAudio(){ if (!attached){ dev.initialise(0,2,nullptr,true); dm.attachToDeviceManager(dev); start.setButtonText("Disable Audio"); } else { dm.detachFromDeviceManager(); dev.closeAudioDevice(); start.setButtonText("Enable Audio"); } attached = !attached; }
    bool attached=false;
};
*/
