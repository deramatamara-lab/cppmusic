#include "AuroraReverbEditor.h"
#include "lookandfeel/DesignSystem.h"

#include <juce_graphics/juce_graphics.h>

namespace cppmusic {
namespace ui {

namespace {
inline juce::Colour backgroundTop()
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    return juce::Colour(Colors::surface0);
}

inline juce::Colour backgroundBottom()
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    return juce::Colour(Colors::surface2);
}

inline juce::Colour accentColour()
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    return juce::Colour(Colors::accent);
}

inline juce::Colour accentSecondary()
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    return juce::Colour(Colors::primary);
}

inline juce::Colour trackColour()
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    return juce::Colour(Colors::surface1);
}

inline juce::Colour textPrimary()
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    return juce::Colour(Colors::text);
}

inline juce::Colour textMuted()
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    return juce::Colour(Colors::textSecondary);
}

inline juce::Rectangle<int> withMargin(juce::Rectangle<int> area, int margin)
{
    return area.reduced(margin);
}
}

AuroraLookAndFeel::AuroraLookAndFeel()
{
    setColour(juce::Slider::thumbColourId, accentColour());
    setColour(juce::Slider::rotarySliderFillColourId, accentColour().withMultipliedAlpha(0.75f));
    setColour(juce::Slider::rotarySliderOutlineColourId, trackColour());
    setColour(juce::Slider::textBoxTextColourId, textPrimary());
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TextButton::buttonColourId, trackColour());
    setColour(juce::TextButton::textColourOnId, textPrimary());
    setColour(juce::TextButton::textColourOffId, textPrimary());
    setColour(juce::ComboBox::backgroundColourId, trackColour());
    setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::textColourId, textPrimary());
}

void AuroraLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                         juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                               static_cast<float>(width), static_cast<float>(height)).reduced(8.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    auto centre = bounds.getCentre();
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto isDisabled = ! slider.isEnabled();

    juce::Path dial;
    dial.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(isDisabled ? trackColour().darker(0.4f) : trackColour());
    g.strokePath(dial, juce::PathStrokeType(2.0f));

    juce::ColourGradient grad(isDisabled ? accentColour().withAlpha(0.2f) : accentColour(),
                              centre.x, bounds.getY(),
                              isDisabled ? accentSecondary().withAlpha(0.15f) : accentSecondary(),
                              centre.x, bounds.getBottom(), true);
    g.setGradientFill(grad);
    juce::Path fill;
    fill.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, angle, true);
    g.strokePath(fill, juce::PathStrokeType(3.0f));

    juce::Path pointer;
    auto pointerLength = radius * 0.8f;
    auto pointerThickness = 4.0f;
    pointer.addRectangle(-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength);
    g.setColour(isDisabled ? accentColour().withAlpha(0.3f) : accentColour());
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
}

DecayScope::DecayScope(std::function<float()> f)
    : feed(std::move(f))
{
    jassert(feed);
    startTimerHz(30);
}

void DecayScope::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient grad(accentColour().withAlpha(0.15f), bounds.getTopLeft(),
                              accentSecondary().withAlpha(0.05f), bounds.getBottomRight(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(bounds, 10.0f);

    g.setColour(accentColour().withAlpha(0.5f));
    g.drawRoundedRectangle(bounds, 10.0f, 1.5f);

    const float decayValue = juce::jlimit(0.0f, 1.0f, feed ? feed() : 0.0f);
    auto scopeArea = bounds.reduced(10.0f);
    juce::Path path;
    path.startNewSubPath(scopeArea.getX(), scopeArea.getBottom());

    constexpr int samples = 80;
    for (int i = 0; i <= samples; ++i)
    {
        const float t = static_cast<float>(i) / samples;
        const float env = juce::jmap(std::pow(1.0f - t, 2.0f), 0.0f, 1.0f, decayValue, 0.02f);
        auto x = scopeArea.getX() + t * scopeArea.getWidth();
        auto y = scopeArea.getBottom() - env * scopeArea.getHeight();
        path.lineTo(x, y);
    }
    path.lineTo(scopeArea.getRight(), scopeArea.getBottom());
    path.closeSubPath();

    g.setColour(accentColour().withAlpha(0.35f));
    g.fillPath(path);
}

GRMeter::GRMeter(std::function<float()> f)
    : feed(std::move(f))
{
    jassert(feed);
    startTimerHz(30);
}

void GRMeter::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour(trackColour());
    g.fillRoundedRectangle(bounds, 8.0f);

    g.setColour(accentSecondary().withAlpha(0.9f));
    const auto value = juce::jlimit(0.0f, 1.0f, feed ? feed() : 0.0f);
    auto fill = bounds;
    fill.setY(bounds.getBottom() - value * bounds.getHeight());
    g.fillRoundedRectangle(fill, 6.0f);

    g.setColour(accentColour().withAlpha(0.6f));
    g.drawRoundedRectangle(bounds, 8.0f, 1.5f);
}

