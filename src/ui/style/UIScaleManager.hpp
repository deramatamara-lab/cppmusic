/**
 * @file UIScaleManager.hpp
 * @brief Global UI scaling for HiDPI and accessibility
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>

namespace cppmusic::ui::style {

/**
 * @brief Global UI scale manager (singleton)
 * 
 * Provides centralized UI scaling for:
 * - HiDPI displays (100%, 150%, 200%)
 * - User accessibility preferences
 * - Consistent scaling across all components
 */
class UIScaleManager {
public:
    static UIScaleManager& getInstance();
    
    /**
     * @brief Set global UI scale factor
     * @param scale Scale factor (1.0 = 100%, 1.5 = 150%, 2.0 = 200%)
     */
    void setGlobalScale(float scale);
    
    /**
     * @brief Get current global scale factor
     */
    float getGlobalScale() const { return globalScale_.load(); }
    
    /**
     * @brief Scale a value by the global scale factor
     */
    float scale(float value) const {
        return value * globalScale_.load();
    }
    
    int scale(int value) const {
        return static_cast<int>(value * globalScale_.load() + 0.5f);
    }
    
    /**
     * @brief Scale a font size
     */
    float scaleFontSize(float size) const {
        return size * globalScale_.load();
    }
    
    /**
     * @brief Scale a rectangle
     */
    juce::Rectangle<float> scale(const juce::Rectangle<float>& rect) const {
        auto s = globalScale_.load();
        return {rect.getX() * s, rect.getY() * s,
                rect.getWidth() * s, rect.getHeight() * s};
    }
    
    juce::Rectangle<int> scale(const juce::Rectangle<int>& rect) const {
        auto s = globalScale_.load();
        return {static_cast<int>(rect.getX() * s + 0.5f),
                static_cast<int>(rect.getY() * s + 0.5f),
                static_cast<int>(rect.getWidth() * s + 0.5f),
                static_cast<int>(rect.getHeight() * s + 0.5f)};
    }
    
    /**
     * @brief Get recommended scale for current display
     */
    float getRecommendedScale() const;
    
    /**
     * @brief Cycle through common scale factors
     */
    void cycleScale();
    
    /**
     * @brief Register a listener for scale changes
     */
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void uiScaleChanged(float newScale) = 0;
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    UIScaleManager();
    ~UIScaleManager() = default;
    UIScaleManager(const UIScaleManager&) = delete;
    UIScaleManager& operator=(const UIScaleManager&) = delete;
    
    void notifyListeners();
    
    std::atomic<float> globalScale_{1.0f};
    juce::ListenerList<Listener> listeners_;
    
    static constexpr float SCALE_FACTORS[] = {1.0f, 1.25f, 1.5f, 1.75f, 2.0f};
    static constexpr int NUM_SCALE_FACTORS = 5;
};

/**
 * @brief RAII helper to apply scaled graphics context
 */
class ScopedScaledGraphics {
public:
    ScopedScaledGraphics(juce::Graphics& g, float scale)
        : graphics_(g)
        , scale_(scale)
    {
        if (scale_ != 1.0f) {
            graphics_.saveState();
            graphics_.addTransform(juce::AffineTransform::scale(scale_));
        }
    }
    
    ~ScopedScaledGraphics() {
        if (scale_ != 1.0f) {
            graphics_.restoreState();
        }
    }
    
private:
    juce::Graphics& graphics_;
    float scale_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScopedScaledGraphics)
};

} // namespace cppmusic::ui::style
