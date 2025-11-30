#include "DrumMachine.h"
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "../lookandfeel/DesignSystem.h"
#include <algorithm>
#include <cmath>
#include <utility>

namespace daw::ui::components
{

namespace
{
    inline float stepPulse(float t, float start) noexcept
    {
        return (t >= start && t < start + 0.01f) ? 1.0f : 0.0f;
    }
}

// ------------------------------ Serialization --------------------------
juce::var toVar(const Pattern& p)
{
    juce::DynamicObject* root = new juce::DynamicObject();
    root->setProperty("steps", p.steps);
    root->setProperty("swing", p.swing);
    root->setProperty("accent", p.accent);
    root->setProperty("accentEvery", p.accentEvery);

    juce::Array<juce::var> lanes;
    for (int i = 0; i < kNumInstruments; ++i)
    {
        juce::Array<juce::var> steps;
        for (int s = 0; s < kMaxSteps; ++s)
        {
            const auto& st = p.grid[i][s];
            juce::DynamicObject* obj = new juce::DynamicObject();
            obj->setProperty("a", st.active);
            obj->setProperty("v", (int) st.velocity);
            obj->setProperty("p", (int) st.prob);
            obj->setProperty("r", (int) st.ratchet);
            steps.add(juce::var(obj));
        }
        lanes.add(juce::var(steps));
    }
    root->setProperty("lanes", lanes);
    return juce::var(root);
}

Pattern fromVar(const juce::var& v)
{
    Pattern p;
    if (auto* o = v.getDynamicObject())
    {
        p.steps = juce::jlimit(1, kMaxSteps, (int) o->getProperty("steps"));
        p.swing = (float) o->getProperty("swing");
        p.accent= (float) o->getProperty("accent");
        p.accentEvery = juce::jmax(1, (int) o->getProperty("accentEvery"));
        auto lanes = o->getProperty("lanes");
        for (int i = 0; i < kNumInstruments; ++i)
        {
            auto arr = lanes[i];
            for (int s = 0; s < kMaxSteps; ++s)
            {
                auto st = arr[s].getDynamicObject();
                if (!st) continue;
                auto& dst = p.grid[i][s];
                dst.active   = (bool) st->getProperty("a");
                dst.velocity = (uint8_t) (int) st->getProperty("v");
                dst.prob     = (uint8_t) (int) st->getProperty("p");
                dst.ratchet  = (uint8_t) juce::jmax(1, (int) st->getProperty("r"));
            }
        }
    }
    return p;
}

// ------------------------------ Simple Synth Voices --------------------
void DrumSynth::setSampleRate(double sr)
{
    sampleRate = juce::jmax(1.0, sr);
}

void DrumSynth::trigger(int instrument, float velocity)
{
    const float g = juce::jlimit(0.0f, 1.0f, velocity);
    Hit h; h.instrument = instrument; h.gain = g; h.samplesLeft = secondsToSamples(1.0);
    h.tone = 0.0f; h.noise = 0.0f; h.pitchEnv = 1.0f;
    hits.add(h);
}

void DrumSynth::process(const juce::AudioSourceChannelInfo& info)
{
    auto* L = info.buffer->getWritePointer(0, info.startSample);
    auto* R = info.buffer->getNumChannels() > 1 ? info.buffer->getWritePointer(1, info.startSample) : L;
    for (int i = 0; i < info.numSamples; ++i)
    {
        float s = 0.0f;
        for (int h = hits.size(); --h >= 0; )
        {
            auto &hit = hits.getReference(h);
            if (hit.samplesLeft <= 0) { hits.remove(h); continue; }
            s += voiceSample(hit);
            --hit.samplesLeft;
        }
        s = limiter(s);
        L[i] += s; R[i] += s;
    }
}

int DrumSynth::secondsToSamples(double sec) const
{
    return (int) (sec * sampleRate);
}

float DrumSynth::sine(float& phase, float freq)
{
    phase += (float) (juce::MathConstants<double>::twoPi * freq / sampleRate);
    if (phase > juce::MathConstants<float>::twoPi) phase -= juce::MathConstants<float>::twoPi;
    return std::sin(phase);
}

float DrumSynth::envelope(float x, float a, float d)
{
    // fast attack, exp decay
    juce::ignoreUnused(a);
    return std::exp(-d * x);
}

float DrumSynth::voiceSample(Hit& h)
{
    static float phaseKick = 0.0f, phaseRide = 0.0f;
    const float t = 1.0f - (float) h.samplesLeft / (float) secondsToSamples(1.0);
    float out = 0.0f;
    switch (h.instrument)
    {
        case 0: // Kick: sine drop + click
        {
            float f = 90.0f + 140.0f * (1.0f - t); // exponential-ish sweep
            float body = sine(phaseKick, f) * envelope(t, 0.001f, 10.5f);
            float click = (t < 0.01f ? (1.0f - t * 100.0f) : 0.0f);
            out = (0.9f * body + 0.3f * click) * (0.6f + 0.4f * h.gain);
            break;
        }
        case 1: // Snare: noise + short tone
        {
            float n = (rand01()*2.0f-1.0f) * envelope(t, 0.001f, 18.0f);
            float tone = std::sin(2.0f * juce::MathConstants<float>::pi * 200.0f * t) * envelope(t,0.001f, 12.0f);
            out = (0.85f * n + 0.15f * tone) * (0.5f + 0.5f * h.gain);
            break;
        }
        case 2: // Clap: bursty noise with repeats
        {
            float bursts = (stepPulse(t, 0.0f) + 0.6f*stepPulse(t, 0.015f) + 0.4f*stepPulse(t, 0.03f));
            float n = (rand01()*2.0f-1.0f) * envelope(t,0.001f, 14.0f) * bursts;
            out = n * (0.4f + 0.6f * h.gain);
            break;
        }
        case 3: // Hat: filtered noise
        {
            float n = highpass(noise(), 0.92f);
            out = n * envelope(t,0.001f, 28.0f) * (0.3f + 0.7f * h.gain);
            break;
        }
        case 4: // Tom: tuned sine + noise
        {
            float f = 150.0f + 40.0f * (1.0f - t);
            float tone = std::sin(2.0f * juce::MathConstants<float>::pi * f * t) * envelope(t,0.001f, 9.0f);
            float n = (rand01()*2.0f-1.0f) * envelope(t,0.001f, 20.0f) * 0.2f;
            out = (tone + n) * (0.4f + 0.6f * h.gain);
            break;
        }
        case 5: // Perc: clicky blip
        {
            float blip = (t < 0.02f ? (1.0f - t * 50.0f) : 0.0f);
            out = blip * (0.3f + 0.7f * h.gain);
            break;
        }
        case 6: // Ride: metallic noise + tone
        {
            float n = bandpass(noise(), 0.02f, 0.92f);
            float tone = sine(phaseRide, 5200.0f) * 0.05f;
            out = (n + tone) * envelope(t,0.001f, 6.0f) * (0.2f + 0.8f * h.gain);
            break;
        }
        case 7: // Crash: wide noise, slow decay
        {
            float n = bandpass(noise(), 0.015f, 0.98f);
            out = n * envelope(t,0.001f, 3.5f) * (0.15f + 0.85f * h.gain);
            break;
        }
    }
    return out;
}

float DrumSynth::noise()
{
    return (rand01()*2.0f - 1.0f);
}

float DrumSynth::rand01()
{
    rng = (rng * 1664525u + 1013904223u); // LCG deterministic
    return (float)((rng >> 9) & 0x7FFFFF) / (float)0x7FFFFF;
}

float DrumSynth::highpass(float x, float c)
{
    hpState = c * (hpState + x - hpPrev);
    hpPrev = x;
    return hpState;
}

float DrumSynth::bandpass(float x, float a, float c)
{
    // simple two‑pole band-ish
    float hp = highpass(x, c);
    bpState = (1.0f - a) * bpState + a * hp;
    return bpState;
}

float DrumSynth::limiter(float x)
{
    float absx = std::abs(x);
    float over = juce::jmax(0.0f, absx - 0.9f);
    float gain = 1.0f / (1.0f + 6.0f * over);
    limiterGain = 0.995f * limiterGain + 0.005f * gain;
    return x * limiterGain;
}


// ------------------------------ Scheduler ------------------------------
void Scheduler::setSampleRate(double sr)
{
    sampleRate = juce::jmax(1.0, sr);
}

void Scheduler::setBPM(double b)
{
    bpm = juce::jlimit(20.0, 999.0, b);
}

void Scheduler::setSwing(float s)
{
    swing = juce::jlimit(0.0f, 1.0f, s);
}

void Scheduler::setExternal(bool e)
{
    external = e;
}

void Scheduler::setRandomSeed(uint32_t s)
{
    rng = s ? s : 1u;
}

void Scheduler::reset()
{
    sampleCursor = 0;
    prevStep = -1;
    subIndex = 0;
    accumBeat = 0.0;
}

float Scheduler::rand01()
{
    rng = rng * 1664525u + 1013904223u;
    return (float)((rng >> 9) & 0x7FFFFF) / (float)0x7FFFFF;
}

template <typename Fn>
void Scheduler::process(int numSamples, const Pattern& pat, bool playing, Fn&& cb)
{
    if (!playing || pat.steps <= 0 || sampleRate <= 0.0)
        return;

    auto callback = std::forward<Fn>(cb);
    const double samplesPerBeat = sampleRate * 60.0 / juce::jmax(1.0, bpm);

    for (int i = 0; i < numSamples; ++i)
    {
        stepAndEmit(accumBeat, pat, callback);

        accumBeat += 1.0 / samplesPerBeat;
        if (accumBeat >= pat.steps * 0.25)
            accumBeat -= pat.steps * 0.25;
    }
}

template <typename Fn>
void Scheduler::stepAndEmit(double beat, const Pattern& pat, Fn&& cb)
{
    const double stepBeats = 0.25; // 16th
    const double stepIndexFloat = beat / stepBeats;
    const int stepIndex = (int) std::floor(stepIndexFloat) % juce::jmax(1, pat.steps);

    // Swing: delay odd steps
    const bool odd = (stepIndex % 2) == 1;
    const double swingBeats = odd ? (double) pat.swing * 0.5 * stepBeats : 0.0;
    // Ratchets: subdivide equally in the step window
    const double stepStart = (double) std::floor(stepIndexFloat) * stepBeats + (odd ? swingBeats : 0.0);
    const double stepEnd   = stepStart + stepBeats + (odd ? 0.0 : 0.0);
    const double win = juce::jmax(1e-6, stepEnd - stepStart);

    if (stepIndex != prevStep)
    {
        prevStep = stepIndex; subIndex = 0;
    }

    for (int inst = 0; inst < kNumInstruments; ++inst)
    {
        const Step& st = pat.grid[inst][stepIndex];
        if (!st.active) continue;

        const int rat = juce::jmax(1, (int) st.ratchet);
        const double subDur = win / (double) rat;

        for (int r = 0; r < rat; ++r)
        {
            const double subStart = stepStart + r * subDur;
            if (beat >= subStart && beat < subStart + 1e-3) // near start boundary
            {
                if (rand01() <= (st.prob / 100.0f))
                {
                    float vel = st.velocity / 127.0f;
                    if (pat.accent > 0.0f && (stepIndex % juce::jmax(1, pat.accentEvery) == 0))
                        vel = juce::jlimit(0.0f, 1.0f, vel + pat.accent);
                    cb(inst, vel);
                }
            }
        }
    }
}

// Template instantiation for process method
template void Scheduler::process<std::function<void(int, float)>>(
    int numSamples, const Pattern& pat, bool playing, std::function<void(int, float)>&& cb);

// ------------------------------ UI Controls ---------------------------
Knob::Knob(juce::String name, float min, float max, float value, std::function<void(float)> onChange)
    : label(std::move(name)), rangeMin(min), rangeMax(max), val(value), onChanged(std::move(onChange))
{
    setSize(64, 64);
}

void Knob::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    auto r = getLocalBounds().toFloat();
    auto c = r.getCentre();
    float radius = juce::jmin(r.getWidth(), r.getHeight()) * 0.45f;

