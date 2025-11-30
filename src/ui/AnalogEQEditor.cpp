#include "AnalogEQEditor.h"
#include "core/RealtimeMessageQueue.h"
#include "lookandfeel/DesignSystem.h"

namespace cppmusic {
namespace ui {

//==============================================================================
AnalogEQEditor::AnalogEQEditor(audio::AnalogModeledEQ& eq, daw::core::EngineContext& context)
    : eq_(eq), engineContext_(context)
{
    // Initialize presets
    presetA_.name = "Preset A";
    presetB_.name = "Preset B";

    // Create all UI components
    createBandControls();
    createGlobalControls();
    createPresetControls();
    createAnalyzerControls();

    // Set up timer for regular updates
    startTimerHz(30); // 30 FPS for smooth UI updates

    // Initialize colors for each band
    for (int i = 0; i < audio::AnalogModeledEQ::NUM_BANDS; ++i) {
        bandControls_[i].bandColour = getBandColour(i);
    }

    // Update UI from current EQ state
    updateFromEQ();

    setSize(900, 600);
}

AnalogEQEditor::~AnalogEQEditor()
{
    stopTimer();
}

//==============================================================================
void AnalogEQEditor::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(getBackgroundColour());

    // Panel backgrounds
    g.setColour(getPanelColour());
    g.fillRoundedRectangle(responseArea_.toFloat(), 4.0f);

    // Draw frequency response
    drawFrequencyResponse(g, responseArea_);

    // Draw spectrum analyzer if enabled
    if (analyzerEnabled_) {
        drawSpectrumAnalyzer(g, responseArea_);
    }

    // Draw band section backgrounds
    for (int i = 0; i < audio::AnalogModeledEQ::NUM_BANDS; ++i) {
        auto& band = bandControls_[i];

        g.setColour(getPanelColour().withAlpha(band.isMouseOver ? 0.8f : 0.6f));
        g.fillRoundedRectangle(band.bounds.toFloat(), 4.0f);

        if (band.enableButton->getToggleState()) {
            g.setColour(band.bandColour.withAlpha(0.3f));
            g.drawRoundedRectangle(band.bounds.toFloat(), 4.0f, 2.0f);
        }

        // Band label
        g.setColour(getTextColour());
        g.setFont(12.0f);
        g.drawText("BAND " + juce::String(i + 1),
                  band.bounds.removeFromTop(20),
                  juce::Justification::centred);
    }

    // Global controls panel
    auto globalArea = getLocalBounds().removeFromBottom(120).reduced(10);
    g.setColour(getPanelColour());
    g.fillRoundedRectangle(globalArea.toFloat(), 4.0f);

