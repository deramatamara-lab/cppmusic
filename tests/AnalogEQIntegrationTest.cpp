#include <gtest/gtest.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "src/audio/AnalogModeledEQ.h"
#include "src/integration/EqualizerService.h"
#include "src/core/EngineContext.h"
#include "src/core/RTMemoryPool.h"

using namespace cppmusic;

class AnalogEQIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize core components
        memoryPool_ = std::make_unique<core::RTMemoryPool>(1024 * 1024); // 1MB
        engineContext_ = std::make_unique<core::EngineContext>();

        // Initialize EQ service
        eqService_ = std::make_unique<integration::EqualizerService>(*engineContext_);

        // Set up EQ configuration
        audio::AnalogModeledEQ::Config config;
        config.sampleRate = 48000.0;
        config.maxBlockSize = 512;
        config.enableOversampling = true;
        config.analogModel = audio::AnalogModeledEQ::AnalogModel::NeveVintage;

        eqService_->initialize(config);
    }

    void TearDown() override {
        eqService_->shutdown();
        eqService_.reset();
        engineContext_.reset();
        memoryPool_.reset();
    }

    std::unique_ptr<core::RTMemoryPool> memoryPool_;
    std::unique_ptr<core::EngineContext> engineContext_;
    std::unique_ptr<integration::EqualizerService> eqService_;
};

TEST_F(AnalogEQIntegrationTest, InitializationAndShutdown) {
    // Test basic initialization - should not crash
    EXPECT_TRUE(eqService_ != nullptr);

    // Test shutdown and re-initialization
    eqService_->shutdown();

    audio::AnalogModeledEQ::Config config;
    config.sampleRate = 44100.0;
    config.maxBlockSize = 256;

    eqService_->initialize(config);
    // Should succeed without exceptions
}

TEST_F(AnalogEQIntegrationTest, AudioProcessing) {
    // Create a test audio buffer
    juce::AudioBuffer<float> testBuffer(2, 512);
    testBuffer.clear();

    // Fill with a test signal (1 kHz sine wave)
    double sampleRate = 48000.0;
    double frequency = 1000.0;
    double phase = 0.0;
    double phaseIncrement = 2.0 * juce::MathConstants<double>::pi * frequency / sampleRate;

    for (int sample = 0; sample < testBuffer.getNumSamples(); ++sample) {
        float sineValue = static_cast<float>(std::sin(phase));
        testBuffer.setSample(0, sample, sineValue * 0.5f);
        testBuffer.setSample(1, sample, sineValue * 0.5f);
        phase += phaseIncrement;
    }

    // Store input RMS for comparison
    float inputRMS = testBuffer.getRMSLevel(0, 0, testBuffer.getNumSamples());

    // Process through EQ
    eqService_->processBlock(testBuffer);

    // Check output is reasonable
    float outputRMS = testBuffer.getRMSLevel(0, 0, testBuffer.getNumSamples());
    EXPECT_GT(outputRMS, 0.0f);
    EXPECT_LT(outputRMS, 1.0f);

    // With default settings, output should be similar to input
    EXPECT_NEAR(inputRMS, outputRMS, 0.1f);
}

TEST_F(AnalogEQIntegrationTest, ParameterUpdates) {
    // Test parameter update messages
    auto update = integration::EqualizerService::EQParameterUpdate::bandFrequency(0, 2000.0f);
    eqService_->sendParameterUpdate(update);

    update = integration::EqualizerService::EQParameterUpdate::bandGain(0, 6.0f);
    eqService_->sendParameterUpdate(update);

    update = integration::EqualizerService::EQParameterUpdate::analogModel(
        audio::AnalogModeledEQ::AnalogModel::SSLChannel);
    eqService_->sendParameterUpdate(update);

    // Process a block to ensure parameters are applied
    juce::AudioBuffer<float> testBuffer(2, 512);
    testBuffer.clear();
    eqService_->processBlock(testBuffer);

    // Should not crash - parameter processing is real-time safe
}

TEST_F(AnalogEQIntegrationTest, PresetManagement) {
    // Save a preset
    eqService_->savePreset("Test Preset", 0);

    // Modify some parameters
    auto update = integration::EqualizerService::EQParameterUpdate::bandGain(1, 12.0f);
    eqService_->sendParameterUpdate(update);

    // Process to apply changes
    juce::AudioBuffer<float> testBuffer(2, 256);
    testBuffer.clear();
    eqService_->processBlock(testBuffer);

    // Load preset back
    bool loaded = eqService_->loadPreset("Test Preset");
    EXPECT_TRUE(loaded);

    // Check available presets
    auto presets = eqService_->getAvailablePresets();
    EXPECT_GT(presets.size(), 0);
    EXPECT_TRUE(presets.contains("Test Preset"));
}