    // Knob body
    auto body = r.reduced(6.0f);
    juce::ColourGradient grad(juce::Colour(Colors::surface2).brighter(0.15f),
                              body.getTopLeft(),
                              juce::Colour(Colors::surface2).darker(0.25f),
                              body.getBottomRight(),
                              false);
    g.setGradientFill(grad);
    g.fillEllipse(body);
    g.setColour(juce::Colour(Colors::outline));
    g.drawEllipse(body, 1.0f);

    // Needle
    float t = (val - rangeMin) / (rangeMax - rangeMin);
    float ang = juce::MathConstants<float>::pi * (1.2f + 1.6f * t);
    juce::Line<float> needle(c, c + juce::Point<float>(std::cos(ang), std::sin(ang)) * (radius - 4.0f));
    g.setColour(juce::Colour(Colors::accent));
    g.drawLine(needle, 2.0f);

    // Label
    g.setColour(juce::Colour(Colors::textSecondary));
    g.setFont(Typography::caption);
    g.drawFittedText(label, getLocalBounds().removeFromBottom(18), juce::Justification::centred, 1);
}

void Knob::mouseDrag(const juce::MouseEvent& e)
{
    auto dy = -e.getDistanceFromDragStartY();
    setValue(val + dy * (rangeMax - rangeMin) / 200.0f);
}

