/**
 * @file IconManager.cpp
 * @brief Implementation of IconManager
 */

#include "IconManager.hpp"

namespace cppmusic::ui::style {

IconManager& IconManager::getInstance() {
    static IconManager instance;
    return instance;
}

void IconManager::setIconPath(const juce::File& path) {
    iconPath_ = path;
    cachedIcons_.clear(); // Clear cache when path changes
}

juce::String IconManager::getIconFilename(IconType type) const {
    switch (type) {
        case IconType::Play:        return "play.svg";
        case IconType::Stop:        return "stop.svg";
        case IconType::Record:      return "record.svg";
        case IconType::Loop:        return "loop.svg";
        case IconType::Pause:       return "pause.svg";
        case IconType::Select:      return "select.svg";
        case IconType::Draw:        return "draw.svg";
        case IconType::Slice:       return "slice.svg";
        case IconType::Eraser:      return "eraser.svg";
        case IconType::Mute:        return "mute.svg";
        case IconType::Solo:        return "solo.svg";
        case IconType::Arm:         return "arm.svg";
        case IconType::Browser:     return "browser.svg";
        case IconType::Pattern:     return "pattern.svg";
        case IconType::Playlist:    return "playlist.svg";
        case IconType::Mixer:       return "mixer.svg";
        case IconType::PianoRoll:   return "pianoroll.svg";
        case IconType::Settings:    return "settings.svg";
        case IconType::Save:        return "save.svg";
        case IconType::Load:        return "load.svg";
        case IconType::Export:      return "export.svg";
        case IconType::Close:       return "close.svg";
        case IconType::Menu:        return "menu.svg";
        case IconType::Cut:         return "cut.svg";
        case IconType::Copy:        return "copy.svg";
        case IconType::Paste:       return "paste.svg";
        case IconType::Delete:      return "delete.svg";
        case IconType::Undo:        return "undo.svg";
        case IconType::Redo:        return "redo.svg";
        default:                    return "";
    }
}

juce::String IconManager::getSVGData(IconType type) const {
    // Fallback built-in SVG icons (simple geometric shapes)
    // These are placeholders - real icons should be in assets/icons/
    
    switch (type) {
        case IconType::Play:
            return R"(<svg viewBox="0 0 24 24"><path d="M8 5v14l11-7z" fill="currentColor"/></svg>)";
        
        case IconType::Stop:
            return R"(<svg viewBox="0 0 24 24"><rect x="6" y="6" width="12" height="12" fill="currentColor"/></svg>)";
        
        case IconType::Record:
            return R"(<svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="8" fill="currentColor"/></svg>)";
        
        case IconType::Loop:
            return R"(<svg viewBox="0 0 24 24"><path d="M7 7h10v3l4-4-4-4v3H5v6h2V7zm10 10H7v-3l-4 4 4 4v-3h12v-6h-2v4z" fill="currentColor"/></svg>)";
        
        case IconType::Pause:
            return R"(<svg viewBox="0 0 24 24"><path d="M6 5h4v14H6V5zm8 0h4v14h-4V5z" fill="currentColor"/></svg>)";
        
        case IconType::Mute:
            return R"(<svg viewBox="0 0 24 24"><path d="M16.5 12c0-1.77-1.02-3.29-2.5-4.03v2.21l2.45 2.45c.03-.2.05-.41.05-.63zm2.5 0c0 .94-.2 1.82-.54 2.64l1.51 1.51C20.63 14.91 21 13.5 21 12c0-4.28-2.99-7.86-7-8.77v2.06c2.89.86 5 3.54 5 6.71zM4.27 3L3 4.27 7.73 9H3v6h4l5 5v-6.73l4.25 4.25c-.67.52-1.42.93-2.25 1.18v2.06c1.38-.31 2.63-.95 3.69-1.81L19.73 21 21 19.73l-9-9L4.27 3zM12 4L9.91 6.09 12 8.18V4z" fill="currentColor"/></svg>)";
        
        case IconType::Solo:
            return R"(<svg viewBox="0 0 24 24"><path d="M12 3v9.28c-.47-.17-.97-.28-1.5-.28C8.01 12 6 14.01 6 16.5S8.01 21 10.5 21c2.31 0 4.2-1.75 4.45-4H15V6h4V3h-7z" fill="currentColor"/></svg>)";
        
        case IconType::Close:
            return R"(<svg viewBox="0 0 24 24"><path d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z" fill="currentColor"/></svg>)";
        
        case IconType::Menu:
            return R"(<svg viewBox="0 0 24 24"><path d="M3 18h18v-2H3v2zm0-5h18v-2H3v2zm0-7v2h18V6H3z" fill="currentColor"/></svg>)";
        
        case IconType::Settings:
            return R"(<svg viewBox="0 0 24 24"><path d="M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58c.18-.14.23-.41.12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94L14.4 2.81c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.47.12.61l2.03 1.58c-.05.3-.09.63-.09.94s.02.64.07.94l-2.03 1.58c-.18.14-.23.41-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z" fill="currentColor"/></svg>)";
        
        default:
            // Generic placeholder icon
            return R"(<svg viewBox="0 0 24 24"><rect x="4" y="4" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2"/></svg>)";
    }
}

std::unique_ptr<juce::Drawable> IconManager::getIcon(IconType type, float size, juce::Colour color) {
    // Try to load from file first
    if (iconPath_.exists() && iconPath_.isDirectory()) {
        auto iconFile = iconPath_.getChildFile(getIconFilename(type));
        if (iconFile.existsAsFile()) {
            auto drawable = juce::Drawable::createFromImageFile(iconFile);
            if (drawable != nullptr) {
                drawable->replaceColour(juce::Colours::black, color);
                drawable->setTransformToFit(juce::Rectangle<float>(0, 0, size, size), 
                                           juce::RectanglePlacement::centred);
                return drawable;
            }
        }
    }
    
    // Fallback to built-in SVG
    auto svgData = getSVGData(type);
    auto drawable = juce::Drawable::createFromSVG(*juce::parseXML(svgData));
    if (drawable != nullptr) {
        drawable->replaceColour(juce::Colours::black, color);
        drawable->setTransformToFit(juce::Rectangle<float>(0, 0, size, size),
                                   juce::RectanglePlacement::centred);
    }
    
    return drawable;
}

void IconManager::drawIcon(juce::Graphics& g, IconType type, 
                          juce::Rectangle<float> bounds, juce::Colour color) {
    auto icon = getIcon(type, bounds.getWidth(), color);
    if (icon != nullptr) {
        icon->drawWithin(g, bounds, juce::RectanglePlacement::centred, 1.0f);
    }
}

} // namespace cppmusic::ui::style
