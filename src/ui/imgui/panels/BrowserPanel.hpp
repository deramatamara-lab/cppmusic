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
    Project,
    Pack,           ///< FL-style sample pack
    Favorite,       ///< Favorite item
    Recent,         ///< Recently used item
    PatternTemplate ///< Pattern preset
};

/**
 * @brief Browser tab type (FL Studio style)
 */
enum class BrowserTab
{
    All,
    Packs,
    CurrentProject,
    PluginDatabase,
    History,
    Favorites
};

/**
 * @brief Waveform preview data
 */
struct WaveformPreview
{
    std::vector<float> peaks;
    float duration{0.0f};
    int sampleRate{44100};
    bool loaded{false};
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
    bool isFavorite{false};

    // FL-style metadata
    std::string author;
    std::string category;
    std::vector<std::string> tags;
    int bpm{0};
    std::string key;
    float duration{0.0f};  // In seconds

    // Preview data
    std::unique_ptr<WaveformPreview> waveform;
    ImVec4 color{0.5f, 0.5f, 0.5f, 1.0f};

    // Rating (FL-style 5-star)
    int rating{0};  // 0-5 stars
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
 * @brief Browser panel for browsing files, presets, and plugins (FL Studio style)
 *
 * Features:
 * - Collapsible tree view with tabs
 * - Sample packs browser
 * - Plugin database
 * - Favorites and history
 * - Filter chips for quick filtering
 * - Search bar with debounced input
 * - Waveform preview with playback
 * - Metadata display (BPM, key, duration)
 * - Star ratings
 * - Drag-drop support
 * - Async loading for large directories
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
     * @brief Set callback for drag start
     */
    void setOnDragStart(std::function<void(const BrowserItem&)> callback)
    {
        onDragStart_ = std::move(callback);
    }

    /**
     * @brief Set callback for preview playback
     */
    void setOnPreviewPlay(std::function<void(const std::string& path)> callback)
    {
        onPreviewPlay_ = std::move(callback);
    }

    /**
     * @brief Add root item to browser
     */
    void addRootItem(std::unique_ptr<BrowserItem> item);

    /**
     * @brief Clear all items
     */
    void clear();

    /**
     * @brief Add item to favorites
     */
    void addToFavorites(const BrowserItem& item);

    /**
     * @brief Remove item from favorites
     */
    void removeFromFavorites(const std::string& path);

    /**
     * @brief Set current tab
     */
    void setCurrentTab(BrowserTab tab) { currentTab_ = tab; }

private:
    std::vector<std::unique_ptr<BrowserItem>> rootItems_;
    std::vector<std::unique_ptr<BrowserItem>> favorites_;
    std::vector<std::unique_ptr<BrowserItem>> recentItems_;
    std::vector<std::unique_ptr<BrowserItem>> packItems_;
    std::vector<FilterChip> filterChips_;
    char searchBuffer_[256]{};
    std::string lastSearch_;
    float searchDebounceTime_{0.0f};
    const BrowserItem* selectedItem_{nullptr};
    BrowserTab currentTab_{BrowserTab::All};

    // Preview state
    bool isPreviewPlaying_{false};
    std::string previewingPath_;
    float previewPosition_{0.0f};
    bool autoPreview_{true};
    float previewVolume_{0.8f};

    // View options
    bool showMetadata_{true};
    bool showWaveforms_{true};
    bool showRatings_{true};
    bool listView_{false};  // false = tree, true = list

    std::function<void(const BrowserItem&)> onItemSelected_;
    std::function<void(const BrowserItem&)> onItemActivated_;
    std::function<void(const BrowserItem&)> onDragStart_;
    std::function<void(const std::string&)> onPreviewPlay_;

    void drawTabs(const Theme& theme);
    void drawSearchBar(const Theme& theme);
    void drawFilterChips(const Theme& theme);
    void drawTreeItem(const BrowserItem& item, const Theme& theme);
    void drawListItem(const BrowserItem& item, const Theme& theme);
    void drawPreviewPanel(const Theme& theme);
    void drawWaveformPreview(const WaveformPreview& waveform, const Theme& theme, ImVec2 size);
    void drawMetadataPanel(const BrowserItem& item, const Theme& theme);
    void drawRatingStars(BrowserItem& item, const Theme& theme);
    void createDemoContent();
    void loadWaveformPreview(BrowserItem& item);

    static const char* getIconForType(BrowserItemType type);
    bool matchesFilter(const BrowserItem& item) const;
};

} // namespace daw::ui::imgui
