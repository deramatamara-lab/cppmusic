#include "AnalogEQEditor.h"
#include "src/audio/MasterAudioProcessor.h"
#include "src/core/EngineContext.h"
#include "src/core/RTMemoryPool.h"
#include "src/core/ServiceLocator.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace cppmusic {
namespace ui {

/**
 * Example demonstrating how to integrate AnalogEQEditor with MasterAudioProcessor
 *
 * This shows the proper wiring pattern for connecting the EQ editor UI
 * to the audio processing engine.
 */
class AnalogEQEditorExample : public juce::Component
{
public:
    AnalogEQEditorExample(audio::MasterAudioProcessor& processor,
                         core::EngineContext& context)
        : processor_(processor)
        , context_(context)
    {
        // Create the EQ editor, connecting it to the master processor's EQ
        eqEditor_ = std::make_unique<AnalogEQEditor>(
            processor_.getEQ(),  // Get reference to the EQ from master processor
            context_             // Pass engine context for real-time communication
        );

        addAndMakeVisible(*eqEditor_);
        setSize(900, 600);
    }

    void resized() override
    {
        if (eqEditor_)
            eqEditor_->setBounds(getLocalBounds());
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
    }

private:
    audio::MasterAudioProcessor& processor_;
    core::EngineContext& context_;
    std::unique_ptr<AnalogEQEditor> eqEditor_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalogEQEditorExample)
};

/**
 * Standalone window for testing the AnalogEQEditor
 *
 * This can be used for development and testing of the EQ editor UI
 * without needing the full DAW application.
 */
class AnalogEQEditorWindow : public juce::DocumentWindow
{
public:
    AnalogEQEditorWindow()
        : juce::DocumentWindow("Analog EQ Editor - Test Window",
                              juce::Colours::darkgrey,
                              juce::DocumentWindow::allButtons)
    {
        // Create engine context and memory pool
        engineContext_ = std::make_unique<core::EngineContext>();
        memoryPool_ = std::make_unique<core::RTMemoryPool>(1024 * 1024); // 1MB pool
        serviceLocator_ = std::make_unique<core::ServiceLocator>();

        // Create master processor
        processor_ = std::make_unique<audio::MasterAudioProcessor>(
            *engineContext_,
            *memoryPool_,
            *serviceLocator_
        );

        // Configure the processor
        audio::MasterAudioProcessor::Config config;
        config.sampleRate = 48000.0;
        config.maxBlockSize = 512;
        config.enableAnalogEQ = true;
        config.enableSynthesizer = false;  // Disable synth for EQ-only testing
        config.enableSpectralAnalysis = false;
        processor_->prepare(config);

        // Configure the EQ with some default settings
        auto& eq = processor_->getEQ();
        audio::AnalogModeledEQ::Config eqConfig;
        eqConfig.sampleRate = 48000.0;
        eqConfig.maxBlockSize = 512;
        eqConfig.analogModel = audio::AnalogModeledEQ::AnalogModel::NeveVintage;
        eqConfig.enableOversampling = true;
        eq.prepare(eqConfig);

        // Enable some bands by default for testing
        eq.setBandEnabled(0, true);
        eq.setBandType(0, audio::AnalogModeledEQ::BandType::LowShelf);
        eq.setBandFrequency(0, 100.0f);
        eq.setBandGain(0, 3.0f);

        eq.setBandEnabled(2, true);
        eq.setBandType(2, audio::AnalogModeledEQ::BandType::Parametric);
        eq.setBandFrequency(2, 1000.0f);
        eq.setBandGain(2, 0.0f);
        eq.setBandQ(2, 2.0f);

        eq.setBandEnabled(4, true);
        eq.setBandType(4, audio::AnalogModeledEQ::BandType::HighShelf);
        eq.setBandFrequency(4, 8000.0f);
        eq.setBandGain(4, 2.0f);

        // Create and show the editor example
        editorExample_ = std::make_unique<AnalogEQEditorExample>(
            *processor_,
            *engineContext_
        );

        setContentOwned(editorExample_.get(), true);
        setResizable(true, true);
        centreWithSize(900, 600);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

private:
    std::unique_ptr<core::EngineContext> engineContext_;
    std::unique_ptr<core::RTMemoryPool> memoryPool_;
    std::unique_ptr<core::ServiceLocator> serviceLocator_;
    std::unique_ptr<audio::MasterAudioProcessor> processor_;
    std::unique_ptr<AnalogEQEditorExample> editorExample_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalogEQEditorWindow)
};

} // namespace ui
} // namespace cppmusic
