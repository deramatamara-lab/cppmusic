#pragma once

#include "imgui.h"
#include <string>
#include <unordered_map>
#include <filesystem>

namespace daw::ui::imgui
{

/**
 * @brief Theme tokens structure for design system consistency
 *
 * All sizes are in logical pixels (DPI-scaled automatically).
 * Colors use 0-255 RGBA values.
 */
struct ThemeTokens
{
    // Colors - Window & Background (FL Studio dark theme)
    ImVec4 windowBg{0.11f, 0.11f, 0.11f, 1.0f};        // #1C1C1C
    ImVec4 childBg{0.13f, 0.13f, 0.13f, 1.0f};         // #212121
    ImVec4 popupBg{0.15f, 0.15f, 0.15f, 1.0f};         // #262626
    ImVec4 border{0.25f, 0.25f, 0.25f, 1.0f};          // #404040
    ImVec4 borderShadow{0.0f, 0.0f, 0.0f, 0.0f};

    // Colors - Headers & Titles (FL-style darker)
    ImVec4 titleBg{0.09f, 0.09f, 0.09f, 1.0f};         // #171717
    ImVec4 titleBgActive{0.85f, 0.45f, 0.15f, 1.0f};   // FL orange accent
    ImVec4 titleBgCollapsed{0.09f, 0.09f, 0.09f, 0.8f};
    ImVec4 menuBarBg{0.11f, 0.11f, 0.11f, 1.0f};       // #1C1C1C

    // Colors - Tabs (FL-style with orange accent)
    ImVec4 tab{0.15f, 0.15f, 0.15f, 1.0f};             // #262626
    ImVec4 tabHovered{0.90f, 0.50f, 0.20f, 1.0f};      // FL orange hover
    ImVec4 tabActive{0.85f, 0.45f, 0.15f, 1.0f};       // FL orange active
    ImVec4 tabUnfocused{0.13f, 0.13f, 0.13f, 1.0f};
    ImVec4 tabUnfocusedActive{0.60f, 0.35f, 0.12f, 1.0f};

    // Colors - Buttons & Interactives (FL-style)
    ImVec4 button{0.20f, 0.20f, 0.20f, 1.0f};          // #333333
    ImVec4 buttonHovered{0.90f, 0.50f, 0.20f, 1.0f};   // FL orange
    ImVec4 buttonActive{0.75f, 0.40f, 0.15f, 1.0f};    // FL orange pressed
    ImVec4 header{0.85f, 0.45f, 0.15f, 0.8f};          // FL orange header
    ImVec4 headerHovered{0.90f, 0.50f, 0.20f, 1.0f};
    ImVec4 headerActive{0.75f, 0.40f, 0.15f, 1.0f};

    // Colors - Frame & Input (darker FL-style)
    ImVec4 frameBg{0.17f, 0.17f, 0.17f, 1.0f};         // #2B2B2B
    ImVec4 frameBgHovered{0.22f, 0.22f, 0.22f, 1.0f};  // #383838
    ImVec4 frameBgActive{0.85f, 0.45f, 0.15f, 1.0f};   // FL orange
    ImVec4 checkMark{0.90f, 0.50f, 0.20f, 1.0f};       // FL orange checkmark

    // Colors - Slider (FL-style orange)
    ImVec4 sliderGrab{0.85f, 0.45f, 0.15f, 1.0f};      // FL orange
    ImVec4 sliderGrabActive{0.90f, 0.50f, 0.20f, 1.0f};

    // Colors - Scrollbar (FL-style)
    ImVec4 scrollbarBg{0.11f, 0.11f, 0.11f, 0.6f};
    ImVec4 scrollbarGrab{0.30f, 0.30f, 0.30f, 1.0f};
    ImVec4 scrollbarGrabHovered{0.85f, 0.45f, 0.15f, 1.0f}; // FL orange
    ImVec4 scrollbarGrabActive{0.75f, 0.40f, 0.15f, 1.0f};

    // Colors - Selection & Focus (FL-style orange)
    ImVec4 textSelectedBg{0.85f, 0.45f, 0.15f, 0.5f};  // FL orange selection
    ImVec4 navHighlight{0.90f, 0.50f, 0.20f, 1.0f};    // FL orange highlight

    // Colors - Text (brighter for contrast)
    ImVec4 text{0.95f, 0.95f, 0.95f, 1.0f};            // #F2F2F2
    ImVec4 textDisabled{0.50f, 0.50f, 0.50f, 1.0f};    // #808080

    // Colors - Resize & Separators (FL-style)
    ImVec4 separator{0.25f, 0.25f, 0.25f, 1.0f};
    ImVec4 separatorHovered{0.85f, 0.45f, 0.15f, 1.0f}; // FL orange
    ImVec4 separatorActive{0.90f, 0.50f, 0.20f, 1.0f};
    ImVec4 resizeGrip{0.30f, 0.30f, 0.30f, 0.4f};
    ImVec4 resizeGripHovered{0.85f, 0.45f, 0.15f, 0.7f};
    ImVec4 resizeGripActive{0.90f, 0.50f, 0.20f, 0.9f};

    // Colors - Docking (FL-style)
    ImVec4 dockingPreview{0.85f, 0.45f, 0.15f, 0.7f};  // FL orange preview
    ImVec4 dockingEmptyBg{0.11f, 0.11f, 0.11f, 1.0f};

