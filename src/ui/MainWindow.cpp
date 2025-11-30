#include "MainWindow.h"
#include <juce_core/juce_core.h>
#include "../core/ServiceLocator.h"
#include "animation/AdaptiveAnimationService.h"
#include "../project/ProjectSerializer.h"

namespace daw::ui
{

MainWindow::MainWindow(const juce::String& name, std::shared_ptr<daw::audio::engine::EngineContext> engineContext)
    : juce::DocumentWindow(name, juce::Desktop::getInstance().getDefaultLookAndFeel()
                                    .findColour(juce::ResizableWindow::backgroundColourId),
                          juce::DocumentWindow::allButtons),
      engineContext(engineContext)
{
    // Setup autosave timer
    class AutosaveTimer : public juce::Timer
    {
    public:
        explicit AutosaveTimer(MainWindow* window) : mainWindow(window) {}
        void timerCallback() override
        {
            if (mainWindow != nullptr)
                mainWindow->performAutosave();
        }
    private:
        MainWindow* mainWindow;
    };

    autosaveTimer = std::make_unique<AutosaveTimer>(this);
    setupAutosave();
    try
    {
        lookAndFeel = std::make_unique<lookandfeel::EnhancedMainLookAndFeel>(lookandfeel::Theme::Dark);
        juce::LookAndFeel::setDefaultLookAndFeel(lookAndFeel.get());

        mainView = std::make_unique<views::MainView>(engineContext);
        mainView->setParentWindow(this); // Enable keyboard shortcuts for project operations
        mainView->setProjectName("Untitled Project"); // Initial project name

        if (auto service = core::ServiceLocator::getInstance().getService<animation::AdaptiveAnimationService>())
        {
            mainView->setAnimationService(service);
        }

        // Initialize browser with recent projects
        if (auto* browser = mainView->getBrowserPanel())
        {
            browser->setRecentProjects(getRecentProjects());
        }

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
            loadRecentProjects();
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
    if (promptSaveIfNeeded())
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
}

void MainWindow::newProject()
{
    if (!promptSaveIfNeeded())
        return;

    // Reset project model
    if (mainView != nullptr)
    {
        auto newModel = std::make_shared<daw::project::ProjectModel>();
        mainView->setProjectModel(newModel);
        mainView->setProjectName("Untitled Project");
        currentProjectPath.clear();
        markProjectClean();
    }
}

void MainWindow::openProject()
{
    if (!promptSaveIfNeeded())
        return;

    auto chooser = std::make_shared<juce::FileChooser>("Open Project", juce::File(), "*.daw");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            if (fc.getResults().isEmpty())
                return;
            openProjectFromPath(fc.getResult().getFullPathName());
        });
}

void MainWindow::openProjectFromPath(const juce::String& filePath)
{
    if (!promptSaveIfNeeded())
        return;

    juce::File file(filePath);
    if (file.existsAsFile())
    {
        daw::project::ProjectSerializer serializer;
        auto model = serializer.loadFromFile(file.getFullPathName().toStdString());
            if (model != nullptr && mainView != nullptr)
            {
                mainView->setProjectModel(std::move(model));
                currentProjectPath = file.getFullPathName();
                addToRecentProjects(currentProjectPath);
                markProjectClean();

                // Update status strip with project name
                mainView->setProjectName(file.getFileNameWithoutExtension());

                // Update browser with recent projects
                if (auto* browser = mainView->getBrowserPanel())
                {
                    browser->setRecentProjects(getRecentProjects());
                }
            }
        else
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Load Failed",
                "Failed to load project file: " + file.getFileName());
        }
    }
}

void MainWindow::saveProject()
{
    if (currentProjectPath.isEmpty())
    {
        saveProjectAs();
        return;
    }

    if (mainView != nullptr)
    {
        auto model = mainView->getProjectModel();
        if (model != nullptr)
        {
            daw::project::ProjectSerializer serializer;
            if (serializer.saveToFile(*model, currentProjectPath.toStdString()))
            {
                markProjectClean();

                // Update status strip with project name
                if (mainView != nullptr)
                {
                    juce::File file(currentProjectPath);
                    mainView->setProjectName(file.getFileNameWithoutExtension());
                }
            }
        }
    }
}

