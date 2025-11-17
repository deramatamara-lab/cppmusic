#include "MainView.h"
#include "../lookandfeel/DesignSystem.h"

namespace daw::ui::views
{

MainView::MainView(std::shared_ptr<daw::audio::engine::EngineContext> engineContext)
    : engineContext(engineContext)
    , projectModel(std::make_shared<daw::project::ProjectModel>())
    , transportBar(engineContext)
    , browserPanel(projectModel, engineContext)
    , arrangeView(projectModel, engineContext)
    , inspectorPanel(projectModel, engineContext)
    , mixerView(engineContext, projectModel)
{
    setupUI();
    
    // Initialize engine (will be done when window is shown)
    // engineContext->initialise() is called from App after window creation
}

void MainView::setupUI()
{
    addAndMakeVisible(transportBar);
    addAndMakeVisible(browserPanel);
    addAndMakeVisible(arrangeView);
    addAndMakeVisible(inspectorPanel);
    addAndMakeVisible(mixerView);
    addAndMakeVisible(flagshipPanel);
    addAndMakeVisible(patternSequencer);
    addAndMakeVisible(sessionLauncher);

    flagshipPanel.setTitle("AI Mastering Suite");
    constexpr double defaultTempoBpm = 128.0;
    constexpr bool defaultPlaying = true;
    patternSequencer.setTempo(defaultTempoBpm);
    patternSequencer.setIsPlaying(defaultPlaying);
    sessionLauncher.setTempo(defaultTempoBpm);
    sessionLauncher.setIsPlaying(defaultPlaying);
    sessionLauncher.setLooping(true);
    
    // Listen to project model changes
    if (projectModel != nullptr)
    {
        projectModel->addModelListener([this] { refreshViews(); });
    }
}

void MainView::refreshViews()
{
    arrangeView.refresh();
    inspectorPanel.refresh();
    mixerView.refreshStrips();
}

void MainView::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    g.fillAll(juce::Colour(Colors::background));
}

void MainView::resized()
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    auto bounds = getLocalBounds();
    
    // Top: TransportBar
    const auto transportHeight = 60;
    transportBar.setBounds(bounds.removeFromTop(transportHeight));
    
    // Bottom stack: mixer + session launcher
    const auto sessionHeight = 140;
    sessionLauncher.setBounds(bounds.removeFromBottom(sessionHeight).reduced(Spacing::medium));
    const auto mixerHeight = 200;
    mixerView.setBounds(bounds.removeFromBottom(mixerHeight));
    
    // Left column: browser + flagship
    const auto browserWidth = 220;
    auto leftColumn = bounds.removeFromLeft(browserWidth);
    browserPanel.setBounds(leftColumn.removeFromTop(static_cast<int>(bounds.getHeight() * 0.5f)));
    flagshipPanel.setBounds(leftColumn.reduced(Spacing::medium));
    
    // Right column: inspector + pattern sequencer
    const auto inspectorWidth = 260;
    auto rightColumn = bounds.removeFromRight(inspectorWidth);
    inspectorPanel.setBounds(rightColumn.removeFromTop(static_cast<int>(bounds.getHeight() * 0.55f)));
    patternSequencer.setBounds(rightColumn.reduced(Spacing::medium));
    
    // Center: ArrangeView
    arrangeView.setBounds(bounds.reduced(Spacing::medium));
}

} // namespace daw::ui::views

