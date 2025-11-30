//==============================================================================
// Advanced Synthesizer Unit Tests
// Comprehensive testing of the AdvancedSynthesizer component
//==============================================================================

#include "src/testing/AudioTestFramework.h"
#include "src/testing/MockComponents.h"
#include "src/audio/AdvancedSynthesizer.h"
#include <gtest/gtest.h>

using namespace cppmusic::testing;
using namespace cppmusic::audio;
using namespace cppmusic::core;

class AdvancedSynthesizerTest : public SynthesizerTest {
protected:
    void SetUp() override {
        SynthesizerTest::SetUp();

        // Configure synthesizer for testing
        AdvancedSynthesizer::Config synthConfig;
        synthConfig.sampleRate = 44100.0;
        synthConfig.maxBlockSize = 512;
        synthConfig.maxVoices = 16;
        synthConfig.enableQuantumSynthesis = true;
        synthConfig.enableNeuralSynthesis = true;
        synthConfig.enableFractalSynthesis = true;

        synthesizer_->prepare(synthConfig);
    }
};

//==============================================================================
// Basic Functionality Tests
//==============================================================================

TEST_F(AdvancedSynthesizerTest, InitializationAndCleanup) {
    // Test that synthesizer initializes properly
    EXPECT_EQ(synthesizer_->getStatistics().activeVoices.load(), 0);
    EXPECT_GE(synthesizer_->getStatistics().cpuUsage.load(), 0.0f);
    EXPECT_LE(synthesizer_->getStatistics().cpuUsage.load(), 1.0f);

    // Test reset functionality
    synthesizer_->reset();
    EXPECT_EQ(synthesizer_->getStatistics().activeVoices.load(), 0);
}

TEST_F(AdvancedSynthesizerTest, BasicSineWaveGeneration) {
    const float testFrequency = 440.0f;
    const int blockSize = 512;
    const int numChannels = 2;

    // Configure sine wave oscillator
    synthesizer_->setOscillatorType(0, AdvancedSynthesizer::OscillatorType::VirtualAnalog);
    synthesizer_->setOscillatorWaveform(0, AdvancedSynthesizer::Waveform::Sine);
    synthesizer_->setOscillatorFrequency(0, testFrequency);
    synthesizer_->setOscillatorAmplitude(0, 0.5f);

    // Create test buffers
    juce::AudioBuffer<float> outputBuffer(numChannels, blockSize);
    juce::MidiBuffer midiBuffer;

    // Generate note on message
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 69, 0.8f); // A4
    midiBuffer.addEvent(noteOn, 0);

    // Process audio block
    outputBuffer.clear();
    EXPECT_REALTIME_SAFE({
        synthesizer_->processBlock(outputBuffer, midiBuffer);
    });

    // Verify audio output
    EXPECT_AUDIO_QUALITY(outputBuffer, 60.0f);

    // Check that we have active voices
    EXPECT_GT(synthesizer_->getStatistics().activeVoices.load(), 0);

    // Verify frequency content (should be close to 440Hz)
    auto spectralFeatures = testFramework_->getAnalysis().getLatestFeatures();
    EXPECT_NEAR(spectralFeatures.fundamentalFrequency, testFrequency, 10.0f);
}

TEST_F(AdvancedSynthesizerTest, PolyphonyManagement) {
    const int maxVoices = 8;
    const int numChannels = 2;
    const int blockSize = 512;

    juce::AudioBuffer<float> outputBuffer(numChannels, blockSize);
    juce::MidiBuffer midiBuffer;

    // Play more notes than available voices
    for (int note = 60; note < 60 + maxVoices + 4; ++note) {
        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, note, 0.7f);
        midiBuffer.addEvent(noteOn, 0);
    }

    // Process block
    EXPECT_REALTIME_SAFE({
        synthesizer_->processBlock(outputBuffer, midiBuffer);
    });

    // Should not exceed max voices
    EXPECT_LE(synthesizer_->getStatistics().activeVoices.load(), maxVoices);

    // Should have triggered voice stealing
    EXPECT_GT(synthesizer_->getStatistics().voiceStealCount.load(), 0);

    // Audio should still be generated
    EXPECT_AUDIO_QUALITY(outputBuffer, 50.0f); // Lower SNR due to voice stealing
}

//==============================================================================
// Advanced Synthesis Mode Tests
//==============================================================================