TEST_F(AnalogEQIntegrationTest, StatisticsCollection) {
    // Process some audio to generate statistics
    juce::AudioBuffer<float> testBuffer(2, 512);

    // Fill with varied amplitude signal
    for (int sample = 0; sample < testBuffer.getNumSamples(); ++sample) {
        float value = static_cast<float>(sample) / testBuffer.getNumSamples();
        testBuffer.setSample(0, sample, value * 0.8f);
        testBuffer.setSample(1, sample, value * 0.8f);
    }

    // Process multiple blocks to accumulate statistics
    for (int block = 0; block < 10; ++block) {
        eqService_->processBlock(testBuffer);
    }

    // Get performance metrics
    auto metrics = eqService_->getPerformanceMetrics();
    EXPECT_GT(metrics.totalProcessedBlocks, 0);
    EXPECT_GE(metrics.averageProcessingTime, 0.0);
    EXPECT_GE(metrics.cpuUsagePercent, 0.0);

    // Get latest statistics
    auto stats = eqService_->getLatestStatistics();
    // Should have reasonable values (not all zeros)
}

TEST_F(AnalogEQIntegrationTest, AutomationInterface) {
    // Test automation parameter access
    int numParams = 64; // Max parameters

    for (int i = 0; i < 10; ++i) { // Test first 10 parameters
        // Get parameter info
        auto name = eqService_->getParameterName(i);
        auto text = eqService_->getParameterText(i);
        auto defaultVal = eqService_->getParameterDefaultValue(i);

        EXPECT_FALSE(name.isEmpty());
        EXPECT_GE(defaultVal, 0.0f);
        EXPECT_LE(defaultVal, 1.0f);

        // Test setting parameter
        eqService_->setAutomationParameter(i, 0.5f);
        float retrievedValue = eqService_->getAutomationParameter(i);
        EXPECT_NEAR(retrievedValue, 0.5f, 0.01f);
    }
}

TEST_F(AnalogEQIntegrationTest, MidiControlIntegration) {
    // Assign MIDI controller to parameter
    eqService_->assignMidiController(0, 74); // CC 74 to parameter 0

    // Send MIDI control change
    eqService_->handleMidiControlChange(74, 64); // Mid-range value

    // Check parameter was updated
    float paramValue = eqService_->getAutomationParameter(0);
    EXPECT_NEAR(paramValue, 64.0f / 127.0f, 0.01f);
}

TEST_F(AnalogEQIntegrationTest, UIEditorCreation) {
    // Test UI editor creation
    auto editor = eqService_->createEditor();

    // Should create successfully
    EXPECT_TRUE(editor != nullptr);

    if (editor) {
        // Basic UI component tests
        EXPECT_GT(editor->getWidth(), 0);
        EXPECT_GT(editor->getHeight(), 0);

        // Test editor update mechanism
        editor->updateFromEQ();
        // Should not crash
    }
}

// Direct EQ class tests (unit tests for the core EQ)
class AnalogModeledEQTest : public ::testing::Test {
protected:
    void SetUp() override {
        memoryPool_ = std::make_unique<core::RTMemoryPool>(1024 * 1024);
        engineContext_ = std::make_unique<core::EngineContext>();

        eq_ = std::make_unique<audio::AnalogModeledEQ>(*engineContext_, *memoryPool_);

        audio::AnalogModeledEQ::Config config;
        config.sampleRate = 48000.0;
        config.maxBlockSize = 512;
        config.enableOversampling = false; // Disable for faster unit tests

        eq_->prepare(config);
        eq_->reset();
    }

    void TearDown() override {
        eq_.reset();
        engineContext_.reset();
        memoryPool_.reset();
    }

    std::unique_ptr<audio::AnalogModeledEQ> eq_;
    std::unique_ptr<core::EngineContext> engineContext_;
    std::unique_ptr<core::RTMemoryPool> memoryPool_;
};

TEST_F(AnalogModeledEQTest, BasicFiltering) {
    // Set up parametric boost at 1 kHz
    eq_->setBandType(2, audio::AnalogModeledEQ::BandType::Parametric);
    eq_->setBandFrequency(2, 1000.0f);
    eq_->setBandGain(2, 6.0f);
    eq_->setBandQ(2, 2.0f);
    eq_->setBandEnabled(2, true);

    // Create test signal - 1 kHz sine wave
    juce::AudioBuffer<float> buffer(2, 512);
    double phase = 0.0;
    double phaseIncrement = 2.0 * juce::MathConstants<double>::pi * 1000.0 / 48000.0;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        float sineValue = static_cast<float>(std::sin(phase));
        buffer.setSample(0, sample, sineValue * 0.5f);
        buffer.setSample(1, sample, sineValue * 0.5f);
        phase += phaseIncrement;
    }

    float inputRMS = buffer.getRMSLevel(0, 0, buffer.getNumSamples());

    // Process through EQ
    eq_->processBlock(buffer);

    float outputRMS = buffer.getRMSLevel(0, 0, buffer.getNumSamples());

    // Output should be boosted (higher RMS) due to 6dB gain at 1kHz
    EXPECT_GT(outputRMS, inputRMS);
    EXPECT_LT(outputRMS / inputRMS, 4.0f); // Reasonable boost ratio
}

