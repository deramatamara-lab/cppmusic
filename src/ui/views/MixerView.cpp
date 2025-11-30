#include "MixerView.h"
#include "../lookandfeel/DesignSystem.h"

namespace daw::ui::views
{

MixerView::MixerView(std::shared_ptr<daw::audio::engine::EngineContext> engineContext,
                     std::shared_ptr<daw::project::ProjectModel> projectModel)
    : engineContext(engineContext)
    , projectModel(projectModel)
{
    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&stripsContainer, false);
    viewport.setScrollBarsShown(true, false); // Horizontal scroll only
    rebuildStrips();
}

MixerView::~MixerView()
{
}

void MixerView::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    // Glassmorphism background
    auto bounds = getLocalBounds().toFloat();
    drawGlassPanel(g, bounds, Radii::none, false);

    // Divider line at top
    g.setColour(juce::Colour(Colors::divider));
    g.drawLine(0.0f, 0.0f, bounds.getWidth(), 0.0f, 1.0f);
}

void MixerView::resized()
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    viewport.setBounds(getLocalBounds());

    const int stripWidth = static_cast<int>(Layout::kMixerStripWidth);
    const int spacing = static_cast<int>(Spacing::xsmall);
    const auto containerHeight = viewport.getHeight();

    // Calculate total width needed
    const auto totalWidth = static_cast<int>(strips.size()) * (stripWidth + spacing) + spacing;
    stripsContainer.setSize(totalWidth, containerHeight);

    auto x = spacing;
    for (auto& strip : strips)
    {
        strip->setBounds(x, spacing, stripWidth, containerHeight - spacing * 2);
        x += stripWidth + spacing;
    }
}

void MixerView::refreshStrips()
{
    rebuildStrips();
    resized();
}

void MixerView::rebuildStrips()
{
    strips.clear();

    if (projectModel == nullptr || engineContext == nullptr)
        return;

    const auto tracks = projectModel->getTracks();
    const auto numTracks = engineContext->getNumTracks();

    // Add strips for each track
    for (int i = 0; i < numTracks && i < static_cast<int>(tracks.size()); ++i)
    {
        auto* track = tracks[i];
        auto strip = std::make_unique<MixerStrip>(engineContext, track, i);
        stripsContainer.addAndMakeVisible(*strip);
        strips.push_back(std::move(strip));
    }

    // Production implementation: Add master strip with master gain control
    if (engineContext != nullptr)
    {
        // Create master strip (track is nullptr for master)
        auto masterStrip = std::make_unique<MixerStrip>(engineContext, nullptr, -1);
        masterStrip->setName("Master");
        stripsContainer.addAndMakeVisible(*masterStrip);
        strips.push_back(std::move(masterStrip));
    }

    resized(); // Update layout after rebuilding
}

} // namespace daw::ui::views
