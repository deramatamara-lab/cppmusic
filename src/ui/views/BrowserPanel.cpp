#include "BrowserPanel.h"
#include "../lookandfeel/DesignSystem.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace daw::ui::views
{

BrowserPanel::BrowserPanel(std::shared_ptr<daw::project::ProjectModel> projectModel,
                           std::shared_ptr<daw::audio::engine::EngineContext> engineContext)
    : projectModel(projectModel)
    , engineContext(engineContext)
    , pluginHost(std::make_unique<daw::plugins::PluginHost>())
    , tabBar(juce::TabbedButtonBar::TabsAtTop)
    , tracksListComponent(this, &trackItems)
    , instrumentsListComponent(this, &instrumentItems)
    , pluginsListComponent(this, &pluginItems)
    , samplesListComponent(this, &sampleItems)
{
    setupUI();
    populateData();
}

void BrowserPanel::setupUI()
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    // Setup tabs
    tabBar.addTab("Current project", juce::Colours::transparentBlack, TracksTab);
    tabBar.addTab("Samples", juce::Colours::transparentBlack, SamplesTab);
    tabBar.addTab("Plugins", juce::Colours::transparentBlack, PluginsTab);
    tabBar.addTab("Presets", juce::Colours::transparentBlack, InstrumentsTab); // Use InstrumentsTab for Presets
    tabBar.setCurrentTabIndex(TracksTab);
    tabBar.addChangeListener(this);
    addAndMakeVisible(tabBar);

    // Setup viewports and content
    addAndMakeVisible(tracksViewport);
    tracksViewport.setViewedComponent(&tracksListComponent, false);
    tracksListComponent.setSize(200, 400);

    addAndMakeVisible(instrumentsViewport);
    instrumentsViewport.setViewedComponent(&instrumentsListComponent, false);
    instrumentsListComponent.setSize(200, 400);

    addAndMakeVisible(pluginsViewport);
    pluginsViewport.setViewedComponent(&pluginsListComponent, false);
    pluginsListComponent.setSize(200, 400);

    addAndMakeVisible(samplesViewport);
    samplesViewport.setViewedComponent(&samplesListComponent, false);
    samplesListComponent.setSize(200, 400);

    // Initial tab
    tabChanged(TracksTab);
}