TEST_F(AdvancedSynthesizerTest, QuantumSynthesisMode) {
    synthesizer_->enableQuantumSynthesis(true);
    synthesizer_->setOscillatorType(0, AdvancedSynthesizer::OscillatorType::Quantum);

    const int blockSize = 1024;
    const int numChannels = 2;

    juce::AudioBuffer<float> outputBuffer(numChannels, blockSize);
    juce::MidiBuffer midiBuffer;

    // Generate note
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60, 0.8f);
    midiBuffer.addEvent(noteOn, 0);

    // Process multiple blocks to ensure quantum field evolution
    for (int block = 0; block < 10; ++block) {
        outputBuffer.clear();

        EXPECT_REALTIME_SAFE({
            synthesizer_->processBlock(outputBuffer, midiBuffer);
        });

        // Quantum synthesis should produce evolving harmonic content
        EXPECT_GT(outputBuffer.getRMSLevel(0, 0, blockSize), 0.001f);

        midiBuffer.clear(); // Only first block has note on
    }

    // Performance should still be within limits
    EXPECT_PERFORMANCE_WITHIN_LIMITS({
        synthesizer_->processBlock(outputBuffer, midiBuffer);
    }, 50.0f);
}

TEST_F(AdvancedSynthesizerTest, NeuralSynthesisMode) {
    synthesizer_->enableNeuralSynthesis(true);
    synthesizer_->setOscillatorType(0, AdvancedSynthesizer::OscillatorType::Neural);

    const int blockSize = 512;
    const int numChannels = 2;

    juce::AudioBuffer<float> outputBuffer(numChannels, blockSize);
    juce::MidiBuffer midiBuffer;

    // Generate note
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 72, 0.6f);
    midiBuffer.addEvent(noteOn, 0);

    EXPECT_REALTIME_SAFE({
        synthesizer_->processBlock(outputBuffer, midiBuffer);
    });

    // Neural synthesis should produce complex harmonic content
    auto spectralFeatures = testFramework_->getAnalysis().getLatestFeatures();
    EXPECT_GT(spectralFeatures.spectralComplexity, 0.1f);
    EXPECT_GT(spectralFeatures.harmonicity, 0.3f);

    // Should maintain audio quality
    EXPECT_AUDIO_QUALITY(outputBuffer, 45.0f); // Neural may have more noise
}

TEST_F(AdvancedSynthesizerTest, FractalSynthesisMode) {
    synthesizer_->enableFractalSynthesis(true);
    synthesizer_->setOscillatorType(0, AdvancedSynthesizer::OscillatorType::Fractal);

    const int blockSize = 1024;
    const int numChannels = 2;

    juce::AudioBuffer<float> outputBuffer(numChannels, blockSize);
    juce::MidiBuffer midiBuffer;

    // Generate note
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 48, 0.7f);
    midiBuffer.addEvent(noteOn, 0);

    EXPECT_REALTIME_SAFE({
        synthesizer_->processBlock(outputBuffer, midiBuffer);
    });

    // Fractal synthesis should have rich harmonic structure
    auto spectralFeatures = testFramework_->getAnalysis().getLatestFeatures();
    EXPECT_GT(spectralFeatures.spectralRolloff85, 2000.0f); // Should have high-frequency content
    EXPECT_LT(spectralFeatures.spectralFlatness, 0.8f);     // Should not be noise-like

    EXPECT_AUDIO_QUALITY(outputBuffer, 40.0f); // Fractal may be complex
}

//==============================================================================
// MPE (MIDI Polyphonic Expression) Tests
//==============================================================================

TEST_F(AdvancedSynthesizerTest, MPESupport) {
    synthesizer_->setMPEEnabled(true);

    const int blockSize = 512;
    const int numChannels = 2;

    juce::AudioBuffer<float> outputBuffer(numChannels, blockSize);
    juce::MidiBuffer midiBuffer;

    // Generate MPE note with per-note pitch bend
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(2, 60, 0.8f); // Channel 2 (MPE)
    juce::MidiMessage pitchBend = juce::MidiMessage::pitchWheel(2, 10000); // Bend up
    juce::MidiMessage pressure = juce::MidiMessage::channelPressureChange(2, 100);

    midiBuffer.addEvent(noteOn, 0);
    midiBuffer.addEvent(pitchBend, 10);
    midiBuffer.addEvent(pressure, 20);

    // Process blocks and capture output for analysis
    std::vector<juce::AudioBuffer<float>> outputs;
    for (int block = 0; block < 5; ++block) {
        outputs.emplace_back(numChannels, blockSize);
        outputs.back().clear();

        EXPECT_REALTIME_SAFE({
            synthesizer_->processBlock(outputs.back(), midiBuffer);
        });

        midiBuffer.clear(); // Only first block has MIDI
    }

    // Verify that pitch bend and pressure affected the sound
    // (This would require more sophisticated analysis in a real test)
    for (const auto& buffer : outputs) {
        EXPECT_GT(buffer.getRMSLevel(0, 0, blockSize), 0.001f);
    }
}

//==============================================================================
// Filter and Envelope Tests
//==============================================================================

