#include "App.h"
#include "../audio/engine/EngineContext.h"

namespace daw::ui
{

const juce::String App::getApplicationName()
{
    return "DAW Project";
}

const juce::String App::getApplicationVersion()
{
    return "1.0.0";
}

bool App::moreThanOneInstanceAllowed()
{
    return false;
}

void App::initialise(const juce::String& /*commandLine*/)
{
    createMainWindow();
}

void App::shutdown()
{
    mainWindow = nullptr;
}

void App::systemRequestedQuit()
{
    quit();
}

void App::anotherInstanceStarted(const juce::String& /*commandLine*/)
{
    if (mainWindow != nullptr)
        mainWindow->toFront(true);
}

void App::createMainWindow()
{
    try
    {
        auto engineContext = std::make_shared<daw::audio::engine::EngineContext>();
        
        // Initialize engine before creating window (but don't fail if audio init fails)
        if (!engineContext->initialise())
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Audio Initialization Failed",
                "Failed to initialize audio device. The application may not work correctly.");
        }
        
        mainWindow = std::make_unique<MainWindow>(getApplicationName(), engineContext);
        if (mainWindow != nullptr)
        {
            mainWindow->setVisible(true);
        }
    }
    catch (const std::exception& e)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Application Error",
            "Failed to create main window: " + juce::String(e.what()));
    }
    catch (...)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Application Error",
            "Failed to create main window: Unknown error");
    }
}

} // namespace daw::ui

