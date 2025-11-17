#include "ThemeManager.h"
#include <juce_core/juce_core.h>
#include <algorithm>

namespace daw::ui::lookandfeel
{

ThemeManager::ThemeManager()
{
    applyTheme(Theme::Dark);
}

void ThemeManager::setTheme(Theme theme)
{
    if (theme != currentTheme)
    {
        currentTheme = theme;
        applyTheme(theme);
        notifyListeners();
    }
}

bool ThemeManager::loadCustomTheme(const std::string& filePath)
{
    juce::ignoreUnused(filePath);
    // TODO: Implement custom theme loading from JSON
    return false;
}

bool ThemeManager::saveTheme(const std::string& filePath) const
{
    juce::ignoreUnused(filePath);
    // TODO: Implement theme saving to JSON
    return false;
}

void ThemeManager::addThemeChangeListener(std::function<void(Theme)> listener)
{
    listeners.push_back(listener);
}

void ThemeManager::removeThemeChangeListener(std::function<void(Theme)> listener)
{
    listeners.erase(
        std::remove_if(listeners.begin(), listeners.end(),
            [&listener](const std::function<void(Theme)>& candidate) {
                if (!candidate || !listener)
                    return false;

                if (candidate.target_type() != listener.target_type())
                    return false;

                if (auto lhs = candidate.target<void(*)(Theme)>())
                    if (auto rhs = listener.target<void(*)(Theme)>())
                        return *lhs == *rhs;

                // Fall back to address comparison for stateful callables copied into std::function
                return candidate.target_type().hash_code() == listener.target_type().hash_code();
            }),
        listeners.end());
}

const ColorTokens& ThemeManager::getColors() const
{
    const auto mapToBaseTheme = [](Theme theme) -> daw::ui::lookandfeel::Theme {
        switch (theme)
        {
            case Theme::Light:
                return daw::ui::lookandfeel::Theme::Light;
            case Theme::HighContrast:
                return daw::ui::lookandfeel::Theme::Light;
            case Theme::Custom:
                return daw::ui::lookandfeel::Theme::Dark;
            case Theme::Dark:
            default:
                return daw::ui::lookandfeel::Theme::Dark;
        }
    };

    return getDesignTokens(mapToBaseTheme(currentTheme)).colours;
}

void ThemeManager::notifyListeners()
{
    for (const auto& listener : listeners)
    {
        if (listener)
            listener(currentTheme);
    }
}

void ThemeManager::applyTheme(Theme theme)
{
    juce::ignoreUnused(theme);
    // TODO: Apply theme-specific color overrides
    // This would modify the DesignSystem::Colors values
}

} // namespace daw::ui::lookandfeel

