#include "StatusStrip.h"
#include "../lookandfeel/DesignSystem.h"
#include <juce_graphics/juce_graphics.h>

namespace daw::ui::components
{

StatusStrip::StatusStrip(std::shared_ptr<daw::audio::engine::EngineContext> ec)
    : engineContext(std::move(ec))
{
    setOpaque(true);
    startTimer(kUpdateIntervalMs);
    updateMetrics();
}

void StatusStrip::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    // Background
    g.fillAll(toColour(Colors::surface));

    // Top border
    g.setColour(toColour(Colors::divider));
    g.drawLine(0.0f, 0.0f, static_cast<float>(getWidth()), 0.0f, 1.0f);

    const auto bounds = getLocalBounds().reduced(Spacing::small, 0);
    const auto font = getBodyFont(Typography::bodySmall);
    g.setFont(font);

    // Layout: [CPU] [XRuns] [RAM] | [Project Name] | [Sample Rate]
    const int separatorWidth = 1;
    const int padding = Spacing::small;
    int x = bounds.getX();

    // CPU
    {
        const auto cpuText = juce::String(cpuLoadPercent, 1) + "%";
        const auto cpuColor = getCpuColor(cpuLoadPercent);
        const auto textWidth = font.getStringWidth("CPU: " + cpuText);
        const auto cpuBounds = juce::Rectangle<int>(x, bounds.getY(), textWidth + padding * 2, bounds.getHeight());

        g.setColour(cpuColor);
        g.drawText("CPU: " + cpuText, cpuBounds, juce::Justification::centredLeft);
        x = cpuBounds.getRight() + padding;
    }

    // Separator
    g.setColour(toColour(Colors::divider));
    g.fillRect(x, bounds.getY() + 2, separatorWidth, bounds.getHeight() - 4);
    x += separatorWidth + padding;

    // XRuns
    {
        const auto xrunText = juce::String(xrunCount);
        const auto xrunColor = xrunCount > 0 ? toColour(Colors::warning) : toColour(Colors::textSecondary);
        const auto textWidth = font.getStringWidth("XRuns: " + xrunText);
        const auto xrunBounds = juce::Rectangle<int>(x, bounds.getY(), textWidth + padding * 2, bounds.getHeight());

        g.setColour(xrunColor);
        g.drawText("XRuns: " + xrunText, xrunBounds, juce::Justification::centredLeft);
        x = xrunBounds.getRight() + padding;
    }

    // Separator
    g.setColour(toColour(Colors::divider));
    g.fillRect(x, bounds.getY() + 2, separatorWidth, bounds.getHeight() - 4);
    x += separatorWidth + padding;

    // RAM
    {
        const auto ramText = juce::String(ramUsageMB, 1) + " MB";
        const auto textWidth = font.getStringWidth("RAM: " + ramText);
        const auto ramBounds = juce::Rectangle<int>(x, bounds.getY(), textWidth + padding * 2, bounds.getHeight());

        g.setColour(toColour(Colors::textSecondary));
        g.drawText("RAM: " + ramText, ramBounds, juce::Justification::centredLeft);
        x = ramBounds.getRight() + padding;
    }

    // Separator (before project name)
    const int centerX = bounds.getCentreX();
    g.setColour(toColour(Colors::divider));
    g.fillRect(centerX, bounds.getY() + 2, separatorWidth, bounds.getHeight() - 4);

    // Project Name (centered)
    {
        const auto projectBounds = juce::Rectangle<int>(centerX + separatorWidth + padding, bounds.getY(),
                                                          bounds.getRight() - (centerX + separatorWidth + padding) - padding, bounds.getHeight());
        g.setColour(toColour(Colors::text));
        g.drawText(projectName.isEmpty() ? "Untitled Project" : projectName, projectBounds, juce::Justification::centred);
    }

    // Sample Rate (right-aligned)
    {
        const auto srText = juce::String(sampleRate / 1000.0, 1) + " kHz";
        const auto textWidth = font.getStringWidth(srText);
        const auto srBounds = juce::Rectangle<int>(bounds.getRight() - textWidth - padding, bounds.getY(),
                                                    textWidth + padding, bounds.getHeight());

        g.setColour(toColour(Colors::textSecondary));
        g.drawText(srText, srBounds, juce::Justification::centredRight);
    }
}

void StatusStrip::resized()
{
    // No child components to resize
}

void StatusStrip::timerCallback()
{
    updateMetrics();
}

void StatusStrip::updateMetrics()
{
    if (engineContext == nullptr)
        return;

    cpuLoadPercent = engineContext->getCpuLoad();
    xrunCount = engineContext->getXrunCount();
    ramUsageMB = engineContext->getRamUsageMB();
    sampleRate = engineContext->getSampleRate();

    repaint();
}

juce::Colour StatusStrip::getCpuColor(float percent) const
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    if (percent < 60.0f)
        return toColour(Colors::success);
    else if (percent < 80.0f)
        return toColour(Colors::warning);
    else
        return toColour(Colors::danger);
}

} // namespace daw::ui::components

