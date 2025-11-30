#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_data_structures/juce_data_structures.h>
#include "lookandfeel/EnhancedMainLookAndFeel.h"
#include "views/MainView.h"
#include "../audio/engine/EngineContext.h"
#include <memory>

namespace daw::ui
{

namespace services {
class AdaptiveAnimationService;
}

/**
 * @brief Main application window
 *
 * DocumentWindow that hosts the main DAW view.
 * Manages window bounds persistence, CustomLookAndFeel, and high-DPI support.
 * Follows DAW_DEV_RULES: responsive, professional UX, proper lifecycle.
 */
class MainWindow : public juce::DocumentWindow
{
public:
    MainWindow(const juce::String& name, std::shared_ptr<daw::audio::engine::EngineContext> engineContext);
    ~MainWindow() override;

    void closeButtonPressed() override;

    // Project management
    void newProject();
    void openProject();
    void openProjectFromPath(const juce::String& filePath);
    void saveProject();
    void saveProjectAs();
    bool hasUnsavedChanges() const { return projectHasUnsavedChanges; }
    void markProjectDirty() { projectHasUnsavedChanges = true; }
    void markProjectClean() { projectHasUnsavedChanges = false; }

    // Recent projects
    static constexpr int MAX_RECENT_PROJECTS = 10;
    void addToRecentProjects(const juce::String& filePath);
    std::vector<juce::String> getRecentProjects() const;

private:
    std::unique_ptr<lookandfeel::EnhancedMainLookAndFeel> lookAndFeel;
    std::shared_ptr<daw::audio::engine::EngineContext> engineContext;
    std::unique_ptr<views::MainView> mainView;
    std::shared_ptr<services::AdaptiveAnimationService> animationService;

    juce::PropertiesFile::Options propertiesOptions;
    std::unique_ptr<juce::PropertiesFile> propertiesFile;

    // Project state
    juce::String currentProjectPath;
    bool projectHasUnsavedChanges{false};
    std::unique_ptr<juce::Timer> autosaveTimer;

    void restoreWindowBounds();
    void saveWindowBounds();
    void setupAutosave();
    void performAutosave();
    bool promptSaveIfNeeded();
    void loadRecentProjects();
    void saveRecentProjects();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

} // namespace daw::ui
