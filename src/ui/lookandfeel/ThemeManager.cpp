#include "ThemeManager.h"
#include "UltraDesignSystem.hpp"
#include <juce_core/juce_core.h>
#include <algorithm>

namespace daw::ui::lookandfeel
{

namespace
{
    inline juce::Colour lighten(juce::Colour c, float amount) noexcept
    {
        return c.interpolatedWith(juce::Colours::white, juce::jlimit(0.0f, 1.0f, amount));
    }

    inline juce::Colour darken(juce::Colour c, float amount) noexcept
    {
        return c.interpolatedWith(juce::Colours::black, juce::jlimit(0.0f, 1.0f, amount));
    }
}

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
    juce::File themeFile(filePath);
    if (!themeFile.existsAsFile())
        return false;

    const auto jsonString = themeFile.loadFileAsString();
    if (jsonString.isEmpty())
        return false;

    auto json = juce::JSON::parse(jsonString);
    if (!json.isObject())
        return false;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr)
        return false;

    customThemeOverrides = jsonString;
    currentTheme = Theme::Custom;
    applyTheme(Theme::Custom);
    notifyListeners();

    return true;
}

bool ThemeManager::saveTheme(const std::string& filePath) const
{
    juce::File themeFile(filePath);

    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("theme", static_cast<int>(currentTheme));

    // Save color tokens
    const auto& colors = getColors();
    auto* colorObj = new juce::DynamicObject();
    colorObj->setProperty("background", colors.background.toString());
    colorObj->setProperty("panelBackground", colors.panelBackground.toString());
    colorObj->setProperty("accentPrimary", colors.accentPrimary.toString());
    colorObj->setProperty("textPrimary", colors.textPrimary.toString());
    colorObj->setProperty("textSecondary", colors.textSecondary.toString());
    obj->setProperty("colors", colorObj);

    juce::var jsonVar(obj);
    auto jsonString = juce::JSON::toString(jsonVar, true);

    return themeFile.replaceWithText(jsonString);
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
    currentTheme = theme;

    ultra::resetTokensToDefaults();

    auto applyLightOverrides = [] {
        ultra::applyTokenOverrides([](ultra::Tokens& tokens) {
            tokens.color.bg0 = lighten(tokens.color.bg0, 0.92f);
            tokens.color.bg1 = lighten(tokens.color.bg1, 0.88f);
            tokens.color.bg2 = lighten(tokens.color.bg2, 0.82f);
            tokens.color.textPrimary = darken(tokens.color.textPrimary, 0.75f);
            tokens.color.textSecondary = darken(tokens.color.textSecondary, 0.55f);
            tokens.color.panelBorder = tokens.color.panelBorder.withAlpha(0.35f);
            tokens.color.shadowSoft = juce::Colours::black.withAlpha(0.25f);
        });
    };

    auto applyHighContrastOverrides = [] {
        ultra::applyTokenOverrides([](ultra::Tokens& tokens) {
            tokens.color.bg0 = juce::Colours::black;
            tokens.color.bg1 = juce::Colours::black.withBrightness(0.12f);
            tokens.color.bg2 = juce::Colours::black.withBrightness(0.18f);
            tokens.color.panelBorder = juce::Colours::white.withAlpha(0.85f);
            tokens.color.textPrimary = juce::Colours::white;
            tokens.color.textSecondary = juce::Colours::silver;
            tokens.color.accentPrimary = juce::Colours::yellow;
            tokens.color.accentSecondary = juce::Colours::aqua;
            tokens.color.shadowSoft = juce::Colours::black.withAlpha(0.5f);
        });
    };

    switch (theme)
    {
        case Theme::Dark:
            // defaults already applied
            break;
        case Theme::Light:
            applyLightOverrides();
            break;
        case Theme::HighContrast:
            applyHighContrastOverrides();
            break;
        case Theme::Custom:
            if (customThemeOverrides.isNotEmpty())
                ultra::loadTokensFromJSON(customThemeOverrides);
            break;
    }
}

#if JUCE_UNIT_TESTS
namespace
{
    struct ThemeManagerOverridesTest final : public juce::UnitTest
    {
        ThemeManagerOverridesTest() : juce::UnitTest("ThemeManager Ultra overrides", "UI") {}

        void runTest() override
        {
            beginTest("Light theme nudges Ultra tokens");
            ultra::resetTokensToDefaults();
            ThemeManager manager;
            const auto darkBg = ultra::tokens().color.bg0;
            manager.setTheme(ThemeManager::Theme::Light);
            expect(ultra::tokens().color.bg0 != darkBg);

            beginTest("High contrast enforces white text");
            manager.setTheme(ThemeManager::Theme::HighContrast);
            expect(ultra::tokens().color.textPrimary == juce::Colours::white);
        }
    };

    static ThemeManagerOverridesTest themeManagerOverridesTest;
}
#endif

} // namespace daw::ui::lookandfeel

