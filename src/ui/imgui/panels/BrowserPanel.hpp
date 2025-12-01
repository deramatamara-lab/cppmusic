#pragma once

#include "../Theme.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace daw::ui::imgui
{

/**
 * @brief Browser item type
 */
enum class BrowserItemType
{
    Folder,
    AudioFile,
    MidiFile,
    Preset,
    Plugin,
    Project
};

/**
 * @brief Browser tree item
 */
struct BrowserItem
{
    std::string name;
    std::string path;
    BrowserItemType type{BrowserItemType::Folder};
    std::vector<std::unique_ptr<BrowserItem>> children;
    bool isExpanded{false};
    bool isLoading{false};
};

/**
 * @brief Filter chip state
 */
struct FilterChip
{
    std::string label;
    bool active{false};
};

/**
 * @brief Browser panel for browsing files, presets, and plugins
 * 
 * Features:
 * - Collapsible tree view
 * - Filter chips for quick filtering
 * - Search bar with debounced input
 * - Async loading placeholder for large directories
 */
class BrowserPanel
{
public:
    BrowserPanel();
    ~BrowserPanel() = default;

    /**
     * @brief Draw the browser panel
     * @param open Reference to visibility flag
     * @param theme Theme for styling
     */
    void draw(bool& open, const Theme& theme);

    /**
     * @brief Set callback for item selection
     */
    void setOnItemSelected(std::function<void(const BrowserItem&)> callback)
    {
        onItemSelected_ = std::move(callback);
    }

    /**
     * @brief Set callback for item double-click
     */
    void setOnItemActivated(std::function<void(const BrowserItem&)> callback)
    {
        onItemActivated_ = std::move(callback);
    }

    /**
     * @brief Add root item to browser
     */
    void addRootItem(std::unique_ptr<BrowserItem> item);

    /**
     * @brief Clear all items
     */
    void clear();

private:
    std::vector<std::unique_ptr<BrowserItem>> rootItems_;
    std::vector<FilterChip> filterChips_;
    char searchBuffer_[256]{};
    std::string lastSearch_;
    float searchDebounceTime_{0.0f};
    const BrowserItem* selectedItem_{nullptr};

    std::function<void(const BrowserItem&)> onItemSelected_;
    std::function<void(const BrowserItem&)> onItemActivated_;

    void drawSearchBar(const Theme& theme);
    void drawFilterChips(const Theme& theme);
    void drawTreeItem(const BrowserItem& item, const Theme& theme);
    void createDemoContent();
    
    static const char* getIconForType(BrowserItemType type);
    bool matchesFilter(const BrowserItem& item) const;
};

} // namespace daw::ui::imgui
