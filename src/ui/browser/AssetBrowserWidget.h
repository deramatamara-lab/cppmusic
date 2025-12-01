/**
 * @file AssetBrowserWidget.h
 * @brief Asset browser with smart tagging header
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace daw::ui::browser {

/**
 * @brief Asset browser with intelligent tagging and search
 *
 * Features:
 * - Smart tag classification
 * - Similarity search
 * - Drag and drop support
 * - Multiple view modes
 */
class AssetBrowserWidget : public juce::Component {
public:
    AssetBrowserWidget();
    ~AssetBrowserWidget() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Search
    void setSearchQuery(const juce::String& query);
    [[nodiscard]] juce::String getSearchQuery() const;

    // View mode (0=Grid, 1=List, 2=Details)
    void setViewMode(int mode);
    [[nodiscard]] int getViewMode() const;

    // Actions
    void refresh();
    [[nodiscard]] int getAssetCount() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AssetBrowserWidget)
};

}  // namespace daw::ui::browser