void Knob::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    setValue(val + w.deltaY * (rangeMax - rangeMin) * 0.05f);
}

void Knob::setValue(float v)
{
    v = juce::jlimit(rangeMin, rangeMax, v);
    if (std::abs(v - val) > 1e-6f) { val = v; if (onChanged) onChanged(val); repaint(); }
}

float Knob::getValue() const
{
    return val;
}

void Knob::setOnChange(std::function<void(float)> cb)
{
    onChanged = std::move(cb);
}

void StepGrid::setPattern(const Pattern* p)
{
    pat = p;
    repaint();
}

void StepGrid::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    if (!pat) return;
    auto r = getLocalBounds();
    const int rows = kNumInstruments;
    const int cols = pat->steps;
    const int cellW = juce::jmax(1, r.getWidth() / juce::jmax(1, cols));
    const int cellH = juce::jmax(1, r.getHeight() / juce::jmax(1, rows));

    g.fillAll(juce::Colour(Colors::background));

    for (int y = 0; y < rows; ++y)
    {
        for (int x = 0; x < cols; ++x)
        {
            auto cell = juce::Rectangle<int>(x * cellW, y * cellH, cellW - 1, cellH - 1);
            const auto& st = pat->grid[y][x];

            const bool barStart = (x % 4) == 0;
            auto base = barStart ? juce::Colour(Colors::surface1)
                                 : juce::Colour(Colors::surface2);

            if (st.active)
            {
                const float velNorm = juce::jlimit(0.0f, 1.0f, st.velocity / 127.0f);
                auto accent = juce::Colour(Colors::accent);
                accent = accent.interpolatedWith(juce::Colours::white, velNorm * 0.25f);
                g.setColour(accent);
                g.fillRect(cell.reduced(1));

                // Ratchet/probability overlay in bottom-right
                g.setColour(juce::Colour(Colors::text));
                juce::String overlay = juce::String((int)st.ratchet) + "× " + juce::String((int)st.prob) + "%";
                g.setFont(10.0f);
                g.drawFittedText(overlay, cell.reduced(3), juce::Justification::bottomRight, 1);
            }
            else
            {
                g.setColour(base);
                g.fillRect(cell);
            }

            g.setColour(juce::Colour(Colors::outline).withAlpha(0.5f));
            g.drawRect(cell);
        }
    }
}

