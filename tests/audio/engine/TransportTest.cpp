#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../../src/audio/engine/Transport.h"
#include <cmath>

class TransportTest : public juce::UnitTest
{
public:
    TransportTest() : juce::UnitTest("Transport Test") {}

    void runTest() override
    {
        beginTest("Transport creation");
        daw::audio::engine::Transport transport;
        expect(!transport.isPlaying(), "Should not be playing initially");
        expectEquals(transport.getTempo(), 120.0, "Default tempo should be 120 BPM");
        expectEquals(transport.getTimeSignatureNumerator(), 4, "Default time signature numerator should be 4");
        expectEquals(transport.getTimeSignatureDenominator(), 4, "Default time signature denominator should be 4");

        beginTest("Play/Stop");
        transport.play();
        expect(transport.isPlaying(), "Should be playing after play()");
        transport.stop();
        expect(!transport.isPlaying(), "Should not be playing after stop()");

        beginTest("Tempo changes");
        transport.setTempo(140.0);
        expectEquals(transport.getTempo(), 140.0, "Tempo should be 140 BPM");
        transport.setTempo(999.0);
        expectEquals(transport.getTempo(), 999.0, "Tempo should clamp to 999");
        transport.setTempo(10.0);
        expectEquals(transport.getTempo(), 20.0, "Tempo should clamp to minimum 20");

        beginTest("Time signature changes");
        transport.setTimeSignature(3, 4);
        expectEquals(transport.getTimeSignatureNumerator(), 3, "Time signature numerator should be 3");
        expectEquals(transport.getTimeSignatureDenominator(), 4, "Time signature denominator should be 4");

        beginTest("Position setting");
        transport.setPositionInBeats(8.5);
        expectWithinAbsoluteError(transport.getPositionInBeats(), 8.5, 0.01, "Position should be 8.5 beats");

        beginTest("Position update during playback");
        transport.setPositionInBeats(0.0);
        transport.setTempo(120.0);
        transport.play();
        
        const double sampleRate = 44100.0;
        const int numSamples = 4410; // 0.1 seconds at 44.1kHz
        
        transport.updatePosition(numSamples, sampleRate);
        const auto positionAfter = transport.getPositionInBeats();
        expect(positionAfter > 0.0, "Position should advance during playback");
        expectWithinAbsoluteError(positionAfter, 0.2, 0.1, "Position should be approximately 0.2 beats (120 BPM = 2 beats/sec, 0.1 sec = 0.2 beats)");
    }
};

static TransportTest transportTest;

