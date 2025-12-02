/**
 * @file UIScaleManager.cpp
 * @brief Implementation of UIScaleManager
 */

#include "UIScaleManager.hpp"

namespace cppmusic::ui::style {

UIScaleManager& UIScaleManager::getInstance() {
    static UIScaleManager instance;
    return instance;
}

UIScaleManager::UIScaleManager() {
    // Initialize with recommended scale for current display
    auto recommendedScale = getRecommendedScale();
    globalScale_.store(recommendedScale);
}

void UIScaleManager::setGlobalScale(float scale) {
    // Clamp to reasonable range
    scale = juce::jlimit(0.5f, 3.0f, scale);
    
    auto oldScale = globalScale_.exchange(scale);
    if (std::abs(oldScale - scale) > 0.01f) {
        notifyListeners();
    }
}

float UIScaleManager::getRecommendedScale() const {
    // Get scale from primary display
    auto* mainDisplay = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
    if (mainDisplay != nullptr) {
        auto dpi = mainDisplay->dpi;
        
        // Standard DPI is 96 on Windows, 72 on macOS
        #if JUCE_MAC
            const float standardDPI = 72.0f;
        #else
            const float standardDPI = 96.0f;
        #endif
        
        auto scale = dpi / standardDPI;
        
        // Round to nearest common scale factor
        float bestScale = 1.0f;
        float minDiff = std::abs(scale - 1.0f);
        
        for (auto s : SCALE_FACTORS) {
            auto diff = std::abs(scale - s);
            if (diff < minDiff) {
                minDiff = diff;
                bestScale = s;
            }
        }
        
        return bestScale;
    }
    
    return 1.0f;
}

void UIScaleManager::cycleScale() {
    auto currentScale = globalScale_.load();
    
    // Find next scale factor
    float nextScale = SCALE_FACTORS[0];
    for (int i = 0; i < NUM_SCALE_FACTORS - 1; ++i) {
        if (std::abs(currentScale - SCALE_FACTORS[i]) < 0.01f) {
            nextScale = SCALE_FACTORS[i + 1];
            break;
        }
    }
    
    setGlobalScale(nextScale);
}

void UIScaleManager::addListener(Listener* listener) {
    listeners_.add(listener);
}

void UIScaleManager::removeListener(Listener* listener) {
    listeners_.remove(listener);
}

void UIScaleManager::notifyListeners() {
    auto scale = globalScale_.load();
    listeners_.call([scale](Listener& l) { l.uiScaleChanged(scale); });
}

} // namespace cppmusic::ui::style