    // Title
    g.setColour(getTextColour());
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.drawText("ANALOG MODELED EQ", getLocalBounds().removeFromTop(30), juce::Justification::centred);
}

void AnalogEQEditor::resized()
{
    auto bounds = getLocalBounds();

    // Title area
    bounds.removeFromTop(40);

    // Response plot area
    responseArea_ = bounds.removeFromTop(200).reduced(10);

    // Band controls area
    auto bandArea = bounds.removeFromTop(200).reduced(10);
    int bandWidth = bandArea.getWidth() / audio::AnalogModeledEQ::NUM_BANDS;

    for (int i = 0; i < audio::AnalogModeledEQ::NUM_BANDS; ++i) {
        auto& band = bandControls_[i];
        band.bounds = bandArea.removeFromLeft(bandWidth).reduced(5);

        auto controlArea = band.bounds.reduced(10);
        controlArea.removeFromTop(20); // For band label

        // Layout controls in a grid
        int knobSize = 50;
        int spacing = 5;

        auto row1 = controlArea.removeFromTop(knobSize);
        band.frequencyKnob->setBounds(row1.removeFromLeft(knobSize));
        row1.removeFromLeft(spacing);
        band.gainKnob->setBounds(row1.removeFromLeft(knobSize));
        row1.removeFromLeft(spacing);
        band.qKnob->setBounds(row1.removeFromLeft(knobSize));

        controlArea.removeFromTop(spacing);
        auto row2 = controlArea.removeFromTop(knobSize);
        band.driveKnob->setBounds(row2.removeFromLeft(knobSize));
        row2.removeFromLeft(spacing);
        band.saturationKnob->setBounds(row2.removeFromLeft(knobSize));
        row2.removeFromLeft(spacing);
        band.mixKnob->setBounds(row2.removeFromLeft(knobSize));

        controlArea.removeFromTop(spacing);
        auto buttonRow = controlArea.removeFromTop(25);
        int buttonWidth = buttonRow.getWidth() / 3;
        band.enableButton->setBounds(buttonRow.removeFromLeft(buttonWidth).reduced(2));
        band.soloButton->setBounds(buttonRow.removeFromLeft(buttonWidth).reduced(2));
        band.bypassButton->setBounds(buttonRow.removeFromLeft(buttonWidth).reduced(2));

        controlArea.removeFromTop(spacing);
        auto comboRow = controlArea.removeFromTop(20);
        band.typeCombo->setBounds(comboRow.removeFromLeft(comboRow.getWidth() / 2).reduced(2));
        band.slopeCombo->setBounds(comboRow.reduced(2));
    }

    // Global controls area
    auto globalArea = bounds.removeFromBottom(120).reduced(10);
    auto globalControlArea = globalArea.reduced(10);

    // Top row - analog modeling
    auto analogRow = globalControlArea.removeFromTop(30);
    analogModelCombo_->setBounds(analogRow.removeFromLeft(150));
    analogRow.removeFromLeft(10);

    auto knobRow = analogRow;
    int globalKnobSize = 40;
    int globalSpacing = 10;

    inputGainKnob_->setBounds(knobRow.removeFromLeft(globalKnobSize));
    knobRow.removeFromLeft(globalSpacing);
    outputGainKnob_->setBounds(knobRow.removeFromLeft(globalKnobSize));
    knobRow.removeFromLeft(globalSpacing);
    transformerDriveKnob_->setBounds(knobRow.removeFromLeft(globalKnobSize));
    knobRow.removeFromLeft(globalSpacing);
    tubeWarmthKnob_->setBounds(knobRow.removeFromLeft(globalKnobSize));
    knobRow.removeFromLeft(globalSpacing);
    tapeSaturationKnob_->setBounds(knobRow.removeFromLeft(globalKnobSize));
    knobRow.removeFromLeft(globalSpacing);
    analogNoiseKnob_->setBounds(knobRow.removeFromLeft(globalKnobSize));

    globalControlArea.removeFromTop(10);

    // Bottom row - presets and analyzer
    auto presetRow = globalControlArea.removeFromTop(30);
    presetAButton_->setBounds(presetRow.removeFromLeft(80));
    presetRow.removeFromLeft(5);
    saveAButton_->setBounds(presetRow.removeFromLeft(60));
    presetRow.removeFromLeft(10);
    morphSlider_->setBounds(presetRow.removeFromLeft(100));
    presetRow.removeFromLeft(10);
    saveBButton_->setBounds(presetRow.removeFromLeft(60));
    presetRow.removeFromLeft(5);
    presetBButton_->setBounds(presetRow.removeFromLeft(80));

    presetRow.removeFromLeft(20);
    analyzerButton_->setBounds(presetRow.removeFromLeft(100));
}

void AnalogEQEditor::mouseMove(const juce::MouseEvent& event)
{
    // Check which band is being hovered
    int newHoveredBand = -1;
    for (int i = 0; i < audio::AnalogModeledEQ::NUM_BANDS; ++i) {
        if (bandControls_[i].bounds.contains(event.getPosition())) {
            newHoveredBand = i;
            break;
        }
    }

    if (newHoveredBand != hoveredBand_) {
        // Update mouse over states
        if (hoveredBand_ >= 0) {
            bandControls_[hoveredBand_].isMouseOver = false;
        }
        if (newHoveredBand >= 0) {
            bandControls_[newHoveredBand].isMouseOver = true;
        }

        hoveredBand_ = newHoveredBand;
        repaint();
    }
}

void AnalogEQEditor::mouseExit(const juce::MouseEvent&)
{
    if (hoveredBand_ >= 0) {
        bandControls_[hoveredBand_].isMouseOver = false;
        hoveredBand_ = -1;
        repaint();
    }
}

//==============================================================================
void AnalogEQEditor::sliderValueChanged(juce::Slider* slider)
{
    // Find which band and parameter
    for (int i = 0; i < audio::AnalogModeledEQ::NUM_BANDS; ++i) {
        auto& band = bandControls_[i];

        if (slider == band.frequencyKnob.get()) {
            updateEQParameter(i, "frequency", static_cast<float>(slider->getValue()));
        }
        else if (slider == band.gainKnob.get()) {
            updateEQParameter(i, "gain", static_cast<float>(slider->getValue()));
        }
        else if (slider == band.qKnob.get()) {
            updateEQParameter(i, "q", static_cast<float>(slider->getValue()));
        }
        else if (slider == band.driveKnob.get()) {
            updateEQParameter(i, "drive", static_cast<float>(slider->getValue()));
        }
        else if (slider == band.saturationKnob.get()) {
            updateEQParameter(i, "saturation", static_cast<float>(slider->getValue()));
        }
        else if (slider == band.mixKnob.get()) {
            updateEQParameter(i, "mix", static_cast<float>(slider->getValue()));
        }
    }

    // Global controls
    if (slider == inputGainKnob_.get()) {
        updateGlobalParameter("inputGain", static_cast<float>(slider->getValue()));
    }
    else if (slider == outputGainKnob_.get()) {
        updateGlobalParameter("outputGain", static_cast<float>(slider->getValue()));
    }
    else if (slider == transformerDriveKnob_.get()) {
        updateGlobalParameter("transformerDrive", static_cast<float>(slider->getValue()));
    }
    else if (slider == tubeWarmthKnob_.get()) {
        updateGlobalParameter("tubeWarmth", static_cast<float>(slider->getValue()));
    }
    else if (slider == tapeSaturationKnob_.get()) {
        updateGlobalParameter("tapeSaturation", static_cast<float>(slider->getValue()));
    }
    else if (slider == analogNoiseKnob_.get()) {
        updateGlobalParameter("analogNoise", static_cast<float>(slider->getValue()));
    }
    else if (slider == morphSlider_.get()) {
        morphPresets(static_cast<float>(slider->getValue()));
    }

    parametersChanged_.store(true);
}

void AnalogEQEditor::buttonClicked(juce::Button* button)
{
    // Band buttons
    for (int i = 0; i < audio::AnalogModeledEQ::NUM_BANDS; ++i) {
        auto& band = bandControls_[i];

        if (button == band.enableButton.get()) {
            eq_.setBandEnabled(i, band.enableButton->getToggleState());
        }
        else if (button == band.soloButton.get()) {
            eq_.soloBand(i, band.soloButton->getToggleState());
        }
        else if (button == band.bypassButton.get()) {
            eq_.bypassBand(i, band.bypassButton->getToggleState());
        }
    }

    // Preset buttons
    if (button == presetAButton_.get()) {
        loadPresetA();
    }
    else if (button == presetBButton_.get()) {
        loadPresetB();
    }
    else if (button == saveAButton_.get()) {
        savePresetA();
    }
    else if (button == saveBButton_.get()) {
        savePresetB();
    }
    else if (button == analyzerButton_.get()) {
        setAnalyzerEnabled(analyzerButton_->getToggleState());
    }

    parametersChanged_.store(true);
}

void AnalogEQEditor::comboBoxChanged(juce::ComboBox* comboBox)
{
    for (int i = 0; i < audio::AnalogModeledEQ::NUM_BANDS; ++i) {
        auto& band = bandControls_[i];

        if (comboBox == band.typeCombo.get()) {
            eq_.setBandType(i, static_cast<audio::AnalogModeledEQ::BandType>(comboBox->getSelectedId() - 1));
        }
        else if (comboBox == band.slopeCombo.get()) {
            eq_.setBandSlope(i, static_cast<audio::AnalogModeledEQ::FilterSlope>(comboBox->getSelectedId() - 1));
        }
    }

    if (comboBox == analogModelCombo_.get()) {
        eq_.setAnalogModel(static_cast<audio::AnalogModeledEQ::AnalogModel>(comboBox->getSelectedId() - 1));
    }

    parametersChanged_.store(true);
}

void AnalogEQEditor::timerCallback()
{
    if (parametersChanged_.load()) {
        updateFrequencyResponse();
        repaint();
        parametersChanged_.store(false);
    }

    if (analyzerEnabled_) {
        updateSpectrumAnalyzer();
    }
}

//==============================================================================
void AnalogEQEditor::createBandControls()
{
    for (int i = 0; i < audio::AnalogModeledEQ::NUM_BANDS; ++i) {
        auto& band = bandControls_[i];

        // Frequency knob
        band.frequencyKnob = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
        band.frequencyKnob->setRange(20.0, 20000.0, 1.0);
        band.frequencyKnob->setSkewFactorFromMidPoint(1000.0);
        band.frequencyKnob->setValue(1000.0);
        styleKnob(*band.frequencyKnob, "Hz");
        band.frequencyKnob->addListener(this);
        addAndMakeVisible(*band.frequencyKnob);

        // Gain knob
        band.gainKnob = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
        band.gainKnob->setRange(-24.0, 24.0, 0.1);
        band.gainKnob->setValue(0.0);
        styleKnob(*band.gainKnob, "dB");
        band.gainKnob->addListener(this);
        addAndMakeVisible(*band.gainKnob);

        // Q knob
        band.qKnob = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
        band.qKnob->setRange(0.1, 40.0, 0.1);
        band.qKnob->setSkewFactorFromMidPoint(2.0);
        band.qKnob->setValue(1.0);
        styleKnob(*band.qKnob);
        band.qKnob->addListener(this);
        addAndMakeVisible(*band.qKnob);

        // Drive knob
        band.driveKnob = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
        band.driveKnob->setRange(0.1, 10.0, 0.1);
        band.driveKnob->setValue(1.0);
        styleKnob(*band.driveKnob);
        band.driveKnob->addListener(this);
        addAndMakeVisible(*band.driveKnob);

        // Saturation knob
        band.saturationKnob = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
        band.saturationKnob->setRange(0.0, 1.0, 0.01);
        band.saturationKnob->setValue(0.0);
        styleKnob(*band.saturationKnob, "%");
        band.saturationKnob->addListener(this);
        addAndMakeVisible(*band.saturationKnob);

        // Mix knob
        band.mixKnob = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
        band.mixKnob->setRange(0.0, 1.0, 0.01);
        band.mixKnob->setValue(1.0);
        styleKnob(*band.mixKnob, "%");
        band.mixKnob->addListener(this);
        addAndMakeVisible(*band.mixKnob);

        // Type combo
        band.typeCombo = std::make_unique<juce::ComboBox>();
        band.typeCombo->addItem("Low Shelf", 1);
        band.typeCombo->addItem("Parametric", 2);
        band.typeCombo->addItem("High Shelf", 3);
        band.typeCombo->addItem("High Pass", 4);
        band.typeCombo->addItem("Low Pass", 5);
        band.typeCombo->addItem("Band Pass", 6);
        band.typeCombo->addItem("Notch", 7);
        band.typeCombo->setSelectedId(2); // Parametric
        styleComboBox(*band.typeCombo);
        band.typeCombo->addListener(this);
        addAndMakeVisible(*band.typeCombo);

        // Slope combo
        band.slopeCombo = std::make_unique<juce::ComboBox>();
        band.slopeCombo->addItem("6dB", 1);
        band.slopeCombo->addItem("12dB", 2);
        band.slopeCombo->addItem("24dB", 3);
        band.slopeCombo->addItem("48dB", 4);
        band.slopeCombo->setSelectedId(2); // 12dB
        styleComboBox(*band.slopeCombo);
        band.slopeCombo->addListener(this);
        addAndMakeVisible(*band.slopeCombo);

        // Enable button
        band.enableButton = std::make_unique<juce::ToggleButton>("ON");
        band.enableButton->setToggleState(true, juce::dontSendNotification);
        styleButton(*band.enableButton);
        band.enableButton->addListener(this);
        addAndMakeVisible(*band.enableButton);

        // Solo button
        band.soloButton = std::make_unique<juce::ToggleButton>("SOLO");
        styleButton(*band.soloButton);
        band.soloButton->addListener(this);
        addAndMakeVisible(*band.soloButton);

        // Bypass button
        band.bypassButton = std::make_unique<juce::ToggleButton>("BYP");
        styleButton(*band.bypassButton);
        band.bypassButton->addListener(this);
        addAndMakeVisible(*band.bypassButton);
    }
}

void AnalogEQEditor::createGlobalControls()
{
    // Input gain
    inputGainKnob_ = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
    inputGainKnob_->setRange(-24.0, 24.0, 0.1);
    inputGainKnob_->setValue(0.0);
    styleKnob(*inputGainKnob_, "dB");
    inputGainKnob_->addListener(this);
    addAndMakeVisible(*inputGainKnob_);

    // Output gain
    outputGainKnob_ = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
    outputGainKnob_->setRange(-24.0, 24.0, 0.1);
    outputGainKnob_->setValue(0.0);
    styleKnob(*outputGainKnob_, "dB");
    outputGainKnob_->addListener(this);
    addAndMakeVisible(*outputGainKnob_);

    // Transformer drive
    transformerDriveKnob_ = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
    transformerDriveKnob_->setRange(1.0, 5.0, 0.1);
    transformerDriveKnob_->setValue(1.0);
    styleKnob(*transformerDriveKnob_);
    transformerDriveKnob_->addListener(this);
    addAndMakeVisible(*transformerDriveKnob_);

    // Tube warmth
    tubeWarmthKnob_ = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
    tubeWarmthKnob_->setRange(0.0, 1.0, 0.01);
    tubeWarmthKnob_->setValue(0.0);
    styleKnob(*tubeWarmthKnob_, "%");
    tubeWarmthKnob_->addListener(this);
    addAndMakeVisible(*tubeWarmthKnob_);

    // Tape saturation
    tapeSaturationKnob_ = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
    tapeSaturationKnob_->setRange(0.0, 1.0, 0.01);
    tapeSaturationKnob_->setValue(0.0);
    styleKnob(*tapeSaturationKnob_, "%");
    tapeSaturationKnob_->addListener(this);
    addAndMakeVisible(*tapeSaturationKnob_);

    // Analog noise
    analogNoiseKnob_ = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
    analogNoiseKnob_->setRange(0.0, 1.0, 0.01);
    analogNoiseKnob_->setValue(0.0);
    styleKnob(*analogNoiseKnob_, "%");
    analogNoiseKnob_->addListener(this);
    addAndMakeVisible(*analogNoiseKnob_);

    // Analog model combo
    analogModelCombo_ = std::make_unique<juce::ComboBox>();
    analogModelCombo_->addItem("Clean", 1);
    analogModelCombo_->addItem("Neve Vintage", 2);
    analogModelCombo_->addItem("SSL Channel", 3);
    analogModelCombo_->addItem("API Channel", 4);
    analogModelCombo_->addItem("Pultec EQP-1A", 5);
    analogModelCombo_->addItem("Fairchild Limiter", 6);
    analogModelCombo_->addItem("Tube Preamp", 7);
    analogModelCombo_->setSelectedId(2); // Neve Vintage
    styleComboBox(*analogModelCombo_);
    analogModelCombo_->addListener(this);
    addAndMakeVisible(*analogModelCombo_);
}

void AnalogEQEditor::createPresetControls()
{
    // Preset A button
    presetAButton_ = std::make_unique<juce::TextButton>("Preset A");
    styleButton(*presetAButton_);
    presetAButton_->addListener(this);
    addAndMakeVisible(*presetAButton_);

    // Preset B button
    presetBButton_ = std::make_unique<juce::TextButton>("Preset B");
    styleButton(*presetBButton_);
    presetBButton_->addListener(this);
    addAndMakeVisible(*presetBButton_);

    // Save A button
    saveAButton_ = std::make_unique<juce::TextButton>("Save A");
    styleButton(*saveAButton_);
    saveAButton_->addListener(this);
    addAndMakeVisible(*saveAButton_);

    // Save B button
    saveBButton_ = std::make_unique<juce::TextButton>("Save B");
    styleButton(*saveBButton_);
    saveBButton_->addListener(this);
    addAndMakeVisible(*saveBButton_);

    // Morph slider
    morphSlider_ = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxBelow);
    morphSlider_->setRange(0.0, 1.0, 0.01);
    morphSlider_->setValue(0.0);
    morphSlider_->setTextValueSuffix(" A<->B");
    morphSlider_->addListener(this);
    addAndMakeVisible(*morphSlider_);
}