TEST_F(AnalogModeledEQTest, AnalogModeling) {
    // Test different analog models
    std::vector<audio::AnalogModeledEQ::AnalogModel> models = {
        audio::AnalogModeledEQ::AnalogModel::Clean,
        audio::AnalogModeledEQ::AnalogModel::NeveVintage,
        audio::AnalogModeledEQ::AnalogModel::SSLChannel,
        audio::AnalogModeledEQ::AnalogModel::TubePreamp
    };

    juce::AudioBuffer<float> buffer(2, 256);
    buffer.clear();

    // Fill with test signal
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        float value = static_cast<float>(sample) / buffer.getNumSamples() * 0.5f;
        buffer.setSample(0, sample, value);
        buffer.setSample(1, sample, value);
    }

    for (auto model : models) {
        eq_->setAnalogModel(model);

        juce::AudioBuffer<float> testBuffer = buffer; // Copy for each test
        eq_->processBlock(testBuffer);

        // Should process without crashing
        float outputRMS = testBuffer.getRMSLevel(0, 0, testBuffer.getNumSamples());
        EXPECT_GT(outputRMS, 0.0f);
        EXPECT_LT(outputRMS, 1.0f);
    }
}

TEST_F(AnalogModeledEQTest, BandSoloAndBypass) {
    // Enable multiple bands
    for (int band = 0; band < 3; ++band) {
        eq_->setBandEnabled(band, true);
        eq_->setBandGain(band, 3.0f); // Small boost
    }

    juce::AudioBuffer<float> buffer(2, 256);
    buffer.clear();

    // Fill with white noise-like signal
    juce::Random random;
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        float value = random.nextFloat() * 0.1f - 0.05f;
        buffer.setSample(0, sample, value);
        buffer.setSample(1, sample, value);
    }

    // Test normal processing
    auto normalBuffer = buffer;
    eq_->processBlock(normalBuffer);
    float normalRMS = normalBuffer.getRMSLevel(0, 0, normalBuffer.getNumSamples());

    // Test solo
    eq_->soloBand(1, true);
    auto soloBuffer = buffer;
    eq_->processBlock(soloBuffer);
    float soloRMS = soloBuffer.getRMSLevel(0, 0, soloBuffer.getNumSamples());

    // Solo should produce different output
    EXPECT_NE(normalRMS, soloRMS);

    // Test bypass all
    eq_->soloBand(1, false);
    eq_->bypassAll(true);
    auto bypassBuffer = buffer;
    eq_->processBlock(bypassBuffer);

    // Bypass should be close to original
    float originalRMS = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
    float bypassRMS = bypassBuffer.getRMSLevel(0, 0, bypassBuffer.getNumSamples());
    EXPECT_NEAR(originalRMS, bypassRMS, 0.01f);
}

// Performance benchmark test
TEST_F(AnalogModeledEQTest, PerformanceBenchmark) {
    // Enable all bands with moderate settings
    for (int band = 0; band < audio::AnalogModeledEQ::NUM_BANDS; ++band) {
        eq_->setBandEnabled(band, true);
        eq_->setBandType(band, audio::AnalogModeledEQ::BandType::Parametric);
        eq_->setBandFrequency(band, 100.0f * std::pow(10.0f, band)); // Spread across spectrum
        eq_->setBandGain(band, 3.0f);
        eq_->setBandQ(band, 1.5f);
        eq_->setBandDrive(band, 1.2f);
        eq_->setBandSaturation(band, 0.1f);
    }

    // Enable analog modeling
    eq_->setAnalogModel(audio::AnalogModeledEQ::AnalogModel::NeveVintage);
    eq_->setTubeWarmth(0.2f);
    eq_->setTapeSaturation(0.15f);

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();

    // Fill with complex signal
    juce::Random random;
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        float value = random.nextFloat() * 0.8f - 0.4f;
        buffer.setSample(0, sample, value);
        buffer.setSample(1, sample, value);
    }

    // Benchmark processing time
    auto startTime = juce::Time::getHighResolutionTicks();

    const int numIterations = 1000;
    for (int i = 0; i < numIterations; ++i) {
        auto testBuffer = buffer; // Fresh copy each time
        eq_->processBlock(testBuffer);
    }

    auto endTime = juce::Time::getHighResolutionTicks();
    auto totalTime = juce::Time::highResolutionTicksToSeconds(endTime - startTime);
    auto averageTime = totalTime / numIterations;

    // Average processing time should be reasonable for real-time use
    // At 48kHz with 512 samples, we have ~10.67ms per block
    // Processing should be much faster than this
    EXPECT_LT(averageTime, 0.005); // Less than 5ms per block

    std::cout << "Average processing time: " << (averageTime * 1000.0) << " ms per block" << std::endl;
    std::cout << "CPU usage estimate: " << (averageTime / 0.01067) * 100.0 << "%" << std::endl;
}

} // anonymous namespace
