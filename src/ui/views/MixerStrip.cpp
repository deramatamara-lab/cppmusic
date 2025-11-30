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
    , trackNameLabel("", track != nullptr ? track->getName() : (trackIndex == -1 ? "Master" : "Track"))
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
    // Production implementation: Initialize fader with current gain (master or track)
    if (trackIndex == -1 && engineContext != nullptr)
    {
        fader.setValue(engineContext->getMasterGain());
    }
    else if (track != nullptr)
    {
        fader.setValue(track->getGainDb());
    }
    else
    {
        fader.setValue(0.0);
    }
    fader.setTextValueSuffix(" dB");
    fader.onValueChange = [this] { faderChanged(); };

    addAndMakeVisible(panSlider);
    panSlider.setRange(-1.0, 1.0, 0.01);
    // Production implementation: Hide pan for master strip
    if (trackIndex == -1)
    {
        panSlider.setVisible(false);
    }
    else
    {
        panSlider.setValue(track != nullptr ? track->getPan() : 0.0);
        panSlider.onValueChange = [this] { panChanged(); };
    }

    addAndMakeVisible(muteButton);
    muteButton.setClickingTogglesState(true);
    // Production implementation: Hide mute/solo for master strip
    if (trackIndex == -1)
    {
        muteButton.setVisible(false);
    }
    else
    {
        muteButton.setToggleState(track != nullptr ? track->isMuted() : false, juce::dontSendNotification);
        muteButton.onClick = [this] { muteButtonClicked(); };
    }

    addAndMakeVisible(soloButton);
    soloButton.setClickingTogglesState(true);
    if (trackIndex == -1)
    {
        soloButton.setVisible(false);
    }
    else
    {
        soloButton.setToggleState(track != nullptr ? track->isSoloed() : false, juce::dontSendNotification);
        soloButton.onClick = [this] { soloButtonClicked(); };
    }
}

void MixerStrip::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    // Enhanced glassmorphism background
    auto bounds = getLocalBounds().toFloat();
    drawGlassPanel(g, bounds, Radii::none, false);

    // Track colour strip (FL-style lane accent)
    juce::Colour trackColour = (trackIndex == -1)
        ? juce::Colour(Colors::accent)
        : Tracks::colourForIndex(trackIndex);
    const auto stripWidth = 4.0f;
    g.setColour(trackColour);
    g.fillRect(bounds.withWidth(stripWidth));

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
        drawMeter(g, meterBounds.toNearestInt(), peakLevel, rmsLevel);
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

void MixerStrip::mouseDown(const juce::MouseEvent& event)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    auto meterBounds = getLocalBounds().removeFromBottom(60).reduced(Spacing::xsmall);
    if (meterBounds.contains(event.getPosition()))
    {
        if (clipLatched)
        {
            clipLatched = false;
            repaint(meterBounds);
            return;
        }
    }

    juce::Component::mouseDown(event);
}

void MixerStrip::timerCallback()
{
    updateMeters();
}

void MixerStrip::updateMeters()
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    if (engineContext == nullptr)
        return;

    // Production implementation: Use master meter for master strip, track meter for tracks
    daw::audio::engine::EngineContext::MeterData meterData;
    if (trackIndex == -1)
    {
        meterData = engineContext->getMasterMeter();
    }
    else
    {
        meterData = engineContext->getTrackMeter(trackIndex);
    }

    peakLevel = meterData.peak;
    rmsLevel = meterData.rms;

    // Repaint only meter area (dirty-rect optimization)
    auto meterBounds = getLocalBounds().removeFromBottom(60).reduced(Spacing::xsmall);
    repaint(meterBounds);
}

void MixerStrip::faderChanged()
{
    if (engineContext == nullptr)
        return;

    const auto gainDb = static_cast<float>(fader.getValue());

    // Production implementation: Handle master strip (trackIndex == -1)
    if (trackIndex == -1)
    {
        engineContext->setMasterGain(gainDb);
    }
    else
    {
        engineContext->setTrackGain(trackIndex, gainDb);
        if (track != nullptr)
            track->setGainDb(gainDb);
    }
}

void MixerStrip::panChanged()
{
    if (engineContext == nullptr)
        return;

    // Production implementation: Master strip doesn't have pan (stereo balance only)
    if (trackIndex == -1)
        return; // Master doesn't have pan control

    const auto pan = static_cast<float>(panSlider.getValue());
    engineContext->setTrackPan(trackIndex, pan);

    if (track != nullptr)
        track->setPan(pan);
}

