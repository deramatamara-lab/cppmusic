/**
 * @file AssetBrowserWidget.cpp
 * @brief Asset browser with smart tagging (stub)
 */

#include "AssetBrowserWidget.h"

namespace daw::ui::browser {

class AssetBrowserWidget::Impl {
public:
    struct Asset {
        juce::String id;
        juce::String name;
        juce::String path;
        juce::String type;  // sample, preset, midi, project
        std::vector<juce::String> tags;
        float similarity{0.0f};  // For search results
    };
    
    std::vector<Asset> assets;
    std::vector<Asset> searchResults;
    juce::String searchQuery;
    juce::String selectedAssetId;
    
    // View modes
    enum class ViewMode { Grid, List, Details };
    ViewMode viewMode{ViewMode::List};
    
    // Filters
    std::vector<juce::String> activeTypeFilters;
    std::vector<juce::String> activeTagFilters;
    
    // Sort
    enum class SortBy { Name, Date, Type, Relevance };
    SortBy sortBy{SortBy::Name};
    bool sortAscending{true};
};

AssetBrowserWidget::AssetBrowserWidget()
    : impl_(std::make_unique<Impl>()) {
}

AssetBrowserWidget::~AssetBrowserWidget() = default;

void AssetBrowserWidget::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();
    
    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    // Header
    auto header = bounds.removeFromTop(40);
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(header);
    
    g.setColour(juce::Colours::white);
    g.drawText("Asset Browser", header, juce::Justification::centredLeft);
    
    // Search bar area
    auto searchArea = bounds.removeFromTop(35);
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(searchArea);
    g.setColour(juce::Colour(0xff404040));
    g.fillRoundedRectangle(searchArea.reduced(5).toFloat(), 5.0f);
    
    if (impl_->searchQuery.isEmpty()) {
        g.setColour(juce::Colour(0xff808080));
        g.drawText("Search assets...", searchArea.reduced(10), 
                   juce::Justification::centredLeft);
    } else {
        g.setColour(juce::Colours::white);
        g.drawText(impl_->searchQuery, searchArea.reduced(10),
                   juce::Justification::centredLeft);
    }
    
    // Filter sidebar
    auto sidebar = bounds.removeFromLeft(150);
    g.setColour(juce::Colour(0xff222222));
    g.fillRect(sidebar);
    
    g.setColour(juce::Colour(0xff808080));
    g.drawText("Filters", sidebar.removeFromTop(30), juce::Justification::centred);
    g.drawText("Type:", sidebar.removeFromTop(25), juce::Justification::centredLeft);
    g.drawText("  Samples", sidebar.removeFromTop(20), juce::Justification::centredLeft);
    g.drawText("  Presets", sidebar.removeFromTop(20), juce::Justification::centredLeft);
    g.drawText("  MIDI", sidebar.removeFromTop(20), juce::Justification::centredLeft);
    
    g.drawText("Tags:", sidebar.removeFromTop(25), juce::Justification::centredLeft);
    g.drawText("  (smart tags)", sidebar.removeFromTop(20), juce::Justification::centredLeft);
    
    // Asset list
    const auto& displayList = impl_->searchQuery.isEmpty() ? 
                              impl_->assets : impl_->searchResults;
    
    if (displayList.empty()) {
        g.setColour(juce::Colour(0xff808080));
        g.drawText("No assets\nDrag files here or import from folder",
                   bounds, juce::Justification::centred);
    } else {
        int y = 0;
        for (const auto& asset : displayList) {
            auto row = bounds.removeFromTop(30);
            
            bool selected = asset.id == impl_->selectedAssetId;
            if (selected) {
                g.setColour(juce::Colour(0xff4080ff).withAlpha(0.3f));
                g.fillRect(row);
            }
            
            g.setColour(juce::Colours::white);
            g.drawText(asset.name, row.reduced(5, 0), juce::Justification::centredLeft);
            
            g.setColour(juce::Colour(0xff808080));
            g.drawText(asset.type, row.removeFromRight(80), 
                       juce::Justification::centredRight);
            
            y += 30;
            if (y > bounds.getHeight()) break;
        }
    }
}

void AssetBrowserWidget::resized() {
    // Layout handled in paint for simplicity
}

void AssetBrowserWidget::setSearchQuery(const juce::String& query) {
    impl_->searchQuery = query;
    // TODO: Perform search
    repaint();
}

juce::String AssetBrowserWidget::getSearchQuery() const {
    return impl_->searchQuery;
}

void AssetBrowserWidget::setViewMode(int mode) {
    impl_->viewMode = static_cast<Impl::ViewMode>(mode);
    repaint();
}

int AssetBrowserWidget::getViewMode() const {
    return static_cast<int>(impl_->viewMode);
}

void AssetBrowserWidget::refresh() {
    // TODO: Reload asset database
    repaint();
}

int AssetBrowserWidget::getAssetCount() const {
    return static_cast<int>(impl_->assets.size());
}

}  // namespace daw::ui::browser
