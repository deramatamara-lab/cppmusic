/**
 * @file AppState.cpp
 * @brief Implementation of central UI state management.
 */

#include "AppState.hpp"
#include <algorithm>

namespace cppmusic::ui::core {

AppState& AppState::getInstance() {
    static AppState instance;
    return instance;
}

AppState::AppState() = default;

// =========================================================================
// Listener Management
// =========================================================================

void AppState::addListener(AppStateListener* listener) {
    if (listener == nullptr) return;
    
    std::lock_guard lock(mutex_);
    if (std::find(listeners_.begin(), listeners_.end(), listener) == listeners_.end()) {
        listeners_.push_back(listener);
    }
}

void AppState::removeListener(AppStateListener* listener) {
    std::lock_guard lock(mutex_);
    listeners_.erase(
        std::remove(listeners_.begin(), listeners_.end(), listener),
        listeners_.end()
    );
}

// =========================================================================
// View Management
// =========================================================================

ViewType AppState::getCurrentView() const noexcept {
    std::lock_guard lock(mutex_);
    return currentView_;
}

void AppState::setCurrentView(ViewType view) {
    {
        std::lock_guard lock(mutex_);
        if (currentView_ == view) return;
        currentView_ = view;
    }
    notifyViewChanged(view);
}

// =========================================================================
// Theme Management
// =========================================================================

ThemeVariant AppState::getThemeVariant() const noexcept {
    std::lock_guard lock(mutex_);
    return themeVariant_;
}

void AppState::setThemeVariant(ThemeVariant variant) {
    {
        std::lock_guard lock(mutex_);
        if (themeVariant_ == variant) return;
        themeVariant_ = variant;
    }
    notifyThemeChanged(variant);
}

// =========================================================================
// Zoom Settings
// =========================================================================

double AppState::getHorizontalZoom() const noexcept {
    std::lock_guard lock(mutex_);
    return horizontalZoom_;
}

void AppState::setHorizontalZoom(double pixelsPerBeat) {
    {
        std::lock_guard lock(mutex_);
        if (horizontalZoom_ == pixelsPerBeat) return;
        horizontalZoom_ = pixelsPerBeat;
    }
    notifyZoomChanged(pixelsPerBeat);
}

double AppState::getVerticalZoom() const noexcept {
    std::lock_guard lock(mutex_);
    return verticalZoom_;
}

void AppState::setVerticalZoom(double multiplier) {
    std::lock_guard lock(mutex_);
    verticalZoom_ = multiplier;
}

// =========================================================================
// Snap Settings
// =========================================================================

SnapOption AppState::getSnapOption() const noexcept {
    std::lock_guard lock(mutex_);
    return snapOption_;
}

void AppState::setSnapOption(SnapOption snap) {
    {
        std::lock_guard lock(mutex_);
        if (snapOption_ == snap) return;
        snapOption_ = snap;
    }
    notifySnapChanged(snap);
}

bool AppState::isSnapEnabled() const noexcept {
    std::lock_guard lock(mutex_);
    return snapOption_ != SnapOption::Off;
}

double AppState::getCustomSnapBeats() const noexcept {
    std::lock_guard lock(mutex_);
    return customSnapBeats_;
}

void AppState::setCustomSnapBeats(double beats) {
    std::lock_guard lock(mutex_);
    customSnapBeats_ = beats;
}

// =========================================================================
// Project State
// =========================================================================

std::string AppState::getProjectName() const {
    std::lock_guard lock(mutex_);
    return projectName_;
}

void AppState::setProjectName(const std::string& name) {
    std::lock_guard lock(mutex_);
    projectName_ = name;
}

bool AppState::hasUnsavedChanges() const noexcept {
    std::lock_guard lock(mutex_);
    return hasUnsavedChanges_;
}

void AppState::markDirty() {
    std::lock_guard lock(mutex_);
    hasUnsavedChanges_ = true;
}

void AppState::markClean() {
    std::lock_guard lock(mutex_);
    hasUnsavedChanges_ = false;
}

void AppState::notifyProjectChanged() {
    std::vector<AppStateListener*> listenersCopy;
    {
        std::lock_guard lock(mutex_);
        listenersCopy = listeners_;
    }
    for (auto* listener : listenersCopy) {
        listener->onProjectChanged();
    }
}

// =========================================================================
// Transport State
// =========================================================================

void AppState::notifyTransportStateChanged(bool isPlaying) {
    std::vector<AppStateListener*> listenersCopy;
    {
        std::lock_guard lock(mutex_);
        listenersCopy = listeners_;
    }
    for (auto* listener : listenersCopy) {
        listener->onTransportStateChanged(isPlaying);
    }
}

// =========================================================================
// Private Notification Methods
// =========================================================================

void AppState::notifyViewChanged(ViewType view) {
    std::vector<AppStateListener*> listenersCopy;
    {
        std::lock_guard lock(mutex_);
        listenersCopy = listeners_;
    }
    for (auto* listener : listenersCopy) {
        listener->onViewChanged(view);
    }
}

void AppState::notifyThemeChanged(ThemeVariant variant) {
    std::vector<AppStateListener*> listenersCopy;
    {
        std::lock_guard lock(mutex_);
        listenersCopy = listeners_;
    }
    for (auto* listener : listenersCopy) {
        listener->onThemeChanged(variant);
    }
}

void AppState::notifyZoomChanged(double zoom) {
    std::vector<AppStateListener*> listenersCopy;
    {
        std::lock_guard lock(mutex_);
        listenersCopy = listeners_;
    }
    for (auto* listener : listenersCopy) {
        listener->onZoomChanged(zoom);
    }
}

void AppState::notifySnapChanged(SnapOption snap) {
    std::vector<AppStateListener*> listenersCopy;
    {
        std::lock_guard lock(mutex_);
        listenersCopy = listeners_;
    }
    for (auto* listener : listenersCopy) {
        listener->onSnapChanged(snap);
    }
}

} // namespace cppmusic::ui::core