void MixerStrip::muteButtonClicked()
{
    if (engineContext == nullptr)
        return;

    // Production implementation: Master strip doesn't have mute (would mute entire output)
    if (trackIndex == -1)
        return; // Master doesn't have mute control

    const auto muted = muteButton.getToggleState();
    engineContext->setTrackMute(trackIndex, muted);

    if (track != nullptr)
        track->setMuted(muted);
}

void MixerStrip::soloButtonClicked()
{
    if (engineContext == nullptr)
        return;

    // Production implementation: Master strip doesn't have solo
    if (trackIndex == -1)
        return; // Master doesn't have solo control

    const auto soloed = soloButton.getToggleState();
    engineContext->setTrackSolo(trackIndex, soloed);

    if (track != nullptr)
        track->setSoloed(soloed);
}

void MixerStrip::drawMeter(juce::Graphics& g, const juce::Rectangle<int>& bounds, float peak, float rms)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    const auto dbPeak = Meters::linearToDecibels(peak);
    const auto dbRms  = Meters::linearToDecibels(rms);

    const auto normalisedPeak = Meters::normalisedFromDb(dbPeak);
    const auto normalisedRms  = Meters::normalisedFromDb(dbRms);

    const auto meterHeight = bounds.getHeight();
    const auto peakHeight  = static_cast<int>(meterHeight * normalisedPeak);
    const auto rmsHeight   = static_cast<int>(meterHeight * normalisedRms);

    // Background
    g.setColour(juce::Colour(Colors::meterBackground));
    g.fillRect(bounds);

    // RMS body
    if (rmsHeight > 0)
    {
        auto rmsBounds = bounds;
        rmsBounds.setY(bounds.getBottom() - rmsHeight);
        rmsBounds.setHeight(rmsHeight);

        juce::Colour base;
        juce::Colour baseEnd;

        if (dbPeak > -3.0f)
        {
            base    = juce::Colour(Colors::meterDangerStart);
            baseEnd = juce::Colour(Colors::meterDangerEnd);
        }
        else if (dbPeak > -6.0f)
        {
            base    = juce::Colour(Colors::meterWarningStart);
            baseEnd = juce::Colour(Colors::meterWarningEnd);
        }
        else
        {
            base    = juce::Colour(Colors::meterNormalStart);
            baseEnd = juce::Colour(Colors::meterNormalEnd);
        }

        juce::ColourGradient gradient(base,
                                      static_cast<float>(rmsBounds.getX()),
                                      static_cast<float>(rmsBounds.getBottom()),
                                      baseEnd,
                                      static_cast<float>(rmsBounds.getX()),
                                      static_cast<float>(rmsBounds.getY()),
                                      false);
        g.setGradientFill(gradient);
        g.fillRect(rmsBounds);
    }

    // Peak indicator stripe
    if (peakHeight > 0)
    {
        auto peakBounds = bounds;
        peakBounds.setY(bounds.getBottom() - peakHeight);
        peakBounds.setHeight(peakHeight);

        const auto highlight = juce::Colour(Colors::meterNormal).withMultipliedBrightness(1.3f);
        g.setColour(highlight.withAlpha(0.7f));
        g.fillRect(peakBounds);

        g.setColour(juce::Colours::white.withAlpha(0.25f));
        auto topLine = peakBounds;
        topLine.setHeight(1);
        g.fillRect(topLine);
    }

    // 0 dB reference line
    const auto zeroY = Meters::zeroDbLineY(bounds.toFloat());
    g.setColour(juce::Colour(Colors::outlineFocus).withAlpha(0.9f));
    g.drawLine(static_cast<float>(bounds.getX()),
               zeroY,
               static_cast<float>(bounds.getRight()),
               zeroY,
               hairline(this));

    // Scale markers (−6, −12, −24, −48 dB)
    g.setColour(juce::Colour(Colors::outline).withAlpha(0.35f));
    constexpr float markersDb[] { -6.0f, -12.0f, -24.0f, -48.0f };
    for (auto db : markersDb)
    {
        const auto normalised = Meters::normalisedFromDb(db);
        const auto y = bounds.getBottom() - static_cast<float>(bounds.getHeight()) * normalised;
        g.drawLine(static_cast<float>(bounds.getX()),
                   y,
                   static_cast<float>(bounds.getRight()),
                   y,
                   hairline(this) * 0.8f);
    }

    // Clip indicator (latching)
    if (dbPeak > 0.0f)
        clipLatched = true;

    auto clipRect = bounds.withHeight(6).reduced(2);
    g.setColour(clipLatched
                    ? juce::Colour(Colors::meterDanger)
                    : juce::Colour(Colors::meterDanger).withAlpha(0.25f));
    g.fillRoundedRectangle(clipRect.toFloat(), 2.0f);
}

} // namespace daw::ui::views
