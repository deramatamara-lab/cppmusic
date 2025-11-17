#include "MixerView.h"
#include "../lookandfeel/DesignSystem.h"

namespace daw::ui::views
{

MixerView::MixerView(std::shared_ptr<daw::audio::engine::EngineContext> engineContext,
                     std::shared_ptr<daw::project::ProjectModel> projectModel)
    : engineContext(engineContext)
    , projectModel(projectModel)
{
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
    
    const auto stripWidth = 80;
    const auto spacing = Spacing::xsmall;
    
    auto x = spacing;
    for (auto& strip : strips)
    {
        strip->setBounds(x, spacing, stripWidth, getHeight() - spacing * 2);
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
    
    for (int i = 0; i < numTracks && i < static_cast<int>(tracks.size()); ++i)
    {
        auto* track = tracks[i];
        auto strip = std::make_unique<MixerStrip>(engineContext, track, i);
        addAndMakeVisible(*strip);
        strips.push_back(std::move(strip));
    }
}

} // namespace daw::ui::views

