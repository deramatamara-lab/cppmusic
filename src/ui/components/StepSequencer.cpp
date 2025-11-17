#include "StepSequencer.h"
#include <algorithm>

namespace daw::ui::components
{

StepSequencer::StepSequencer()
{
    setNumSteps(16);
    millisecondsPerStep = calculateStepDurationMs(tempoBpm);
    lastStepAdvanceTimeMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz(60); // smooth UI updates
}

void StepSequencer::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    // Background
    g.fillAll(toColour(Colors::background));
    
    const auto padding = Spacing::small;
    
    // Draw steps
    for (int i = 0; i < numSteps; ++i)
    {
        const auto x = padding + i * (stepWidth + padding);
        const auto rect = juce::Rectangle<float>(x, padding, stepWidth, stepHeight);
        
        const auto& step = steps[i];
        
        // Background
        if (i == currentPlayPosition)
        {
            g.setColour(toColour(Colors::primary).withAlpha(0.3f));
        }
        else
        {
            g.setColour(toColour(Colors::surface));
        }
        g.fillRoundedRectangle(rect, Radii::small);
        
        // Active indicator
        if (step.active)
        {
            g.setColour(toColour(Colors::accent));
            g.fillEllipse(rect.reduced(4.0f));
        }
        
        // Border
        g.setColour(toColour(Colors::outline));
        g.drawRoundedRectangle(rect, Radii::small, 1.0f);
        
        // Step number
        g.setColour(toColour(Colors::textSecondary));
        g.setFont(Typography::caption);
        g.drawText(juce::String(i + 1), rect, juce::Justification::centredBottom);
    }
}

void StepSequencer::resized()
{
    const auto bounds = getLocalBounds().toFloat();
    const auto padding = static_cast<float>(daw::ui::lookandfeel::DesignSystem::Spacing::small);
    const auto availableWidth = bounds.getWidth() - (padding * (numSteps + 1));
    stepWidth = availableWidth / numSteps;
    stepHeight = bounds.getHeight() - (padding * 2);
    repaint();
}

void StepSequencer::mouseDown(const juce::MouseEvent& e)
{
    const auto stepIndex = getStepAtPosition(e.getPosition());
    if (stepIndex >= 0 && stepIndex < numSteps)
    {
        steps[stepIndex].active = !steps[stepIndex].active;
        updatePattern();
        repaint();
    }
}

void StepSequencer::mouseDrag(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    // Could implement velocity editing on drag
}

void StepSequencer::setTempo(double bpm)
{
    const auto clamped = juce::jlimit(20.0, 300.0, bpm);
    tempoBpm = clamped;
    millisecondsPerStep = calculateStepDurationMs(tempoBpm);
}

void StepSequencer::play()
{
    if (isPlaying)
        return;

    isPlaying = true;
    if (currentPlayPosition < 0)
        currentPlayPosition = 0;
    lastStepAdvanceTimeMs = juce::Time::getMillisecondCounterHiRes();
}

void StepSequencer::stop()
{
    if (!isPlaying)
        return;

    isPlaying = false;
    currentPlayPosition = -1;
    repaint();
}

void StepSequencer::setNumSteps(int newNumSteps)
{
    numSteps = juce::jmax(1, newNumSteps);
    steps.resize(numSteps);
    repaint();
}

void StepSequencer::setStep(int stepIndex, const StepData& data)
{
    if (stepIndex >= 0 && stepIndex < numSteps)
    {
        steps[stepIndex] = data;
        updatePattern();
        repaint();
    }
}

StepSequencer::StepData StepSequencer::getStep(int stepIndex) const
{
    if (stepIndex >= 0 && stepIndex < numSteps)
    {
        return steps[stepIndex];
    }
    return StepData{};
}

void StepSequencer::setPlayPosition(int step)
{
    currentPlayPosition = step;
    repaint();
}

void StepSequencer::setPattern(std::shared_ptr<daw::project::Pattern> newPattern)
{
    pattern = newPattern;
    updateFromPattern();
}

void StepSequencer::timerCallback()
{
    if (!isPlaying || numSteps <= 0)
        return;

    const auto now = juce::Time::getMillisecondCounterHiRes();
    if ((now - lastStepAdvanceTimeMs) < millisecondsPerStep)
        return;

    lastStepAdvanceTimeMs = now;
    currentPlayPosition = (currentPlayPosition + 1) % numSteps;
    repaint();
}

double StepSequencer::calculateStepDurationMs(double bpm) const noexcept
{
    constexpr double stepsPerBeat = 4.0; // 16th notes
    constexpr double msPerMinute = 60000.0;
    const auto safeBpm = juce::jmax(0.001, bpm);
    return (msPerMinute / safeBpm) / stepsPerBeat;
}

void StepSequencer::updateFromPattern()
{
    if (!pattern)
        return;
    
    // TODO: Convert pattern notes to step data
    // For now, this is a placeholder
}

void StepSequencer::updatePattern()
{
    if (!pattern)
        return;
    
    // TODO: Convert step data to pattern notes
    // For now, this is a placeholder
}

int StepSequencer::getStepAtPosition(const juce::Point<int>& pos) const
{
    const auto padding = static_cast<float>(daw::ui::lookandfeel::DesignSystem::Spacing::small);
    const auto x = static_cast<float>(pos.x) - padding;
    
    if (x < 0 || stepWidth <= 0)
        return -1;
    
    const auto stepIndex = static_cast<int>(x / (stepWidth + padding));
    return (stepIndex >= 0 && stepIndex < numSteps) ? stepIndex : -1;
}

} // namespace daw::ui::components

