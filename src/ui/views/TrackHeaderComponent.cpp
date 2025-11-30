#include "TrackHeaderComponent.h"
#include <iomanip>
#include <sstream>

using namespace daw::ui::lookandfeel::DesignSystem;

namespace daw::ui::views
{

TrackHeaderComponent::TrackHeaderComponent(std::shared_ptr<daw::audio::engine::EngineContext> engineContext,
                                           daw::project::Track* track)
    : engineContext(engineContext)
    , track(track)
    , muteButton("M")
    , soloButton("S")
    , recordButton("R")
{
    setupUI();
}

void TrackHeaderComponent::setupUI()
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    // Mute button
    addAndMakeVisible(muteButton);
    muteButton.setClickingTogglesState(true);
    muteButton.setTooltip("Mute track");
    muteButton.onClick = [this] { muteButtonClicked(); };

    // Solo button
    addAndMakeVisible(soloButton);
    soloButton.setClickingTogglesState(true);
    soloButton.setTooltip("Solo track");
    soloButton.onClick = [this] { soloButtonClicked(); };

    // Record arm button
    addAndMakeVisible(recordButton);
    recordButton.setClickingTogglesState(true);
    recordButton.setTooltip("Arm for recording");
    recordButton.onClick = [this] { recordButtonClicked(); };

    // Volume slider
    addAndMakeVisible(volumeSlider);
    volumeSlider.setRange(-60.0, 12.0, 0.1);
    volumeSlider.setValue(0.0); // 0dB default
    volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.setTooltip("Track volume");
    volumeSlider.onValueChange = [this] { volumeSliderChanged(); };

    // Pan slider
    addAndMakeVisible(panSlider);
    panSlider.setRange(-1.0, 1.0, 0.01);
    panSlider.setValue(0.0); // Center pan
    panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panSlider.setTooltip("Pan position");
    panSlider.onValueChange = [this] { panSliderChanged(); };

    // Volume label
    addAndMakeVisible(volumeLabel);
    volumeLabel.setJustificationType(juce::Justification::centred);
    volumeLabel.setFont(getMonoFont(Typography::caption));
    volumeLabel.setText("0dB", juce::dontSendNotification);

    // Pan label
    addAndMakeVisible(panLabel);
    panLabel.setJustificationType(juce::Justification::centred);
    panLabel.setFont(getMonoFont(Typography::caption));
    panLabel.setText("C", juce::dontSendNotification);

    // Track name label
    addAndMakeVisible(trackNameLabel);
    trackNameLabel.setJustificationType(juce::Justification::centredLeft);
    trackNameLabel.setFont(getBodyFont(Typography::bodySmall));
    trackNameLabel.setEditable(true, true);
    trackNameLabel.onTextChange = [this]
    {
        if (onNameChanged && track)
        {
            onNameChanged(track->getIndex(), trackNameLabel.getText());
        }
    };

    updateButtonStates();
}

void TrackHeaderComponent::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    auto bounds = getLocalBounds().toFloat();

    // Background with glass effect
    drawGlassPanel(g, bounds, Radii::small, false);

    // Track colour accent strip
    if (track)
    {
        drawTrackColourStrip(g, bounds);
    }

    // Level meters area
    auto meterBounds = bounds.removeFromRight(8.0f).reduced(2.0f);
    if (meterBounds.getWidth() > 0)
    {
        drawLevelMeter(g, meterBounds);
    }

    // Border
    g.setColour(juce::Colour(Colors::divider));
    g.drawRect(bounds, 1.0f);
}

void TrackHeaderComponent::resized()
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    auto bounds = getLocalBounds().reduced(Spacing::xsmall);

    // Track colour strip space
    bounds.removeFromLeft(6);

    // Level meter space
    bounds.removeFromRight(12);

    const int buttonSize = 20;
    const int sliderHeight = bounds.getHeight() - 60; // Leave space for name and labels

    // Top row: MSR buttons
    auto topRow = bounds.removeFromTop(buttonSize + Spacing::xsmall);
    muteButton.setBounds(topRow.removeFromLeft(buttonSize));
    topRow.removeFromLeft(Spacing::xsmall);
    soloButton.setBounds(topRow.removeFromLeft(buttonSize));
    topRow.removeFromLeft(Spacing::xsmall);
    recordButton.setBounds(topRow.removeFromLeft(buttonSize));

    bounds.removeFromTop(Spacing::small);

    // Volume slider (vertical)
    auto volumeArea = bounds.removeFromLeft(30);
    volumeSlider.setBounds(volumeArea.removeFromTop(sliderHeight));
    volumeArea.removeFromTop(Spacing::xsmall);
    volumeLabel.setBounds(volumeArea.removeFromTop(12));

    bounds.removeFromLeft(Spacing::small);

    // Pan slider and controls
    auto panArea = bounds.removeFromTop(24);
    panSlider.setBounds(panArea.removeFromTop(16));
    panArea.removeFromTop(Spacing::xsmall);
    panLabel.setBounds(panArea);

    bounds.removeFromTop(Spacing::small);

    // Track name at bottom
    trackNameLabel.setBounds(bounds.removeFromBottom(20));
}