void StepGrid::mouseDown(const juce::MouseEvent& e)
{
    handle(e);
}

void StepGrid::mouseDrag(const juce::MouseEvent& e)
{
    handle(e);
}

void StepGrid::handle(const juce::MouseEvent& e)
{
    if (!pat) return;
    auto r = getLocalBounds();
    int cols = pat->steps; int rows = kNumInstruments;
    int cellW = juce::jmax(1, r.getWidth() / juce::jmax(1, cols));
    int cellH = juce::jmax(1, r.getHeight() / juce::jmax(1, rows));
    const int x = juce::jlimit(0, cols - 1, static_cast<int>(e.position.x / cellW));
    const int y = juce::jlimit(0, rows - 1, static_cast<int>(e.position.y / cellH));
    bool shift = e.mods.isShiftDown();
    bool alt   = e.mods.isAltDown();
    if (onToggle) onToggle(y, x, shift, alt);
}

// ------------------------------ DrumMachine Component ------------------
DrumMachine::DrumMachine()
{
    // Controls
    addAndMakeVisible(knobTempo);
    addAndMakeVisible(knobSwing);
    addAndMakeVisible(knobAccent);
    addAndMakeVisible(lenBox);
    addAndMakeVisible(playButton);
    addAndMakeVisible(randButton);
    addAndMakeVisible(clearButton);

    lenBox.addItemList({"16","24","32","48","64"}, 1);
    lenBox.onChange = [this]{ pattern.steps = lenFromIndex(lenBox.getSelectedItemIndex()); grid.repaint(); };
    lenBox.setSelectedId(1, juce::dontSendNotification);
    playButton.setButtonText("Play");
    playButton.onClick = [this]{ transport.playing = !transport.playing; playButton.setToggleState(transport.playing, juce::dontSendNotification); };
    playButton.setClickingTogglesState(true);
    randButton.setButtonText("Randomize"); randButton.onClick = [this]{ randomize(); };
    clearButton.setButtonText("Clear"); clearButton.onClick = [this]{ clearPattern(); };

    knobTempo.setValue((float) transport.bpm);
    knobTempo.setOnChange([this](float v){ transport.bpm = v; scheduler.setBPM(v); });
    knobSwing.setOnChange([this](float v){ pattern.swing = v; scheduler.setSwing(v); });
    knobAccent.setOnChange([this](float v){ pattern.accent = v; });

    addAndMakeVisible(grid);
    grid.setPattern(&pattern);
    grid.onToggle = [this](int lane, int step, bool shift, bool alt){ toggleStep(lane, step, shift, alt); };

    scheduler.setBPM(transport.bpm);
    scheduler.setSampleRate(44100.0);
    synth.setSampleRate(44100.0);

    startTimerHz(30);
}