TEST_F(AdvancedSynthesizerTest, FilterSweep) {
    const int blockSize = 512;
    const int numChannels = 2;
    const int numBlocks = 20;

    // Set up filter with envelope modulation
    synthesizer_->setFilterType(0, AdvancedSynthesizer::FilterType::LowPass);
    synthesizer_->setFilterCutoff(0, 500.0f);
    synthesizer_->setFilterResonance(0, 0.7f);

    // Set up envelope
    synthesizer_->setEnvelopeADSR(1, 0.01f, 2.0f, 0.3f, 1.0f); // Filter envelope

    juce::AudioBuffer<float> outputBuffer(numChannels, blockSize);
    juce::MidiBuffer midiBuffer;

    // Generate note
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60, 0.8f);
    midiBuffer.addEvent(noteOn, 0);

    std::vector<float> spectralCentroids;

    for (int block = 0; block < numBlocks; ++block) {
        outputBuffer.clear();

        EXPECT_REALTIME_SAFE({
            synthesizer_->processBlock(outputBuffer, midiBuffer);
        });

        // Analyze spectral content
        auto features = testFramework_->getAnalysis().getLatestFeatures();
        spectralCentroids.push_back(features.spectralCentroid);

        midiBuffer.clear(); // Only first block has note on
    }

    // Filter envelope should cause spectral centroid to change over time
    // (In attack phase, it should increase, then decrease in decay)
    EXPECT_GT(spectralCentroids[5], spectralCentroids[0]);    // Should increase during attack
    EXPECT_LT(spectralCentroids[15], spectralCentroids[5]);   // Should decrease during decay
}

TEST_F(AdvancedSynthesizerTest, LFOModulation) {
    const int blockSize = 256;
    const int numChannels = 2;
    const int numBlocks = 50; // Enough for several LFO cycles

    // Set up LFO modulating oscillator frequency
    synthesizer_->setLFOFrequency(0, 5.0f); // 5 Hz LFO
    synthesizer_->setLFOWaveform(0, AdvancedSynthesizer::Waveform::Sine);

    juce::AudioBuffer<float> outputBuffer(numChannels, blockSize);
    juce::MidiBuffer midiBuffer;

    // Generate note
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 69, 0.7f); // A4
    midiBuffer.addEvent(noteOn, 0);

    std::vector<float> fundamentalFreqs;

    for (int block = 0; block < numBlocks; ++block) {
        outputBuffer.clear();

        EXPECT_REALTIME_SAFE({
            synthesizer_->processBlock(outputBuffer, midiBuffer);
        });

        // Track fundamental frequency changes due to LFO
        auto features = testFramework_->getAnalysis().getLatestFeatures();
        if (features.pitchConfidence > 0.5f) {
            fundamentalFreqs.push_back(features.fundamentalFrequency);
        }

        midiBuffer.clear();
    }

    // LFO should cause frequency modulation
    if (fundamentalFreqs.size() > 10) {
        float minFreq = *std::min_element(fundamentalFreqs.begin(), fundamentalFreqs.end());
        float maxFreq = *std::max_element(fundamentalFreqs.begin(), fundamentalFreqs.end());

        // Should have noticeable frequency variation due to LFO
        EXPECT_GT(maxFreq - minFreq, 10.0f); // At least 10Hz variation
    }
}

//==============================================================================
// Performance and Stress Tests
//==============================================================================

TEST_F(AdvancedSynthesizerTest, PerformanceUnderLoad) {
    // Configure for maximum load
    synthesizer_->enableQuantumSynthesis(true);
    synthesizer_->enableNeuralSynthesis(true);
    synthesizer_->enableFractalSynthesis(true);

    const int blockSize = 1024;
    const int numChannels = 2;

    juce::AudioBuffer<float> outputBuffer(numChannels, blockSize);
    juce::MidiBuffer midiBuffer;

    // Generate full polyphony
    for (int note = 60; note < 76; ++note) {
        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, note, 0.6f);
        midiBuffer.addEvent(noteOn, 0);
    }

    // Test performance with full load
    EXPECT_PERFORMANCE_WITHIN_LIMITS({
        synthesizer_->processBlock(outputBuffer, midiBuffer);
    }, 80.0f); // Allow higher CPU usage for stress test

    // Should still maintain realtime safety
    EXPECT_REALTIME_SAFE({
        synthesizer_->processBlock(outputBuffer, midiBuffer);
    });

    // Audio quality might be lower but should still be acceptable
    EXPECT_AUDIO_QUALITY(outputBuffer, 30.0f);
}