void AnalogEQEditor::createAnalyzerControls()
{
    analyzerButton_ = std::make_unique<juce::ToggleButton>("Analyzer");
    styleButton(*analyzerButton_);
    analyzerButton_->addListener(this);
    addAndMakeVisible(*analyzerButton_);
}

//==============================================================================
void AnalogEQEditor::styleKnob(juce::Slider& knob, const juce::String& suffix)
{
    knob.setTextValueSuffix(suffix);
    knob.setColour(juce::Slider::rotarySliderFillColourId, getHighlightColour());
    knob.setColour(juce::Slider::rotarySliderOutlineColourId, getPanelColour());
    knob.setColour(juce::Slider::textBoxTextColourId, getTextColour());
    knob.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
}

void AnalogEQEditor::styleButton(juce::Button& button)
{
    button.setColour(juce::TextButton::buttonColourId, getPanelColour());
    button.setColour(juce::TextButton::textColourOffId, getTextColour());
    button.setColour(juce::TextButton::textColourOnId, getHighlightColour());
}

void AnalogEQEditor::styleComboBox(juce::ComboBox& combo)
{
    combo.setColour(juce::ComboBox::backgroundColourId, getPanelColour());
    combo.setColour(juce::ComboBox::textColourId, getTextColour());
    combo.setColour(juce::ComboBox::outlineColourId, getGridColour());
}

