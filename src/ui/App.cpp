#include "App.h"
#include "../audio/engine/EngineContext.h"
#include "../core/ServiceLocator.h"
#include "animation/AdaptiveAnimationService.h"

namespace daw::ui
{

const juce::String App::getApplicationName()
{
    return "DAW Project";
}

void App::configureServices()
{
    auto& locator = core::ServiceLocator::getInstance();
    locator.initializeServices();

    if (locator.getFeatureFlag("animation"))
    {
        auto animationService = std::make_shared<animation::AdaptiveAnimationService>();
        if (!animationService->initialize())
        {
            juce::Logger::writeToLog("AdaptiveAnimationService failed to initialize; GPU animations disabled");
        }
        else
        {
            locator.registerService<animation::AdaptiveAnimationService>(animationService);
        }
    }
}

void App::shutdownServices()
{
    auto& locator = core::ServiceLocator::getInstance();
    if (auto animationService = locator.getService<animation::AdaptiveAnimationService>())
    {
        animationService->shutdown();
        locator.unregisterService<animation::AdaptiveAnimationService>();
    }

    locator.shutdownServices();
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
    configureServices();
    createMainWindow();
}

void App::shutdown()
{
    shutdownServices();
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

