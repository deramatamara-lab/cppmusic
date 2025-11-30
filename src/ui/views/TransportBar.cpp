#include "TransportBar.h"
#include "MainView.h"
#include "../lookandfeel/DesignSystem.h"
#include "../lookandfeel/UltraDesignSystem.hpp"
#include <iomanip>
#include <sstream>

using namespace daw::ui::lookandfeel::DesignSystem;

namespace daw::ui::views
{

// ------------------------ TransportIconButton -------------------------------

TransportIconButton::TransportIconButton(const juce::String& buttonName, bool isToggle)
    : juce::Button(buttonName)
{
    setClickingTogglesState(isToggle);
    setRepaintsOnMouseActivity(true);

    // Default colour scheme
    auto bg = juce::Colour(Colors::surface2);
    setColourScheme(bg,
                    bg.brighter(0.06f),
                    bg.darker(0.12f),
                    juce::Colour(Colors::active).withAlpha(0.18f),
                    juce::Colour(Colors::textSecondary),
                    juce::Colour(Colors::text));

    // Acquire adaptive animation service (if available)
    if (auto service = daw::core::ServiceLocator::getInstance()
                           .getService<daw::ui::animation::AdaptiveAnimationService>())
    {
        animationService = service;
    }
}

void TransportIconButton::setIcons(const juce::Path& defaultIcon, const juce::Path& toggledIcon)
{
    iconOff = defaultIcon;
    iconOn  = toggledIcon.isEmpty() ? defaultIcon : toggledIcon;
    repaint();
}

void TransportIconButton::setColourScheme(juce::Colour normalFill,
                                          juce::Colour hoverFill,
                                          juce::Colour downFill,
                                          juce::Colour toggledFill,
                                          juce::Colour normalIconColour,
                                          juce::Colour toggledIconColour)
{
    fillNormal = normalFill;
    fillHover = hoverFill;
    fillDown = downFill;
    fillToggled = toggledFill;
    iconColourNormal = normalIconColour;
    iconColourToggled = toggledIconColour;
    repaint();
}

void TransportIconButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds().toFloat();
    const float radius = autoRadius(bounds.getHeight(), Radii::large);

    // Background
    juce::Colour fill = fillNormal;
    if (getToggleState())
        fill = fillToggled;
    else if (shouldDrawButtonAsDown)
        fill = fillDown;
    else if (shouldDrawButtonAsHighlighted)
        fill = fillHover;

    // Apply micro animation modulation
    const float brightnessBoost = juce::jlimit(0.0f, 0.35f, hoverAmount * 0.15f + pressAmount * 0.35f);
    if (brightnessBoost > 0.0f)
        fill = fill.brighter(brightnessBoost);

    applyShadow(g, Shadows::small, bounds, radius);
    g.setColour(fill);
    g.fillRoundedRectangle(bounds, radius);
    g.setColour(juce::Colour(Colors::outline).withAlpha(0.6f));
    g.drawRoundedRectangle(bounds, radius, 1.0f);

    // Icon
    const auto iconBounds = bounds.reduced(6.0f);
    const bool isOn = getToggleState();
    const auto& iconPath = isOn ? iconOn : iconOff;
    if (!iconPath.isEmpty())
    {
        auto path = iconPath; // copy before transform
        const float scaleX = iconBounds.getWidth() / 18.0f;
        const float scaleY = iconBounds.getHeight() / 18.0f;
        const float scale  = std::min(scaleX, scaleY);
        auto tx = juce::AffineTransform::scale(scale)
                    .translated(iconBounds.getX() + (iconBounds.getWidth() - 18.0f * scale) * 0.5f,
                                iconBounds.getY() + (iconBounds.getHeight() - 18.0f * scale) * 0.5f);
        path.applyTransform(tx);
        g.setColour(isOn ? iconColourToggled : iconColourNormal);
        g.fillPath(path);
    }
}

void TransportIconButton::mouseEnter(const juce::MouseEvent& event)
{
    animateState(1.0f, static_cast<float>(Animation::fast), hoverAmount, hoverAnimationId);
    juce::Button::mouseEnter(event);
}

