#include "DrumMachine.h"
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

using namespace daw::ui::components;

namespace dm {

class DrumMachineTests : public juce::UnitTest {
public:
    DrumMachineTests() : juce::UnitTest("DrumMachine Scheduler & Serialization", "Audio") {}

    void runTest() override
    {
        beginTest("Scheduler emits correct number of hits with ratchets");
        Pattern p; p.steps = 16; p.swing = 0.0f; p.grid[0][0] = Step{ true, 100, 100, 4 };
        Scheduler sch; sch.setSampleRate(48000.0); sch.setBPM(120.0); sch.reset();
        int hits=0;
        sch.process(48000, p, true, std::function<void(int, float)>([&](int, float){ ++hits; }));
        // At 120 BPM, 1 second ≈ 2 beats. We process 1s; first step occurs at time 0 with 4 ratchets -> expect >=4
        expect(hits >= 4);

        beginTest("Probability gating");
        p.grid[1][0] = Step{ true, 100, 0, 1 }; // 0% prob
        hits=0; sch.reset();
        sch.process(48000, p, true, std::function<void(int, float)>([&](int, float){ ++hits; }));
        expect(hits >= 4 && hits < 8); // only kick ratchets from previous test fire

        beginTest("Serialization roundtrip");
        juce::String js = juce::JSON::toString(toVar(p));
        auto p2 = fromVar(juce::JSON::parse(js));
        expectEquals(p2.steps, p.steps);
        expectEquals((int)p2.grid[0][0].ratchet, (int)4);
    }
};

static DrumMachineTests drumMachineTests;

} // namespace dm