void DrumMachine::setExternalClock(bool external, double bpm, bool playing, double beatPos)
{
    scheduler.setExternal(external);
    transport.bpm = bpm; scheduler.setBPM(bpm);
    transport.playing = playing; transport.beatPos = beatPos;
}

void DrumMachine::setNoteCallback(std::function<void(int,float)> cb)
{
    noteCallback = std::move(cb);
}

void DrumMachine::attachToDeviceManager(juce::AudioDeviceManager& dm)
{
    deviceManager = &dm; deviceManager->addAudioCallback(this);
}

void DrumMachine::detachFromDeviceManager()
{
    if (deviceManager) deviceManager->removeAudioCallback(this);
    deviceManager = nullptr;
}

juce::String DrumMachine::toJson() const
{
    return juce::JSON::toString(toVar(pattern));
}

void DrumMachine::fromJson(const juce::String& s)
{
    pattern = fromVar(juce::JSON::parse(s));
    grid.repaint();
}

void DrumMachine::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    auto bounds = getLocalBounds().toFloat();
    drawGlassPanel(g, bounds, Radii::medium, true);

    auto top = bounds.toNearestInt().removeFromTop(80).reduced(Spacing::small);
    g.setColour(toColour(Colors::text));
    g.setFont(getHeadingFont(Typography::heading2));
    g.drawText("Drum Machine", top.removeFromLeft(180), juce::Justification::centredLeft, true);
}