void TransportIconButton::mouseExit(const juce::MouseEvent& event)
{
    animateState(0.0f, static_cast<float>(Animation::normal), hoverAmount, hoverAnimationId);
    juce::Button::mouseExit(event);
}

void TransportIconButton::mouseDown(const juce::MouseEvent& event)
{
    animateState(1.0f, static_cast<float>(Animation::fast), pressAmount, pressAnimationId);
    juce::Button::mouseDown(event);
}

void TransportIconButton::mouseUp(const juce::MouseEvent& event)
{
    animateState(0.0f, static_cast<float>(Animation::normal), pressAmount, pressAnimationId);
    juce::Button::mouseUp(event);
}

void TransportIconButton::animateState(float target,
                                       float durationMs,
                                       float& storage,
                                       uint32_t& handle)
{
    const auto current = storage;

    if (auto service = animationService.lock())
    {
        if (!service->isInitialized())
        {
            storage = target;
            repaint();
            return;
        }

        if (handle != 0)
            service->cancelAnimation(handle);

        auto self = juce::Component::SafePointer<TransportIconButton>(this);
        const auto id = service->animateFloat(current,
                                              target,
                                              durationMs,
                                              [self, storagePtr = &storage](float value)
                                              {
                                                  if (self != nullptr)
                                                  {
                                                      *storagePtr = value;
                                                      self->repaint();
                                                  }
                                              },
                                              [self, handlePtr = &handle]()
                                              {
                                                  if (self != nullptr)
                                                      *handlePtr = 0;
                                              });

        if (id == 0)
        {
            storage = target;
            repaint();
        }
        else
        {
            handle = id;
        }
    }
    else
    {
        storage = target;
        repaint();
    }
}

void TransportIconButton::cancelAnimation(uint32_t& handle)
{
    if (handle == 0)
        return;

    if (auto service = animationService.lock())
        service->cancelAnimation(handle);

    handle = 0;
}

TransportBar::TransportBar(std::shared_ptr<daw::audio::engine::EngineContext> engineContext)
    : engineContext(engineContext)
    , playButton("Play", true)
    , stopButton("Stop", false)
    , recordButton("Record", true)
    , patternSongToggle()
    , metronomeToggle("METRO")
    , loopToggle("LOOP")
    , snapToggle("SNAP")
    , tempoLabel("Tempo", "Tempo:")
    , tempoSlider(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight)
    , tempoValueLabel("", "120.0")
    , timeSigLabel("TimeSig", "Time:")
    , timeSigValueLabel("", "4/4")
    , positionLabel("Position", "Position:")
    , positionValueLabel("", "1:1:000")
    , cpuLabel("CPU", "CPU:")
    , cpuValueLabel("", "0.0%")
{
    setupUI();
    startTimer(50); // Update every 50ms
}

TransportBar::~TransportBar()
{
    stopTimer();
}

