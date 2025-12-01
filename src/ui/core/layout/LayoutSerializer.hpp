/**
 * @file LayoutSerializer.hpp
 * @brief Layout persistence with versioned JSON and autosave
 */
#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace daw::ui::layout
{

/**
 * @brief Panel state
 */
struct PanelState
{
    std::string id;
    bool visible{true};
    bool collapsed{false};
    float posX{0.0f};
    float posY{0.0f};
    float width{0.0f};
    float height{0.0f};
    int dockId{-1};
    std::string dockPosition;  // "left", "right", "top", "bottom", "center"
};

/**
 * @brief Complete layout state
 */
struct LayoutState
{
    static constexpr int CURRENT_VERSION = 1;
    
    int version{CURRENT_VERSION};
    std::string themePath;
    float fontScale{1.0f};
    float dpiScale{1.0f};
    int windowWidth{1920};
    int windowHeight{1080};
    bool windowMaximized{false};
    
    std::vector<PanelState> panels;
    std::string dockLayoutIni;  // ImGui docking layout INI data
    
    // Additional settings
    std::unordered_map<std::string, std::string> customSettings;
};

/**
 * @brief Migration function for layout upgrades
 */
using MigrationFunc = std::function<bool(LayoutState&, int fromVersion)>;

/**
 * @brief Layout serializer with versioning and autosave
 */
class LayoutSerializer
{
public:
    LayoutSerializer();
    ~LayoutSerializer();

    /**
     * @brief Load layout from file
     * @param filepath Path to layout JSON file
     * @return Loaded layout state, or nullopt on failure
     */
    std::optional<LayoutState> load(const std::filesystem::path& filepath);

    /**
     * @brief Save layout to file
     * @param state Layout state to save
     * @param filepath Path to save JSON file
     * @return true if successful
     */
    bool save(const LayoutState& state, const std::filesystem::path& filepath);

    /**
     * @brief Register migration function for version upgrade
     * @param fromVersion Source version
     * @param migration Migration function
     */
    void registerMigration(int fromVersion, MigrationFunc migration);

    /**
     * @brief Enable autosave on layout change
     * @param filepath Path for autosave file
     * @param debounceMs Debounce delay in milliseconds
     */
    void enableAutosave(const std::filesystem::path& filepath, int debounceMs = 2000);

    /**
     * @brief Disable autosave
     */
    void disableAutosave();

    /**
     * @brief Mark layout as changed (triggers autosave after debounce)
     */
    void markChanged();

    /**
     * @brief Update autosave timer (call every frame)
     * @param state Current layout state
     */
    void update(const LayoutState& state);

    /**
     * @brief Force save now (bypass debounce)
     * @param state Current layout state
     */
    void saveNow(const LayoutState& state);

    /**
     * @brief Get default layout state
     */
    [[nodiscard]] static LayoutState getDefaultLayout();

    /**
     * @brief Validate layout state
     */
    [[nodiscard]] static bool validate(const LayoutState& state);

    /**
     * @brief Export layout to INI format (for ImGui docking)
     */
    [[nodiscard]] static std::string toIni(const LayoutState& state);

    /**
     * @brief Import dock layout from INI string
     */
    static void fromIni(LayoutState& state, const std::string& ini);

private:
    bool migrateIfNeeded(LayoutState& state);
    std::string serializeToJson(const LayoutState& state) const;
    std::optional<LayoutState> deserializeFromJson(const std::string& json);

    std::unordered_map<int, MigrationFunc> migrations_;
    
    // Autosave state
    bool autosaveEnabled_{false};
    std::filesystem::path autosavePath_;
    int autosaveDebounceMs_{2000};
    std::chrono::steady_clock::time_point lastChangeTime_;
    bool pendingAutosave_{false};
};

/**
 * @brief Global layout serializer instance
 */
inline LayoutSerializer& getGlobalLayoutSerializer()
{
    static LayoutSerializer instance;
    return instance;
}

} // namespace daw::ui::layout