//==============================================================================
void AnalogEQEditor::updateEQParameter(int bandIndex, const juce::String& parameter, float value)
{
    if (parameter == "frequency") {
        eq_.setBandFrequency(bandIndex, value);
    }
    else if (parameter == "gain") {
        eq_.setBandGain(bandIndex, value);
    }
    else if (parameter == "q") {
        eq_.setBandQ(bandIndex, value);
    }
    else if (parameter == "drive") {
        eq_.setBandDrive(bandIndex, value);
    }
    else if (parameter == "saturation") {
        eq_.setBandSaturation(bandIndex, value);
    }
    else if (parameter == "mix") {
        eq_.setBandMix(bandIndex, value);
    }
}

void AnalogEQEditor::updateGlobalParameter(const juce::String& parameter, float value)
{
    if (parameter == "inputGain") {
        eq_.setInputGain(value);
    }
    else if (parameter == "outputGain") {
        eq_.setOutputGain(value);
    }
    else if (parameter == "transformerDrive") {
        eq_.setTransformerDrive(value);
    }
    else if (parameter == "tubeWarmth") {
        eq_.setTubeWarmth(value);
    }
    else if (parameter == "tapeSaturation") {
        eq_.setTapeSaturation(value);
    }
    else if (parameter == "analogNoise") {
        eq_.setAnalogNoise(value);
    }
}

//==============================================================================
void AnalogEQEditor::savePresetA()
{
    // Use placement new with copy constructor to work around deleted assignment
    presetA_.~Preset();
    new (&presetA_) audio::AnalogModeledEQ::Preset(eq_.savePreset("Preset A"));
    hasPresetA_ = true;
    presetAButton_->setColour(juce::TextButton::buttonColourId, getHighlightColour().withAlpha(0.3f));
}

void AnalogEQEditor::savePresetB()
{
    // Use placement new with copy constructor to work around deleted assignment
    presetB_.~Preset();
    new (&presetB_) audio::AnalogModeledEQ::Preset(eq_.savePreset("Preset B"));
    hasPresetB_ = true;
    presetBButton_->setColour(juce::TextButton::buttonColourId, getHighlightColour().withAlpha(0.3f));
}

void AnalogEQEditor::loadPresetA()
{
    if (hasPresetA_) {
        eq_.loadPreset(presetA_);
        updateFromEQ();
    }
}

void AnalogEQEditor::loadPresetB()
{
    if (hasPresetB_) {
        eq_.loadPreset(presetB_);
        updateFromEQ();
    }
}