void MainWindow::saveProjectAs()
{
    auto chooser = std::make_shared<juce::FileChooser>("Save Project As", juce::File(), "*.daw");
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            if (fc.getResults().isEmpty())
                return;

            auto file = fc.getResult();
            auto filePath = file.getFullPathName();
            if (!filePath.endsWith(".daw"))
                filePath += ".daw";

            if (mainView != nullptr)
            {
                auto model = mainView->getProjectModel();
                if (model != nullptr)
                {
                    daw::project::ProjectSerializer serializer;
                    if (serializer.saveToFile(*model, filePath.toStdString()))
                    {
                        currentProjectPath = filePath;
                        addToRecentProjects(currentProjectPath);
                        markProjectClean();

                        // Update status strip with project name
                        if (mainView != nullptr)
                            mainView->setProjectName(file.getFileNameWithoutExtension());
                    }
                }
            }
        });
}

void MainWindow::addToRecentProjects(const juce::String& filePath)
{
    if (filePath.isEmpty())
        return;

    // Load current recent projects
    auto recent = getRecentProjects();

    // Remove if already exists
    recent.erase(std::remove(recent.begin(), recent.end(), filePath), recent.end());

    // Add to front
    recent.insert(recent.begin(), filePath);

    // Limit to max
    if (recent.size() > MAX_RECENT_PROJECTS)
        recent.resize(MAX_RECENT_PROJECTS);

    // Save
    if (propertiesFile != nullptr)
    {
        juce::StringArray recentArray;
        for (const auto& path : recent)
            recentArray.add(path);
        propertiesFile->setValue("recentProjects", recentArray.joinIntoString("|"));
        propertiesFile->saveIfNeeded();
    }
}

std::vector<juce::String> MainWindow::getRecentProjects() const
{
    std::vector<juce::String> recent;
    if (propertiesFile != nullptr)
    {
        const auto recentStr = propertiesFile->getValue("recentProjects");
        if (recentStr.isNotEmpty())
        {
            juce::StringArray tokens;
            tokens.addTokens(recentStr, "|", "");
            for (const auto& token : tokens)
            {
                if (token.isNotEmpty())
                    recent.push_back(token);
            }
        }
    }
    return recent;
}

void MainWindow::loadRecentProjects()
{
    // Recent projects are loaded on-demand via getRecentProjects()
    juce::ignoreUnused(this);
}

void MainWindow::saveRecentProjects()
{
    // Recent projects are saved immediately when added
    juce::ignoreUnused(this);
}

void MainWindow::setupAutosave()
{
    // Autosave every 5 minutes (300000 ms)
    autosaveTimer->startTimer(300000);
}

void MainWindow::performAutosave()
{
    if (!projectHasUnsavedChanges || currentProjectPath.isEmpty())
        return;

    // Create autosave path
    juce::File projectFile(currentProjectPath);
    if (projectFile.exists())
    {
        auto autosavePath = projectFile.getParentDirectory().getChildFile(
            projectFile.getFileNameWithoutExtension() + "_autosave.daw");

        if (mainView != nullptr)
        {
            auto model = mainView->getProjectModel();
            if (model != nullptr)
            {
                daw::project::ProjectSerializer serializer;
                serializer.saveToFile(*model, autosavePath.getFullPathName().toStdString());
            }
        }
    }
}

bool MainWindow::promptSaveIfNeeded()
{
    if (!projectHasUnsavedChanges)
        return true;

    const auto result = juce::AlertWindow::showYesNoCancelBox(
        juce::AlertWindow::QuestionIcon,
        "Unsaved Changes",
        "You have unsaved changes. Do you want to save?",
        "Save",
        "Don't Save",
        "Cancel",
        nullptr,
        nullptr);

    if (result == 1) // Yes - Save
    {
        saveProject();
        return true;
    }
    else if (result == 2) // No - Don't Save
    {
        return true;
    }

    return false; // Cancel
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
