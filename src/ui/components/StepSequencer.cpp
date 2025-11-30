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

    const auto padding = static_cast<float>(Spacing::small);

    // Draw steps
    for (int i = 0; i < numSteps; ++i)
    {
        const auto x = padding + i * (stepWidth + padding);
        const auto rect = juce::Rectangle<float>(x, padding, stepWidth, stepHeight);

        const auto& step = steps[static_cast<size_t>(i)];

        // Base background – slight bar grouping every 4 steps
        const bool isBarStart = (i % 4) == 0;
        auto baseColour = juce::Colour(Colors::surface);
        if (isBarStart)
            baseColour = juce::Colour(Colors::surface1);

        if (i == currentPlayPosition)
            baseColour = juce::Colour(Colors::primary).withAlpha(0.35f);

        g.setColour(baseColour);
        g.fillRoundedRectangle(rect, Radii::small);

        // Active indicator: pill with velocity-based brightness
        if (step.active)
        {
            const float velNorm = juce::jlimit(0.0f, 1.0f, static_cast<float>(step.velocity) / 127.0f);
            auto accent = juce::Colour(Colors::accent);
            accent = accent.interpolatedWith(juce::Colours::white, velNorm * 0.25f);

            auto filled = rect.reduced(4.0f);
            juce::ColourGradient grad(accent.brighter(0.2f),
                                      filled.getX(), filled.getY(),
                                      accent.darker(0.3f),
                                      filled.getX(), filled.getBottom(),
                                      false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(filled, Radii::small);

            // Top highlight
            auto hi = filled.withHeight(juce::jlimit(2.0f, 8.0f, filled.getHeight() * 0.2f));
            g.setColour(juce::Colour(Colors::glassHighlight));
            g.fillRoundedRectangle(hi, Radii::small);
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
        steps[static_cast<size_t>(stepIndex)].active = !steps[static_cast<size_t>(stepIndex)].active;
        updatePattern();
        if (onPatternChanged)
            onPatternChanged();
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

    // Clear existing steps
    steps.clear();
    steps.resize(static_cast<size_t>(numSteps));

    // Convert pattern MIDI notes to step data
    const auto& patternNotes = pattern->getNotes();
    for (const auto& midiNote : patternNotes)
    {
        const auto stepIndex = static_cast<int>(std::floor(midiNote.startBeat));
        if (stepIndex >= 0 && stepIndex < numSteps)
        {
            StepData stepData;
            stepData.active = true;
            stepData.velocity = midiNote.velocity;
            stepData.probability = juce::jlimit(0.0f, 1.0f, midiNote.probability);
            if (!juce::approximatelyEqual(midiNote.microTiming, 0.0f))
            {
                stepData.microTiming = juce::jlimit(-1.0f, 1.0f, midiNote.microTiming);
            }
            else
            {
                const auto offsetBeats = midiNote.startBeat - stepIndex;
                stepData.microTiming = juce::jlimit(-1.0f, 1.0f, static_cast<float>(offsetBeats * 2.0));
            }
            stepData.trigCondition = midiNote.trigCondition;
            steps[static_cast<size_t>(stepIndex)] = stepData;
        }
    }

    repaint();
}

void StepSequencer::updatePattern()
{
    if (!pattern)
        return;

    pattern->setNotes(buildMidiNotesFromSteps());
    commitSteps();
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

void StepSequencer::commitSteps()
{
    if (onStepsCommitted)
    {
        onStepsCommitted(buildMidiNotesFromSteps());
    }
}

std::vector<daw::project::Pattern::MIDINote> StepSequencer::buildMidiNotesFromSteps() const
{
    std::vector<daw::project::Pattern::MIDINote> notes;
    notes.reserve(steps.size());

    for (size_t i = 0; i < steps.size(); ++i)
    {
        const auto& step = steps[i];
        if (!step.active)
            continue;

        daw::project::Pattern::MIDINote midiNote;
        midiNote.note = 60; // TODO configurable pitch per lane
        midiNote.velocity = step.velocity;
        midiNote.startBeat = static_cast<double>(i) + (step.microTiming * 0.5);
        midiNote.lengthBeats = 0.25;
        midiNote.channel = 0;
        midiNote.probability = juce::jlimit(0.0f, 1.0f, step.probability);
        midiNote.microTiming = juce::jlimit(-1.0f, 1.0f, step.microTiming);
        midiNote.trigCondition = step.trigCondition;
        notes.push_back(midiNote);
    }

    return notes;
}

} // namespace daw::ui::components