void TransportBar::setupUI()
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    // Transport buttons with icons
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(recordButton);

    playButton.setTooltip("Play / Pause");
    stopButton.setTooltip("Stop");
    recordButton.setTooltip("Record");

    playButton.setIcons(ultra::Icons::play());
    stopButton.setIcons(ultra::Icons::stop());
    recordButton.setIcons(ultra::Icons::record());

    // Colour accents
    playButton.setColourScheme(
        juce::Colour(Colors::surface2), juce::Colour(Colors::surface3), juce::Colour(Colors::surface1),
        juce::Colour(Colors::meterNormal).withAlpha(0.22f), juce::Colour(Colors::meterNormal), juce::Colour(Colors::text));
    stopButton.setColourScheme(
        juce::Colour(Colors::surface2), juce::Colour(Colors::surface3), juce::Colour(Colors::surface1),
        juce::Colour(Colors::active).withAlpha(0.18f), juce::Colour(Colors::textSecondary), juce::Colour(Colors::text));
    recordButton.setColourScheme(
        juce::Colour(Colors::surface2), juce::Colour(Colors::surface3), juce::Colour(Colors::surface1),
        juce::Colour(Colors::danger).withAlpha(0.20f), juce::Colour(Colors::danger), juce::Colour(Colors::danger));

    playButton.onClick   = [this] { playButtonClicked(); };
    stopButton.onClick   = [this] { stopButtonClicked(); };
    recordButton.onClick = [this] { recordButtonClicked(); };

    // Pattern/Song mode toggle
    addAndMakeVisible(patternSongToggle);
    patternSongToggle.setTabs({"PAT", "SONG"});
    patternSongToggle.setSelectedTab(0); // Start with Pattern
    patternSongToggle.onChange = [this](int tab) { patternSongModeChanged(tab); };

    // Snap controls
    addAndMakeVisible(snapToggle);
    snapToggle.setClickingTogglesState(true);
    snapToggle.setToggleState(snapEnabled, juce::dontSendNotification);
    snapToggle.setTooltip("Snap clips and edits to grid");
    snapToggle.onClick = [this] { snapToggleClicked(); };

    addAndMakeVisible(snapDivisionCombo);
    snapDivisionCombo.addItem("1/1", 1);
    snapDivisionCombo.addItem("1/2", 2);
    snapDivisionCombo.addItem("1/4", 3);
    snapDivisionCombo.addItem("1/8", 4);
    snapDivisionCombo.addItem("1/16", 5);
    snapDivisionCombo.addItem("1/32", 6);
    snapDivisionCombo.addItem("1/64", 7);
    snapDivisionCombo.addSeparator();
    snapDivisionCombo.addItem("1/4T", 8);  // Triplets
    snapDivisionCombo.addItem("1/8T", 9);
    snapDivisionCombo.addItem("1/16T", 10);
    snapDivisionCombo.setSelectedId(5); // Default to 1/16
    snapDivisionCombo.setTooltip("Grid division for snapping");
    snapDivisionCombo.onChange = [this] { snapDivisionChanged(); };

    // Metronome / loop toggles (UI-only for now)
    addAndMakeVisible(metronomeToggle);
    metronomeToggle.setClickingTogglesState(true);
    metronomeToggle.setTooltip("Enable / disable metronome");
    metronomeToggle.onClick = [this]
    {
        metronomeOn = metronomeToggle.getToggleState();
        animationHelper.pulse(metronomeToggle, 260, 1);
        metronomeButtonClicked();
    };

    // Metronome volume slider
    addAndMakeVisible(metronomeVolumeSlider);
    metronomeVolumeSlider.setRange(0.0, 1.0, 0.01);
    metronomeVolumeSlider.setValue(0.5); // Default volume
    metronomeVolumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    metronomeVolumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    metronomeVolumeSlider.setTooltip("Metronome volume");
    metronomeVolumeSlider.onValueChange = [this]
    {
        if (engineContext)
        {
            engineContext->setMetronomeVolume(static_cast<float>(metronomeVolumeSlider.getValue()));
        }
    };

    addAndMakeVisible(loopToggle);
    loopToggle.setClickingTogglesState(true);
    loopToggle.setTooltip("Loop playback over a 4-beat region");
    loopToggle.onClick = [this]
    {
        loopOn = loopToggle.getToggleState();
        animationHelper.pulse(loopToggle, 260, 1);
        loopButtonClicked();
    };

    // Enhanced labels with better typography
    addAndMakeVisible(tempoLabel);
    tempoLabel.setJustificationType(juce::Justification::centredRight);
    tempoLabel.setFont(getBodyFont(Typography::bodySmall));

    addAndMakeVisible(tempoSlider);
    tempoSlider.setRange(20.0, 999.0, 0.1);
    tempoSlider.setValue(120.0);
    tempoSlider.onValueChange = [this] { tempoChanged(); };

    addAndMakeVisible(tempoValueLabel);
    tempoValueLabel.setJustificationType(juce::Justification::centred);
    tempoValueLabel.setFont(getMonoFont(Typography::body));

    addAndMakeVisible(timeSigLabel);
    timeSigLabel.setJustificationType(juce::Justification::centredRight);
    timeSigLabel.setFont(getBodyFont(Typography::bodySmall));

    addAndMakeVisible(timeSigValueLabel);
    timeSigValueLabel.setJustificationType(juce::Justification::centred);
    timeSigValueLabel.setFont(getMonoFont(Typography::body));

    addAndMakeVisible(positionLabel);
    positionLabel.setJustificationType(juce::Justification::centredRight);
    positionLabel.setFont(getBodyFont(Typography::bodySmall));

    addAndMakeVisible(positionValueLabel);
    positionValueLabel.setJustificationType(juce::Justification::centred);
    positionValueLabel.setFont(getMonoFont(Typography::body));

    addAndMakeVisible(cpuLabel);
    cpuLabel.setJustificationType(juce::Justification::centredRight);
    cpuLabel.setFont(getBodyFont(Typography::bodySmall));

    addAndMakeVisible(cpuValueLabel);
    cpuValueLabel.setJustificationType(juce::Justification::centred);
    cpuValueLabel.setFont(getMonoFont(Typography::body));

    updatePositionDisplay();
    updateCpuDisplay();
}