void DrumMachine::resized()
{
    auto r = getLocalBounds();
    auto top = r.removeFromTop(80).reduced(12);
    auto left = top.removeFromLeft(240);
    knobTempo.setBounds(left.removeFromLeft(80));
    knobSwing.setBounds(left.removeFromLeft(80));
    knobAccent.setBounds(left.removeFromLeft(80));

    auto right = top.removeFromRight(360);
    playButton.setBounds(right.removeFromLeft(80).reduced(6));
    randButton.setBounds(right.removeFromLeft(120).reduced(6));
    clearButton.setBounds(right.removeFromLeft(80).reduced(6));
    lenBox.setBounds(right.removeFromLeft(60).reduced(6));

    grid.setBounds(r.reduced(12));
}

int DrumMachine::lenFromIndex(int idx)
{
    static int map[]{16,24,32,48,64};
    idx = juce::jlimit(0,4,idx);
    return map[idx];
}

void DrumMachine::toggleStep(int lane, int step, bool shift, bool alt)
{
    auto& st = pattern.grid[lane][step];
    if (!shift && !alt)
    {
        st.active = !st.active;
        if (st.active && st.velocity == 0) st.velocity = 100;
    }
    if (shift) // adjust velocity with circular pattern for quick edits
    {
        st.velocity = (uint8_t) juce::jlimit(1, 127, (int) st.velocity + 16);
    }
    if (alt) // cycle ratchet
    {
        st.ratchet = (uint8_t) ((st.ratchet % 4) + 1); // 1..4
    }
    grid.repaint();
}

void DrumMachine::clearPattern()
{
    for (int i=0;i<kNumInstruments;++i)
        for (int s=0;s<kMaxSteps;++s) pattern.grid[i][s] = Step{};
    grid.repaint();
}

void DrumMachine::randomize()
{
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> vel(40,115);
    std::bernoulli_distribution on(0.25);
    for (int i=0;i<kNumInstruments;++i)
        for (int s=0;s<pattern.steps;++s)
        {
            auto& st = pattern.grid[i][s];
            st.active = on(rng);
            st.velocity = (uint8_t) vel(rng);
            st.prob = 100;
            st.ratchet = 1;
        }
    grid.repaint();
}

void DrumMachine::timerCallback()
{
    repaint();
}

void DrumMachine::audioDeviceIOCallbackWithContext (const float* const* inputChannelData, int numInputChannels,
                                                    float* const* outputChannelData, int numOutputChannels,
                                                    int numSamples,
                                                    const juce::AudioIODeviceCallbackContext& context)
{
    juce::ignoreUnused(inputChannelData, numInputChannels, context);
    auto** outputs = const_cast<float**>(outputChannelData);
    juce::AudioBuffer<float> buf(outputs, numOutputChannels, numSamples);
    buf.clear();

    scheduler.process(numSamples, pattern, transport.playing, [this](int inst, float vel){ onHit(inst, vel); });
    juce::AudioSourceChannelInfo info(&buf, 0, numSamples);
    synth.process(info);
}

void DrumMachine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    auto sr = device->getCurrentSampleRate();
    scheduler.setSampleRate(sr); synth.setSampleRate(sr); scheduler.reset();
}

void DrumMachine::audioDeviceStopped()
{
}

void DrumMachine::onHit(int inst, float vel)
{
    if (noteCallback) noteCallback(inst, vel);
    synth.trigger(inst, vel);
}

} // namespace daw::ui::components
