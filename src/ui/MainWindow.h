#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_data_structures/juce_data_structures.h>
#include "lookandfeel/MainLookAndFeel.h"
#include "views/MainView.h"
#include "../../audio/engine/EngineContext.h"
#include <memory>

namespace daw::ui
{

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

private:
    std::unique_ptr<lookandfeel::MainLookAndFeel> lookAndFeel;
    std::shared_ptr<daw::audio::engine::EngineContext> engineContext;
    std::unique_ptr<views::MainView> mainView;
    
    juce::PropertiesFile::Options propertiesOptions;
    std::unique_ptr<juce::PropertiesFile> propertiesFile;
    
    void restoreWindowBounds();
    void saveWindowBounds();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

} // namespace daw::ui

