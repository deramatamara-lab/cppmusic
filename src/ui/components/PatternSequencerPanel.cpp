#include "PatternSequencerPanel.h"
#include "../lookandfeel/DesignSystem.h"
#include "../../project/PatternJsonParser.h"

namespace daw::ui::components
{

using daw::ui::lookandfeel::getDesignTokens;
using namespace daw::ui::lookandfeel::DesignSystem;

PatternSequencerPanel::PatternSequencerPanel()
{
    tokens = &getDesignTokens();
    setupUI();
}

void PatternSequencerPanel::setupUI()
{
    headerLabel.setText("Pattern Sequencer", juce::dontSendNotification);
    headerLabel.setJustificationType(juce::Justification::centredLeft);
    if (tokens != nullptr)
    {
        headerLabel.setFont(tokens->type.title());
        headerLabel.setColour(juce::Label::textColourId, tokens->colours.textSecondary);
    }
    addAndMakeVisible(headerLabel);

    patternSelector.addItem("No Pattern", 1);
    patternSelector.setSelectedId(1, juce::dontSendNotification);
    patternSelector.onChange = [this] { patternSelectorChanged(); };
    addAndMakeVisible(patternSelector);

    newPatternButton.setButtonText("New");
    newPatternButton.onClick = [this] { newPatternButtonClicked(); };
    addAndMakeVisible(newPatternButton);

    renamePatternButton.setButtonText("Rename");
    renamePatternButton.onClick = [this] { renamePatternButtonClicked(); };
    addAndMakeVisible(renamePatternButton);

    // AI buttons
    aiGeneratePatternButton.setButtonText("AI Generate");
    aiGeneratePatternButton.onClick = [this] { aiGeneratePatternClicked(); };
    aiGeneratePatternButton.setEnabled(false);
    addAndMakeVisible(aiGeneratePatternButton);

    aiFillPatternButton.setButtonText("AI Fill");
    aiFillPatternButton.onClick = [this] { aiFillPatternClicked(); };
    aiFillPatternButton.setEnabled(false);
    addAndMakeVisible(aiFillPatternButton);

    addAndMakeVisible(stepSequencer);

    // Wire step sequencer to update pattern when steps change
    stepSequencer.onPatternChanged = [this] { updateStepSequencerFromPattern(); };
}

void PatternSequencerPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    drawGlassPanel(g, bounds, Radii::medium, true);
}

void PatternSequencerPanel::resized()
{
    auto area = getLocalBounds().reduced(Spacing::small);

    auto header = area.removeFromTop(24);
    headerLabel.setBounds(header.removeFromLeft(200));

    // Pattern selector and buttons
    auto controls = area.removeFromTop(24);
    const int gap = Spacing::xsmall;

    patternSelector.setBounds(controls.removeFromLeft(140));
    controls.removeFromLeft(gap);
    newPatternButton.setBounds(controls.removeFromLeft(60));
    controls.removeFromLeft(gap);
    renamePatternButton.setBounds(controls.removeFromLeft(80));
    controls.removeFromLeft(gap);
    aiGeneratePatternButton.setBounds(controls.removeFromLeft(90));
    controls.removeFromLeft(gap);
    aiFillPatternButton.setBounds(controls.removeFromLeft(80));

    area.removeFromTop(gap);
    stepSequencer.setBounds(area);
}

void PatternSequencerPanel::setTempo(double bpm)
{
    stepSequencer.setTempo(bpm);
}

void PatternSequencerPanel::setIsPlaying(bool isPlaying)
{
    if (isPlaying)
        stepSequencer.play();
    else
        stepSequencer.stop();
}

void PatternSequencerPanel::setProjectModel(std::shared_ptr<daw::project::ProjectModel> model)
{
    projectModel = model;
    updatePatternList();
}

void PatternSequencerPanel::setPattern(uint32_t patternId)
{
    currentPatternId = patternId;

    if (projectModel != nullptr)
    {
        auto* pattern = projectModel->getPattern(patternId);
        if (pattern != nullptr)
        {
            stepSequencer.setNumSteps(pattern->getNumSteps());
            // Create a shared_ptr that doesn't own the pattern (projectModel owns it)
            // This is safe because projectModel outlives the step sequencer
            stepSequencer.setPattern(std::shared_ptr<daw::project::Pattern>(projectModel, pattern));

            // Update selector
            const auto patterns = projectModel->getPatterns();
            for (size_t i = 0; i < patterns.size(); ++i)
            {
                if (patterns[i]->getId() == patternId)
                {
                    patternSelector.setSelectedItemIndex(static_cast<int>(i) + 1, juce::sendNotification);
                    break;
                }
            }
        }
    }
}

void PatternSequencerPanel::updatePatternList()
{
    patternSelector.clear(juce::dontSendNotification);
    patternSelector.addItem("No Pattern", 1);

    if (projectModel != nullptr)
    {
        const auto patterns = projectModel->getPatterns();
        for (size_t i = 0; i < patterns.size(); ++i)
        {
            const auto* pattern = patterns[i];
            patternSelector.addItem(pattern->getName(), static_cast<int>(i + 2));
        }
    }

    if (currentPatternId != 0)
    {
        setPattern(currentPatternId);
    }
}

void PatternSequencerPanel::patternSelectorChanged()
{
    const auto selectedId = patternSelector.getSelectedId();
    if (selectedId == 1) // "No Pattern"
    {
        currentPatternId = 0;
        stepSequencer.setPattern(nullptr);
    }
    else if (projectModel != nullptr)
    {
        const auto patterns = projectModel->getPatterns();
        const auto index = selectedId - 2; // Account for "No Pattern" item
        if (index >= 0 && index < static_cast<int>(patterns.size()))
        {
            setPattern(patterns[static_cast<size_t>(index)]->getId());
        }
    }
}

