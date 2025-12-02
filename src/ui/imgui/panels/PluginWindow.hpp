#pragma once

#include "../Theme.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace daw::ui::imgui
{

/**
 * @brief Plugin parameter for automation linking
 */
struct PluginParameter
{
    int id{0};
    std::string name;
    float value{0.5f};
    float minValue{0.0f};
    float maxValue{1.0f};
    float defaultValue{0.5f};
    std::string unit;
    bool isAutomated{false};
    bool isLinked{false};
    int linkedControlId{-1};
};

/**
 * @brief Plugin preset
 */
struct PluginPreset
{
    std::string name;
    std::string author;
    std::string category;
    std::vector<std::string> tags;
    std::vector<float> parameterValues;
    bool isFactory{false};
    bool isFavorite{false};
};

/**
 * @brief Plugin state for wrapper window
 */
struct PluginState
{
    int instanceId{-1};
    std::string name;
    std::string vendor;
    std::string version;
    std::string format;  // VST3, AU, LV2, CLAP

    // Parameters
    std::vector<PluginParameter> parameters;

    // Presets
    std::vector<PluginPreset> presets;
    int currentPresetIndex{-1};

    // State
    bool bypass{false};
    float mix{1.0f};  // Dry/wet
    bool showPresetBrowser{false};
    bool showParameterList{false};

    // FL-style linking
    bool linkingMode{false};
    int selectedParameterForLink{-1};
};

/**
 * @brief Plugin window wrapper (FL Studio style)
 *
 * Features:
 * - Preset browser with categories and favorites
 * - Parameter list with automation linking
 * - Bypass and dry/wet controls
 * - A/B comparison
 * - Undo/redo for parameter changes
 * - Quick link mode for automation
 * - Floating and dockable modes
 * - MIDI learn
 */
class PluginWindow
{
public:
    PluginWindow();
    ~PluginWindow() = default;

    /**
     * @brief Draw the plugin window
     * @param open Reference to visibility flag
     * @param theme Theme for styling
     */
    void draw(bool& open, const Theme& theme);

    /**
     * @brief Set plugin state
     */
    void setPluginState(std::unique_ptr<PluginState> state)
    {
        state_ = std::move(state);
    }

    /**
     * @brief Get plugin state
     */
    [[nodiscard]] PluginState* getPluginState() { return state_.get(); }

    /**
     * @brief Set callback for parameter change
     */
    void setOnParameterChanged(std::function<void(int paramId, float value)> callback)
    {
        onParameterChanged_ = std::move(callback);
    }

    /**
     * @brief Set callback for preset change
     */
    void setOnPresetChanged(std::function<void(int presetIndex)> callback)
    {
        onPresetChanged_ = std::move(callback);
    }

    /**
     * @brief Set callback for link parameter
     */
    void setOnLinkParameter(std::function<void(int paramId)> callback)
    {
        onLinkParameter_ = std::move(callback);
    }

    /**
     * @brief Set callback for MIDI learn
     */
    void setOnMidiLearn(std::function<void(int paramId)> callback)
    {
        onMidiLearn_ = std::move(callback);
    }

private:
    std::unique_ptr<PluginState> state_;

    // A/B comparison
    std::vector<float> stateA_;
    std::vector<float> stateB_;
    bool isStateA_{true};

    // Undo/redo
    std::vector<std::pair<int, float>> undoStack_;
    std::vector<std::pair<int, float>> redoStack_;

    // Search/filter
    char presetSearchBuffer_[256]{};
    std::string presetCategoryFilter_;

    // View state
    bool compactView_{false};
    float windowWidth_{400.0f};
    float windowHeight_{600.0f};

    std::function<void(int, float)> onParameterChanged_;
    std::function<void(int)> onPresetChanged_;
    std::function<void(int)> onLinkParameter_;
    std::function<void(int)> onMidiLearn_;

    void drawTitleBar(const Theme& theme);
    void drawPresetSelector(const Theme& theme);
    void drawPresetBrowser(const Theme& theme);
    void drawPluginContent(const Theme& theme);
    void drawParameterList(const Theme& theme);
    void drawBypassMix(const Theme& theme);
    void drawABComparison(const Theme& theme);
    void drawContextMenu(const Theme& theme);

    void saveStateToA();
    void saveStateToB();
    void loadStateFromA();
    void loadStateFromB();
    void pushUndo(int paramId, float oldValue);
    void undo();
    void redo();
};

/**
 * @brief Plugin picker dialog (FL Studio style)
 *
 * Features:
 * - Categorized plugin list
 * - Search with fuzzy matching
 * - Favorites
 * - Recently used
 * - Generator vs Effect tabs
 */
class PluginPicker
{
public:
    PluginPicker();
    ~PluginPicker() = default;

    /**
     * @brief Draw the plugin picker
     * @param open Reference to visibility flag
     * @param theme Theme for styling
     */
    void draw(bool& open, const Theme& theme);

    /**
     * @brief Set available plugins
     */
    void setPlugins(const std::vector<std::pair<std::string, std::string>>& plugins);

    /**
     * @brief Set callback for plugin selection
     */
    void setOnPluginSelected(std::function<void(const std::string& pluginId)> callback)
    {
        onPluginSelected_ = std::move(callback);
    }

private:
    struct PluginInfo
    {
        std::string id;
        std::string name;
        std::string vendor;
        std::string category;
        std::string format;
        bool isGenerator{false};
        bool isFavorite{false};
        int useCount{0};
    };

    std::vector<PluginInfo> plugins_;
    std::vector<PluginInfo> filteredPlugins_;
    std::vector<std::string> categories_;
    std::vector<std::string> recentlyUsed_;

    char searchBuffer_[256]{};
    std::string selectedCategory_;
    bool showGenerators_{true};
    bool showEffects_{true};
    bool showFavoritesOnly_{false};

    std::function<void(const std::string&)> onPluginSelected_;

    void drawTabs(const Theme& theme);
    void drawSearchBar(const Theme& theme);
    void drawCategoryList(const Theme& theme);
    void drawPluginList(const Theme& theme);
    void filterPlugins();
};

} // namespace daw::ui::imgui