void TransportBar::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    // Enhanced background with elevated glassmorphism
    auto bounds = getLocalBounds().toFloat();
    drawGlassPanel(g, bounds, Radii::none, true);

    // Enhanced divider line with gradient along the bottom edge
    juce::ColourGradient dividerGradient(juce::Colour(Colors::divider).withAlpha(0.0f),
                                        bounds.getX(),
                                        bounds.getHeight() - 1.0f,
                                        juce::Colour(Colors::divider),
                                        bounds.getCentreX(),
                                        bounds.getHeight() - 1.0f,
                                        false);
    g.setGradientFill(dividerGradient);
    g.drawLine(0.0f, bounds.getHeight() - 1.0f, bounds.getWidth(), bounds.getHeight() - 1.0f, 1.5f);

    // CPU meter bar (right side), FL-style
    const auto cpuArea = bounds.removeFromRight(140.0f).reduced(Spacing::small);
    g.setColour(juce::Colour(Colors::meterBackground));
    auto meterBounds = cpuArea.withHeight(cpuArea.getHeight() * 0.4f)
                              .withY(cpuArea.getCentreY() - cpuArea.getHeight() * 0.2f);
    g.fillRoundedRectangle(meterBounds, Radii::small);

    const float cpuLinear = juce::jlimit(0.0f, 1.0f, cpuSmoothed / 100.0f);
    auto levelBounds = meterBounds.withWidth(meterBounds.getWidth() * cpuLinear);

    juce::Colour meterColour;
    if (cpuSmoothed < 60.0f)
        meterColour = juce::Colour(Colors::meterNormal);
    else if (cpuSmoothed < 80.0f)
        meterColour = juce::Colour(Colors::meterWarning);
    else
        meterColour = juce::Colour(Colors::meterDanger);

    juce::ColourGradient meterGradient(meterColour.brighter(0.2f),
                                       levelBounds.getX(), levelBounds.getY(),
                                       meterColour.darker(0.2f),
                                       levelBounds.getRight(), levelBounds.getBottom(),
                                       false);
    g.setGradientFill(meterGradient);
    g.fillRoundedRectangle(levelBounds, Radii::small);

    // Icons are drawn inside TransportIconButton
}