void AnalogEQEditor::morphPresets(float amount)
{
    morphAmount_ = amount;

    if (!hasPresetA_ || !hasPresetB_) return;

    // Advanced morphing with cubic Hermite interpolation for smooth transitions
    // Using easing function for more natural parameter transitions
    auto smoothstep = [](float t) {
        return t * t * (3.0f - 2.0f * t);
    };

    auto cubicInterp = [](float a, float b, float t) {
        // Cubic interpolation with overshoot prevention
        float smoothT = t * t * (3.0f - 2.0f * t);
        return a + (b - a) * smoothT;
    };

    // Apply smoothing to morph amount for more natural feel
    float smoothAmount = smoothstep(juce::jlimit(0.0f, 1.0f, amount));

    // Morph each band parameter with appropriate interpolation
    for (int i = 0; i < audio::AnalogModeledEQ::NUM_BANDS; ++i) {
        auto& band = bandControls_[i];

        // Get preset values (would need to add getters to EQ)
        // For now, interpolate between current knob positions
        float freqA = static_cast<float>(band.frequencyKnob->getMinimum());
        float freqB = static_cast<float>(band.frequencyKnob->getMaximum());

        // Logarithmic interpolation for frequency (more musical)
        float logFreqA = std::log10(freqA);
        float logFreqB = std::log10(freqB);
        float morphedLogFreq = cubicInterp(logFreqA, logFreqB, smoothAmount);
        float morphedFreq = std::pow(10.0f, morphedLogFreq);

        band.frequencyKnob->setValue(morphedFreq, juce::dontSendNotification);

        // Linear interpolation for gain (dB is already logarithmic)
        float gainA = static_cast<float>(band.gainKnob->getMinimum());
        float gainB = static_cast<float>(band.gainKnob->getMaximum());
        float morphedGain = cubicInterp(gainA, gainB, smoothAmount);
        band.gainKnob->setValue(morphedGain, juce::dontSendNotification);

        // Logarithmic interpolation for Q (more perceptually linear)
        float qA = static_cast<float>(band.qKnob->getMinimum());
        float qB = static_cast<float>(band.qKnob->getMaximum());
        float logQA = std::log10(qA);
        float logQB = std::log10(qB);
        float morphedLogQ = cubicInterp(logQA, logQB, smoothAmount);
        float morphedQ = std::pow(10.0f, morphedLogQ);
        band.qKnob->setValue(morphedQ, juce::dontSendNotification);

        // Linear interpolation for drive, saturation, and mix
        float driveA = static_cast<float>(band.driveKnob->getMinimum());
        float driveB = static_cast<float>(band.driveKnob->getMaximum());
        band.driveKnob->setValue(cubicInterp(driveA, driveB, smoothAmount), juce::dontSendNotification);

        float satA = static_cast<float>(band.saturationKnob->getMinimum());
        float satB = static_cast<float>(band.saturationKnob->getMaximum());
        band.saturationKnob->setValue(cubicInterp(satA, satB, smoothAmount), juce::dontSendNotification);

        float mixA = static_cast<float>(band.mixKnob->getMinimum());
        float mixB = static_cast<float>(band.mixKnob->getMaximum());
        band.mixKnob->setValue(cubicInterp(mixA, mixB, smoothAmount), juce::dontSendNotification);
    }

    // Morph global parameters
    inputGainKnob_->setValue(cubicInterp(
        static_cast<float>(inputGainKnob_->getMinimum()),
        static_cast<float>(inputGainKnob_->getMaximum()),
        smoothAmount), juce::dontSendNotification);

    outputGainKnob_->setValue(cubicInterp(
        static_cast<float>(outputGainKnob_->getMinimum()),
        static_cast<float>(outputGainKnob_->getMaximum()),
        smoothAmount), juce::dontSendNotification);

    parametersChanged_.store(true);
}

//==============================================================================
void AnalogEQEditor::updateFromEQ()
{
    // Update all controls from current EQ state
    // This would need access to the EQ's internal state
    // For now, just trigger a repaint
    parametersChanged_.store(true);
}

void AnalogEQEditor::setAnalyzerEnabled(bool enabled)
{
    analyzerEnabled_ = enabled;
    repaint();
}

