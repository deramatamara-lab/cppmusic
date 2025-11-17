#include "MixerStrip.h"
#include "../lookandfeel/DesignSystem.h"
#include "../lookandfeel/CustomLookAndFeel.h"

namespace daw::ui::views
{

MixerStrip::MixerStrip(std::shared_ptr<daw::audio::engine::EngineContext> engineContext,
                       daw::project::Track* track, int trackIndex)
    : engineContext(engineContext)
    , track(track)
    , trackIndex(trackIndex)
    , trackNameLabel("", track != nullptr ? track->getName() : "Track")
    , fader(juce::Slider::LinearVertical, juce::Slider::TextBoxBelow)
    , panSlider(juce::Slider::LinearHorizontal, juce::Slider::TextBoxLeft)
    , muteButton("M")
    , soloButton("S")
    , peakLevel(0.0f)
    , rmsLevel(0.0f)
{
    setupUI();
    startTimer(30); // Update meters every 30ms
}

MixerStrip::~MixerStrip()
{
    stopTimer();
}

void MixerStrip::setupUI()
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    // Enhanced label with better typography
    addAndMakeVisible(trackNameLabel);
    trackNameLabel.setJustificationType(juce::Justification::centred);
    trackNameLabel.setColour(juce::Label::textColourId, juce::Colour(Colors::textSoft));
    trackNameLabel.setFont(getBodyFont(Typography::bodySmall));
    
    addAndMakeVisible(fader);
    fader.setRange(-60.0, 12.0, 0.1);
    fader.setValue(0.0);
    fader.setTextValueSuffix(" dB");
    fader.onValueChange = [this] { faderChanged(); };
    
    addAndMakeVisible(panSlider);
    panSlider.setRange(-1.0, 1.0, 0.01);
    panSlider.setValue(0.0);
    panSlider.onValueChange = [this] { panChanged(); };
    
    addAndMakeVisible(muteButton);
    muteButton.onClick = [this] { muteButtonClicked(); };
    muteButton.setClickingTogglesState(true);
    
    addAndMakeVisible(soloButton);
    soloButton.onClick = [this] { soloButtonClicked(); };
    soloButton.setClickingTogglesState(true);
}

void MixerStrip::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    // Enhanced glassmorphism background
    auto bounds = getLocalBounds().toFloat();
    drawGlassPanel(g, bounds, Radii::none, false);
    
    // Enhanced divider line with gradient
    juce::ColourGradient dividerGradient(juce::Colour(Colors::divider).withAlpha(0.0f),
                                        bounds.getWidth() - 1.0f,
                                        bounds.getY(),
                                        juce::Colour(Colors::divider),
                                        bounds.getWidth() - 1.0f,
                                        bounds.getCentreY(),
                                        false);
    g.setGradientFill(dividerGradient);
    g.drawLine(bounds.getWidth() - 1.0f, 0.0f, bounds.getWidth() - 1.0f, bounds.getHeight(), 1.5f);
    
    // Draw meter using modern meter rendering with enhanced visuals
    auto meterBounds = getLocalBounds().removeFromBottom(60).reduced(Spacing::xsmall).toFloat();
    const auto* laf = dynamic_cast<const daw::ui::lookandfeel::CustomLookAndFeel*>(&getLookAndFeel());
    if (laf != nullptr)
    {
        laf->drawModernMeter(g, meterBounds, peakLevel, 0.0f);
    }
    else
    {
        drawMeter(g, meterBounds.toNearestInt(), peakLevel);
    }
}

void MixerStrip::resized()
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    auto bounds = getLocalBounds().reduced(Spacing::xsmall);
    
    trackNameLabel.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(Spacing::xsmall);
    
    auto buttonArea = bounds.removeFromTop(30);
    muteButton.setBounds(buttonArea.removeFromLeft(30));
    buttonArea.removeFromLeft(Spacing::xsmall);
    soloButton.setBounds(buttonArea.removeFromLeft(30));
    
    bounds.removeFromTop(Spacing::xsmall);
    
    panSlider.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(Spacing::xsmall);
    
    fader.setBounds(bounds.removeFromTop(bounds.getHeight() - 60));
}

void MixerStrip::timerCallback()
{
    updateMeters();
}

void MixerStrip::updateMeters()
{
    if (engineContext == nullptr)
        return;
    
    const auto meterData = engineContext->getTrackMeter(trackIndex);
    peakLevel = meterData.peak;
    rmsLevel = meterData.rms;
    
    repaint();
}

void MixerStrip::faderChanged()
{
    if (engineContext == nullptr)
        return;
    
    const auto gainDb = static_cast<float>(fader.getValue());
    engineContext->setTrackGain(trackIndex, gainDb);
    
    if (track != nullptr)
        track->setGainDb(gainDb);
}

void MixerStrip::panChanged()
{
    if (engineContext == nullptr)
        return;
    
    const auto pan = static_cast<float>(panSlider.getValue());
    engineContext->setTrackPan(trackIndex, pan);
    
    if (track != nullptr)
        track->setPan(pan);
}

void MixerStrip::muteButtonClicked()
{
    if (engineContext == nullptr)
        return;
    
    const auto muted = muteButton.getToggleState();
    engineContext->setTrackMute(trackIndex, muted);
    
    if (track != nullptr)
        track->setMuted(muted);
}

void MixerStrip::soloButtonClicked()
{
    if (engineContext == nullptr)
        return;
    
    const auto soloed = soloButton.getToggleState();
    engineContext->setTrackSolo(trackIndex, soloed);
    
    if (track != nullptr)
        track->setSoloed(soloed);
}

void MixerStrip::drawMeter(juce::Graphics& g, juce::Rectangle<int> bounds, float level)
{
    // Legacy method - use modern meter rendering if available
    const auto* laf = dynamic_cast<const daw::ui::lookandfeel::CustomLookAndFeel*>(&getLookAndFeel());
    if (laf != nullptr)
    {
        laf->drawModernMeter(g, bounds.toFloat(), level, 0.0f);
    }
    else
    {
        // Fallback to simple meter
        using namespace daw::ui::lookandfeel::DesignSystem;
        g.setColour(juce::Colour(Colors::meterBackground));
        g.fillRect(bounds);
        
        const auto db = juce::Decibels::gainToDecibels(level);
        const auto normalized = juce::jlimit(0.0f, 1.0f, juce::jmap(db, -60.0f, 0.0f));
        const auto fillHeight = static_cast<int>(bounds.getHeight() * normalized);
        
        juce::Colour meterColor;
        if (db > -3.0f)
            meterColor = juce::Colour(Colors::meterDanger);
        else if (db > -6.0f)
            meterColor = juce::Colour(Colors::meterWarning);
        else
            meterColor = juce::Colour(Colors::meterNormal);
        
        auto fillBounds = bounds.removeFromBottom(fillHeight);
        g.setColour(meterColor);
        g.fillRect(fillBounds);
    }
}

} // namespace daw::ui::views