TEST_F(AdvancedSynthesizerTest, MemoryUsageStability) {
    const int blockSize = 512;
    const int numChannels = 2;
    const int numIterations = 1000;

    juce::AudioBuffer<float> outputBuffer(numChannels, blockSize);
    juce::MidiBuffer midiBuffer;

    // Generate random MIDI events
    auto& random = testFramework_->getBufferGenerator();
    auto midiSequence = random.createScaleSequence(60, 12, 0.1f);

    // Measure initial memory usage
    float initialMemory = testFramework_->getPerformanceMonitor().getCurrentMemoryUsage();

    // Process many blocks
    for (int i = 0; i < numIterations; ++i) {
        outputBuffer.clear();
        midiBuffer.clear();

        // Generate MIDI for this block
        random.generateMidiForBlock(midiBuffer, blockSize, i * blockSize / 44100.0);

        EXPECT_REALTIME_SAFE({
            synthesizer_->processBlock(outputBuffer, midiBuffer);
        });

        // Check for memory leaks every 100 iterations
        if (i % 100 == 0) {
            float currentMemory = testFramework_->getPerformanceMonitor().getCurrentMemoryUsage();
            EXPECT_LT(currentMemory - initialMemory, 10.0f); // No more than 10MB growth
        }
    }

    // Final memory usage should be stable
    float finalMemory = testFramework_->getPerformanceMonitor().getCurrentMemoryUsage();
    EXPECT_LT(finalMemory - initialMemory, 5.0f); // Should be very stable
}

//==============================================================================
// Error Handling and Edge Cases
//==============================================================================

TEST_F(AdvancedSynthesizerTest, ExtremeMIDIValues) {
    const int blockSize = 512;
    const int numChannels = 2;

    juce::AudioBuffer<float> outputBuffer(numChannels, blockSize);
    juce::MidiBuffer midiBuffer;

    // Test extreme MIDI values
    std::vector<juce::MidiMessage> extremeMessages = {
        juce::MidiMessage::noteOn(1, 0, 1.0f),      // Lowest note
        juce::MidiMessage::noteOn(1, 127, 1.0f),    // Highest note
        juce::MidiMessage::pitchWheel(1, 0),        // Full down bend
        juce::MidiMessage::pitchWheel(1, 16383),    // Full up bend
        juce::MidiMessage::controllerEvent(1, 1, 127), // Max modulation
        juce::MidiMessage::channelPressureChange(1, 127) // Max pressure
    };

    for (const auto& message : extremeMessages) {
        midiBuffer.clear();
        midiBuffer.addEvent(message, 0);

        outputBuffer.clear();

        // Should handle extreme values gracefully
        EXPECT_NO_THROW({
            synthesizer_->processBlock(outputBuffer, midiBuffer);
        });

        // Should not produce NaN or infinite values
        for (int ch = 0; ch < numChannels; ++ch) {
            for (int sample = 0; sample < blockSize; ++sample) {
                float value = outputBuffer.getSample(ch, sample);
                EXPECT_TRUE(std::isfinite(value))
                    << "Non-finite value at channel " << ch << " sample " << sample;
                EXPECT_LE(std::abs(value), 2.0f)
                    << "Excessive amplitude " << value << " at channel " << ch << " sample " << sample;
            }
        }
    }
}

TEST_F(AdvancedSynthesizerTest, ZeroSizedBuffers) {
    // Test with zero-sized buffer (edge case)
    juce::AudioBuffer<float> emptyBuffer(2, 0);
    juce::MidiBuffer midiBuffer;

    // Should handle gracefully
    EXPECT_NO_THROW({
        synthesizer_->processBlock(emptyBuffer, midiBuffer);
    });
}

TEST_F(AdvancedSynthesizerTest, RapidParameterChanges) {
    const int blockSize = 64; // Small blocks for rapid changes
    const int numChannels = 2;
    const int numBlocks = 100;

    juce::AudioBuffer<float> outputBuffer(numChannels, blockSize);
    juce::MidiBuffer midiBuffer;

    // Start a note
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, 60, 0.8f);
    midiBuffer.addEvent(noteOn, 0);
    synthesizer_->processBlock(outputBuffer, midiBuffer);

    // Rapidly change parameters
    for (int block = 0; block < numBlocks; ++block) {
        // Change oscillator parameters every block
        synthesizer_->setOscillatorFrequency(0, 200.0f + block * 10.0f);
        synthesizer_->setFilterCutoff(0, 500.0f + block * 50.0f);
        synthesizer_->setFilterResonance(0, (block % 10) / 10.0f);

        outputBuffer.clear();
        midiBuffer.clear();

        EXPECT_REALTIME_SAFE({
            synthesizer_->processBlock(outputBuffer, midiBuffer);
        });

        // Should continue producing valid audio
        EXPECT_GT(outputBuffer.getRMSLevel(0, 0, blockSize), 0.001f);

        // Check for audio artifacts
        for (int ch = 0; ch < numChannels; ++ch) {
            for (int sample = 0; sample < blockSize; ++sample) {
                float value = outputBuffer.getSample(ch, sample);
                EXPECT_TRUE(std::isfinite(value));
                EXPECT_LE(std::abs(value), 2.0f);
            }
        }
    }
}