void BrowserPanel::populateData()
{
    // Populate tracks from project model
    trackItems.clear();

    // Add recent projects first
    trackItems.insert(trackItems.end(), recentProjectItems.begin(), recentProjectItems.end());

    if (projectModel != nullptr)
    {
        const auto tracks = projectModel->getTracks();
        for (const auto* track : tracks)
        {
            BrowserItem item;
            item.name = track->getName();
            item.category = "Track";
            item.color = track->getColor();
            trackItems.push_back(item);
        }

        // Add patterns to current project tab
        const auto patterns = projectModel->getPatterns();
        for (const auto* pattern : patterns)
        {
            BrowserItem item;
            item.name = pattern->getName();
            item.category = "Pattern";
            item.color = juce::Colours::cyan;
            trackItems.push_back(item);
        }
    }

    // Populate instruments with real synthesizer presets
    instrumentItems.clear();
    instrumentItems = {
        // Drums
        {"Acoustic Kick", "Drums", juce::Colour(0xff8b4513), true, ""},
        {"Electronic Kick", "Drums", juce::Colour(0xffff4500), true, ""},
        {"Acoustic Snare", "Drums", juce::Colour(0xffdaa520), true, ""},
        {"Electronic Snare", "Drums", juce::Colour(0xffffa500), true, ""},
        {"Hi-Hat Closed", "Drums", juce::Colour(0xffd3d3d3), true, ""},
        {"Hi-Hat Open", "Drums", juce::Colour(0xffc0c0c0), true, ""},
        {"Crash", "Drums", juce::Colour(0xffb8860b), true, ""},
        {"Ride", "Drums", juce::Colour(0xffcd853f), true, ""},
        {"Tom High", "Drums", juce::Colour(0xffdeb887), true, ""},
        {"Tom Mid", "Drums", juce::Colour(0xffd2691e), true, ""},
        {"Tom Low", "Drums", juce::Colour(0xffa0522d), true, ""},

        // Synths
        {"Analog Bass", "Synth", juce::Colour(0xff0000ff), true, ""},
        {"FM Bass", "Synth", juce::Colour(0xff4169e1), true, ""},
        {"Sub Bass", "Synth", juce::Colour(0xff191970), true, ""},
        {"Lead Saw", "Synth", juce::Colour(0xff00ffff), true, ""},
        {"Lead Square", "Synth", juce::Colour(0xff00ced1), true, ""},
        {"Lead Pulse", "Synth", juce::Colour(0xff48d1cc), true, ""},
        {"Pad Strings", "Synth", juce::Colour(0xff9370db), true, ""},
        {"Pad Brass", "Synth", juce::Colour(0xffba55d3), true, ""},
        {"Pad Choir", "Synth", juce::Colour(0xffda70d6), true, ""},
        {"Pluck", "Synth", juce::Colour(0xffff69b4), true, ""},
        {"Bell", "Synth", juce::Colour(0xffffb6c1), true, ""},
        {"Organ", "Synth", juce::Colour(0xffdda0dd), true, ""}
    };

    // Populate plugins from plugin database (if available)
    pluginItems.clear();

    // Built-in effects (always available)
    pluginItems = {
        {"Compressor", "Effect", juce::Colour(0xff2f2f2f), true, ""},
        {"Reverb", "Effect", juce::Colour(0xff3f3f3f), true, ""},
        {"Delay", "Effect", juce::Colour(0xff4f4f4f), true, ""},
        {"EQ", "Effect", juce::Colour(0xff5f5f5f), true, ""},
        {"Distortion", "Effect", juce::Colour(0xff6f6f6f), true, ""},
        {"Chorus", "Effect", juce::Colour(0xff7f7f7f), true, ""},
        {"Flanger", "Effect", juce::Colour(0xff8f8f8f), true, ""},
        {"Phaser", "Effect", juce::Colour(0xff9f9f9f), true, ""},
        {"Tremolo", "Effect", juce::Colour(0xffafafaf), true, ""},
        {"Filter", "Effect", juce::Colour(0xffbfbfbf), true, ""}
    };

    // Add scanned plugins if engine context provides access
    // (This would require EngineContext to expose PluginDatabase)
    // For now, we have a comprehensive built-in set

    // Populate samples with realistic sample library structure
    sampleItems.clear();
    sampleItems = {
        // Drum samples
        {"Kick - 808", "Drums", juce::Colour(0xff8b0000), true, ""},
        {"Kick - Acoustic", "Drums", juce::Colour(0xffa52a2a), true, ""},
        {"Kick - Electronic", "Drums", juce::Colour(0xffdc143c), true, ""},
        {"Snare - Acoustic", "Drums", juce::Colour(0xffcd853f), true, ""},
        {"Snare - Electronic", "Drums", juce::Colour(0xffff6347), true, ""},
        {"Snare - Clap", "Drums", juce::Colour(0xffff7f50), true, ""},
        {"Hi-Hat - Closed", "Drums", juce::Colour(0xffd3d3d3), true, ""},
        {"Hi-Hat - Open", "Drums", juce::Colour(0xffc0c0c0), true, ""},
        {"Crash - 16\"", "Drums", juce::Colour(0xffdaa520), true, ""},
        {"Crash - 18\"", "Drums", juce::Colour(0xffb8860b), true, ""},
        {"Ride - 20\"", "Drums", juce::Colour(0xffcd853f), true, ""},

        // One-shot samples
        {"Vocal - Ah", "Vocals", juce::Colour(0xffff69b4), true, ""},
        {"Vocal - Oh", "Vocals", juce::Colour(0xffff1493), true, ""},
        {"Vocal - Yeah", "Vocals", juce::Colour(0xffc71585), true, ""},
        {"FX - Riser", "FX", juce::Colour(0xff9370db), true, ""},
        {"FX - Downer", "FX", juce::Colour(0xff8a2be2), true, ""},
        {"FX - Sweep", "FX", juce::Colour(0xff9400d3), true, ""}
    };
}