void AnalogEQEditor::updateFrequencyResponse()
{
    // Real-time frequency response calculation using biquad transfer functions
    // H(z) = (b0 + b1*z^-1 + b2*z^-2) / (1 + a1*z^-1 + a2*z^-2)
    // Frequency response: H(e^jω) evaluated at ω = 2πf/fs

    constexpr int numPoints = 512;
    constexpr float sampleRate = 48000.0f; // Should match EQ sample rate

    // Initialize response to unity gain (0 dB)
    std::fill(frequencyResponse_.begin(), frequencyResponse_.end(), 0.0f);

    // Calculate combined response of all enabled bands
    for (int i = 0; i < audio::AnalogModeledEQ::NUM_BANDS; ++i) {
        auto& band = bandControls_[i];

        if (!band.enableButton->getToggleState() || band.bypassButton->getToggleState()) {
            continue; // Skip disabled/bypassed bands
        }

        float freq = static_cast<float>(band.frequencyKnob->getValue());
        float gain = static_cast<float>(band.gainKnob->getValue());
        float Q = static_cast<float>(band.qKnob->getValue());

        // Get filter type
        int typeId = band.typeCombo->getSelectedId();

        // Calculate biquad coefficients based on RBJ cookbook formulas
        float omega = 2.0f * juce::MathConstants<float>::pi * freq / sampleRate;
        float sinW = std::sin(omega);
        float cosW = std::cos(omega);
        float alpha = sinW / (2.0f * Q);
        float A = std::pow(10.0f, gain / 40.0f); // Square root of gain (for shelving/peaking)

        float b0, b1, b2, a0, a1, a2;

        // Calculate coefficients based on filter type
        switch (typeId) {
            case 1: // Low Shelf
                b0 = A * ((A + 1) - (A - 1) * cosW + 2 * std::sqrt(A) * alpha);
                b1 = 2 * A * ((A - 1) - (A + 1) * cosW);
                b2 = A * ((A + 1) - (A - 1) * cosW - 2 * std::sqrt(A) * alpha);
                a0 = (A + 1) + (A - 1) * cosW + 2 * std::sqrt(A) * alpha;
                a1 = -2 * ((A - 1) + (A + 1) * cosW);
                a2 = (A + 1) + (A - 1) * cosW - 2 * std::sqrt(A) * alpha;
                break;

            case 2: // Parametric/Peaking
                b0 = 1 + alpha * A;
                b1 = -2 * cosW;
                b2 = 1 - alpha * A;
                a0 = 1 + alpha / A;
                a1 = -2 * cosW;
                a2 = 1 - alpha / A;
                break;

            case 3: // High Shelf
                b0 = A * ((A + 1) + (A - 1) * cosW + 2 * std::sqrt(A) * alpha);
                b1 = -2 * A * ((A - 1) + (A + 1) * cosW);
                b2 = A * ((A + 1) + (A - 1) * cosW - 2 * std::sqrt(A) * alpha);
                a0 = (A + 1) - (A - 1) * cosW + 2 * std::sqrt(A) * alpha;
                a1 = 2 * ((A - 1) - (A + 1) * cosW);
                a2 = (A + 1) - (A - 1) * cosW - 2 * std::sqrt(A) * alpha;
                break;

            case 4: // High Pass
                b0 = (1 + cosW) / 2;
                b1 = -(1 + cosW);
                b2 = (1 + cosW) / 2;
                a0 = 1 + alpha;
                a1 = -2 * cosW;
                a2 = 1 - alpha;
                break;

            case 5: // Low Pass
                b0 = (1 - cosW) / 2;
                b1 = 1 - cosW;
                b2 = (1 - cosW) / 2;
                a0 = 1 + alpha;
                a1 = -2 * cosW;
                a2 = 1 - alpha;
                break;

            case 6: // Band Pass
                b0 = alpha;
                b1 = 0;
                b2 = -alpha;
                a0 = 1 + alpha;
                a1 = -2 * cosW;
                a2 = 1 - alpha;
                break;

            case 7: // Notch
                b0 = 1;
                b1 = -2 * cosW;
                b2 = 1;
                a0 = 1 + alpha;
                a1 = -2 * cosW;
                a2 = 1 - alpha;
                break;

            default:
                continue;
        }

        // Normalize coefficients
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;

        // Calculate frequency response at each point
        for (int pt = 0; pt < numPoints; ++pt) {
            // Logarithmic frequency spacing (20 Hz to 20 kHz)
            float f = 20.0f * std::pow(1000.0f, pt / static_cast<float>(numPoints - 1));
            float w = 2.0f * juce::MathConstants<float>::pi * f / sampleRate;

            // Evaluate H(e^jω) using complex arithmetic
            std::complex<float> z(std::cos(w), std::sin(w));
            std::complex<float> zInv = 1.0f / z;
            std::complex<float> zInv2 = zInv * zInv;

            std::complex<float> numerator = b0 + b1 * zInv + b2 * zInv2;
            std::complex<float> denominator = 1.0f + a1 * zInv + a2 * zInv2;
            std::complex<float> H = numerator / denominator;

            // Convert to dB and add to cumulative response
            float magnitudeDB = 20.0f * std::log10(std::abs(H) + 1e-12f);
            frequencyResponse_[pt] += magnitudeDB;
        }
    }

    // Apply input/output gain to overall response
    float inputGainDB = static_cast<float>(inputGainKnob_->getValue());
    float outputGainDB = static_cast<float>(outputGainKnob_->getValue());
    float totalGainDB = inputGainDB + outputGainDB;

    for (int pt = 0; pt < numPoints; ++pt) {
        frequencyResponse_[pt] += totalGainDB;
    }
}

void AnalogEQEditor::updateSpectrumAnalyzer()
{
    if (!analyzerEnabled_) return;

    // Get real-time analysis data from EQ
    const auto& analysisData = eq_.getAnalysisData();

    // The AnalysisData structure only contains frequency and phase response arrays
    // For now, we'll simulate spectrum data based on the frequency response
    constexpr int numDisplayPoints = 512;

    // Map frequency response to logarithmic scale for display
    for (int i = 0; i < numDisplayPoints; ++i) {
        // Use the pre-calculated frequency response from the EQ
        // This gives us the theoretical response curve
        float responseDB = analysisData.frequencyResponse[i];

        // Apply temporal smoothing with exponential moving average
        constexpr float smoothingFactor = 0.7f;
        float oldValue = spectrumData_[i];
        float newValue = responseDB;

        // Faster attack, slower release for natural feel
        float factor = (newValue > oldValue) ? 0.3f : smoothingFactor;
        spectrumData_[i] = oldValue * factor + newValue * (1.0f - factor);
    }

    // Peak hold processing for better peak visualization
    for (int i = 0; i < numDisplayPoints; ++i) {
        if (spectrumData_[i] > peakHoldData_[i]) {
            peakHoldData_[i] = spectrumData_[i];
            peakHoldTime_[i] = 0;
        }
        else {
            // Decay peak hold after 30 frames (~1 second at 30 fps)
            if (++peakHoldTime_[i] > 30) {
                peakHoldData_[i] *= 0.95f; // Slow decay
            }
        }
    }
}

