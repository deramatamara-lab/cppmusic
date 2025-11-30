#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <unordered_map>
#include <vector>

#include "AuroraReverb.h"
#include "effects/NebulaDelay.hpp"

namespace cppmusic {
namespace ui { class NebulaAuroraAudioEditor; }
namespace audio {

class NebulaAuroraAudioProcessor final : public juce::AudioProcessor
{
public:
    NebulaAuroraAudioProcessor();
    ~NebulaAuroraAudioProcessor() override;

    // AudioProcessor overrides
    const juce::String getName() const override { return "NebulaAurora"; }
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Init"; }
    void changeProgramName(int, const juce::String&) override {}

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void setPlayHead(juce::AudioPlayHead* newPlayHead) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Preset API
    struct Preset { juce::String name; juce::ValueTree state; bool isFactory = false; };
    const std::vector<Preset>& getPresets() const noexcept { return presets_; }
    int getCurrentPresetIndex() const noexcept { return currentPresetIndex_; }
    void loadPreset(int index);
    void storeUserPreset(const juce::String& name);

    // Expose child processors (read-only) for embedded UIs
    NebulaDelayAudioProcessor& getDelayProcessor() noexcept { return delayProcessor_; }
    AuroraReverbAudioProcessor& getReverbProcessor() noexcept { return reverbProcessor_; }
    const juce::AudioProcessorValueTreeState& getValueTreeState() const noexcept { return apvts_; }
    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return apvts_; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    struct ParameterBridge;
    struct ListenerRegistration
    {
        juce::AudioProcessorValueTreeState* state = nullptr;
        std::unique_ptr<ParameterBridge> listener;
    };

    void initialiseParameterSync();
    void registerParameterBridge(const juce::String& compositeId,
                                 juce::AudioProcessorValueTreeState& sourceState,
                                 const juce::String& sourceId,
                                 juce::RangedAudioParameter* targetParameter);
    void syncAllParametersToSubProcessors();
    void syncAllParametersFromSubProcessors();
    void initialiseFactoryPresets();

    NebulaDelayAudioProcessor delayProcessor_;
    AuroraReverbAudioProcessor reverbProcessor_;
    juce::AudioProcessorValueTreeState apvts_;

    std::unordered_map<juce::String, juce::RangedAudioParameter*> compositeToSub_;
    std::vector<ListenerRegistration> listenerRegistrations_;
    std::vector<Preset> presets_;
    int currentPresetIndex_ = 0;
    int factoryPresetCount_ = 0;
    std::atomic<int> recursionGuard_{0};
};

} // namespace audio
} // namespace cppmusic