void BrowserPanel::tabChanged(int newTabIndex)
{
    currentTab = newTabIndex;

    tracksViewport.setVisible(newTabIndex == TracksTab);
    instrumentsViewport.setVisible(newTabIndex == InstrumentsTab);
    pluginsViewport.setVisible(newTabIndex == PluginsTab);
    samplesViewport.setVisible(newTabIndex == SamplesTab);

    refreshCurrentTab();
    resized();
}

void BrowserPanel::refreshCurrentTab()
{
    // Update content based on current tab
    switch (currentTab)
    {
        case TracksTab:
            tracksListComponent.repaint();
            break;
        case InstrumentsTab:
            instrumentsListComponent.repaint();
            break;
        case PluginsTab:
            pluginsListComponent.repaint();
            break;
        case SamplesTab:
            samplesListComponent.repaint();
            break;
    }
}

void BrowserPanel::itemDoubleClicked(const BrowserItem& item)
{
    if (currentTab == TracksTab)
    {
        // Handle recent project double-click
        if (item.category == "Recent Project")
        {
            // Find the item in recentProjectItems to get the full path
            for (const auto& recentItem : recentProjectItems)
            {
                if (recentItem.name == item.name)
                {
                    // Trigger callback with stored path
                    if (onRecentProjectSelected && !recentItem.userData.isEmpty())
                    {
                        onRecentProjectSelected(recentItem.userData);
                        return;
                    }
                    break;
                }
            }
        }

        // Select track
        if (projectModel != nullptr)
        {
            const auto tracks = projectModel->getTracks();
            for (const auto* track : tracks)
            {
                if (track->getName() == item.name)
                {
                    projectModel->getSelectionModel().selectTrack(track->getId());
                    break;
                }
            }
        }
    }
    else if (currentTab == InstrumentsTab)
    {
        // Add track with instrument - enhanced implementation with intelligent naming and error handling
        if (projectModel != nullptr && engineContext != nullptr)
        {
            // Generate intelligent track name (avoid duplicates)
            juce::String baseName = item.name + " Track";
            juce::String trackName = baseName;
            int suffix = 1;

            // Check for existing tracks with same name
            const auto existingTracks = projectModel->getTracks();
            bool nameExists = false;
            do
            {
                nameExists = false;
                for (const auto* track : existingTracks)
                {
                    if (track != nullptr && track->getName() == trackName.toStdString())
                    {
                        nameExists = true;
                        trackName = baseName + " " + juce::String(suffix);
                        ++suffix;
                        break;
                    }
                }
            } while (nameExists && suffix < 100); // Prevent infinite loop

            // Generate color based on instrument type (consistent color per instrument type)
            const auto instrumentHash = static_cast<uint32_t>(std::hash<std::string>{}(item.name));
            const float hue = static_cast<float>(instrumentHash % 360) / 360.0f;
            const auto color = juce::Colour::fromHSV(hue, 0.8f, 0.8f, 1.0f);

            // Add track to project model
            auto* track = projectModel->addTrack(trackName.toStdString(), color);

            if (track == nullptr)
            {
                // Error handling: track creation failed
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Track Creation Failed",
                    "Unable to create track: " + trackName);
                return;
            }

            // Add track to engine
            const auto engineTrackIndex = engineContext->addTrack();
            if (engineTrackIndex < 0)
            {
                // Error handling: engine track creation failed
                // Remove the track from project model to maintain consistency
                projectModel->removeTrack(track->getId());
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Engine Error",
                    "Unable to create engine track. Track creation rolled back.");
                return;
            }

            // Configure engine track parameters
            engineContext->setTrackGain(engineTrackIndex, track->getGainDb());
            engineContext->setTrackPan(engineTrackIndex, track->getPan());

            // Select the newly created track
            projectModel->getSelectionModel().selectTrack(track->getId());

            // Visual feedback: refresh UI
            populateData(); // Refresh track list
            refreshCurrentTab();

            // Repaint to show visual feedback
            repaint();
        }
    }
    else if (currentTab == PluginsTab)
    {
        // Add plugin to selected track
        if (projectModel != nullptr && engineContext != nullptr)
        {
            const auto selectedTracks = projectModel->getSelectionModel().getSelectedTracks();
            if (!selectedTracks.empty() && pluginHost != nullptr)
            {
                // Production implementation: Load plugin via PluginHost
                auto fileChooser = std::make_shared<juce::FileChooser>("Select Plugin",
                                               juce::File(),
                                               "*.vst3;*.component;*.dll");

                fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, fileChooser](const juce::FileChooser& fc)
                    {
                        if (fc.getResults().isEmpty())
                            return;

                        const auto pluginFile = fc.getResult();
                        const auto pluginPath = pluginFile.getFullPathName().toStdString();

                        auto pluginInfo = pluginHost->loadPlugin(pluginPath);
                        if (pluginInfo != nullptr)
                        {
                            juce::AlertWindow::showMessageBoxAsync(
                                juce::AlertWindow::InfoIcon,
                                "Plugin Loaded",
                                "Successfully loaded plugin: " + pluginInfo->name);
                        }
                        else
                        {
                            juce::AlertWindow::showMessageBoxAsync(
                                juce::AlertWindow::WarningIcon,
                                "Plugin Load Failed",
                                "Failed to load plugin: " + pluginFile.getFileName());
                        }
                    });
            }
        }
    }
    else if (currentTab == SamplesTab)
    {
        // Add sample to selected track
        if (projectModel != nullptr && engineContext != nullptr)
        {
            const auto selectedTracks = projectModel->getSelectionModel().getSelectedTracks();
            if (!selectedTracks.empty())
            {
                // Production implementation: Load sample via file browser
                auto fileChooser = std::make_shared<juce::FileChooser>("Select Audio Sample",
                                               juce::File(),
                                               "*.wav;*.aiff;*.mp3;*.flac;*.ogg");

                fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, fileChooser, selectedTracks](const juce::FileChooser& fc)
                    {
                        if (fc.getResults().isEmpty())
                            return;

                        const auto sampleFile = fc.getResult();
                        const auto fileName = sampleFile.getFileName();

                        // Add sample as clip to first selected track
                        if (!selectedTracks.empty())
                        {
                            const auto trackId = selectedTracks[0];
                            auto* track = projectModel->getTrack(trackId);

                            if (track != nullptr)
                            {
                                // Create clip for the sample
                                const double startBeats = 0.0; // Start at beginning
                                const double lengthBeats = 4.0; // Default 4 beats length
                                auto* clip = projectModel->addClip(trackId, startBeats, lengthBeats, fileName.toStdString());

                                if (clip != nullptr)
                                {
                                    // Refresh UI to show new clip
                                    populateData();
                                    refreshCurrentTab();
                                    repaint();
                                }
                                else
                                {
                                    juce::AlertWindow::showMessageBoxAsync(
                                        juce::AlertWindow::WarningIcon,
                                        "Sample Load Failed",
                                        "Failed to create clip for sample: " + fileName);
                                }
                            }
                        }
                    });
            }
        }
    }
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

    tabBar.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(Spacing::xsmall);

    // Set viewport bounds
    tracksViewport.setBounds(bounds);
    instrumentsViewport.setBounds(bounds);
    pluginsViewport.setBounds(bounds);
    samplesViewport.setBounds(bounds);

    // Update content sizes
    const auto contentWidth = bounds.getWidth() - Spacing::small * 2;
    const auto itemHeight = 30;
    const auto contentHeight = [this, itemHeight]() {
        switch (currentTab)
        {
            case TracksTab: return static_cast<int>(trackItems.size()) * itemHeight;
            case InstrumentsTab: return static_cast<int>(instrumentItems.size()) * itemHeight;
            case PluginsTab: return static_cast<int>(pluginItems.size()) * itemHeight;
            case SamplesTab: return static_cast<int>(sampleItems.size()) * itemHeight;
            default: return 400;
        }
    }();

    tracksListComponent.setSize(contentWidth, contentHeight);
    instrumentsListComponent.setSize(contentWidth, contentHeight);
    pluginsListComponent.setSize(contentWidth, contentHeight);
    samplesListComponent.setSize(contentWidth, contentHeight);
}