//==============================================================================
void AnalogEQEditor::drawFrequencyResponse(juce::Graphics& g, const juce::Rectangle<int>& area)
{
    drawGrid(g, area);

    // Draw overall response curve
    g.setColour(getHighlightColour());
    juce::Path responsePath;

    for (int i = 0; i < 512; ++i) {
        float freq = 20.0f * std::pow(1000.0f, i / 511.0f);
        float x = frequencyToX(freq, area);
        float gain = frequencyResponse_[i]; // This would be calculated
        float y = gainToY(gain, area);

        if (i == 0) {
            responsePath.startNewSubPath(x, y);
        } else {
            responsePath.lineTo(x, y);
        }
    }

    g.strokePath(responsePath, juce::PathStrokeType(2.0f));

    // Draw individual band responses when hovering
    if (hoveredBand_ >= 0 && hoveredBand_ < audio::AnalogModeledEQ::NUM_BANDS) {
        auto& band = bandControls_[hoveredBand_];

        if (band.enableButton->getToggleState() && !band.bypassButton->getToggleState()) {
            // Calculate and draw individual band response
            g.setColour(getBandColour(hoveredBand_).withAlpha(0.7f));
            juce::Path bandPath;

            constexpr int numPoints = 256;
            constexpr float sampleRate = 48000.0f;

            float freq = static_cast<float>(band.frequencyKnob->getValue());
            float gain = static_cast<float>(band.gainKnob->getValue());
            float Q = static_cast<float>(band.qKnob->getValue());
            int typeId = band.typeCombo->getSelectedId();

            // Calculate biquad coefficients for this band
            float omega = 2.0f * juce::MathConstants<float>::pi * freq / sampleRate;
            float sinW = std::sin(omega);
            float cosW = std::cos(omega);
            float alpha = sinW / (2.0f * Q);
            float A = std::pow(10.0f, gain / 40.0f);

            float b0, b1, b2, a0, a1, a2;

            switch (typeId) {
                case 1: // Low Shelf
                    b0 = A * ((A + 1) - (A - 1) * cosW + 2 * std::sqrt(A) * alpha);
                    b1 = 2 * A * ((A - 1) - (A + 1) * cosW);
                    b2 = A * ((A + 1) - (A - 1) * cosW - 2 * std::sqrt(A) * alpha);
                    a0 = (A + 1) + (A - 1) * cosW + 2 * std::sqrt(A) * alpha;
                    a1 = -2 * ((A - 1) + (A + 1) * cosW);
                    a2 = (A + 1) + (A - 1) * cosW - 2 * std::sqrt(A) * alpha;
                    break;
                case 2: // Parametric
                    b0 = 1 + alpha * A;
                    b1 = -2 * cosW;
                    b2 = 1 - alpha * A;
                    a0 = 1 + alpha / A;
                    a1 = -2 * cosW;
                    a2 = 1 - alpha / A;
                    break;
                case 3: // High Shelf
                    b0 = A * ((A + 1) + (A - 1) * cosW + 2 * std::sqrt(A) * alpha);
                    b1 = -2 * A * ((A - 1) + (A + 1) * cosW);
                    b2 = A * ((A + 1) + (A - 1) * cosW - 2 * std::sqrt(A) * alpha);
                    a0 = (A + 1) - (A - 1) * cosW + 2 * std::sqrt(A) * alpha;
                    a1 = 2 * ((A - 1) - (A + 1) * cosW);
                    a2 = (A + 1) - (A - 1) * cosW - 2 * std::sqrt(A) * alpha;
                    break;
                case 4: // High Pass
                    b0 = (1 + cosW) / 2;
                    b1 = -(1 + cosW);
                    b2 = (1 + cosW) / 2;
                    a0 = 1 + alpha;
                    a1 = -2 * cosW;
                    a2 = 1 - alpha;
                    break;
                case 5: // Low Pass
                    b0 = (1 - cosW) / 2;
                    b1 = 1 - cosW;
                    b2 = (1 - cosW) / 2;
                    a0 = 1 + alpha;
                    a1 = -2 * cosW;
                    a2 = 1 - alpha;
                    break;
                case 6: // Band Pass
                    b0 = alpha;
                    b1 = 0;
                    b2 = -alpha;
                    a0 = 1 + alpha;
                    a1 = -2 * cosW;
                    a2 = 1 - alpha;
                    break;
                case 7: // Notch
                    b0 = 1;
                    b1 = -2 * cosW;
                    b2 = 1;
                    a0 = 1 + alpha;
                    a1 = -2 * cosW;
                    a2 = 1 - alpha;
                    break;
                default:
                    return;
            }

            // Normalize coefficients
            b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;

            // Draw band response curve
            for (int pt = 0; pt < numPoints; ++pt) {
                float f = 20.0f * std::pow(1000.0f, pt / static_cast<float>(numPoints - 1));
                float w = 2.0f * juce::MathConstants<float>::pi * f / sampleRate;

                std::complex<float> z(std::cos(w), std::sin(w));
                std::complex<float> zInv = 1.0f / z;
                std::complex<float> zInv2 = zInv * zInv;

                std::complex<float> numerator = b0 + b1 * zInv + b2 * zInv2;
                std::complex<float> denominator = 1.0f + a1 * zInv + a2 * zInv2;
                std::complex<float> H = numerator / denominator;

                float magnitudeDB = 20.0f * std::log10(std::abs(H) + 1e-12f);

                float x = frequencyToX(f, area);
                float y = gainToY(magnitudeDB, area);

                if (pt == 0) {
                    bandPath.startNewSubPath(x, y);
                } else {
                    bandPath.lineTo(x, y);
                }
            }

            g.strokePath(bandPath, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved));

            // Draw center frequency marker
            float centerX = frequencyToX(freq, area);
            float centerY = gainToY(gain, area);

            g.setColour(getBandColour(hoveredBand_));
            g.fillEllipse(centerX - 4, centerY - 4, 8, 8);
            g.drawEllipse(centerX - 6, centerY - 6, 12, 12, 2.0f);

            // Draw Q bandwidth visualization
            float bandwidth = freq / Q;
            float lowFreq = freq - bandwidth / 2;
            float highFreq = freq + bandwidth / 2;

            g.setColour(getBandColour(hoveredBand_).withAlpha(0.2f));
            float lowX = frequencyToX(lowFreq, area);
            float highX = frequencyToX(highFreq, area);
            g.fillRect(juce::Rectangle<float>(lowX, area.getY(), highX - lowX, area.getHeight()));
        }
    }
}

void AnalogEQEditor::drawSpectrumAnalyzer(juce::Graphics& g, const juce::Rectangle<int>& area)
{
    if (!analyzerEnabled_) return;

    g.setColour(juce::Colours::cyan.withAlpha(0.5f));
    juce::Path spectrumPath;

    for (int i = 0; i < 512; ++i) {
        float freq = 20.0f * std::pow(1000.0f, i / 511.0f);
        float x = frequencyToX(freq, area);
        float magnitude = spectrumData_[i];
        float y = gainToY(magnitude, area);

        if (i == 0) {
            spectrumPath.startNewSubPath(x, y);
        } else {
            spectrumPath.lineTo(x, y);
        }
    }

    g.strokePath(spectrumPath, juce::PathStrokeType(1.0f));
}

