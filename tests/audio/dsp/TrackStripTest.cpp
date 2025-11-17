#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../../src/audio/dsp/TrackStrip.h"

class TrackStripTest : public juce::UnitTest
{
public:
    TrackStripTest() : juce::UnitTest("TrackStrip Test") {}

    void runTest() override
    {
        beginTest("TrackStrip creation");
        daw::audio::dsp::TrackStrip trackStrip;
        expectEquals(trackStrip.getGain(), 1.0f, "Default gain should be 1.0 (0 dB)");
        expectEquals(trackStrip.getPan(), 0.0f, "Default pan should be 0.0 (center)");
        expect(!trackStrip.isMuted(), "Should not be muted initially");
        expect(!trackStrip.isSoloed(), "Should not be soloed initially");

        beginTest("Prepare to play");
        trackStrip.prepareToPlay(44100.0, 512);
        trackStrip.prepareToPlay(48000.0, 1024);
        expect(true, "Should handle different sample rates");

        beginTest("Gain control");
        trackStrip.setGain(-6.0f);
        const auto gainLinear = trackStrip.getGain();
        expect(gainLinear < 1.0f, "Gain should be less than 1.0 for -6 dB");
        expect(gainLinear > 0.0f, "Gain should be positive");

        beginTest("Pan control");
        trackStrip.setPan(1.0f);
        expectEquals(trackStrip.getPan(), 1.0f, "Pan should be 1.0 (full right)");
        trackStrip.setPan(-1.0f);
        expectEquals(trackStrip.getPan(), -1.0f, "Pan should be -1.0 (full left)");
        trackStrip.setPan(2.0f);
        expectEquals(trackStrip.getPan(), 1.0f, "Pan should clamp to 1.0");
        trackStrip.setPan(-2.0f);
        expectEquals(trackStrip.getPan(), -1.0f, "Pan should clamp to -1.0");

        beginTest("Mute control");
        trackStrip.setMute(true);
        expect(trackStrip.isMuted(), "Should be muted");
        trackStrip.setMute(false);
        expect(!trackStrip.isMuted(), "Should not be muted");

        beginTest("Solo control");
        trackStrip.setSolo(true);
        expect(trackStrip.isSoloed(), "Should be soloed");
        trackStrip.setSolo(false);
        expect(!trackStrip.isSoloed(), "Should not be soloed");

        beginTest("Audio processing");
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        
        // Fill with test signal
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                data[i] = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * i / 44100.0f);
            }
        }
        
        juce::MidiBuffer midiMessages;
        trackStrip.processBlock(buffer, midiMessages);
        
        // After processing, meters should be updated
        const auto peak = trackStrip.getPeakLevel();
        const auto rms = trackStrip.getRmsLevel();
        expect(peak > 0.0f, "Peak should be greater than 0 after processing");
        expect(rms > 0.0f, "RMS should be greater than 0 after processing");

        beginTest("Mute processing");
        buffer.clear();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                data[i] = 0.5f;
            }
        }
        
        trackStrip.setMute(true);
        trackStrip.processBlock(buffer, midiMessages);
        
        // Check that buffer is cleared when muted
        bool allZero = true;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                if (std::abs(data[i]) > 0.001f)
                {
                    allZero = false;
                    break;
                }
            }
            if (!allZero)
                break;
        }
        expect(allZero, "Buffer should be cleared when muted");

        beginTest("Meter reset");
        trackStrip.resetMeters();
        expectEquals(trackStrip.getPeakLevel(), 0.0f, "Peak should be 0 after reset");
        expectEquals(trackStrip.getRmsLevel(), 0.0f, "RMS should be 0 after reset");
    }
};

static TrackStripTest trackStripTest;