XYPadRV::XYPadRV(std::function<void(float, float)> cb)
    : onChange(std::move(cb))
{
    jassert(onChange);
    setRepaintsOnMouseActivity(true);
}

void XYPadRV::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient grad(accentColour().withAlpha(0.1f), bounds.getBottomLeft(),
                              accentSecondary().withAlpha(0.1f), bounds.getTopRight(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(bounds, 12.0f);

    g.setColour(trackColour());
    g.drawRoundedRectangle(bounds, 12.0f, 1.5f);

    const auto crossX = bounds.getX() + x * bounds.getWidth();
    const auto crossY = bounds.getY() + y * bounds.getHeight();

    g.setColour(trackColour());
    g.drawLine(crossX, bounds.getY(), crossX, bounds.getBottom(), 1.0f);
    g.drawLine(bounds.getX(), crossY, bounds.getRight(), crossY, 1.0f);

    juce::ColourGradient knobGrad(accentColour(), crossX, crossY,
                                  accentSecondary(), crossX + 10.0f, crossY + 10.0f, true);
    g.setGradientFill(knobGrad);
    g.fillEllipse(crossX - 8.0f, crossY - 8.0f, 16.0f, 16.0f);
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawEllipse(crossX - 8.0f, crossY - 8.0f, 16.0f, 16.0f, 1.0f);
}

void XYPadRV::mouseDown(const juce::MouseEvent& e)
{
    mouseDrag(e);
}

void XYPadRV::mouseDrag(const juce::MouseEvent& e)
{
    auto bounds = getLocalBounds().toFloat();
    auto pos = e.position;
    x = juce::jlimit(0.0f, 1.0f, (pos.x - bounds.getX()) / bounds.getWidth());
    y = juce::jlimit(0.0f, 1.0f, (pos.y - bounds.getY()) / bounds.getHeight());
    repaint();
    if (onChange)
        onChange(x, y);
}

void XYPadRV::setPosition(float newX, float newY)
{
    x = juce::jlimit(0.0f, 1.0f, newX);
    y = juce::jlimit(0.0f, 1.0f, newY);
    repaint();
}

AuroraReverbEditor::AuroraReverbEditor(audio::AuroraReverbAudioProcessor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    setLookAndFeel(&lnf);
    setSize(820, 520);

    xy = std::make_unique<XYPadRV>([this](float nx, float ny)
    {
        const auto mixRange = mix.getRange();
        const auto decayRange = decay.getRange();
        const double mixVal = mixRange.getStart() + nx * mixRange.getLength();
        const double decayVal = decayRange.getStart() + (1.0 - ny) * decayRange.getLength();
        mix.setValue(mixVal, juce::NotificationType::sendNotificationAsync);
        decay.setValue(decayVal, juce::NotificationType::sendNotificationAsync);
    });
    addAndMakeVisible(*xy);

    scope = std::make_unique<DecayScope>([this]() -> float
    {
        const auto energy = proc.getWetEnergy();
        return juce::jlimit(0.0f, 1.0f, energy);
    });
    addAndMakeVisible(*scope);

    gr = std::make_unique<GRMeter>([this]() -> float
    {
        return juce::jlimit(0.0f, 1.0f, proc.getDuckGR());
    });
    addAndMakeVisible(*gr);

    const auto configureKnob = [this](juce::Slider& slider, const juce::String& suffix)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 24);
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setTextValueSuffix(suffix);
        slider.setLookAndFeel(&lnf);
        addAndMakeVisible(slider);
    };

    const auto configureLinear = [this](juce::Slider& slider, bool isVertical)
    {
        slider.setSliderStyle(isVertical ? juce::Slider::LinearBarVertical
                                         : juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 24);
        slider.setColour(juce::Slider::backgroundColourId, trackColour());
        slider.setLookAndFeel(&lnf);
        addAndMakeVisible(slider);
    };

    configureKnob(mix, "%");
    configureKnob(size, "x");
    configureKnob(decay, "s");
    configureKnob(predelay, "ms");
    configureKnob(damp, "Hz");
    configureKnob(cut, "Hz");
    configureKnob(diff, "");
    configureKnob(mrate, "Hz");
    configureKnob(mdepth, "");
    configureKnob(width, "");
    configureKnob(out, " dB");

    configureLinear(duckAmt, false);
    configureLinear(duckAtk, false);
    configureLinear(duckRel, false);

    gate.setButtonText("Gate");
    freeze.setButtonText("Freeze");
    gate.setLookAndFeel(&lnf);
    freeze.setLookAndFeel(&lnf);
    addAndMakeVisible(gate);
    addAndMakeVisible(freeze);

    algo.addItemList({"Plate", "Hall", "Room"}, 1);
    algo.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(algo);

    duckLabel.setText("Ducking", juce::dontSendNotification);
    duckLabel.setJustificationType(juce::Justification::centredLeft);
    duckLabel.setColour(juce::Label::textColourId, textPrimary());
    addAndMakeVisible(duckLabel);

    mixA   = std::make_unique<SA>(proc.apvts, audio::AuroraReverbAudioProcessor::IDs::mix, mix);
    sizeA  = std::make_unique<SA>(proc.apvts, audio::AuroraReverbAudioProcessor::IDs::size, size);
    decayA = std::make_unique<SA>(proc.apvts, audio::AuroraReverbAudioProcessor::IDs::decay, decay);
    preA   = std::make_unique<SA>(proc.apvts, audio::AuroraReverbAudioProcessor::IDs::predelay, predelay);
    dampA  = std::make_unique<SA>(proc.apvts, audio::AuroraReverbAudioProcessor::IDs::dampHF, damp);
    cutA   = std::make_unique<SA>(proc.apvts, audio::AuroraReverbAudioProcessor::IDs::cutLF, cut);
    diffA  = std::make_unique<SA>(proc.apvts, audio::AuroraReverbAudioProcessor::IDs::diffusion, diff);
    mrA    = std::make_unique<SA>(proc.apvts, audio::AuroraReverbAudioProcessor::IDs::modRate, mrate);
    mdA    = std::make_unique<SA>(proc.apvts, audio::AuroraReverbAudioProcessor::IDs::modDepth, mdepth);
    widthA = std::make_unique<SA>(proc.apvts, audio::AuroraReverbAudioProcessor::IDs::width, width);
    outA   = std::make_unique<SA>(proc.apvts, audio::AuroraReverbAudioProcessor::IDs::outTrim, out);

    duckAmtA = std::make_unique<SA>(proc.apvts, audio::AuroraReverbAudioProcessor::IDs::duckAmt, duckAmt);
    duckAtkA = std::make_unique<SA>(proc.apvts, audio::AuroraReverbAudioProcessor::IDs::duckAtk, duckAtk);
    duckRelA = std::make_unique<SA>(proc.apvts, audio::AuroraReverbAudioProcessor::IDs::duckRel, duckRel);

    gateA   = std::make_unique<BA>(proc.apvts, audio::AuroraReverbAudioProcessor::IDs::gateOn, gate);
    freezeA = std::make_unique<BA>(proc.apvts, audio::AuroraReverbAudioProcessor::IDs::freeze, freeze);
    algoA   = std::make_unique<CA>(proc.apvts, audio::AuroraReverbAudioProcessor::IDs::algo, algo);

    mix.onValueChange   = [this]() { updateXYFromSliders(); };
    decay.onValueChange = [this]() { updateXYFromSliders(); };

    updateXYFromSliders();
}

