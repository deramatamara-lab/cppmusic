#include "SpectralAnalyzerWidget.h"
#include "../lookandfeel/DesignSystem.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <cmath>

namespace daw::ui::components {

SpectralAnalyzerWidget::SpectralAnalyzerWidget()
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    // Initialize with zeros
    std::fill(magnitudeSpectrum_.begin(), magnitudeSpectrum_.end(), 0.0f);
    std::fill(mfcc_.begin(), mfcc_.end(), 0.0f);
    std::fill(chroma_.begin(), chroma_.end(), 0.0f);

    for (auto& hist : spectrumHistory_) std::fill(hist.begin(), hist.end(), 0.0f);
    for (auto& hist : mfccHistory_) std::fill(hist.begin(), hist.end(), 0.0f);
    for (auto& hist : chromaHistory_) std::fill(hist.begin(), hist.end(), 0.0f);
    std::fill(centroidHistory_.begin(), centroidHistory_.end(), 0.0f);
    std::fill(tempoHistory_.begin(), tempoHistory_.end(), 120.0f);

    // Theme-aware colours from design system
    backgroundColor_ = juce::Colour(Colors::surface0).withAlpha(0.96f);
    gridColor_       = juce::Colour(Colors::divider).withAlpha(0.35f);
    spectrumColor_   = juce::Colour(Colors::accent);
    mfccColor_       = juce::Colour(Colors::primary);
    chromaColor_     = juce::Colour(Colors::secondary);

    // Start timer for 30fps updates
    startTimerHz(30);
}

void SpectralAnalyzerWidget::setSpectralData(const std::array<float, 4096>& magnitudeSpectrum,
                                           const std::array<float, 13>& mfcc,
                                           const std::array<float, 12>& chroma,
                                           float centroid, float spread, float flatness,
                                           float tempo, float onsetStrength)
{
    // Thread-safe copy (assuming single producer)
    magnitudeSpectrum_ = magnitudeSpectrum;
    mfcc_ = mfcc;
    chroma_ = chroma;
    spectralCentroid_ = centroid;
    spectralSpread_ = spread;
    spectralFlatness_ = flatness;
    tempoEstimate_ = tempo;
    onsetStrength_ = onsetStrength;

    // Update history
    spectrumHistory_[historyIndex_] = magnitudeSpectrum;
    mfccHistory_[historyIndex_] = mfcc;
    chromaHistory_[historyIndex_] = chroma;
    centroidHistory_[historyIndex_] = centroid;
    tempoHistory_[historyIndex_] = tempo;
    historyIndex_ = (historyIndex_ + 1) % HISTORY_SIZE;
}

void SpectralAnalyzerWidget::setEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        enabled_ = enabled;
        if (enabled_)
            startTimerHz(30);
        else
            stopTimer();
        repaint();
    }
}

void SpectralAnalyzerWidget::timerCallback()
{
    if (enabled_)
        repaint();
}

void SpectralAnalyzerWidget::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    // Panel chrome
    auto fullBounds = getLocalBounds().toFloat();
    drawGlassPanel(g, fullBounds, Radii::medium, true);

    if (!enabled_)
        return;

    auto bounds = fullBounds.reduced(Spacing::small).toNearestInt();

    // Draw background
    g.setColour(backgroundColor_);
    g.fillRoundedRectangle(bounds.toFloat(), Radii::small);

    // Draw grid
    g.setColour(gridColor_);
    const int vStep = juce::jmax(8, bounds.getWidth() / 8);
    const int hStep = juce::jmax(8, bounds.getHeight() / 4);
    for (int x = bounds.getX(); x < bounds.getRight(); x += vStep)
        g.drawVerticalLine(x, static_cast<float>(bounds.getY()), static_cast<float>(bounds.getBottom()));
    for (int y = bounds.getY(); y < bounds.getBottom(); y += hStep)
        g.drawHorizontalLine(y, static_cast<float>(bounds.getX()), static_cast<float>(bounds.getRight()));

    // Draw based on display mode
    switch (displayMode_)
    {
        case DisplayMode::Spectrum:
            drawSpectrum(g, bounds);
            break;
        case DisplayMode::MFCC:
            drawMFCC(g, bounds);
            break;
        case DisplayMode::Chroma:
            drawChroma(g, bounds);
            break;
        case DisplayMode::Combined:
            drawCombined(g, bounds);
            break;
    }

    // Draw feature bars at bottom
    drawFeatureBars(g, bounds.removeFromBottom(40));
}

void SpectralAnalyzerWidget::resized()
{
    auto bounds = getLocalBounds();
    spectrumArea_ = bounds.removeFromTop(bounds.getHeight() * 2 / 3);
    featuresArea_ = bounds;
}

