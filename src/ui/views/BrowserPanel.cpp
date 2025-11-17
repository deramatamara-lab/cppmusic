#include "BrowserPanel.h"
#include "../lookandfeel/DesignSystem.h"

namespace daw::ui::views
{

BrowserPanel::BrowserPanel(std::shared_ptr<daw::project::ProjectModel> projectModel,
                           std::shared_ptr<daw::audio::engine::EngineContext> engineContext)
    : projectModel(projectModel)
    , engineContext(engineContext)
    , addTrackButton("Add Track")
{
    setupUI();
}

void BrowserPanel::setupUI()
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    // Enhanced button with better typography
    addAndMakeVisible(addTrackButton);
    addTrackButton.onClick = [this] { addTrackButtonClicked(); };
    // Note: TextButton font is controlled via LookAndFeel
}

void BrowserPanel::paint(juce::Graphics& g)
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
}

void BrowserPanel::resized()
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    auto bounds = getLocalBounds().reduced(Spacing::small);
    
    addTrackButton.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(Spacing::small);
    
    // Tree view would go here
    // For now, just show the add track button
}

void BrowserPanel::addTrackButtonClicked()
{
    if (!projectModel || !engineContext)
        return;
    
    static int trackNum = 1;
    const auto color = juce::Colour::fromHSV(static_cast<float>(trackNum) * 0.1f, 0.8f, 0.8f, 1.0f);
    auto* track = projectModel->addTrack(("Track " + juce::String(trackNum++)).toStdString(), color);
    
    // Add track to engine
    const auto engineTrackIndex = engineContext->addTrack();
    if (engineTrackIndex >= 0 && track != nullptr)
    {
        // Sync initial parameters
        engineContext->setTrackGain(engineTrackIndex, track->getGainDb());
        engineContext->setTrackPan(engineTrackIndex, track->getPan());
        engineContext->setTrackMute(engineTrackIndex, track->isMuted());
        engineContext->setTrackSolo(engineTrackIndex, track->isSoloed());
    }
    
    // Notify listeners that model changed
    projectModel->getSelectionModel().clearAll();
}

} // namespace daw::ui::views