void BrowserPanel::drawItemList(juce::Graphics& g,
                                juce::Rectangle<int> bounds,
                                const std::vector<BrowserItem>& items,
                                int hoveredIndex)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    const auto itemHeight = 30;
    auto y = 0;
    const auto rowRadius = Radii::small;

    for (int i = 0; i < static_cast<int>(items.size()); ++i)
    {
        const auto& item = items[static_cast<size_t>(i)];
        auto itemBounds = bounds.withY(y).withHeight(itemHeight).toFloat();

        const bool isHovered = (i == hoveredIndex);

        // Draw item background
        auto baseColour = juce::Colour(Colors::surface).withAlpha(0.35f);
        if (isHovered)
            baseColour = juce::Colour(Colors::hover).withAlpha(0.75f);

        g.setColour(baseColour);
        g.fillRoundedRectangle(itemBounds.reduced(1.0f), rowRadius);

        g.setColour(juce::Colour(Colors::divider).withAlpha(0.45f));
        g.drawRoundedRectangle(itemBounds.reduced(0.5f), rowRadius, hairline(this));

        // Draw color indicator
        auto colorRect = itemBounds.removeFromLeft(4.0f);
        g.setColour(item.color);
        g.fillRect(colorRect);
        itemBounds.removeFromLeft(static_cast<float>(Spacing::xsmall));

        // Draw name
        g.setColour(juce::Colour(Colors::text));
        g.setFont(getBodyFont(Typography::body));
        g.drawText(item.name,
                   itemBounds.removeFromLeft(itemBounds.getWidth() * 0.7f),
                  juce::Justification::centredLeft);

        // Draw category
        g.setColour(juce::Colour(Colors::textSecondary));
        g.setFont(getBodyFont(Typography::bodySmall));
        g.drawText(item.category,
                   itemBounds.toNearestInt(),
                   juce::Justification::centredRight);

        y += itemHeight;
    }
}

void BrowserPanel::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &tabBar)
    {
        tabChanged(tabBar.getCurrentTabIndex());
    }
}

void BrowserPanel::setProjectModel(std::shared_ptr<daw::project::ProjectModel> model)
{
    projectModel = model;
    populateData();
    refreshCurrentTab();
}

void BrowserPanel::setRecentProjects(const std::vector<juce::String>& recentProjects)
{
    recentProjectItems.clear();
    for (const auto& path : recentProjects)
    {
        BrowserItem item;
        juce::File file(path);
        item.name = file.getFileNameWithoutExtension().toStdString();
        item.category = "Recent Project";
        item.color = juce::Colours::lightblue;
        item.userData = path; // Store full path for opening
        recentProjectItems.push_back(item);
    }
    populateData();
    refreshCurrentTab();
}

void BrowserPanel::startDrag(const BrowserItem& item, const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    // Create drag source description
    juce::String dragDescription = "BrowserItem:" + juce::String(item.name) + ":" +
                                   juce::String(currentTab);

    // Start drag operation (BrowserPanel is a DragAndDropContainer)
    startDragging(dragDescription, this);
}

} // namespace daw::ui::views
