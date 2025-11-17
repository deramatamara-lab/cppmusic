#include "MainComponent.h"

namespace daw
{

MainComponent::MainComponent()
    : titleLabel("titleLabel", "DAW Project")
{
    tokens = &daw::ui::lookandfeel::getDesignTokens();
    jassert(tokens != nullptr);
    setupUI();
    applyDesignSystem();
}

MainComponent::~MainComponent()
{
    setLookAndFeel(nullptr);
}

void MainComponent::setupUI()
{
    lookAndFeel = std::make_unique<daw::ui::lookandfeel::MainLookAndFeel>();
    setLookAndFeel(lookAndFeel.get());
    
    // Setup title
    titleLabel.setJustificationType(juce::Justification::centred);
    if (tokens != nullptr)
    {
        titleLabel.setFont(tokens->type.heading());
        titleLabel.setColour(juce::Label::textColourId, tokens->colours.textPrimary);
    }
    addAndMakeVisible(titleLabel);
    
    // Add components
    addAndMakeVisible(waveformViewer);
    addAndMakeVisible(controlPanel);
    flagshipPanel.setTitle("AI Mastering Suite");
    addAndMakeVisible(flagshipPanel);
    addAndMakeVisible(patternSequencer);
    addAndMakeVisible(sessionLauncher);

    // Temporary transport wiring until real engine hooks exist
    constexpr double demoTempo = 128.0;
    constexpr bool demoIsPlaying = true;
    patternSequencer.setTempo(demoTempo);
    patternSequencer.setIsPlaying(demoIsPlaying);
    sessionLauncher.setTempo(demoTempo);
    sessionLauncher.setIsPlaying(demoIsPlaying);
    sessionLauncher.setLooping(true);
}

void MainComponent::applyDesignSystem()
{
    if (tokens == nullptr)
        return;

    // Apply design system colors
    setColour(juce::DocumentWindow::backgroundColourId, tokens->colours.background);
}

void MainComponent::paint(juce::Graphics& g)
{
    if (tokens == nullptr)
        return;

    // Fill background with design system color
    g.fillAll(tokens->colours.background);

    // Draw header area
    auto headerBounds = getLocalBounds().removeFromTop(60);
    g.setColour(tokens->colours.panelBackground);
    g.fillRect(headerBounds);

    // Draw border
    g.setColour(tokens->colours.accentPrimary.withAlpha(0.3f));
    g.drawRect(headerBounds, 1);
}

void MainComponent::resized()
{
    if (tokens == nullptr)
        return;
    
    auto bounds = getLocalBounds();
    const auto margin = tokens->spacing.md;
    
    // Header area
    auto headerBounds = bounds.removeFromTop(60);
    titleLabel.setBounds(headerBounds.reduced(margin));
    
    // Main content area
    bounds.reduce(margin, margin);
    
    // Reserve space for flagship panel on the right
    auto flagshipBounds = bounds.removeFromRight(360);
    flagshipPanel.setBounds(flagshipBounds.reduced(margin));
    bounds.removeFromRight(margin);
    
    // Split remaining area into waveform, mid controls, and bottom session launcher
    auto waveformBounds = bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.45f));
    auto bottomBounds = bounds.removeFromBottom(juce::jmax(180, bounds.getHeight() / 2));
    auto middleBounds = bounds;

    // Middle: control panel (left) and pattern sequencer (right)
    auto sequencerBounds = middleBounds.removeFromRight(middleBounds.getWidth() / 2);

    waveformViewer.setBounds(waveformBounds.reduced(margin));
    controlPanel.setBounds(middleBounds.reduced(margin));
    patternSequencer.setBounds(sequencerBounds.reduced(margin));
    sessionLauncher.setBounds(bottomBounds.reduced(margin));
}

} // namespace daw