    // Colors - Plot & Table
    ImVec4 tableBorderStrong{0.25f, 0.25f, 0.30f, 1.0f};
    ImVec4 tableBorderLight{0.20f, 0.20f, 0.25f, 1.0f};
    ImVec4 tableHeaderBg{0.15f, 0.15f, 0.18f, 1.0f};
    ImVec4 tableRowBg{0.0f, 0.0f, 0.0f, 0.0f};
    ImVec4 tableRowBgAlt{0.12f, 0.12f, 0.14f, 0.4f};

    // Custom DAW Colors - Meters
    ImVec4 meterGreen{0.20f, 0.80f, 0.35f, 1.0f};
    ImVec4 meterYellow{0.95f, 0.85f, 0.20f, 1.0f};
    ImVec4 meterRed{0.95f, 0.25f, 0.25f, 1.0f};
    ImVec4 meterBackground{0.10f, 0.10f, 0.12f, 1.0f};

    // Custom DAW Colors - Timeline & Piano Roll (FL-style)
    ImVec4 gridLine{0.22f, 0.22f, 0.22f, 0.5f};        // Subtle grid
    ImVec4 gridLineBeat{0.28f, 0.28f, 0.28f, 0.7f};
    ImVec4 gridLineBar{0.35f, 0.35f, 0.35f, 0.9f};
    ImVec4 playhead{0.95f, 0.40f, 0.20f, 1.0f};        // FL orange-red playhead
    ImVec4 selection{0.85f, 0.45f, 0.15f, 0.3f};       // FL orange selection
    ImVec4 noteOn{0.40f, 0.70f, 0.95f, 1.0f};          // FL blue notes
    ImVec4 noteOff{0.25f, 0.50f, 0.70f, 0.5f};

    // Custom DAW Colors - Transport (FL-style)
    ImVec4 playButton{0.30f, 0.85f, 0.45f, 1.0f};      // FL green play
    ImVec4 stopButton{0.70f, 0.70f, 0.70f, 1.0f};      // FL grey stop
    ImVec4 recordButton{0.95f, 0.30f, 0.30f, 1.0f};    // FL red record

    // Layout - Spacing (8px grid system)
    float spacingXs{4.0f};
    float spacingSm{8.0f};
    float spacingMd{16.0f};
    float spacingLg{24.0f};
    float spacingXl{32.0f};

    // Layout - Radii
    float radiusSm{2.0f};
    float radiusMd{4.0f};
    float radiusLg{8.0f};

    // Layout - Borders
    float borderWidth{1.0f};
    float scrollbarSize{12.0f};
    float grabMinSize{10.0f};

    // Typography - Font Sizes (will be scaled by DPI)
    float fontSizeXs{11.0f};
    float fontSizeSm{12.0f};
    float fontSizeMd{14.0f};
    float fontSizeLg{18.0f};
    float fontSizeXl{24.0f};

    // Animation - Timing (in seconds)
    float animDurationFast{0.1f};
    float animDurationNormal{0.2f};
    float animDurationSlow{0.4f};
};

/**
 * @brief Theme manager for the DAW UI
 *
 * Handles loading themes from JSON files, applying to ImGui style,
 * and live reloading during development.
 */
class Theme
{
public:
    Theme();
    ~Theme() = default;

    // Non-copyable, non-movable (singleton-like usage)
    Theme(const Theme&) = delete;
    Theme& operator=(const Theme&) = delete;
    Theme(Theme&&) = delete;
    Theme& operator=(Theme&&) = delete;

    /**
     * @brief Load theme from JSON file
     * @param filepath Path to theme JSON file
     * @return true if loaded successfully
     */
    bool loadFromFile(const std::filesystem::path& filepath);

    /**
     * @brief Save current theme to JSON file
     * @param filepath Path to save theme JSON file
     * @return true if saved successfully
     */
    bool saveToFile(const std::filesystem::path& filepath) const;

    /**
     * @brief Apply current theme tokens to ImGui style
     */
    void applyToImGui();

    /**
     * @brief Get current theme tokens (read-only)
     */
    [[nodiscard]] const ThemeTokens& getTokens() const { return tokens_; }

    /**
     * @brief Get mutable theme tokens for modification
     */
    [[nodiscard]] ThemeTokens& getTokensMutable() { return tokens_; }

    /**
     * @brief Set DPI scale factor
     * @param scale Scale factor (1.0 = 100%, 2.0 = 200%)
     */
    void setDpiScale(float scale);

    /**
     * @brief Get current DPI scale factor
     */
    [[nodiscard]] float getDpiScale() const { return dpiScale_; }

    /**
     * @brief Check if theme file has been modified (for live reload)
     * @return true if file was modified since last load
     */
    [[nodiscard]] bool checkFileModified() const;

    /**
     * @brief Reload theme if file was modified
     * @return true if theme was reloaded
     */
    bool reloadIfModified();

    /**
     * @brief Get scaled spacing value
     */
    [[nodiscard]] float spacing(int level) const;

    /**
     * @brief Get the path to the currently loaded theme file
     */
    [[nodiscard]] const std::filesystem::path& getCurrentPath() const { return currentPath_; }

private:
    ThemeTokens tokens_;
    float dpiScale_{1.0f};
    std::filesystem::path currentPath_;
    std::filesystem::file_time_type lastModified_;

    void applyDefaultTokens();
    static ImVec4 parseColor(const std::string& json);
    static std::string colorToJson(const ImVec4& color);
};

} // namespace daw::ui::imgui
