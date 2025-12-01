/**
 * @file ThemeManager.hpp
 * @brief Enhanced theme system with nested tokens and derived values
 */
#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace daw::ui::theme
{

/**
 * @brief RGBA color
 */
struct Color
{
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    float a{1.0f};

    Color() = default;
    Color(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}

    /**
     * @brief Parse from hex string (#RRGGBB or #RRGGBBAA)
     */
    static Color fromHex(const std::string& hex);

    /**
     * @brief Convert to hex string
     */
    [[nodiscard]] std::string toHex() const;

    /**
     * @brief Derive hover state (lighter)
     */
    [[nodiscard]] Color hover(float amount = 0.1f) const;

    /**
     * @brief Derive disabled state (desaturated, less opaque)
     */
    [[nodiscard]] Color disabled() const;

    /**
     * @brief Derive pressed state (darker)
     */
    [[nodiscard]] Color pressed(float amount = 0.1f) const;

    /**
     * @brief Mix with another color
     */
    [[nodiscard]] Color mix(const Color& other, float t) const;

    /**
     * @brief Adjust alpha
     */
    [[nodiscard]] Color withAlpha(float alpha) const;

    /**
     * @brief Convert to ImVec4
     */
    [[nodiscard]] float* toImVec4(float* out) const;
};

/**
 * @brief Spacing value with semantic meaning
 */
struct Spacing
{
    float xs{4.0f};   // Extra small
    float sm{8.0f};   // Small
    float md{16.0f};  // Medium (base)
    float lg{24.0f};  // Large
    float xl{32.0f};  // Extra large
    float xxl{48.0f}; // 2x Extra large

    [[nodiscard]] float get(const std::string& size) const;
};

/**
 * @brief Border radius tokens
 */
struct Radii
{
    float none{0.0f};
    float sm{2.0f};
    float md{4.0f};
    float lg{8.0f};
    float xl{12.0f};
    float full{9999.0f};  // For pills/circles

    [[nodiscard]] float get(const std::string& size) const;
};

/**
 * @brief Typography tokens
 */
struct Typography
{
    float fontSizeXs{10.0f};
    float fontSizeSm{12.0f};
    float fontSizeMd{14.0f};
    float fontSizeLg{18.0f};
    float fontSizeXl{24.0f};
    float fontSizeXxl{32.0f};

    float lineHeightTight{1.2f};
    float lineHeightNormal{1.5f};
    float lineHeightRelaxed{1.75f};

    std::string fontFamilyUI{"Inter"};
    std::string fontFamilyMono{"JetBrains Mono"};

    [[nodiscard]] float getFontSize(const std::string& size) const;
};

/**
 * @brief Shadow definition
 */
struct Shadow
{
    float offsetX{0.0f};
    float offsetY{2.0f};
    float blur{4.0f};
    float spread{0.0f};
    Color color{0.0f, 0.0f, 0.0f, 0.25f};
};

/**
 * @brief Elevation shadows (Material Design inspired)
 */
struct Elevations
{
    Shadow none;
    Shadow sm{{0, 1, 2, 0, {0, 0, 0, 0.1f}}};
    Shadow md{{0, 2, 4, 0, {0, 0, 0, 0.15f}}};
    Shadow lg{{0, 4, 8, 0, {0, 0, 0, 0.2f}}};
    Shadow xl{{0, 8, 16, 0, {0, 0, 0, 0.25f}}};

    [[nodiscard]] const Shadow& get(const std::string& level) const;
};

/**
 * @brief Color palette with semantic and component tokens
 */
struct ColorPalette
{
    // Base colors
    Color primary{0.2f, 0.5f, 0.8f, 1.0f};
    Color secondary{0.5f, 0.5f, 0.6f, 1.0f};
    Color accent{0.8f, 0.4f, 0.2f, 1.0f};

    // Semantic colors
    Color success{0.2f, 0.75f, 0.4f, 1.0f};
    Color warning{0.95f, 0.75f, 0.2f, 1.0f};
    Color error{0.9f, 0.25f, 0.25f, 1.0f};
    Color info{0.3f, 0.6f, 0.9f, 1.0f};

    // Background colors
    Color bgPrimary{0.08f, 0.08f, 0.1f, 1.0f};
    Color bgSecondary{0.1f, 0.1f, 0.12f, 1.0f};
    Color bgTertiary{0.12f, 0.12f, 0.14f, 1.0f};
    Color bgElevated{0.15f, 0.15f, 0.18f, 1.0f};

    // Text colors
    Color textPrimary{0.92f, 0.92f, 0.94f, 1.0f};
    Color textSecondary{0.7f, 0.7f, 0.72f, 1.0f};
    Color textMuted{0.5f, 0.5f, 0.52f, 1.0f};
    Color textInverse{0.1f, 0.1f, 0.12f, 1.0f};

    // Border colors
    Color borderLight{0.2f, 0.2f, 0.25f, 1.0f};
    Color borderMedium{0.3f, 0.3f, 0.35f, 1.0f};
    Color borderFocus{0.3f, 0.5f, 0.7f, 1.0f};

    // DAW-specific colors
    Color meterGreen{0.2f, 0.8f, 0.35f, 1.0f};
    Color meterYellow{0.95f, 0.85f, 0.2f, 1.0f};
    Color meterRed{0.95f, 0.25f, 0.25f, 1.0f};
    Color playhead{0.95f, 0.35f, 0.35f, 1.0f};
    Color selection{0.3f, 0.5f, 0.7f, 0.3f};
    Color noteActive{0.3f, 0.6f, 0.9f, 1.0f};
    Color noteGhost{0.3f, 0.6f, 0.9f, 0.4f};
    Color gridLine{0.2f, 0.2f, 0.25f, 0.5f};
    Color gridBeat{0.3f, 0.3f, 0.35f, 0.7f};
    Color gridBar{0.4f, 0.4f, 0.45f, 0.9f};

    // Transport colors
    Color playButton{0.2f, 0.75f, 0.4f, 1.0f};
    Color stopButton{0.85f, 0.3f, 0.3f, 1.0f};
    Color recordButton{0.95f, 0.2f, 0.2f, 1.0f};
};

/**
 * @brief Complete theme with all tokens
 */
struct ThemeTokens
{
    std::string name{"Default"};
    std::string version{"1.0.0"};
    std::string description;
    bool isDark{true};

    ColorPalette colors;
    Spacing spacing;
    Radii radii;
    Typography typography;
    Elevations elevations;

    // Animation timing (seconds)
    float animFast{0.1f};
    float animNormal{0.2f};
    float animSlow{0.4f};

    // DPI scale
    float dpiScale{1.0f};
    float fontScale{1.0f};
};

/**
 * @brief Theme change listener
 */
using ThemeChangeCallback = std::function<void(const ThemeTokens&)>;

/**
 * @brief Enhanced theme manager
 */
class ThemeManager
{
public:
    ThemeManager();
    ~ThemeManager() = default;

    /**
     * @brief Load theme from JSON file
     * @param filepath Path to theme JSON file
     * @return true if successful
     */
    bool loadFromFile(const std::filesystem::path& filepath);

    /**
     * @brief Save current theme to JSON file
     * @param filepath Path to save JSON file
     * @return true if successful
     */
    bool saveToFile(const std::filesystem::path& filepath) const;

    /**
     * @brief Load all themes from directory
     * @param directory Path to themes directory
     * @return Number of themes loaded
     */
    int loadAllThemes(const std::filesystem::path& directory);

    /**
     * @brief Get available theme names
     */
    [[nodiscard]] std::vector<std::string> getAvailableThemes() const;

    /**
     * @brief Set active theme by name
     */
    bool setTheme(const std::string& name);

    /**
     * @brief Get current theme name
     */
    [[nodiscard]] const std::string& getCurrentThemeName() const { return currentThemeName_; }

    /**
     * @brief Get current theme tokens
     */
    [[nodiscard]] const ThemeTokens& getTokens() const { return currentTokens_; }

    /**
     * @brief Get mutable tokens for editing
     */
    [[nodiscard]] ThemeTokens& getTokensMutable() { return currentTokens_; }

    /**
     * @brief Apply current theme to ImGui
     */
    void applyToImGui();

    /**
     * @brief Set DPI scale
     */
    void setDpiScale(float scale);

    /**
     * @brief Set font scale
     */
    void setFontScale(float scale);

    /**
     * @brief Check if theme file has been modified (for live reload)
     */
    [[nodiscard]] bool checkFileModified() const;

    /**
     * @brief Reload theme if file was modified
     */
    bool reloadIfModified();

    /**
     * @brief Subscribe to theme changes
     */
    void onThemeChanged(ThemeChangeCallback callback);

    /**
     * @brief Get default theme tokens
     */
    [[nodiscard]] static ThemeTokens getDefaultTheme();

    /**
     * @brief Get high contrast theme tokens
     */
    [[nodiscard]] static ThemeTokens getHighContrastTheme();

    /**
     * @brief Export theme diff (changes from base)
     * @param base Base theme to compare against
     * @return JSON string with differences
     */
    [[nodiscard]] std::string exportDiff(const ThemeTokens& base) const;

private:
    void notifyListeners();
    bool parseThemeJson(const std::string& json, ThemeTokens& tokens);
    std::string serializeTheme(const ThemeTokens& tokens) const;

    std::unordered_map<std::string, ThemeTokens> themes_;
    ThemeTokens currentTokens_;
    std::string currentThemeName_;
    std::filesystem::path currentPath_;
    std::filesystem::file_time_type lastModified_;
    std::vector<ThemeChangeCallback> changeCallbacks_;
};

/**
 * @brief Global theme manager instance
 */
inline ThemeManager& getGlobalThemeManager()
{
    static ThemeManager instance;
    return instance;
}

} // namespace daw::ui::theme
