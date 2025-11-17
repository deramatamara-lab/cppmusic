#pragma once

#include "DesignTokens.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <functional>
#include <vector>

namespace daw::ui::lookandfeel
{

struct ColorTokens;

/**
 * @brief Theme management system
 * 
 * Supports multiple themes (dark, light, custom) with runtime switching.
 * Follows DAW_DEV_RULES: accessibility support, high-contrast theme.
 */
class ThemeManager
{
public:
    enum class Theme
    {
        Dark,
        Light,
        HighContrast,
        Custom
    };

    ThemeManager();
    ~ThemeManager() = default;

    // Non-copyable, movable
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;
    ThemeManager(ThemeManager&&) noexcept = default;
    ThemeManager& operator=(ThemeManager&&) noexcept = default;

    /**
     * @brief Set current theme
     */
    void setTheme(Theme theme);

    /**
     * @brief Get current theme
     */
    [[nodiscard]] Theme getCurrentTheme() const noexcept { return currentTheme; }

    /**
     * @brief Load custom theme from file
     */
    bool loadCustomTheme(const std::string& filePath);

    /**
     * @brief Save current theme to file
     */
    bool saveTheme(const std::string& filePath) const;

    /**
     * @brief Add theme change listener
     */
    void addThemeChangeListener(std::function<void(Theme)> listener);

    /**
     * @brief Remove theme change listener
     */
    void removeThemeChangeListener(std::function<void(Theme)> listener);

    /**
     * @brief Get design system colors for current theme
     */
    [[nodiscard]] const ColorTokens& getColors() const;

private:
    Theme currentTheme{Theme::Dark};
    std::vector<std::function<void(Theme)>> listeners;
    
    void notifyListeners();
    void applyTheme(Theme theme);
};

} // namespace daw::ui::lookandfeel

