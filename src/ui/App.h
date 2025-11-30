#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "MainWindow.h"

namespace daw::ui
{

/**
 * @brief Main JUCE Application class
 *
 * Manages the application lifecycle and main window creation.
 * Follows DAW_DEV_RULES: single-instance, clean shutdown, proper initialization.
 */
class App : public juce::JUCEApplication
{
public:
    App() = default;
    ~App() override = default;

    const juce::String getApplicationName() override;
    const juce::String getApplicationVersion() override;
    bool moreThanOneInstanceAllowed() override;

    void initialise(const juce::String& commandLine) override;
    void shutdown() override;

    void systemRequestedQuit() override;
    void anotherInstanceStarted(const juce::String& commandLine) override;

private:
    std::unique_ptr<MainWindow> mainWindow;

    void createMainWindow();
    void configureServices();
    void shutdownServices();
};

} // namespace daw::ui
