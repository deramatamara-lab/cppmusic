#include "MainApplication.h"

namespace daw
{

void MainApplication::initialise(const juce::String& /*commandLine*/)
{
    createMainWindow();
}

void MainApplication::shutdown()
{
    mainWindow = nullptr;
    mainComponent = nullptr;
}

void MainApplication::systemRequestedQuit()
{
    quit();
}

void MainApplication::anotherInstanceStarted(const juce::String& /*commandLine*/)
{
    // Bring existing window to front
    if (mainWindow != nullptr)
        mainWindow->toFront(true);
}

void MainApplication::createMainWindow()
{
    mainComponent = std::make_unique<MainComponent>();
    
    mainWindow = std::make_unique<juce::DocumentWindow>(
        getApplicationName(),
        juce::Desktop::getInstance().getDefaultLookAndFeel()
            .findColour(juce::ResizableWindow::backgroundColourId),
        juce::DocumentWindow::allButtons,
        true);
    
    mainWindow->setUsingNativeTitleBar(true);
    mainWindow->setContentOwned(mainComponent.get(), true);
    mainWindow->setResizable(true, true);
    mainWindow->setResizeLimits(800, 600, 10000, 10000);
    
    // Center window on screen
    mainWindow->centreWithSize(1200, 800);
    mainWindow->setVisible(true);
}

} // namespace daw