void PatternSequencerPanel::setInferenceEngine(std::shared_ptr<daw::ai::inference::InferenceEngine> engine)
{
    inferenceEngine = engine;
    const bool enabled = engine != nullptr && engine->isReady();
    aiGeneratePatternButton.setEnabled(enabled);
    aiFillPatternButton.setEnabled(enabled);
}

void PatternSequencerPanel::aiGeneratePatternClicked()
{
    if (inferenceEngine == nullptr || !inferenceEngine->isReady() || isAIGenerating)
        return;

    if (projectModel == nullptr || currentPatternId == 0)
        return;

    isAIGenerating = true;
    aiGeneratePatternButton.setEnabled(false);

    std::string prompt = "Generate a 16-step drum pattern with kick, snare, and hi-hat. Return step data in JSON format.";

    inferenceEngine->queueTextInference(prompt, [this](const std::string& result, bool success)
    {
        juce::MessageManager::callAsync([this, result, success]()
        {
            isAIGenerating = false;
            aiGeneratePatternButton.setEnabled(true);

            if (success && projectModel != nullptr && currentPatternId != 0)
            {
                daw::project::ParsedPatternFromJson parsed;
                if (daw::project::parsePatternFromJson(juce::String(result), parsed))
                {
                    auto* pattern = projectModel->getPattern(currentPatternId);
                    if (pattern != nullptr)
                    {
                        pattern->setNumSteps(parsed.numSteps);
                        projectModel->setPatternNotes(currentPatternId, parsed.notes);
                        stepSequencer.setNumSteps(parsed.numSteps);
                        stepSequencer.setPattern(std::shared_ptr<daw::project::Pattern>(projectModel, pattern));
                        updateStepSequencerFromPattern();
                        return;
                    }
                }

                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "AI Pattern Error",
                    "Could not parse AI pattern response. The pattern was not changed.");
            }
        });
    });
}

void PatternSequencerPanel::aiFillPatternClicked()
{
    if (inferenceEngine == nullptr || !inferenceEngine->isReady() || isAIGenerating)
        return;

    if (projectModel == nullptr || currentPatternId == 0)
        return;

    isAIGenerating = true;
    aiFillPatternButton.setEnabled(false);

    // Get current pattern state
    const auto* pattern = projectModel->getPattern(currentPatternId);
    if (pattern == nullptr)
    {
        isAIGenerating = false;
        aiFillPatternButton.setEnabled(true);
        return;
    }

    const auto existingNotes = pattern->getNotes();
    std::string prompt = "Fill in the remaining steps of this pattern. Existing notes: ";
    for (const auto& note : existingNotes)
    {
        prompt += std::to_string(note.note) + "@" + std::to_string(static_cast<int>(note.startBeat)) + " ";
    }
    prompt += ". Return complete step data in JSON format.";

    inferenceEngine->queueTextInference(prompt, [this](const std::string& result, bool success)
    {
        juce::MessageManager::callAsync([this, result, success]()
        {
            isAIGenerating = false;
            aiFillPatternButton.setEnabled(true);

            if (success && projectModel != nullptr && currentPatternId != 0)
            {
                daw::project::ParsedPatternFromJson parsed;
                if (daw::project::parsePatternFromJson(juce::String(result), parsed))
                {
                    auto* patternToFill = projectModel->getPattern(currentPatternId);
                    if (patternToFill != nullptr)
                    {
                        patternToFill->setNumSteps(parsed.numSteps);
                        projectModel->setPatternNotes(currentPatternId, parsed.notes);
                        stepSequencer.setNumSteps(parsed.numSteps);
                        stepSequencer.setPattern(std::shared_ptr<daw::project::Pattern>(projectModel, patternToFill));
                        updateStepSequencerFromPattern();
                        return;
                    }
                }

                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "AI Pattern Error",
                    "Could not parse AI fill response. The pattern was not changed.");
            }
        });
    });
}

void PatternSequencerPanel::newPatternButtonClicked()
{
    if (projectModel != nullptr)
    {
        auto* pattern = projectModel->addPattern("New Pattern");
        updatePatternList();
        setPattern(pattern->getId());
    }
}

void PatternSequencerPanel::renamePatternButtonClicked()
{
    if (projectModel != nullptr && currentPatternId != 0)
    {
        auto* pattern = projectModel->getPattern(currentPatternId);
        if (pattern != nullptr)
        {
            auto* window = new juce::AlertWindow("Rename Pattern", "Enter new name:", juce::AlertWindow::NoIcon);
            window->addTextEditor("patternName", pattern->getName(), "Pattern Name");
            window->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
            window->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
            window->centreAroundComponent(this, 360, 180);

            juce::Component::SafePointer<juce::AlertWindow> safeWindow(window);
            window->enterModalState(true, juce::ModalCallbackFunction::create([this, pattern, safeWindow](int result)
            {
                if (result == 1 && safeWindow != nullptr)
                {
                    const auto input = safeWindow->getTextEditor("patternName")->getText().trim();
                    if (input.isNotEmpty() && projectModel != nullptr)
                    {
                        pattern->setName(input.toStdString());
                        updatePatternList();
                    }
                }
            }));
        }
    }
}

void PatternSequencerPanel::updateStepSequencerFromPattern()
{
    // Step sequencer calls this when steps are toggled
    // The pattern is already updated by StepSequencer::updatePattern()
    // This is a hook for any additional UI updates
}

} // namespace daw::ui::components