AuroraReverbEditor::~AuroraReverbEditor()
{
    mix.onValueChange = nullptr;
    decay.onValueChange = nullptr;
    setLookAndFeel(nullptr);
}

void AuroraReverbEditor::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient bg(backgroundTop(), bounds.getTopLeft(),
                            backgroundBottom(), bounds.getBottomRight(), false);
    g.setGradientFill(bg);
    g.fillAll();

    auto header = bounds.removeFromTop(72.0f);
    g.setColour(textPrimary());
    g.setFont(juce::Font("Montserrat", 26.0f, juce::Font::bold));
    g.drawText("Aurora Reverb", header, juce::Justification::centredLeft);

    g.setFont(juce::Font(14.0f));
    g.setColour(textMuted());
    g.drawText("Lush diffusion + spectral ducking", header.removeFromBottom(28.0f), juce::Justification::centredLeft);

    g.setColour(trackColour());
    g.drawRoundedRectangle(bounds, 18.0f, 1.2f);
}

void AuroraReverbEditor::resized()
{
    auto bounds = getLocalBounds().reduced(24);
    bounds.removeFromTop(72); // header consumed in paint

    auto bottom = bounds.removeFromBottom(120);
    auto duckRow = bottom.removeFromTop(64);

    auto duckArea = duckRow.removeFromLeft(duckRow.getWidth() / 2).reduced(8);
    duckLabel.setBounds(duckArea.removeFromTop(24));
    auto duckSliders = duckArea.reduced(0, 10);
    auto third = duckSliders.getWidth() / 3;
    duckAmt.setBounds(withMargin(duckSliders.removeFromLeft(third), 6));
    duckAtk.setBounds(withMargin(duckSliders.removeFromLeft(third), 6));
    duckRel.setBounds(withMargin(duckSliders, 6));

    auto toggles = duckRow.reduced(12);
    gate.setBounds(toggles.removeFromLeft(120).removeFromTop(32));
    freeze.setBounds(toggles.removeFromLeft(120).removeFromTop(32));
    algo.setBounds(toggles.removeFromLeft(160).removeFromTop(32));

    auto centre = bounds.removeFromLeft(bounds.getWidth() / 2);
    auto xyArea = centre.removeFromTop(centre.getWidth());
    xy->setBounds(withMargin(xyArea, 8));

    auto meterArea = centre.reduced(8);
    scope->setBounds(meterArea.removeFromTop(110));
    meterArea.removeFromTop(12);
    gr->setBounds(meterArea.removeFromTop(110));

    auto knobArea = bounds.reduced(8);
    auto rowHeight = 150;
    auto row1 = knobArea.removeFromTop(rowHeight);
    auto row2 = knobArea.removeFromTop(rowHeight);

    const auto placeKnob = [](juce::Slider& slider, juce::Rectangle<int> area)
    {
        slider.setBounds(area.reduced(12));
    };

    auto segmentWidth = row1.getWidth() / 4;
    placeKnob(mix, row1.removeFromLeft(segmentWidth));
    placeKnob(size, row1.removeFromLeft(segmentWidth));
    placeKnob(decay, row1.removeFromLeft(segmentWidth));
    placeKnob(predelay, row1);

    segmentWidth = row2.getWidth() / 4;
    placeKnob(damp, row2.removeFromLeft(segmentWidth));
    placeKnob(cut, row2.removeFromLeft(segmentWidth));
    placeKnob(diff, row2.removeFromLeft(segmentWidth));
    placeKnob(width, row2);

    auto row3 = knobArea;
    segmentWidth = row3.getWidth() / 3;
    placeKnob(mrate, row3.removeFromLeft(segmentWidth));
    placeKnob(mdepth, row3.removeFromLeft(segmentWidth));
    placeKnob(out, row3);
}

void AuroraReverbEditor::updateXYFromSliders()
{
    if (xy == nullptr)
        return;

    const auto mixRange = mix.getRange();
    const auto decayRange = decay.getRange();
    const auto mixNorm = static_cast<float>((mix.getValue() - mixRange.getStart()) / mixRange.getLength());
    const auto decayNorm = static_cast<float>(1.0 - (decay.getValue() - decayRange.getStart()) / decayRange.getLength());
    xy->setPosition(juce::jlimit(0.0f, 1.0f, mixNorm), juce::jlimit(0.0f, 1.0f, decayNorm));
}

} // namespace ui
} // namespace cppmusic