void TransportBar::resized()
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    auto bounds = getLocalBounds().reduced(Spacing::small);

    const auto buttonWidth = 36;
    const auto buttonHeight = 36;
    const auto labelWidth = 60;
    const auto valueWidth = 80;
    const auto sliderWidth = 150;

    // Transport cluster: [Play][Stop][Record] [PAT/SONG] [SNAP][SnapDiv] [Met][Vol][Loop]
    auto left = bounds.removeFromLeft(static_cast<int>(buttonWidth * 9.5f + Spacing::small * 12));

    // Main transport
    playButton.setBounds(left.removeFromLeft(buttonWidth).withHeight(buttonHeight));
    left.removeFromLeft(Spacing::small);
    stopButton.setBounds(left.removeFromLeft(buttonWidth).withHeight(buttonHeight));
    left.removeFromLeft(Spacing::small);
    recordButton.setBounds(left.removeFromLeft(buttonWidth).withHeight(buttonHeight));
    left.removeFromLeft(Spacing::medium);

    // Pattern/Song mode toggle
    patternSongToggle.setBounds(left.removeFromLeft(static_cast<int>(buttonWidth * 1.8f)).withHeight(buttonHeight));
    left.removeFromLeft(Spacing::medium);

    // Snap controls
    snapToggle.setBounds(left.removeFromLeft(static_cast<int>(buttonWidth * 1.2f)).withHeight(buttonHeight));
    left.removeFromLeft(Spacing::xsmall);
    snapDivisionCombo.setBounds(left.removeFromLeft(static_cast<int>(buttonWidth * 1.5f)).withHeight(buttonHeight));
    left.removeFromLeft(Spacing::medium);

    // Metro/Loop
    metronomeToggle.setBounds(left.removeFromLeft(static_cast<int>(buttonWidth * 1.2f)).withHeight(buttonHeight));
    left.removeFromLeft(Spacing::xsmall);
    metronomeVolumeSlider.setBounds(left.removeFromLeft(static_cast<int>(buttonWidth * 1.5f)).withHeight(buttonHeight));
    left.removeFromLeft(Spacing::small);
    loopToggle.setBounds(left.removeFromLeft(buttonWidth).withHeight(buttonHeight));

    bounds.removeFromLeft(Spacing::medium);

    // Tempo
    auto tempoLabelBounds = bounds.removeFromLeft(labelWidth);
    tempoLabel.setBounds(tempoLabelBounds);
    bounds.removeFromLeft(Spacing::xsmall);
    auto tempoSliderBounds = bounds.removeFromLeft(sliderWidth);
    tempoSlider.setBounds(tempoSliderBounds);
    bounds.removeFromLeft(Spacing::xsmall);
    auto tempoValueBounds = bounds.removeFromLeft(valueWidth);
    tempoValueLabel.setBounds(tempoValueBounds);

    bounds.removeFromLeft(Spacing::medium);

    // Time signature
    auto timeSigLabelBounds = bounds.removeFromLeft(labelWidth);
    timeSigLabel.setBounds(timeSigLabelBounds);
    bounds.removeFromLeft(Spacing::xsmall);
    auto timeSigValueBounds = bounds.removeFromLeft(valueWidth);
    timeSigValueLabel.setBounds(timeSigValueBounds);

    bounds.removeFromLeft(Spacing::medium);

    // Position
    auto positionLabelBounds = bounds.removeFromLeft(labelWidth);
    positionLabel.setBounds(positionLabelBounds);
    bounds.removeFromLeft(Spacing::xsmall);
    auto positionValueBounds = bounds.removeFromLeft(valueWidth);
    positionValueLabel.setBounds(positionValueBounds);

    bounds.removeFromLeft(Spacing::medium);

    // CPU
    auto cpuLabelBounds = bounds.removeFromLeft(labelWidth);
    cpuLabel.setBounds(cpuLabelBounds);
    bounds.removeFromLeft(Spacing::xsmall);
    auto cpuValueBounds = bounds.removeFromLeft(valueWidth);
    cpuValueLabel.setBounds(cpuValueBounds);
}

void TransportBar::timerCallback()
{
    updatePositionDisplay();
    updateCpuDisplay();
    // Sync button toggle states to engine
    if (engineContext)
        playButton.setToggleState(engineContext->isPlaying(), juce::dontSendNotification);
}

void TransportBar::updatePositionDisplay()
{
    if (!engineContext)
        return;

    const auto positionBeats = engineContext->getPositionInBeats();
    const auto bars = static_cast<int>(positionBeats / 4.0);
    const auto beats = static_cast<int>(positionBeats) % 4;
    const auto ticks = static_cast<int>((positionBeats - static_cast<int>(positionBeats)) * 1000);

    std::ostringstream oss;
    oss << bars << ":" << beats << ":" << std::setfill('0') << std::setw(3) << ticks;
    positionValueLabel.setText(oss.str(), juce::dontSendNotification);

    const auto tempo = engineContext->getTempo();
    tempoSlider.setValue(tempo, juce::dontSendNotification);
    tempoValueLabel.setText(juce::String(tempo, 1), juce::dontSendNotification);

    const auto num = engineContext->getTimeSignatureNumerator();
    const auto den = engineContext->getTimeSignatureDenominator();
    timeSigValueLabel.setText(juce::String(num) + "/" + juce::String(den), juce::dontSendNotification);
}

