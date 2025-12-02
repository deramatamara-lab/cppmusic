/**
 * @file AppState.hpp
 * @brief Central non-audio state management for the UI layer.
 *
 * Part of the FL-style DAW UI implementation.
 * Manages application-wide UI state including:
 * - Active project reference
 * - Current view (playlist, piano roll, mixer, channel rack)
 * - Global UI settings (theme, zoom levels, snap options)
 * - Notification hooks for components
 */

#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace cppmusic::ui::core {

/**
 * @brief Available main views in the DAW
 */
enum class ViewType {
    Playlist,     ///< Arrangement/timeline view
    PianoRoll,    ///< MIDI note editing view
    Mixer,        ///< Mixing console view
    ChannelRack,  ///< FL-style channel rack / step sequencer
    Devices       ///< Plugin/device chain view
};

/**
 * @brief Theme variants for the UI
 */
enum class ThemeVariant {
    Dark,      ///< Default dark theme (NI/iZotope-grade)
    Light,     ///< Light theme variant
    HighContrast  ///< High contrast for accessibility
};

/**
 * @brief Snap grid options for editing
 */
enum class SnapOption {
    Off,
    Beat,
    HalfBeat,
    Quarter,
    Eighth,
    Sixteenth,
    ThirtySecond,
    Triplet,
    Dotted,
    Custom
};

/**
 * @brief Listener interface for state changes
 */
class AppStateListener {
public:
    virtual ~AppStateListener() = default;
    virtual void onViewChanged(ViewType newView) { (void)newView; }
    virtual void onThemeChanged(ThemeVariant newTheme) { (void)newTheme; }
    virtual void onZoomChanged(double newZoomLevel) { (void)newZoomLevel; }
    virtual void onSnapChanged(SnapOption newSnap) { (void)newSnap; }
    virtual void onProjectChanged() {}
    virtual void onTransportStateChanged(bool isPlaying) { (void)isPlaying; }
};

/**
 * @brief Central UI state management singleton
 *
 * Thread-safe state management for the UI layer.
 * All state changes notify registered listeners.
 *
 * Usage:
 * @code
 * auto& state = AppState::getInstance();
 * state.addListener(myComponent);
 * state.setCurrentView(ViewType::PianoRoll);
 * @endcode
 */
class AppState {
public:
    /**
     * @brief Get singleton instance
     */
    static AppState& getInstance();

    // Disable copy/move
    AppState(const AppState&) = delete;
    AppState& operator=(const AppState&) = delete;
    AppState(AppState&&) = delete;
    AppState& operator=(AppState&&) = delete;

    // =========================================================================
    // Listener Management
    // =========================================================================

    /**
     * @brief Add a listener for state changes
     * @param listener Pointer to listener (must outlive registration or call removeListener in destructor)
     * @note Listeners should call removeListener() before destruction to prevent use-after-free.
     *       Consider using RAII wrappers (e.g., juce::Component which already removes itself on destruction).
     */
    void addListener(AppStateListener* listener);

    /**
     * @brief Remove a listener
     * @param listener Pointer to listener to remove
     * @note Safe to call multiple times or with nullptr
     */
    void removeListener(AppStateListener* listener);

    // =========================================================================
    // View Management
    // =========================================================================

    /**
     * @brief Get the current active view
     */
    [[nodiscard]] ViewType getCurrentView() const noexcept;

    /**
     * @brief Set the current active view
     */
    void setCurrentView(ViewType view);

    // =========================================================================
    // Theme Management
    // =========================================================================

    /**
     * @brief Get the current theme variant
     */
    [[nodiscard]] ThemeVariant getThemeVariant() const noexcept;

    /**
     * @brief Set the theme variant
     */
    void setThemeVariant(ThemeVariant variant);

    // =========================================================================
    // Zoom Settings
    // =========================================================================

    /**
     * @brief Get horizontal zoom level (pixels per beat)
     */
    [[nodiscard]] double getHorizontalZoom() const noexcept;

    /**
     * @brief Set horizontal zoom level
     * @param pixelsPerBeat Zoom level in pixels per beat
     */
    void setHorizontalZoom(double pixelsPerBeat);

    /**
     * @brief Get vertical zoom level (track height multiplier)
     */
    [[nodiscard]] double getVerticalZoom() const noexcept;

    /**
     * @brief Set vertical zoom level
     */
    void setVerticalZoom(double multiplier);

    // =========================================================================
    // Snap Settings
    // =========================================================================

    /**
     * @brief Get the current snap setting
     */
    [[nodiscard]] SnapOption getSnapOption() const noexcept;

    /**
     * @brief Set the snap setting
     */
    void setSnapOption(SnapOption snap);

    /**
     * @brief Check if snap is enabled (not SnapOption::Off)
     */
    [[nodiscard]] bool isSnapEnabled() const noexcept;

    /**
     * @brief Get the custom snap value in beats
     */
    [[nodiscard]] double getCustomSnapBeats() const noexcept;

    /**
     * @brief Set a custom snap value
     */
    void setCustomSnapBeats(double beats);

    // =========================================================================
    // Project State
    // =========================================================================

    /**
     * @brief Get the current project name
     */
    [[nodiscard]] std::string getProjectName() const;

    /**
     * @brief Set the project name
     */
    void setProjectName(const std::string& name);

    /**
     * @brief Check if the project has unsaved changes
     */
    [[nodiscard]] bool hasUnsavedChanges() const noexcept;

    /**
     * @brief Mark the project as having unsaved changes
     */
    void markDirty();

    /**
     * @brief Mark the project as clean (saved)
     */
    void markClean();

    /**
     * @brief Notify listeners that the project has changed
     */
    void notifyProjectChanged();

    // =========================================================================
    // Transport State
    // =========================================================================

    /**
     * @brief Notify listeners of transport state change
     */
    void notifyTransportStateChanged(bool isPlaying);

private:
    AppState();
    ~AppState() = default;

    mutable std::mutex mutex_;
    std::vector<AppStateListener*> listeners_;

    ViewType currentView_{ViewType::Playlist};
    ThemeVariant themeVariant_{ThemeVariant::Dark};
    double horizontalZoom_{50.0};  // pixels per beat
    double verticalZoom_{1.0};     // track height multiplier
    SnapOption snapOption_{SnapOption::Sixteenth};
    double customSnapBeats_{0.25};
    std::string projectName_{"Untitled"};
    bool hasUnsavedChanges_{false};

    void notifyViewChanged(ViewType view);
    void notifyThemeChanged(ThemeVariant variant);
    void notifyZoomChanged(double zoom);
    void notifySnapChanged(SnapOption snap);
};

} // namespace cppmusic::ui::core