void TrackHeaderComponent::updateButtonStates()
{
    if (!track)
        return;

    muteButton.setToggleState(track->isMuted(), juce::dontSendNotification);
    soloButton.setToggleState(track->isSoloed(), juce::dontSendNotification);
    recordButton.setToggleState(track->isRecordArmed(), juce::dontSendNotification);

    // Update track name
    trackNameLabel.setText(track->getName(), juce::dontSendNotification);

    // Update volume/pan values if needed
    volumeSlider.setValue(track->getGainDb(), juce::dontSendNotification);
    panSlider.setValue(track->getPan(), juce::dontSendNotification);

    // Update labels
    volumeLabel.setText(juce::String(track->getGainDb(), 1) + "dB", juce::dontSendNotification);

    auto panValue = track->getPan();
    juce::String panText = "C";
    if (panValue < -0.01f)
        panText = "L" + juce::String(static_cast<int>(std::abs(panValue) * 100));
    else if (panValue > 0.01f)
        panText = "R" + juce::String(static_cast<int>(panValue * 100));
    panLabel.setText(panText, juce::dontSendNotification);
}

void TrackHeaderComponent::updateMeters()
{
    if (!engineContext || !track)
        return;

    const auto meterData = engineContext->getTrackMeter(track->getIndex());
    leftPeak = meterData.peak;
    rightPeak = meterData.peak; // Mono for now
    leftRms = meterData.rms;
    rightRms = meterData.rms;

    repaint(); // Only repaint meter area in optimized version
}

void TrackHeaderComponent::setTrack(daw::project::Track* newTrack)
{
    track = newTrack;
    updateButtonStates();
    repaint();
}

void TrackHeaderComponent::refresh()
{
    updateButtonStates();
    updateMeters();
}

void TrackHeaderComponent::drawLevelMeter(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    // Background
    g.setColour(juce::Colour(Colors::surface1));
    g.fillRoundedRectangle(bounds, 2.0f);

    // Peak level bars
    const float peakHeight = bounds.getHeight() * juce::jlimit(0.0f, 1.0f, (leftPeak + 60.0f) / 72.0f);
    const float rmsHeight = bounds.getHeight() * juce::jlimit(0.0f, 1.0f, (leftRms + 60.0f) / 72.0f);

    // RMS level (darker)
    auto rmsColour = juce::Colour(Colors::meterNormal).withAlpha(0.6f);
    if (leftRms > -6.0f) rmsColour = juce::Colour(Colors::warning).withAlpha(0.6f);
    if (leftRms > -1.0f) rmsColour = juce::Colour(Colors::danger).withAlpha(0.6f);

    g.setColour(rmsColour);
    g.fillRoundedRectangle(bounds.withTop(bounds.getBottom() - rmsHeight), 2.0f);

    // Peak level (brighter)
    auto peakColour = juce::Colour(Colors::meterNormal);
    if (leftPeak > -6.0f) peakColour = juce::Colour(Colors::warning);
    if (leftPeak > -1.0f) peakColour = juce::Colour(Colors::danger);

    g.setColour(peakColour);
    g.fillRoundedRectangle(bounds.withTop(bounds.getBottom() - peakHeight).withWidth(2.0f), 1.0f);
}

void TrackHeaderComponent::drawTrackColourStrip(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    const auto accentColour = Tracks::colourForIndex(track->getIndex());
    g.setColour(accentColour);
    g.fillRoundedRectangle(bounds.withWidth(4.0f), 2.0f);
}

void TrackHeaderComponent::muteButtonClicked()
{
    if (!track)
        return;

    const bool newMuteState = muteButton.getToggleState();

    if (onMuteChanged)
        onMuteChanged(track->getIndex(), newMuteState);

    if (engineContext)
        engineContext->setTrackMute(track->getIndex(), newMuteState);
}

void TrackHeaderComponent::soloButtonClicked()
{
    if (!track)
        return;

    const bool newSoloState = soloButton.getToggleState();

    if (onSoloChanged)
        onSoloChanged(track->getIndex(), newSoloState);

    if (engineContext)
        engineContext->setTrackSolo(track->getIndex(), newSoloState);
}

void TrackHeaderComponent::recordButtonClicked()
{
    if (!track)
        return;

    const bool newRecordState = recordButton.getToggleState();

    if (onRecordArmChanged)
        onRecordArmChanged(track->getIndex(), newRecordState);

    // TODO: Add record arm support to EngineContext when available
}

void TrackHeaderComponent::volumeSliderChanged()
{
    if (!track)
        return;

    const auto newVolume = static_cast<float>(volumeSlider.getValue());

    if (onVolumeChanged)
        onVolumeChanged(track->getIndex(), newVolume);

    if (engineContext)
        engineContext->setTrackGain(track->getIndex(), newVolume);

    // Update label
    volumeLabel.setText(juce::String(newVolume, 1) + "dB", juce::dontSendNotification);
}

void TrackHeaderComponent::panSliderChanged()
{
    if (!track)
        return;

    const auto newPan = static_cast<float>(panSlider.getValue());

    if (onPanChanged)
        onPanChanged(track->getIndex(), newPan);

    if (engineContext)
        engineContext->setTrackPan(track->getIndex(), newPan);

    // Update label
    juce::String panText = "C";
    if (newPan < -0.01f)
        panText = "L" + juce::String(static_cast<int>(std::abs(newPan) * 100));
    else if (newPan > 0.01f)
        panText = "R" + juce::String(static_cast<int>(newPan * 100));
    panLabel.setText(panText, juce::dontSendNotification);
}

void TrackHeaderComponent::mouseDown(const juce::MouseEvent& e)
{
    // Handle track selection
    Component::mouseDown(e);
}

void TrackHeaderComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    // Double-click on track name area to edit
    if (trackNameLabel.getBounds().contains(e.getPosition()))
    {
        startNameEdit();
    }
}

void TrackHeaderComponent::startNameEdit()
{
    trackNameLabel.showEditor();
}

void TrackHeaderComponent::finishNameEdit()
{
    // This is handled by the label's onTextChange callback
}

} // namespace daw::ui::views
