#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "MainComponent.h"

namespace daw
{

/**
 * @brief Main JUCE Application class
 * 
 * Manages the application lifecycle and main window
 */
class MainApplication : public juce::JUCEApplication
{
public:
    MainApplication() = default;
    ~MainApplication() override = default;

    const juce::String getApplicationName() override { return "DAW Project"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const juce::String& commandLine) override;
    void shutdown() override;

    void systemRequestedQuit() override;
    void anotherInstanceStarted(const juce::String& commandLine) override;

private:
    std::unique_ptr<juce::DocumentWindow> mainWindow;
    std::unique_ptr<MainComponent> mainComponent;
    
    void createMainWindow();
};

} // namespace daw