void SpectralAnalyzerWidget::drawSpectrum(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(spectrumColor_);

    const int width = bounds.getWidth();
    const int height = bounds.getHeight();
    const int bins = magnitudeSpectrum_.size() / 4; // Display first quarter for visibility

    juce::Path path;
    path.startNewSubPath(0, height);

    for (int i = 0; i < bins; ++i)
    {
        float mag = magnitudeSpectrum_[i];
        float db = juce::Decibels::gainToDecibels(mag + 1e-12f);
        float y = juce::jmap(db, -60.0f, 0.0f, (float)height, 0.0f);
        float x = (float)i / (float)bins * (float)width;
        path.lineTo(x, y);
    }

    path.lineTo(width, height);
    path.closeSubPath();

    g.fillPath(path);

    // Draw centroid line
    g.setColour(juce::Colours::red);
    float centroidX = spectralCentroid_ / 8000.0f * width; // Assume 8kHz max
    g.drawLine(centroidX, 0, centroidX, height, 2.0f);
}

void SpectralAnalyzerWidget::drawMFCC(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(mfccColor_);

    const int width = bounds.getWidth();
    const int height = bounds.getHeight();
    const int numCoeffs = mfcc_.size();

    juce::Path path;
    path.startNewSubPath(0, height / 2);

    for (int i = 0; i < numCoeffs; ++i)
    {
        float coeff = mfcc_[i];
        float y = juce::jmap(coeff, -10.0f, 10.0f, (float)height, 0.0f);
        float x = (float)i / (float)(numCoeffs - 1) * (float)width;
        path.lineTo(x, y);
    }

    g.strokePath(path, juce::PathStrokeType(2.0f));
}

void SpectralAnalyzerWidget::drawChroma(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(chromaColor_);

    const int width = bounds.getWidth();
    const int height = bounds.getHeight();
    const int numBins = chroma_.size();

    const float barWidth = (float)width / (float)numBins;

    for (int i = 0; i < numBins; ++i)
    {
        float value = chroma_[i];
        float barHeight = value * height;
        float x = i * barWidth;
        g.fillRect(x, height - barHeight, barWidth - 1, barHeight);
    }
}

void SpectralAnalyzerWidget::drawCombined(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    auto topHalf = bounds.removeFromTop(bounds.getHeight() / 2);
    auto bottomLeft = bounds.removeFromLeft(bounds.getWidth() / 2);
    auto bottomRight = bounds;

    drawSpectrum(g, topHalf);
    drawMFCC(g, bottomLeft);
    drawChroma(g, bottomRight);
}

void SpectralAnalyzerWidget::drawFeatureBars(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    const int numFeatures = 4;
    const float barWidth = (float)bounds.getWidth() / (float)numFeatures;

    // Tempo
    g.setColour(juce::Colour(Colors::meterNormal));
    float tempoHeight = juce::jmap(tempoEstimate_, 60.0f, 200.0f, 0.0f, (float)bounds.getHeight());
    g.fillRect(0.0f, bounds.getHeight() - tempoHeight, barWidth - 1, tempoHeight);

    // Centroid
    g.setColour(juce::Colour(Colors::meterWarning));
    float centroidHeight = juce::jmap(spectralCentroid_, 0.0f, 8000.0f, 0.0f, (float)bounds.getHeight());
    g.fillRect(barWidth, bounds.getHeight() - centroidHeight, barWidth - 1, centroidHeight);

    // Spread
    g.setColour(juce::Colour(Colors::meterWarning).withAlpha(0.8f));
    float spreadHeight = juce::jmap(spectralSpread_, 0.0f, 4000.0f, 0.0f, (float)bounds.getHeight());
    g.fillRect(barWidth * 2, bounds.getHeight() - spreadHeight, barWidth - 1, spreadHeight);

    // Onset strength
    g.setColour(juce::Colour(Colors::meterDanger));
    float onsetHeight = onsetStrength_ * bounds.getHeight();
    g.fillRect(barWidth * 3, bounds.getHeight() - onsetHeight, barWidth - 1, onsetHeight);

    // Labels
    g.setColour(juce::Colour(Colors::text));
    g.setFont(getBodyFont(Typography::caption));
    const int labelH = 14;
    const int y = bounds.getBottom() - labelH;
    g.drawText("Tempo", 0, y, (int)barWidth, labelH, juce::Justification::centred);
    g.drawText("Centroid", (int)barWidth, y, (int)barWidth, labelH, juce::Justification::centred);
    g.drawText("Spread", (int)(barWidth * 2), y, (int)barWidth, labelH, juce::Justification::centred);
    g.drawText("Onset", (int)(barWidth * 3), y, (int)barWidth, labelH, juce::Justification::centred);
}

} // namespace daw::ui::components
