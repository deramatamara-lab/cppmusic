/**
 * @file I18n.hpp
 * @brief Internationalization service with JSON locale files
 */
#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace daw::ui::i18n
{

/**
 * @brief Locale information
 */
struct LocaleInfo
{
    std::string code;        // e.g., "en-US", "de-DE"
    std::string name;        // e.g., "English (US)"
    std::string nativeName;  // e.g., "English (US)" or "Deutsch"
    std::string direction;   // "ltr" or "rtl"
};

/**
 * @brief Internationalization service
 */
class I18n
{
public:
    I18n();
    ~I18n() = default;

    /**
     * @brief Load locale file
     * @param filepath Path to JSON locale file
     * @return true if successful
     */
    bool loadLocale(const std::filesystem::path& filepath);

    /**
     * @brief Load all locales from directory
     * @param directory Path to i18n directory
     * @return Number of locales loaded
     */
    int loadAllLocales(const std::filesystem::path& directory);

    /**
     * @brief Set current locale
     * @param localeCode Locale code (e.g., "en-US")
     * @return true if locale exists and was set
     */
    bool setLocale(const std::string& localeCode);

    /**
     * @brief Get current locale code
     */
    [[nodiscard]] const std::string& getCurrentLocale() const { return currentLocale_; }

    /**
     * @brief Get available locales
     */
    [[nodiscard]] std::vector<LocaleInfo> getAvailableLocales() const;

    /**
     * @brief Translate a key
     * @param key Translation key (e.g., "menu.file.new")
     * @return Translated string, or key if not found
     */
    [[nodiscard]] std::string translate(const std::string& key) const;

    /**
     * @brief Translate with parameter substitution
     * @param key Translation key
     * @param params Parameters to substitute ({0}, {1}, etc.)
     * @return Translated string with substitutions
     */
    [[nodiscard]] std::string translate(const std::string& key,
                                         const std::vector<std::string>& params) const;

    /**
     * @brief Translate with named parameter substitution
     * @param key Translation key
     * @param params Named parameters ({name}, {count}, etc.)
     * @return Translated string with substitutions
     */
    [[nodiscard]] std::string translateNamed(
        const std::string& key,
        const std::unordered_map<std::string, std::string>& params) const;

    /**
     * @brief Check if key exists in current locale
     */
    [[nodiscard]] bool hasKey(const std::string& key) const;

    /**
     * @brief Get fallback locale (used when key not found)
     */
    [[nodiscard]] const std::string& getFallbackLocale() const { return fallbackLocale_; }
    void setFallbackLocale(const std::string& localeCode) { fallbackLocale_ = localeCode; }

    /**
     * @brief Subscribe to locale changes
     * @return Callback will be invoked when locale changes
     */
    void onLocaleChanged(std::function<void(const std::string&)> callback);

    /**
     * @brief Clear all loaded locales
     */
    void clear();

    /**
     * @brief Format number according to locale
     */
    [[nodiscard]] std::string formatNumber(double value, int decimals = 2) const;

    /**
     * @brief Format percentage
     */
    [[nodiscard]] std::string formatPercent(double value, int decimals = 0) const;

    /**
     * @brief Get text direction for current locale
     */
    [[nodiscard]] bool isRightToLeft() const;

private:
    struct LocaleData
    {
        LocaleInfo info;
        std::unordered_map<std::string, std::string> translations;
    };

    std::string lookupKey(const std::string& localeCode, const std::string& key) const;
    bool parseLocaleFile(const std::string& content, LocaleData& data);

    std::unordered_map<std::string, LocaleData> locales_;
    std::string currentLocale_{"en-US"};
    std::string fallbackLocale_{"en-US"};
    std::vector<std::function<void(const std::string&)>> localeChangedCallbacks_;
};

/**
 * @brief Global I18n instance
 */
inline I18n& getGlobalI18n()
{
    static I18n instance;
    return instance;
}

/**
 * @brief Translation helper function
 * @param key Translation key
 * @return Translated string
 */
inline std::string tr(const std::string& key)
{
    return getGlobalI18n().translate(key);
}

/**
 * @brief Translation helper with parameters
 */
inline std::string tr(const std::string& key, const std::vector<std::string>& params)
{
    return getGlobalI18n().translate(key, params);
}

} // namespace daw::ui::i18n
