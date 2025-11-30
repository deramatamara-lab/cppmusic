#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "StepSequencer.h"
#include "../lookandfeel/DesignTokens.h"
#include "../../project/ProjectModel.h"
#include "../../ai/inference/InferenceEngine.h"
#include <memory>

namespace daw::ui::components
{

class PatternSequencerPanel : public juce::Component
{
public:
    PatternSequencerPanel();
    ~PatternSequencerPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setTempo(double bpm);
    void setIsPlaying(bool isPlaying);

    /**
     * @brief Set project model for pattern management
     */
    void setProjectModel(std::shared_ptr<daw::project::ProjectModel> model);

    /**
     * @brief Set current pattern to edit
     */
    void setPattern(uint32_t patternId);

    /**
     * @brief Set AI inference engine
     */
    void setInferenceEngine(std::shared_ptr<daw::ai::inference::InferenceEngine> engine);

private:
    const daw::ui::lookandfeel::DesignTokens* tokens { nullptr };
    std::shared_ptr<daw::project::ProjectModel> projectModel;
    StepSequencer stepSequencer;
    juce::Label headerLabel;
    juce::ComboBox patternSelector;
    juce::TextButton newPatternButton;
    juce::TextButton renamePatternButton;

    // AI buttons
    juce::TextButton aiGeneratePatternButton;
    juce::TextButton aiFillPatternButton;
    std::shared_ptr<daw::ai::inference::InferenceEngine> inferenceEngine;
    bool isAIGenerating{false};

    uint32_t currentPatternId{0};

    void setupUI();
    void updatePatternList();
    void patternSelectorChanged();
    void newPatternButtonClicked();
    void renamePatternButtonClicked();
    void updateStepSequencerFromPattern();
    void aiGeneratePatternClicked();
    void aiFillPatternClicked();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternSequencerPanel)
};

} // namespace daw::ui::components