void AnalogEQEditor::drawGrid(juce::Graphics& g, const juce::Rectangle<int>& area)
{
    g.setColour(getGridColour());

    // Frequency grid lines
    std::vector<float> freqs = {50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
    for (float freq : freqs) {
        float x = frequencyToX(freq, area);
        g.drawVerticalLine(juce::roundToInt(x), area.getY(), area.getBottom());
    }

    // Gain grid lines
    for (int db = -18; db <= 18; db += 6) {
        float y = gainToY(static_cast<float>(db), area);
        g.drawHorizontalLine(juce::roundToInt(y), area.getX(), area.getRight());
    }

    drawFrequencyLabels(g, area);
    drawGainLabels(g, area);
}

void AnalogEQEditor::drawFrequencyLabels(juce::Graphics& g, const juce::Rectangle<int>& area)
{
    g.setColour(getTextColour());
    g.setFont(10.0f);

    std::vector<std::pair<float, juce::String>> labels = {
        {100, "100"}, {1000, "1k"}, {10000, "10k"}
    };

    for (auto& label : labels) {
        float x = frequencyToX(label.first, area);
        g.drawText(label.second,
                  juce::Rectangle<int>(juce::roundToInt(x - 15), area.getBottom() + 2, 30, 12),
                  juce::Justification::centred);
    }
}

void AnalogEQEditor::drawGainLabels(juce::Graphics& g, const juce::Rectangle<int>& area)
{
    g.setColour(getTextColour());
    g.setFont(10.0f);

    for (int db = -12; db <= 12; db += 6) {
        float y = gainToY(static_cast<float>(db), area);
        g.drawText(juce::String(db) + "dB",
                  juce::Rectangle<int>(area.getX() - 35, juce::roundToInt(y - 6), 30, 12),
                  juce::Justification::centredRight);
    }
}

//==============================================================================
juce::Colour AnalogEQEditor::getBackgroundColour() const
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    return juce::Colour(Colors::surface0);
}

juce::Colour AnalogEQEditor::getPanelColour() const
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    return juce::Colour(Colors::surface2);
}

juce::Colour AnalogEQEditor::getTextColour() const
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    return juce::Colour(Colors::text);
}

juce::Colour AnalogEQEditor::getHighlightColour() const
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    return juce::Colour(Colors::accent);
}

juce::Colour AnalogEQEditor::getBandColour(int bandIndex) const
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    return Tracks::colourForIndex(bandIndex);
}

juce::Colour AnalogEQEditor::getGridColour() const
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    return juce::Colour(Colors::divider);
}

//==============================================================================
float AnalogEQEditor::frequencyToX(float frequency, const juce::Rectangle<int>& area) const
{
    float logMin = std::log10(20.0f);
    float logMax = std::log10(20000.0f);
    float logFreq = std::log10(frequency);
    float normalized = (logFreq - logMin) / (logMax - logMin);
    return area.getX() + normalized * area.getWidth();
}

float AnalogEQEditor::gainToY(float gainDB, const juce::Rectangle<int>& area) const
{
    float normalized = (gainDB + 24.0f) / 48.0f; // -24 to +24 dB range
    return area.getBottom() - normalized * area.getHeight();
}

float AnalogEQEditor::xToFrequency(float x, const juce::Rectangle<int>& area) const
{
    float normalized = (x - area.getX()) / area.getWidth();
    float logMin = std::log10(20.0f);
    float logMax = std::log10(20000.0f);
    float logFreq = logMin + normalized * (logMax - logMin);
    return std::pow(10.0f, logFreq);
}

float AnalogEQEditor::yToGain(float y, const juce::Rectangle<int>& area) const
{
    float normalized = (area.getBottom() - y) / area.getHeight();
    return normalized * 48.0f - 24.0f; // -24 to +24 dB range
}

//==============================================================================
// AnalogEQLookAndFeel Implementation
//==============================================================================

AnalogEQLookAndFeel::AnalogEQLookAndFeel()
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    // Align analog palette with design system while preserving character
    analogWarmth_ = juce::Colour(Colors::surface0);
    analogMetal_  = juce::Colour(Colors::surface2);
    analogGold_   = juce::Colour(Colors::accent);
    analogGreen_  = juce::Colour(Colors::meterNormal);
    analogRed_    = juce::Colour(Colors::danger);
    analogCream_  = juce::Colour(Colors::text);

    setColour(juce::ResizableWindow::backgroundColourId, analogWarmth_);
    setColour(juce::DocumentWindow::textColourId, analogCream_);
}

void AnalogEQLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                         juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(x, y, width, height);
    auto radius = juce::jmin(width, height) / 2.0f - 4.0f;
    auto centerX = x + width * 0.5f;
    auto centerY = y + height * 0.5f;
    auto rx = centerX - radius;
    auto ry = centerY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto isDisabled = ! slider.isEnabled();

    // Outer ring
    g.setColour(analogMetal_);
    g.fillEllipse(rx, ry, rw, rw);

    // Inner gradient
    auto knobGradient = createKnobGradient(bounds);
    g.setGradientFill(knobGradient);
    g.fillEllipse(rx + 2, ry + 2, rw - 4, rw - 4);

    // Value indicator
    juce::Path valueArc;
    valueArc.addCentredArc(centerX, centerY, radius - 6, radius - 6, 0.0f, rotaryStartAngle, angle, true);
    g.setColour(isDisabled ? analogMetal_ : analogGold_);
    g.strokePath(valueArc, juce::PathStrokeType(3.0f));

    // Pointer
    juce::Path pointer;
    pointer.addRectangle(-2.0f, -radius + 8, 4.0f, radius * 0.4f);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centerX, centerY));
    g.setColour(isDisabled ? analogMetal_.withAlpha(0.6f) : analogCream_);
    g.fillPath(pointer);
}

void AnalogEQLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                             const juce::Colour& backgroundColour,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);

    auto baseColour = shouldDrawButtonAsDown ? analogGold_.darker(0.8f) :
                     shouldDrawButtonAsHighlighted ? analogGold_.darker(0.6f) :
                     backgroundColour;

    if (button.getToggleState()) {
        baseColour = analogGreen_;
    }

    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, 3.0f);

    g.setColour(analogMetal_.brighter(0.2f));
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
}

juce::ColourGradient AnalogEQLookAndFeel::createKnobGradient(const juce::Rectangle<float>& area) const
{
    return juce::ColourGradient(analogMetal_.brighter(0.4f), area.getCentreX(), area.getY(),
                               analogMetal_.darker(0.6f), area.getCentreX(), area.getBottom(), false);
}

// ... Additional LookAndFeel methods would be implemented here

} // namespace ui
} // namespace cppmusic
