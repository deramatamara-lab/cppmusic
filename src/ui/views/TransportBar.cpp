#include "TransportBar.h"
#include "../lookandfeel/DesignSystem.h"
#include <iomanip>
#include <sstream>

using namespace daw::ui::lookandfeel::DesignSystem;

namespace daw::ui::views
{

TransportBar::TransportBar(std::shared_ptr<daw::audio::engine::EngineContext> engineContext)
    : engineContext(engineContext)
    , playButton("Play")
    , stopButton("Stop")
    , recordButton("Record")
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
    
    // Enhanced button styling with better fonts
    addAndMakeVisible(playButton);
    playButton.onClick = [this] { playButtonClicked(); };
    // Note: TextButton font is controlled via LookAndFeel
    
    addAndMakeVisible(stopButton);
    stopButton.onClick = [this] { stopButtonClicked(); };
    // Note: TextButton font is controlled via LookAndFeel
    
    addAndMakeVisible(recordButton);
    recordButton.onClick = [this] { recordButtonClicked(); };
    recordButton.setEnabled(false); // Not implemented yet
    // Note: TextButton font is controlled via LookAndFeel
    
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
    
    // Enhanced divider line with gradient
    juce::ColourGradient dividerGradient(juce::Colour(Colors::divider).withAlpha(0.0f),
                                        bounds.getX(),
                                        bounds.getHeight() - 1.0f,
                                        juce::Colour(Colors::divider),
                                        bounds.getCentreX(),
                                        bounds.getHeight() - 1.0f,
                                        false);
    g.setGradientFill(dividerGradient);
    g.drawLine(0.0f, bounds.getHeight() - 1.0f, bounds.getWidth(), bounds.getHeight() - 1.0f, 1.5f);
}

void TransportBar::resized()
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    auto bounds = getLocalBounds().reduced(Spacing::small);
    
    const auto buttonWidth = 80;
    [[maybe_unused]] const auto buttonHeight = 30;
    const auto labelWidth = 60;
    const auto valueWidth = 80;
    const auto sliderWidth = 150;
    
    auto left = bounds.removeFromLeft(buttonWidth * 3 + Spacing::small * 2);
    playButton.setBounds(left.removeFromLeft(buttonWidth));
    left.removeFromLeft(Spacing::small);
    stopButton.setBounds(left.removeFromLeft(buttonWidth));
    left.removeFromLeft(Spacing::small);
    recordButton.setBounds(left.removeFromLeft(buttonWidth));
    
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
    // Not implemented yet
}

void TransportBar::tempoChanged()
{
    if (!engineContext)
        return;
    
    const auto tempo = tempoSlider.getValue();
    engineContext->setTempo(tempo);
}

} // namespace daw::ui::views

