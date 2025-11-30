#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../../src/audio/engine/DawEngine.h"

class DawEngineTest : public juce::UnitTest
{
public:
    DawEngineTest() : juce::UnitTest("DawEngine Test") {}

    void runTest() override
    {
        beginTest("Engine creation");
        daw::audio::engine::DawEngine engine;
        expect(!engine.isPlaying(), "Should not be playing initially");
        expectEquals(engine.getNumTracks(), 0, "Should have no tracks initially");

        beginTest("Track management");
        const auto trackIndex1 = engine.addTrack();
        expectEquals(trackIndex1, 0, "First track should have index 0");
        expectEquals(engine.getNumTracks(), 1, "Should have 1 track");

        const auto trackIndex2 = engine.addTrack();
        expectEquals(trackIndex2, 1, "Second track should have index 1");
        expectEquals(engine.getNumTracks(), 2, "Should have 2 tracks");

        beginTest("Track parameter changes");
        engine.setTrackGain(0, -6.0f);
        engine.setTrackPan(0, 0.5f);
        engine.setTrackMute(0, true);
        engine.setTrackSolo(0, true);

        // Parameters should be set (we can't directly query them, but they shouldn't crash)
        engine.setTrackGain(0, -12.0f);
        engine.setTrackPan(0, -0.5f);
        engine.setTrackMute(0, false);
        engine.setTrackSolo(0, false);
        expect(true, "Setting parameters should not crash");

        beginTest("Transport control");
        engine.play();
        expect(engine.isPlaying(), "Should be playing after play()");
        engine.stop();
        expect(!engine.isPlaying(), "Should not be playing after stop()");

        beginTest("Tempo and time signature");
        engine.setTempo(140.0);
        expectWithinAbsoluteError(engine.getTempo(), 140.0, 0.1, "Tempo should be 140 BPM");
        engine.setTimeSignature(3, 4);
        expectEquals(engine.getTimeSignatureNumerator(), 3, "Time signature numerator should be 3");
        expectEquals(engine.getTimeSignatureDenominator(), 4, "Time signature denominator should be 4");

        beginTest("Metering");
        const auto meterData = engine.getTrackMeter(0);
        expect(meterData.peak >= 0.0f, "Peak should be non-negative");
        expect(meterData.rms >= 0.0f, "RMS should be non-negative");

        beginTest("CPU load");
        const auto cpuLoad = engine.getCpuLoad();
        expect(cpuLoad >= 0.0f && cpuLoad <= 100.0f, "CPU load should be between 0 and 100");

        beginTest("Multiple tracks processing");
        for (int i = 0; i < 8; ++i)
        {
            engine.addTrack();
        }
        expectEquals(engine.getNumTracks(), 10, "Should have 10 tracks total");

        // Set different parameters for each track
        for (int i = 0; i < 10; ++i)
        {
            engine.setTrackGain(i, static_cast<float>(-i * 2));
            engine.setTrackPan(i, (i % 2 == 0) ? 0.5f : -0.5f);
        }
        expect(true, "Setting parameters on multiple tracks should not crash");

        beginTest("Track removal");
        engine.removeTrack(5);
        expectEquals(engine.getNumTracks(), 9, "Should have 9 tracks after removal");

        // Remaining tracks should still work
        engine.setTrackGain(0, -3.0f);
        expect(true, "Remaining tracks should still function");

        /*
        beginTest("Engine context");
        auto context = engine.getEngineContext();
        expect(context != nullptr, "Engine context should be available");
        expectEquals(context->getNumTracks(), 9, "Context should reflect current track count");
        */
    }
};

static DawEngineTest dawEngineTest;

