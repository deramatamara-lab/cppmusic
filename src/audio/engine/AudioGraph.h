#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>
#include <memory>
#include <atomic>

namespace daw::audio::dsp
{
    class TrackStrip;
}

namespace daw::audio::synthesis
{
    class Oscillator;
}

namespace daw::audio::engine
{

/**
 * @brief Audio processing graph
 *
 * Implements JUCE AudioProcessor interface.
 * Manages collection of TrackStrip instances and sums to master bus.
 * Follows DAW_DEV_RULES: real-time safe, no allocations/locks in processBlock.
 */
class AudioGraph : public juce::AudioProcessor
{
public:
    AudioGraph();
    ~AudioGraph() override;

    // JUCE AudioProcessor interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    // Processor info
    const juce::String getName() const override { return "DAW Audio Graph"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void getStateInformation(juce::MemoryBlock& destData) override { juce::ignoreUnused(destData); }
    void setStateInformation(const void* data, int sizeInBytes) override { juce::ignoreUnused(data, sizeInBytes); }

    // Track management (call from non-audio thread)
    daw::audio::dsp::TrackStrip* addTrack();
    void removeTrack(int index);
    daw::audio::dsp::TrackStrip* getTrack(int index);
    [[nodiscard]] int getNumTracks() const noexcept { return static_cast<int>(trackStrips.size()); }

    // Master gain (atomic for thread safety)
    void setMasterGain(float gainDb) noexcept;
    [[nodiscard]] float getMasterGain() const noexcept;

    // Master metering (lock-free, safe from UI thread)
    struct MeterData
    {
        float peak;
        float rms;
    };
    [[nodiscard]] MeterData getMasterMeter() const noexcept;

private:
    std::vector<std::unique_ptr<daw::audio::dsp::TrackStrip>> trackStrips;
    std::atomic<float> masterGainLinear;

    // Test oscillator for basic audio verification (defined in .cpp to avoid incomplete type)
    std::unique_ptr<daw::audio::synthesis::Oscillator> testOscillator;

    // Master metering (lock-free, updated in audio thread)
    std::atomic<float> masterPeakLevel{0.0f};
    std::atomic<float> masterRmsLevel{0.0f};

    // Pre-allocated buffers (no allocations in processBlock)
    juce::AudioBuffer<float> masterBuffer;
    std::vector<juce::AudioBuffer<float>> trackBuffers; // One per track

    [[nodiscard]] static float dbToLinear(float db) noexcept;
    void updateMasterMeters(const juce::AudioBuffer<float>& buffer) noexcept;
};

} // namespace daw::audio::engine