void TransportBar::updateCpuDisplay()
{
    if (!engineContext)
        return;

    const auto cpuLoad = engineContext->getCpuLoad();
    cpuValueLabel.setText(juce::String(cpuLoad, 1) + "%", juce::dontSendNotification);
    // Simple smoothing to keep the meter fluid
    const float alpha = 0.3f;
    cpuSmoothed = cpuSmoothed * (1.0f - alpha) + cpuLoad * alpha;
}

void TransportBar::playButtonClicked()
{
    if (!engineContext)
        return;

    if (engineContext->isPlaying())
        engineContext->stop();
    else
        engineContext->play();
}

void TransportBar::stopButtonClicked()
{
    if (!engineContext)
        return;

    engineContext->stop();
    engineContext->setPositionInBeats(0.0);
}

void TransportBar::recordButtonClicked()
{
    if (!engineContext)
        return;

    isRecording = !isRecording;
    if (isRecording)
    {
        // Ensure transport is playing to capture audio
        engineContext->play();
        recordButton.setToggleState(true, juce::dontSendNotification);
    }
    else
    {
        // Stop transport when recording stops
        engineContext->stop();
        recordButton.setToggleState(false, juce::dontSendNotification);
    }
}

void TransportBar::tempoChanged()
{
    if (!engineContext)
        return;

    const auto tempo = tempoSlider.getValue();
    engineContext->setTempo(tempo);
}

void TransportBar::metronomeButtonClicked()
{
    if (engineContext)
    {
        engineContext->setMetronomeEnabled(metronomeOn);
    }
}

void TransportBar::loopButtonClicked()
{
    if (engineContext)
    {
        engineContext->setLoopEnabled(loopOn);
        // Default loop region: 4 beats from current position
        if (loopOn)
        {
            const auto currentPos = engineContext->getPositionInBeats();
            engineContext->setLoopRegion(currentPos, currentPos + 4.0);
        }
    }
}

void TransportBar::patternSongModeChanged(int selectedTab)
{
    currentPlayMode = (selectedTab == 0) ? PlayMode::Pattern : PlayMode::Song;

    if (auto* mainView = findParentComponentOfClass<MainView>())
    {
        const auto preset = (currentPlayMode == PlayMode::Pattern)
                                ? MainView::LayoutPreset::Live
                                : MainView::LayoutPreset::Arrange;
        mainView->applyLayoutPreset(preset);
    }
}

void TransportBar::snapToggleClicked()
{
    snapEnabled = snapToggle.getToggleState();

    // Update arrange view snap settings
    if (auto* mainView = findParentComponentOfClass<MainView>())
    {
        mainView->getArrangeView().setSnapEnabled(snapEnabled);
    }
}

void TransportBar::snapDivisionChanged()
{
    const auto selectedId = snapDivisionCombo.getSelectedId();

    // Map combo box IDs to snap divisions in beats
    switch (selectedId)
    {
        case 1: snapDivision = 4.0; break;    // 1/1 = 4 beats
        case 2: snapDivision = 2.0; break;    // 1/2 = 2 beats
        case 3: snapDivision = 1.0; break;    // 1/4 = 1 beat
        case 4: snapDivision = 0.5; break;    // 1/8 = 0.5 beats
        case 5: snapDivision = 0.25; break;   // 1/16 = 0.25 beats
        case 6: snapDivision = 0.125; break;  // 1/32 = 0.125 beats
        case 7: snapDivision = 0.0625; break; // 1/64 = 0.0625 beats
        case 8: snapDivision = 1.0 / 3.0; break;     // 1/4T = triplet
        case 9: snapDivision = 0.5 / 3.0; break;     // 1/8T = triplet
        case 10: snapDivision = 0.25 / 3.0; break;   // 1/16T = triplet
        default: snapDivision = 0.25; break;
    }

    // Update arrange view snap division
    if (auto* mainView = findParentComponentOfClass<MainView>())
    {
        mainView->getArrangeView().setSnapDivision(snapDivision);
    }
}

} // namespace daw::ui::views
