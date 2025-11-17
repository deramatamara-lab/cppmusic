#include "MainWindow.h"
#include <juce_core/juce_core.h>

namespace daw::ui
{

MainWindow::MainWindow(const juce::String& name, std::shared_ptr<daw::audio::engine::EngineContext> engineContext)
    : juce::DocumentWindow(name, juce::Desktop::getInstance().getDefaultLookAndFeel()
                                    .findColour(juce::ResizableWindow::backgroundColourId),
                          juce::DocumentWindow::allButtons),
      engineContext(engineContext)
{
    try
    {
        lookAndFeel = std::make_unique<lookandfeel::MainLookAndFeel>();
        juce::LookAndFeel::setDefaultLookAndFeel(lookAndFeel.get());
        
        mainView = std::make_unique<views::MainView>(engineContext);
        setContentOwned(mainView.get(), true);
        
        setResizable(true, true);
        setResizeLimits(1024, 768, 10000, 10000);
        
        // Setup properties file (but don't fail if it doesn't work)
        try
        {
            propertiesOptions.applicationName = "DAWProject";
            propertiesOptions.filenameSuffix = ".settings";
            propertiesOptions.osxLibrarySubFolder = "Application Support";
            propertiesOptions.folderName = "DAWProject";
            propertiesOptions.storageFormat = juce::PropertiesFile::storeAsXML;
            
            propertiesFile = std::make_unique<juce::PropertiesFile>(propertiesOptions);
            restoreWindowBounds();
        }
        catch (...)
        {
            // If properties file fails, just use default bounds
            centreWithSize(1280, 720);
        }
        
        // Ensure window is visible and on screen
        if (getWidth() == 0 || getHeight() == 0)
        {
            centreWithSize(1280, 720);
        }
        
        setVisible(true);
        toFront(true);
    }
    catch (const std::exception& e)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Window Initialization Error",
            "Failed to initialize window: " + juce::String(e.what()));
    }
    catch (...)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Window Initialization Error",
            "Failed to initialize window: Unknown error");
    }
}

MainWindow::~MainWindow()
{
    saveWindowBounds();
    
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    lookAndFeel.reset();
    mainView.reset();
}

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

void MainWindow::restoreWindowBounds()
{
    if (propertiesFile == nullptr)
        return;
    
    auto props = propertiesFile->getXmlValue("windowBounds");
    if (props != nullptr)
    {
        auto x = props->getIntAttribute("x", 100);
        auto y = props->getIntAttribute("y", 100);
        auto w = props->getIntAttribute("w", 1280);
        auto h = props->getIntAttribute("h", 720);
        
        setBounds(x, y, w, h);
    }
    else
    {
        centreWithSize(1280, 720);
    }
}

void MainWindow::saveWindowBounds()
{
    if (propertiesFile == nullptr)
        return;
    
    auto bounds = getBounds();
    auto xml = std::make_unique<juce::XmlElement>("windowBounds");
    xml->setAttribute("x", bounds.getX());
    xml->setAttribute("y", bounds.getY());
    xml->setAttribute("w", bounds.getWidth());
    xml->setAttribute("h", bounds.getHeight());
    
    propertiesFile->setValue("windowBounds", xml.get());
    propertiesFile->saveIfNeeded();
}

} // namespace daw::ui

