/**
 * @file I18n.cpp
 * @brief Internationalization service implementation
 */

#include "I18n.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace daw::ui::i18n
{

// Simple JSON parsing helpers (same as LayoutSerializer)
namespace json
{

std::string unescape(const std::string& s)
{
    std::string result;
    result.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            switch (s[i + 1]) {
                case '"': result += '"'; ++i; break;
                case '\\': result += '\\'; ++i; break;
                case 'n': result += '\n'; ++i; break;
                case 'r': result += '\r'; ++i; break;
                case 't': result += '\t'; ++i; break;
                default: result += s[i]; break;
            }
        } else {
            result += s[i];
        }
    }
    return result;
}

std::string extractString(const std::string& json, const std::string& key)
{
    std::string searchKey = "\"" + key + "\"";
    auto pos = json.find(searchKey);
    if (pos == std::string::npos) return "";
    
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return "";
    
    auto endPos = pos + 1;
    while (endPos < json.size() && !(json[endPos] == '"' && json[endPos - 1] != '\\')) {
        ++endPos;
    }
    
    return unescape(json.substr(pos + 1, endPos - pos - 1));
}

// Extract all key-value pairs from a JSON object
std::unordered_map<std::string, std::string> extractAllStrings(const std::string& json)
{
    std::unordered_map<std::string, std::string> result;
    
    std::size_t pos = 0;
    while (pos < json.size()) {
        // Find next key
        auto keyStart = json.find('"', pos);
        if (keyStart == std::string::npos) break;
        
        auto keyEnd = json.find('"', keyStart + 1);
        if (keyEnd == std::string::npos) break;
        
        std::string key = json.substr(keyStart + 1, keyEnd - keyStart - 1);
        
        // Find colon
        auto colonPos = json.find(':', keyEnd);
        if (colonPos == std::string::npos) break;
        
        // Find value
        auto valueStart = json.find('"', colonPos);
        if (valueStart == std::string::npos) {
            pos = colonPos + 1;
            continue;
        }
        
        auto valueEnd = valueStart + 1;
        while (valueEnd < json.size() && !(json[valueEnd] == '"' && json[valueEnd - 1] != '\\')) {
            ++valueEnd;
        }
        
        std::string value = unescape(json.substr(valueStart + 1, valueEnd - valueStart - 1));
        
        // Skip metadata keys
        if (key[0] != '_') {
            result[key] = value;
        }
        
        pos = valueEnd + 1;
    }
    
    return result;
}

} // namespace json

I18n::I18n() = default;

bool I18n::loadLocale(const std::filesystem::path& filepath)
{
    if (!std::filesystem::exists(filepath)) {
        return false;
    }
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    LocaleData data;
    if (!parseLocaleFile(content, data)) {
        return false;
    }
    
    // Extract locale code from filename (e.g., "en-US.json" -> "en-US")
    if (data.info.code.empty()) {
        data.info.code = filepath.stem().string();
    }
    
    locales_[data.info.code] = std::move(data);
    return true;
}

int I18n::loadAllLocales(const std::filesystem::path& directory)
{
    if (!std::filesystem::exists(directory)) {
        return 0;
    }
    
    int count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            if (loadLocale(entry.path())) {
                count++;
            }
        }
    }
    
    return count;
}

bool I18n::setLocale(const std::string& localeCode)
{
    if (locales_.find(localeCode) == locales_.end()) {
        return false;
    }
    
    currentLocale_ = localeCode;
    
    // Notify listeners
    for (const auto& callback : localeChangedCallbacks_) {
        callback(currentLocale_);
    }
    
    return true;
}

std::vector<LocaleInfo> I18n::getAvailableLocales() const
{
    std::vector<LocaleInfo> result;
    result.reserve(locales_.size());
    
    for (const auto& [code, data] : locales_) {
        result.push_back(data.info);
    }
    
    // Sort by code
    std::sort(result.begin(), result.end(),
              [](const LocaleInfo& a, const LocaleInfo& b) {
                  return a.code < b.code;
              });
    
    return result;
}

std::string I18n::translate(const std::string& key) const
{
    // Try current locale
    std::string result = lookupKey(currentLocale_, key);
    if (!result.empty()) {
        return result;
    }
    
    // Try fallback locale
    if (currentLocale_ != fallbackLocale_) {
        result = lookupKey(fallbackLocale_, key);
        if (!result.empty()) {
            return result;
        }
    }
    
    // Return key as fallback
    return key;
}

std::string I18n::translate(const std::string& key, const std::vector<std::string>& params) const
{
    std::string result = translate(key);
    
    // Replace {0}, {1}, etc.
    for (std::size_t i = 0; i < params.size(); ++i) {
        std::string placeholder = "{" + std::to_string(i) + "}";
        std::size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), params[i]);
            pos += params[i].length();
        }
    }
    
    return result;
}

std::string I18n::translateNamed(
    const std::string& key,
    const std::unordered_map<std::string, std::string>& params) const
{
    std::string result = translate(key);
    
    // Replace {name}, {count}, etc.
    for (const auto& [name, value] : params) {
        std::string placeholder = "{" + name + "}";
        std::size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }
    
    return result;
}

bool I18n::hasKey(const std::string& key) const
{
    auto it = locales_.find(currentLocale_);
    if (it != locales_.end()) {
        return it->second.translations.find(key) != it->second.translations.end();
    }
    return false;
}

void I18n::onLocaleChanged(std::function<void(const std::string&)> callback)
{
    localeChangedCallbacks_.push_back(std::move(callback));
}

void I18n::clear()
{
    locales_.clear();
    localeChangedCallbacks_.clear();
}

std::string I18n::formatNumber(double value, int decimals) const
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(decimals) << value;
    return ss.str();
}

std::string I18n::formatPercent(double value, int decimals) const
{
    return formatNumber(value * 100.0, decimals) + "%";
}

bool I18n::isRightToLeft() const
{
    auto it = locales_.find(currentLocale_);
    if (it != locales_.end()) {
        return it->second.info.direction == "rtl";
    }
    return false;
}

std::string I18n::lookupKey(const std::string& localeCode, const std::string& key) const
{
    auto localeIt = locales_.find(localeCode);
    if (localeIt == locales_.end()) {
        return "";
    }
    
    const auto& translations = localeIt->second.translations;
    
    // Direct lookup
    auto transIt = translations.find(key);
    if (transIt != translations.end()) {
        return transIt->second;
    }
    
    // Try dotted path lookup (e.g., "menu.file.new" -> nested object)
    // For now, we flatten dotted keys in the translation file
    
    return "";
}

bool I18n::parseLocaleFile(const std::string& content, LocaleData& data)
{
    // Extract metadata
    data.info.code = json::extractString(content, "_locale");
    data.info.name = json::extractString(content, "_name");
    data.info.nativeName = json::extractString(content, "_nativeName");
    data.info.direction = json::extractString(content, "_direction");
    
    if (data.info.direction.empty()) {
        data.info.direction = "ltr";
    }
    
    // Extract all translations
    data.translations = json::extractAllStrings(content);
    
    return !data.translations.empty() || !data.info.code.empty();
}

} // namespace daw::ui::i18n
