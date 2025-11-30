#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/DesignSystem.h"
#include "../../project/ProjectModel.h"
#include "../../audio/engine/EngineContext.h"
#include "../../plugins/PluginHost.h"
#include <memory>
#include <vector>
#include <string>

namespace daw::ui::views
{

/**
 * @brief Browser/library panel
 *
 * Displays tracks, instruments, plugins, and samples.
 * Supports selection and double-click actions.
 * Follows DAW_DEV_RULES: uses design system, responsive layout.
 */
class BrowserPanel : public juce::Component, public juce::ChangeListener, public juce::DragAndDropContainer
{
public:
    BrowserPanel(std::shared_ptr<daw::project::ProjectModel> projectModel,
                 std::shared_ptr<daw::audio::engine::EngineContext> engineContext);
    ~BrowserPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setProjectModel(std::shared_ptr<daw::project::ProjectModel> model);
    void setRecentProjects(const std::vector<juce::String>& recentProjects);

    // Callback for recent project selection
    std::function<void(const juce::String&)> onRecentProjectSelected;

private:
    std::shared_ptr<daw::project::ProjectModel> projectModel;
    std::shared_ptr<daw::audio::engine::EngineContext> engineContext;
    std::unique_ptr<daw::plugins::PluginHost> pluginHost;

    // Tabs
    juce::TabbedButtonBar tabBar;
    enum TabIndex { TracksTab = 0, InstrumentsTab, PluginsTab, SamplesTab };
    int currentTab{0};

    // Content views (one per tab)
    juce::Viewport tracksViewport;
    juce::Viewport instrumentsViewport;
    juce::Viewport pluginsViewport;
    juce::Viewport samplesViewport;

    // Data models (in-memory for now)
    struct BrowserItem
    {
        std::string name;
        std::string category;
        juce::Colour color;
        bool isSelectable{true};
        juce::String userData; // For storing paths, IDs, etc.
    };
    std::vector<BrowserItem> trackItems;
    std::vector<BrowserItem> instrumentItems;
    std::vector<BrowserItem> pluginItems;
    std::vector<BrowserItem> sampleItems;
    std::vector<BrowserItem> recentProjectItems;

    void setupUI();
    void tabChanged(int newTabIndex);
    void populateData();
    void refreshCurrentTab();
    void itemDoubleClicked(const BrowserItem& item);
    void drawItemList(juce::Graphics& g,
                      juce::Rectangle<int> bounds,
                      const std::vector<BrowserItem>& items,
                      int hoveredIndex);
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // Content component paint helpers
    class ItemListComponent : public juce::Component, public juce::DragAndDropTarget
    {
    public:
        ItemListComponent(BrowserPanel* parent, const std::vector<BrowserItem>* items)
            : parentPanel(parent), itemsPtr(items) {}

        void paint(juce::Graphics& g) override
        {
            if (parentPanel != nullptr && itemsPtr != nullptr)
            {
                parentPanel->drawItemList(g, getLocalBounds(), *itemsPtr, hoveredIndex);
            }
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            if (e.getNumberOfClicks() == 2 && itemsPtr != nullptr && parentPanel != nullptr)
            {
                const auto itemHeight = 30;
                const auto index = e.y / itemHeight;
                if (index >= 0 && index < static_cast<int>(itemsPtr->size()))
                {
                    parentPanel->itemDoubleClicked((*itemsPtr)[static_cast<size_t>(index)]);
                }
            }
            else if (itemsPtr != nullptr && parentPanel != nullptr)
            {
                // Start drag
                const auto itemHeight = 30;
                const auto index = e.y / itemHeight;
                if (index >= 0 && index < static_cast<int>(itemsPtr->size()))
                {
                    const auto& item = (*itemsPtr)[static_cast<size_t>(index)];
                    parentPanel->startDrag(item, e);
                }
            }
        }

        void mouseMove(const juce::MouseEvent& e) override
        {
            if (itemsPtr == nullptr)
                return;

            const auto itemHeight = 30;
            const auto index = e.y / itemHeight;
            if (index != hoveredIndex)
            {
                hoveredIndex = index;
                repaint();
            }
        }

        void mouseExit(const juce::MouseEvent&) override
        {
            if (hoveredIndex != -1)
            {
                hoveredIndex = -1;
                repaint();
            }
        }

        bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override
        {
            juce::ignoreUnused(dragSourceDetails);
            return false; // This component is a drag source, not a drop target
        }

        void itemDropped(const SourceDetails& dragSourceDetails) override
        {
            juce::ignoreUnused(dragSourceDetails);
        }

    private:
        BrowserPanel* parentPanel;
        const std::vector<BrowserItem>* itemsPtr;
        int hoveredIndex { -1 };
    };

    void startDrag(const BrowserItem& item, const juce::MouseEvent& e);

    ItemListComponent tracksListComponent;
    ItemListComponent instrumentsListComponent;
    ItemListComponent pluginsListComponent;
    ItemListComponent samplesListComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserPanel)
};

} // namespace daw::ui::views
