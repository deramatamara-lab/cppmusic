/**
 * @file IconManager.hpp
 * @brief Centralized icon management for the DAW UI
 *
 * Provides a unified system for loading and managing SVG icons with:
 * - Consistent icon style across the application
 * - HiDPI support with vector scaling
 * - Cached icon instances for performance
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <unordered_map>

namespace cppmusic::ui::style {

/**
 * @brief Icon identifiers
 */
enum class IconType {
    // Transport controls
    Play,
    Stop,
    Record,
    Loop,
    Pause,
    
    // Tools
    Select,
    Draw,
    Slice,
    Eraser,
    
    // Mixer/Channel controls
    Mute,
    Solo,
    Arm,
    
    // View controls
    Browser,
    Pattern,
    Playlist,
    Mixer,
    PianoRoll,
    
    // General UI
    Settings,
    Save,
    Load,
    Export,
    Close,
    Menu,
    
    // Edit tools
    Cut,
    Copy,
    Paste,
    Delete,
    Undo,
    Redo
};

/**
 * @brief Icon manager singleton for loading and caching icons
 */
class IconManager {
public:
    /**
     * @brief Get singleton instance
     */
    static IconManager& getInstance();
    
    /**
     * @brief Load an icon by type
     * @param type Icon type
     * @param size Icon size in pixels
     * @param color Optional color override
     * @return Drawable icon, or nullptr if not found
     */
    std::unique_ptr<juce::Drawable> getIcon(IconType type, 
                                            float size = 24.0f,
                                            juce::Colour color = juce::Colours::white);
    
    /**
     * @brief Draw an icon directly to graphics context
     * @param g Graphics context
     * @param type Icon type
     * @param bounds Bounds to draw within
     * @param color Icon color
     */
    void drawIcon(juce::Graphics& g, IconType type, 
                  juce::Rectangle<float> bounds,
                  juce::Colour color = juce::Colours::white);
    
    /**
     * @brief Set the base path for icon assets
     */
    void setIconPath(const juce::File& path);
    
private:
    IconManager() = default;
    ~IconManager() = default;
    IconManager(const IconManager&) = delete;
    IconManager& operator=(const IconManager&) = delete;
    
    /**
     * @brief Get SVG data for an icon type (fallback to built-in icons)
     */
    juce::String getSVGData(IconType type) const;
    
    /**
     * @brief Get filename for an icon type
     */
    juce::String getIconFilename(IconType type) const;
    
    juce::File iconPath_;
    std::unordered_map<IconType, std::unique_ptr<juce::Drawable>> cachedIcons_;
};

} // namespace cppmusic::ui::style
