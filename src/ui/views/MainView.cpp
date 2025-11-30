#include "MainView.h"
#include "../MainWindow.h"
#include "../lookandfeel/DesignSystem.h"
#include "../components/CommandPalette.h"
#include "../../ai/inference/InferenceEngine.h"
#include "../../ai/config/AIConfig.h"
#include "../../core/utilities/Logger.h"
#include "../animation/AdaptiveAnimationService.h"
#include <juce_gui_extra/juce_gui_extra.h>

namespace daw::ui::views
{

//=========================== ThemedResizerBar Implementation ==========================

ThemedResizerBar::ThemedResizerBar(juce::StretchableLayoutManager* layoutToUse,
                                     int itemIndexInLayout,
                                     bool isBarVertical,
                                     MainView* parentView)
    : juce::StretchableLayoutResizerBar(layoutToUse, itemIndexInLayout, isBarVertical)
    , mainView(parentView)
    , isBarVerticalMember(isBarVertical)
{
    setMouseCursor(isBarVertical ? juce::MouseCursor::LeftRightResizeCursor
                                 : juce::MouseCursor::UpDownResizeCursor);
    setRepaintsOnMouseActivity(true);
}

MainView::~MainView()
{
    if (drumMachineDeviceManager != nullptr)
    {
        drumMachine.detachFromDeviceManager();
        drumMachineDeviceManager = nullptr;
    }
}

void ThemedResizerBar::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    auto bg = juce::Colour(Colors::surface);
    auto grip = juce::Colour(Colors::outline);
    auto accent = juce::Colour(Colors::accent);

    g.fillAll(bg.withAlpha(0.9f));

    auto r = getLocalBounds().toFloat();
    const bool vertical = isVertical();
    auto mid = vertical ? r.getCentreX() : r.getCentreY();
    auto length = (vertical ? r.getHeight() : r.getWidth()) - 8.0f;

    juce::Path p;
    const int dots = 5;
    const float step = length / (dots - 1);
    for (int i = 0; i < dots; ++i)
    {
        auto pos = 4.0f + i * step;
        juce::Rectangle<float> dot = vertical
            ? juce::Rectangle<float>(2.0f, 2.0f).withCentre({ mid, pos })
            : juce::Rectangle<float>(2.0f, 2.0f).withCentre({ pos, mid });
        g.setColour(grip);
        g.fillEllipse(dot);
    }

    if (isMouseOver() || isMouseButtonDown())
    {
        g.setColour(accent.withAlpha(0.35f));
        g.drawRect(getLocalBounds(), 1);
    }
}

void ThemedResizerBar::mouseDoubleClick(const juce::MouseEvent&)
{
    if (mainView != nullptr)
    {
        mainView->onResizerDoubleClick(this);
    }
}

//=========================== MainView Implementation =========================

MainView::MainView(std::shared_ptr<daw::audio::engine::EngineContext> ec)
    : engineContext(std::move(ec))
    , projectModel(std::make_shared<daw::project::ProjectModel>())
    , undoManager()
    , transportBar(engineContext)
    , statusStrip(engineContext)
    , browserPanel(projectModel, engineContext)
    , flagshipPanel()
    , arrangeView(projectModel, engineContext, &undoManager)
    , inspectorPanel(projectModel, engineContext)
    , patternSequencer()
    , sessionLauncher()
    , pianoRollView()
    , mixerView(engineContext, projectModel)
{
    setupUI();
    setupLayouts();
    setupPanelTabs();
    restoreLayoutState();
    setupKeyFocus();
    setupCommandPalette();
    setupTooltips();

    // Initialize AI system
    aiConfig = std::make_shared<daw::ai::config::AIConfig>();
    // Load config from file (defaults to LocalLLM if not configured)
    // Use application data directory for config file
    juce::File aiConfigFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                .getChildFile("DAWProject")
                                .getChildFile("ai_config.xml");
    // Create directory if it doesn't exist
    aiConfigFile.getParentDirectory().createDirectory();
    aiConfig->loadFromFile(aiConfigFile.getFullPathName().toStdString());

    inferenceEngine = std::make_shared<daw::ai::inference::InferenceEngine>(2); // 2 worker threads
    inferenceEngine->initialize(aiConfig);

    // Wire AI to components
    pianoRollView.setInferenceEngine(inferenceEngine);
    patternSequencer.setInferenceEngine(inferenceEngine);

    // Wire DrumMachine to shared audio system
    if (engineContext != nullptr)
    {
        auto* sharedDeviceManager = engineContext->getDeviceManager();
        if (sharedDeviceManager != nullptr)
        {
            drumMachine.attachToDeviceManager(*sharedDeviceManager);
            drumMachineDeviceManager = sharedDeviceManager;
            daw::core::utilities::Logger::info("MainView: DrumMachine attached to shared AudioDeviceManager");
        }
        else
        {
            daw::core::utilities::Logger::warning("MainView: EngineContext returned null AudioDeviceManager; DrumMachine audio disabled");
        }
    }
    else
    {
        daw::core::utilities::Logger::warning("MainView: EngineContext unavailable; DrumMachine audio disabled");
    }

    // Initial content hints for FL-style workflow
    flagshipPanel.setTitle("AI Mastering Suite");
    patternSequencer.setProjectModel(projectModel);

    // Wire browser recent project selection to MainWindow
    browserPanel.onRecentProjectSelected = [this](const juce::String& path) {
        if (parentWindow != nullptr)
        {
            // MainWindow will handle the open
            parentWindow->openProjectFromPath(path);
        }
    };
    constexpr double defaultTempoBpm = 128.0;
    constexpr bool defaultPlaying = true;
    patternSequencer.setTempo(defaultTempoBpm);
    patternSequencer.setIsPlaying(defaultPlaying);
    sessionLauncher.setTempo(defaultTempoBpm);
    sessionLauncher.setIsPlaying(defaultPlaying);
    sessionLauncher.setLooping(true);

    if (projectModel != nullptr)
    {
        projectModel->addModelListener([this] {
            refreshViews();
            // Mark project as dirty when model changes
            if (parentWindow != nullptr)
                parentWindow->markProjectDirty();
        });

        projectModel->getSelectionModel().addSelectionListener([this] {
            updatePatternSequencerFromSelection();
        });
    }
}

void MainView::setupUI()
{
    using juce::Component;

    addAndMakeVisible(transportBar);
    addAndMakeVisible(statusStrip);

    // Body composition
    addAndMakeVisible(bodyContainer);
    bodyContainer.addAndMakeVisible(centerContainer);
    bodyContainer.addAndMakeVisible(bottomContainer);

    // Center (left dock | arrange | right dock)
    centerContainer.addAndMakeVisible(leftContainer);
    centerContainer.addAndMakeVisible(arrangeView);
    centerContainer.addAndMakeVisible(rightContainer);

    // Left dock: browser (top) | flagship (bottom)
    leftContainer.addAndMakeVisible(browserPanel);
    leftContainer.addAndMakeVisible(flagshipPanel);

    // Right dock: inspector (top) | pattern (middle) | drum machine (bottom)
    rightContainer.addAndMakeVisible(inspectorPanel);
    rightContainer.addAndMakeVisible(patternSequencer);
    rightContainer.addAndMakeVisible(drumMachine);

    // Bottom dock: session (top) | piano roll (middle) | mixer (bottom)
    bottomContainer.addAndMakeVisible(sessionLauncher);
    bottomContainer.addAndMakeVisible(pianoRollView);
    bottomContainer.addAndMakeVisible(mixerView);

    // Enable context menus on panels (right-click)
    browserPanel.addMouseListener(this, true);
    flagshipPanel.addMouseListener(this, true);
    arrangeView.addMouseListener(this, true);
    inspectorPanel.addMouseListener(this, true);
    patternSequencer.addMouseListener(this, true);
    sessionLauncher.addMouseListener(this, true);
    pianoRollView.addMouseListener(this, true);
    mixerView.addMouseListener(this, true);
    drumMachine.addMouseListener(this, true);

    // Setup piano roll with project model
    pianoRollView.setProjectModel(projectModel);
}

void MainView::setupLayouts()
{
    // Create resizers (must be children of their respective containers)
    rootResizer         = std::make_unique<ThemedResizerBar>(&bodyLayout,   1, false, this);
    leftResizer         = std::make_unique<ThemedResizerBar>(&centerLayout, 1, true, this);
    rightResizer        = std::make_unique<ThemedResizerBar>(&centerLayout, 3, true, this);
    leftInnerResizer     = std::make_unique<ThemedResizerBar>(&leftLayout,   1, false, this);
    rightInnerResizer    = std::make_unique<ThemedResizerBar>(&rightLayout,  1, false, this);
    bottomInnerResizer1  = std::make_unique<ThemedResizerBar>(&bottomLayout, 1, false, this);
    bottomInnerResizer2  = std::make_unique<ThemedResizerBar>(&bottomLayout, 3, false, this);

    // Attach them to parents
    bodyContainer.addAndMakeVisible(rootResizer.get());
    centerContainer.addAndMakeVisible(leftResizer.get());
    centerContainer.addAndMakeVisible(rightResizer.get());
    leftContainer.addAndMakeVisible(leftInnerResizer.get());
    rightContainer.addAndMakeVisible(rightInnerResizer.get());
    bottomContainer.addAndMakeVisible(bottomInnerResizer1.get());
    bottomContainer.addAndMakeVisible(bottomInnerResizer2.get());

    // Initial constraints; actual values will be set by updateLayoutConstraints()
    // Body: [ center | bar | bottom ]
    bodyLayout.setItemLayout(0, 50, -1.0, -1.0);  // center (stretch)
    bodyLayout.setItemLayout(1, 4,   8,    6);    // resizer
    bodyLayout.setItemLayout(2, 0,  -1.0, 200);   // bottom

    // Center: [ left | bar | arrange | bar | right ]
    centerLayout.setItemLayout(0, 0,  8000, 300); // left
    centerLayout.setItemLayout(1, 4,    8,   6);  // resizer
    centerLayout.setItemLayout(2, 50, -1.0, -1.0);// arrange (stretch)
    centerLayout.setItemLayout(3, 4,    8,   6);  // resizer
    centerLayout.setItemLayout(4, 0,  8000, 320); // right

    // Left dock (vertical): [ browser | bar | flagship ]
    leftLayout.setItemLayout(0, 40, -1.0, -1.0); // browser
    leftLayout.setItemLayout(1, 4,    8,   6);   // resizer
    leftLayout.setItemLayout(2, 40, -1.0, 120);  // flagship

    // Right dock (vertical): [ inspector | bar | pattern | bar | drum machine ]
    rightLayout.setItemLayout(0, 40, -1.0, -1.0); // inspector
    rightLayout.setItemLayout(1, 4,    8,   6);   // resizer
    rightLayout.setItemLayout(2, 40, -1.0, 120);  // pattern
    rightLayout.setItemLayout(3, 4,    8,   6);   // resizer
    rightLayout.setItemLayout(4, 40, -1.0, 120);  // drum machine

    // Bottom dock (vertical): [ session | bar | piano roll | bar | mixer ]
    bottomLayout.setItemLayout(0, 0,  -1.0, 140); // session
    bottomLayout.setItemLayout(1, 4,    8,   6);  // resizer 1
    bottomLayout.setItemLayout(2, 0,  -1.0, 200); // piano roll
    bottomLayout.setItemLayout(3, 4,    8,   6);  // resizer 2
    bottomLayout.setItemLayout(4, 60, -1.0, 200); // mixer
}

void MainView::updateLayoutConstraints()
{
    // Visibility -> sizes for CENTER (left/right widths)
    const int leftPref  = (layoutState.browserVisible || layoutState.flagshipVisible) ? layoutState.browserWidth : 0;
    const int rightPref = (layoutState.inspectorVisible || layoutState.patternSeqVisible) ? layoutState.inspectorWidth : 0;

    centerLayout.setItemLayout(0, 0,  8000, leftPref);
    centerLayout.setItemLayout(1, 4,    8, (leftPref > 0 ? 6 : 0));
    centerLayout.setItemLayout(2, 50, -1.0, -1.0);
    centerLayout.setItemLayout(3, 4,    8, (rightPref > 0 ? 6 : 0));
    centerLayout.setItemLayout(4, 0,  8000, rightPref);

    leftContainer.setVisible(leftPref > 0);
    leftResizer->setVisible(leftPref > 0);
    rightContainer.setVisible(rightPref > 0);
    rightResizer->setVisible(rightPref > 0);

    // BODY (center vs bottom)
    const int bottomPref = layoutState.mixerVisible || layoutState.sessionVisible
        ? juce::jmax(0, layoutState.mixerHeight + layoutState.sessionHeight + 6)
        : 0;

    bodyLayout.setItemLayout(0, 50, -1.0, -1.0);
    bodyLayout.setItemLayout(1, 4,    8, (bottomPref > 0 ? 6 : 0));
    bodyLayout.setItemLayout(2, 0,  4000, bottomPref);

    bottomContainer.setVisible(bottomPref > 0);
    rootResizer->setVisible(bottomPref > 0);

    // LEFT DOCK internal split
    const bool showBrowser  = layoutState.browserVisible;
    const bool showFlagship = layoutState.flagshipVisible;
    float ratioL = juce::jlimit(0.1f, 0.9f, layoutState.leftSplitRatio);

    if (!showBrowser && !showFlagship)
    {
        leftLayout.setItemLayout(0, 0, 0, 0);
        leftLayout.setItemLayout(1, 0, 0, 0);
        leftLayout.setItemLayout(2, 0, 0, 0);
        browserPanel.setVisible(false);
        flagshipPanel.setVisible(false);
        leftInnerResizer->setVisible(false);
    }
    else if (showBrowser && showFlagship)
    {
        const int total = leftContainer.getHeight() > 0 ? leftContainer.getHeight() : 400;
        const int topPx = int(total * ratioL);
        leftLayout.setItemLayout(0, 40, -1.0, topPx);
        leftLayout.setItemLayout(1, 4,    8,   6);
        leftLayout.setItemLayout(2, 40, -1.0, total - topPx - 6);

        browserPanel.setVisible(true);
        flagshipPanel.setVisible(true);
        leftInnerResizer->setVisible(true);
    }
    else
    {
        // Only one visible -> it takes all
        leftLayout.setItemLayout(0, 0, -1.0, showBrowser ? -1.0 : 0.0);
        leftLayout.setItemLayout(1, 0,    0, 0);
        leftLayout.setItemLayout(2, 0, -1.0, showFlagship ? -1.0 : 0.0);

        browserPanel.setVisible(showBrowser);
        flagshipPanel.setVisible(showFlagship);
        leftInnerResizer->setVisible(false);
    }

    // RIGHT DOCK internal split
    const bool showInspector = layoutState.inspectorVisible;
    const bool showPattern   = layoutState.patternSeqVisible;
    const bool showDrumMachine = layoutState.drumMachineVisible;
    float ratioR = juce::jlimit(0.1f, 0.9f, layoutState.rightSplitRatio);

    if (!showInspector && !showPattern && !showDrumMachine)
    {
        rightLayout.setItemLayout(0, 0, 0, 0);
        rightLayout.setItemLayout(1, 0, 0, 0);
        rightLayout.setItemLayout(2, 0, 0, 0);
        rightLayout.setItemLayout(3, 0, 0, 0);
        rightLayout.setItemLayout(4, 0, 0, 0);
        inspectorPanel.setVisible(false);
        patternSequencer.setVisible(false);
        drumMachine.setVisible(false);
        rightInnerResizer->setVisible(false);
    }
    else if (showInspector && showPattern && showDrumMachine)
    {
        // All three visible - inspector takes top, pattern middle, drum machine bottom
        const int total = rightContainer.getHeight() > 0 ? rightContainer.getHeight() : 400;
        const int inspectorHeight = int(total * ratioR * 0.5f); // Inspector gets 50% of split
        const int patternHeight = int(total * (1.0f - ratioR)); // Pattern gets remainder
        const int drumMachineHeight = total - inspectorHeight - patternHeight - 12; // Drum machine gets rest

        rightLayout.setItemLayout(0, 40, -1.0, inspectorHeight);
        rightLayout.setItemLayout(1, 4,    8,   6);
        rightLayout.setItemLayout(2, 40, -1.0, patternHeight);
        rightLayout.setItemLayout(3, 4,    8,   6);
        rightLayout.setItemLayout(4, 40, -1.0, drumMachineHeight);

        inspectorPanel.setVisible(true);
        patternSequencer.setVisible(true);
        drumMachine.setVisible(true);
        rightInnerResizer->setVisible(true);
    }
    else
    {
        // Handle various combinations of visible panels
        // For simplicity, show inspector first, then pattern, then drum machine
        std::vector<bool> visible = {showInspector, showPattern, showDrumMachine};
        int visibleCount = 0;
        for (bool v : visible) if (v) visibleCount++;

        if (visibleCount == 1)
        {
            // Only one visible - it takes all space
            rightLayout.setItemLayout(0, 0, -1.0, showInspector ? -1.0 : 0.0);
            rightLayout.setItemLayout(1, 0,    0, 0);
            rightLayout.setItemLayout(2, 0, -1.0, showPattern ? -1.0 : 0.0);
            rightLayout.setItemLayout(3, 0,    0, 0);
            rightLayout.setItemLayout(4, 0, -1.0, showDrumMachine ? -1.0 : 0.0);
        }
        else if (visibleCount == 2)
        {
            // Two visible - split space between them
            if (showInspector && showPattern)
            {
                rightLayout.setItemLayout(0, 0, -1.0, -1.0);
                rightLayout.setItemLayout(1, 4,    8,   6);
                rightLayout.setItemLayout(2, 0, -1.0, -1.0);
                rightLayout.setItemLayout(3, 0,    0, 0);
                rightLayout.setItemLayout(4, 0,    0, 0);
            }
            else if (showInspector && showDrumMachine)
            {
                rightLayout.setItemLayout(0, 0, -1.0, -1.0);
                rightLayout.setItemLayout(1, 0,    0, 0);
                rightLayout.setItemLayout(2, 0,    0, 0);
                rightLayout.setItemLayout(3, 4,    8,   6);
                rightLayout.setItemLayout(4, 0, -1.0, -1.0);
            }
            else if (showPattern && showDrumMachine)
            {
                rightLayout.setItemLayout(0, 0,    0,   0);
                rightLayout.setItemLayout(1, 4,    8,   6);
                rightLayout.setItemLayout(2, 0, -1.0, -1.0);
                rightLayout.setItemLayout(3, 4,    8,   6);
                rightLayout.setItemLayout(4, 0, -1.0, -1.0);
            }
        }

        inspectorPanel.setVisible(showInspector);
        patternSequencer.setVisible(showPattern);
        drumMachine.setVisible(showDrumMachine);
        rightInnerResizer->setVisible(visibleCount > 1);
    }

    // BOTTOM DOCK internal split (session | piano roll | mixer)
    const bool showSession = layoutState.sessionVisible;
    const bool showPianoRoll = layoutState.pianoRollVisible;
    const bool showMixer   = layoutState.mixerVisible;
    const int pianoRollHeight = 300;

    if (!showSession && !showPianoRoll && !showMixer)
    {
        bottomLayout.setItemLayout(0, 0, 0, 0);
        bottomLayout.setItemLayout(1, 0, 0, 0);
        bottomLayout.setItemLayout(2, 0, 0, 0);
        bottomLayout.setItemLayout(3, 0, 0, 0);
        bottomLayout.setItemLayout(4, 0, 0, 0);
        sessionLauncher.setVisible(false);
        pianoRollView.setVisible(false);
        mixerView.setVisible(false);
        bottomInnerResizer1->setVisible(false);
        bottomInnerResizer2->setVisible(false);
    }
    else if (showSession && showPianoRoll && showMixer)
    {
        // All three visible
        bottomLayout.setItemLayout(0, 40, -1.0, layoutState.sessionHeight);
        bottomLayout.setItemLayout(1, 4,    8,   6);
        bottomLayout.setItemLayout(2, 60, -1.0, pianoRollHeight);
        bottomLayout.setItemLayout(3, 4,    8,   6);
        bottomLayout.setItemLayout(4, 80, -1.0, layoutState.mixerHeight);

        sessionLauncher.setVisible(true);
        pianoRollView.setVisible(true);
        mixerView.setVisible(true);
        bottomInnerResizer1->setVisible(true);
        bottomInnerResizer2->setVisible(true);
    }
    else if (showSession && showPianoRoll)
    {
        // Session + Piano Roll
        bottomLayout.setItemLayout(0, 40, -1.0, layoutState.sessionHeight);
        bottomLayout.setItemLayout(1, 4,    8,   6);
        bottomLayout.setItemLayout(2, 60, -1.0, -1.0);
        bottomLayout.setItemLayout(3, 0,    0,   0);
        bottomLayout.setItemLayout(4, 0,    0,   0);

        sessionLauncher.setVisible(true);
        pianoRollView.setVisible(true);
        mixerView.setVisible(false);
        bottomInnerResizer1->setVisible(true);
        bottomInnerResizer2->setVisible(false);
    }
    else if (showPianoRoll && showMixer)
    {
        // Piano Roll + Mixer
        bottomLayout.setItemLayout(0, 0,    0,   0);
        bottomLayout.setItemLayout(1, 0,    0,   0);
        bottomLayout.setItemLayout(2, 60, -1.0, pianoRollHeight);
        bottomLayout.setItemLayout(3, 4,    8,   6);
        bottomLayout.setItemLayout(4, 80, -1.0, layoutState.mixerHeight);

        sessionLauncher.setVisible(false);
        pianoRollView.setVisible(true);
        mixerView.setVisible(true);
        bottomInnerResizer1->setVisible(false);
        bottomInnerResizer2->setVisible(true);
    }
    else if (showSession && showMixer)
    {
        // Session + Mixer (no piano roll)
        bottomLayout.setItemLayout(0, 40, -1.0, layoutState.sessionHeight);
        bottomLayout.setItemLayout(1, 0,    0,   0);
        bottomLayout.setItemLayout(2, 0,    0,   0);
        bottomLayout.setItemLayout(3, 4,    8,   6);
        bottomLayout.setItemLayout(4, 80, -1.0, layoutState.mixerHeight);

        sessionLauncher.setVisible(true);
        pianoRollView.setVisible(false);
        mixerView.setVisible(true);
        bottomInnerResizer1->setVisible(false);
        bottomInnerResizer2->setVisible(true);
    }
    else
    {
        // Only one visible
        bottomLayout.setItemLayout(0, 0, -1.0, showSession ? -1.0 : 0.0);
        bottomLayout.setItemLayout(1, 0,    0, 0);
        bottomLayout.setItemLayout(2, 0, -1.0, showPianoRoll ? -1.0 : 0.0);
        bottomLayout.setItemLayout(3, 0,    0, 0);
        bottomLayout.setItemLayout(4, 0, -1.0, showMixer   ? -1.0 : 0.0);

        sessionLauncher.setVisible(showSession);
        pianoRollView.setVisible(showPianoRoll);
        mixerView.setVisible(showMixer);
        bottomInnerResizer1->setVisible(false);
        bottomInnerResizer2->setVisible(false);
    }
}

void MainView::applyLayout(bool animated)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    auto bounds = getLocalBounds();

    // Top: TransportBar (fixed) - using professional design system height
    auto transport = bounds.removeFromTop(static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kTransportHeight));
    setBoundsWithAnimation(transportBar, transport, animated);

    // Status strip below transport bar
    auto status = bounds.removeFromTop(static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kStatusStripHeight));
    setBoundsWithAnimation(statusStrip, status, animated);

    // Body occupies the rest
    setBoundsWithAnimation(bodyContainer, bounds, animated);

    // Lay out body: [center | rootResizer | bottom]
    {
        juce::Component* comps[] = { &centerContainer, rootResizer.get(), &bottomContainer };
        bodyLayout.layOutComponents(comps, 3, 0, 0, bodyContainer.getWidth(), bodyContainer.getHeight(),
                                    false, true);

        // No animation inside layOutComponents; animate by moving children after.
        centerContainer.setBounds(comps[0]->getBounds());
        rootResizer->setBounds(comps[1]->getBounds());
        bottomContainer.setBounds(comps[2]->getBounds());
    }

    // Center: [left | res | arrange | res | right]
    {
        juce::Component* comps[] = { &leftContainer, leftResizer.get(), &arrangeView, rightResizer.get(), &rightContainer };
        centerLayout.layOutComponents(comps, 5, 0, 0, centerContainer.getWidth(), centerContainer.getHeight(),
                                      true, true);

        leftContainer.setBounds(comps[0]->getBounds());
        leftResizer->setBounds(comps[1]->getBounds());
        arrangeView.setBounds(comps[2]->getBounds().reduced(Spacing::medium));
        rightResizer->setBounds(comps[3]->getBounds());
        rightContainer.setBounds(comps[4]->getBounds());
    }

    // Left dock internal (vertical)
    {
        juce::Component* comps[] = { &browserPanel, leftInnerResizer.get(), &flagshipPanel };
        leftLayout.layOutComponents(comps, 3, 0, 0, leftContainer.getWidth(), leftContainer.getHeight(),
                                    false, true);
        browserPanel.setBounds(comps[0]->getBounds().reduced(Spacing::medium));
        leftInnerResizer->setBounds(comps[1]->getBounds());
        flagshipPanel.setBounds(comps[2]->getBounds().reduced(Spacing::medium));
    }

    // Right dock internal (vertical)
    {
        juce::Component* comps[] = { &inspectorPanel, rightInnerResizer.get(), &patternSequencer, nullptr, &drumMachine };
        rightLayout.layOutComponents(comps, 5, 0, 0, rightContainer.getWidth(), rightContainer.getHeight(),
                                     false, true);
        inspectorPanel.setBounds(comps[0]->getBounds().reduced(Spacing::medium));
        rightInnerResizer->setBounds(comps[1]->getBounds());
        patternSequencer.setBounds(comps[2]->getBounds().reduced(Spacing::medium));
        drumMachine.setBounds(comps[4]->getBounds().reduced(Spacing::medium));
    }

    // Bottom dock internal (vertical): [session | resizer1 | piano roll | resizer2 | mixer]
    {
        juce::Component* comps[] = { &sessionLauncher, bottomInnerResizer1.get(), &pianoRollView,
                                      bottomInnerResizer2.get(), &mixerView };
        bottomLayout.layOutComponents(comps, 5, 0, 0, bottomContainer.getWidth(), bottomContainer.getHeight(),
                                      false, true);
        sessionLauncher.setBounds(comps[0]->getBounds().reduced(Spacing::medium));
        bottomInnerResizer1->setBounds(comps[1]->getBounds());
        pianoRollView.setBounds(comps[2]->getBounds().reduced(Spacing::medium));
        bottomInnerResizer2->setBounds(comps[3]->getBounds());
        mixerView.setBounds(comps[4]->getBounds());
    }
}

void MainView::refreshViews()
{
    arrangeView.refresh();
    inspectorPanel.refresh();
    mixerView.refreshStrips();
}

void MainView::updatePatternSequencerFromSelection()
{
    if (projectModel == nullptr)
        return;

    const auto& selection = projectModel->getSelectionModel();
    const auto selectedClips = selection.getSelectedClips();

    // If a clip is selected and it has a pattern, load that pattern
    if (!selectedClips.empty())
    {
        const auto* clip = projectModel->getClip(selectedClips[0]);
        if (clip != nullptr && clip->hasPattern())
        {
            const auto patternId = clip->getPatternId();
            patternSequencer.setPattern(patternId);

            // Also update piano roll to show the pattern
            pianoRollView.setProjectModel(projectModel);
            pianoRollView.setCurrentClip(selectedClips[0]);

            // Show piano roll if it's not visible
            if (!layoutState.pianoRollVisible)
            {
                layoutState.pianoRollVisible = true;
                updateLayoutConstraints();
                applyLayout(true);
            }
            return;
        }
    }

    // No clip selected or clip has no pattern - clear pattern sequencer and hide piano roll
    patternSequencer.setPattern(0);
    pianoRollView.setCurrentClip(0);

    // Hide piano roll if no pattern is selected
    if (layoutState.pianoRollVisible)
    {
        layoutState.pianoRollVisible = false;
        updateLayoutConstraints();
        applyLayout(true);
    }
}

void MainView::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    g.fillAll(juce::Colour(Colors::background));

    // Optional: Show current preset indicator in corner (subtle)
    // This could be enhanced to show a small badge
}

void MainView::resized()
{
    updateLayoutConstraints();
    applyLayout(false);
    updatePanelTabs();
}

bool MainView::keyPressed(const juce::KeyPress& key)
{
    const auto mods = key.getModifiers();
    const bool isCtrlOrCmd = mods.isCommandDown() || mods.isCtrlDown();
    const bool isShift = mods.isShiftDown();

    // Space: Play/Stop (most common DAW shortcut)
    if (key == juce::KeyPress::spaceKey && !isCtrlOrCmd && !isShift && !mods.isAltDown())
    {
        if (engineContext != nullptr)
        {
            if (engineContext->isPlaying())
                engineContext->stop();
            else
                engineContext->play();
        }
        return true;
    }

    // Ctrl/Cmd + Z: Undo
    if (isCtrlOrCmd && !isShift && key.getKeyCode() == 'Z')
    {
        if (projectModel != nullptr && undoManager.canUndo())
        {
            undoManager.undo(*projectModel);
            refreshViews();
        }
        return true;
    }

    // Ctrl/Cmd + Shift + Z or Ctrl/Cmd + Y: Redo
    if (isCtrlOrCmd && ((isShift && key.getKeyCode() == 'Z') || key.getKeyCode() == 'Y'))
    {
        if (projectModel != nullptr && undoManager.canRedo())
        {
            undoManager.redo(*projectModel);
            refreshViews();
        }
        return true;
    }

    // Ctrl/Cmd + S: Save
    if (isCtrlOrCmd && !isShift && key.getKeyCode() == 'S')
    {
        if (parentWindow != nullptr)
        {
            parentWindow->saveProject();
        }
        return true;
    }

    // Ctrl/Cmd + Shift + S: Save As
    if (isCtrlOrCmd && isShift && key.getKeyCode() == 'S')
    {
        if (parentWindow != nullptr)
        {
            parentWindow->saveProjectAs();
        }
        return true;
    }

    // Ctrl/Cmd + O: Open
    if (isCtrlOrCmd && !isShift && key.getKeyCode() == 'O')
    {
        if (parentWindow != nullptr)
        {
            parentWindow->openProject();
        }
        return true;
    }

    // Ctrl/Cmd + N: New Project
    if (isCtrlOrCmd && !isShift && key.getKeyCode() == 'N')
    {
        if (parentWindow != nullptr)
        {
            parentWindow->newProject();
        }
        return true;
    }

    // Shift + Ctrl/Cmd + P: Command Palette
    if (isCtrlOrCmd && isShift && key.getKeyCode() == 'P')
    {
        showCommandPalette();
        return true;
    }

    // Layout presets with Ctrl/Cmd + number
    if (isCtrlOrCmd && !isShift)
    {
        switch (key.getKeyCode())
        {
            case '1': applyLayoutPreset(LayoutPreset::Arrange); return true;
            case '2': applyLayoutPreset(LayoutPreset::Mix); return true;
            case '3': applyLayoutPreset(LayoutPreset::Edit); return true;
            case '4': applyLayoutPreset(LayoutPreset::Live); return true;
            default: break;
        }
    }

    // Pattern switching with number keys (when pattern sequencer is focused)
    if (!isCtrlOrCmd && !isShift && !mods.isAltDown())
    {
        const auto keyCode = key.getKeyCode();
        if (keyCode >= '1' && keyCode <= '9')
        {
            const int patternIndex = keyCode - '1'; // 0-8
            if (projectModel != nullptr)
            {
                const auto patterns = projectModel->getPatterns();
                if (patternIndex >= 0 && patternIndex < static_cast<int>(patterns.size()))
                {
                    patternSequencer.setPattern(patterns[static_cast<size_t>(patternIndex)]->getId());
                    return true;
                }
            }
        }
    }

    // FL-like quick toggles (no modifier)
    if (!isCtrlOrCmd && !isShift && !mods.isAltDown())
    {
        switch (key.getTextCharacter())
        {
            case 'b': toggleBrowser();   return true;
            case 'f': toggleFlagship();  return true;
            case 'i': toggleInspector(); return true;
            case 'p': togglePattern();   return true;
            case 'd': toggleDrumMachine(); return true;
            case 'm': toggleMixer();     return true;
            case 's': toggleSession();   return true;
            case '?':
                // Show command palette cheat sheet overlay
                commandPalette.showModal(this);
                return true;
            case '=': case '+': // Zoom in
                if (arrangeView.isMouseOver(true))
                    arrangeView.zoomIn();
                return true;
            case '-': case '_': // Zoom out
                if (arrangeView.isMouseOver(true))
                    arrangeView.zoomOut();
                return true;
            case '0': // Zoom to fit
                if (arrangeView.isMouseOver(true))
                    arrangeView.zoomToFit();
                return true;
            default: break;
        }
    }

    return false;
}

bool MainView::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    juce::ignoreUnused(originatingComponent);
    return keyPressed(key);
}

void MainView::modifierKeysChanged(const juce::ModifierKeys&)
{
    // Could show keyboard shortcut hints here
    repaint();
}

juce::String MainView::getPresetName(LayoutPreset preset) const
{
    switch (preset)
    {
        case LayoutPreset::Arrange: return "Arrange";
        case LayoutPreset::Mix:     return "Mix";
        case LayoutPreset::Edit:    return "Edit";
        case LayoutPreset::Live:     return "Live";
        default: return "Unknown";
    }
}

bool MainView::exportLayoutToFile(const juce::File& file) const
{
    try
    {
        auto xml = std::make_unique<juce::XmlElement>("LayoutState");

        xml->setAttribute("browserWidth", layoutState.browserWidth);
        xml->setAttribute("inspectorWidth", layoutState.inspectorWidth);
        xml->setAttribute("mixerHeight", layoutState.mixerHeight);
        xml->setAttribute("sessionHeight", layoutState.sessionHeight);
        xml->setAttribute("leftSplitRatio", layoutState.leftSplitRatio);
        xml->setAttribute("rightSplitRatio", layoutState.rightSplitRatio);
        xml->setAttribute("browserVisible", layoutState.browserVisible);
        xml->setAttribute("flagshipVisible", layoutState.flagshipVisible);
        xml->setAttribute("inspectorVisible", layoutState.inspectorVisible);
        xml->setAttribute("patternSeqVisible", layoutState.patternSeqVisible);
        xml->setAttribute("mixerVisible", layoutState.mixerVisible);
        xml->setAttribute("sessionVisible", layoutState.sessionVisible);
        xml->setAttribute("pianoRollVisible", layoutState.pianoRollVisible);
        xml->setAttribute("lastPreset", static_cast<int>(layoutState.lastPreset));

        return xml->writeTo(file, juce::XmlElement::TextFormat());
    }
    catch (...)
    {
        return false;
    }
}

bool MainView::importLayoutFromFile(const juce::File& file)
{
    try
    {
        auto xml = juce::XmlDocument::parse(file);
        if (xml == nullptr || !xml->hasTagName("LayoutState"))
            return false;

        layoutState.browserWidth   = xml->getIntAttribute("browserWidth", layoutState.browserWidth);
        layoutState.inspectorWidth = xml->getIntAttribute("inspectorWidth", layoutState.inspectorWidth);
        layoutState.mixerHeight     = xml->getIntAttribute("mixerHeight", layoutState.mixerHeight);
        layoutState.sessionHeight   = xml->getIntAttribute("sessionHeight", layoutState.sessionHeight);
        layoutState.leftSplitRatio  = static_cast<float>(xml->getDoubleAttribute("leftSplitRatio", layoutState.leftSplitRatio));
        layoutState.rightSplitRatio = static_cast<float>(xml->getDoubleAttribute("rightSplitRatio", layoutState.rightSplitRatio));
        layoutState.browserVisible  = xml->getBoolAttribute("browserVisible", layoutState.browserVisible);
        layoutState.flagshipVisible = xml->getBoolAttribute("flagshipVisible", layoutState.flagshipVisible);
        layoutState.inspectorVisible = xml->getBoolAttribute("inspectorVisible", layoutState.inspectorVisible);
        layoutState.patternSeqVisible = xml->getBoolAttribute("patternSeqVisible", layoutState.patternSeqVisible);
        layoutState.drumMachineVisible = xml->getBoolAttribute("drumMachineVisible", layoutState.drumMachineVisible);
        layoutState.mixerVisible    = xml->getBoolAttribute("mixerVisible", layoutState.mixerVisible);
        layoutState.sessionVisible   = xml->getBoolAttribute("sessionVisible", layoutState.sessionVisible);
        layoutState.pianoRollVisible = xml->getBoolAttribute("pianoRollVisible", layoutState.pianoRollVisible);

        const int presetValue = xml->getIntAttribute("lastPreset", static_cast<int>(layoutState.lastPreset));
        layoutState.lastPreset = static_cast<LayoutPreset>(juce::jlimit(0, 3, presetValue));

        clampLayoutState();
        updateLayoutConstraints();
        applyLayout(true);
        saveLayoutState();

        return true;
    }
    catch (...)
    {
        return false;
    }
}

void MainView::maximizePanel(juce::Component* panel)
{
    if (isMaximized && maximizedPanel == panel)
    {
        restorePanels();
        return;
    }

    // Save current state
    savedLayoutState = layoutState;
    isMaximized = true;
    maximizedPanel = panel;

    // Hide all other panels
    if (panel == &browserPanel || panel == &flagshipPanel)
    {
        layoutState.inspectorVisible = false;
        layoutState.patternSeqVisible = false;
        layoutState.mixerVisible = false;
        layoutState.sessionVisible = false;
        if (panel == &browserPanel)
            layoutState.flagshipVisible = false;
        else
            layoutState.browserVisible = false;
    }
    else if (panel == &inspectorPanel || panel == &patternSequencer || panel == &drumMachine)
    {
        layoutState.browserVisible = false;
        layoutState.flagshipVisible = false;
        layoutState.mixerVisible = false;
        layoutState.sessionVisible = false;
        if (panel == &inspectorPanel)
        {
            layoutState.patternSeqVisible = false;
            layoutState.drumMachineVisible = false;
        }
        else if (panel == &patternSequencer)
        {
            layoutState.inspectorVisible = false;
            layoutState.drumMachineVisible = false;
        }
        else // drumMachine
        {
            layoutState.inspectorVisible = false;
            layoutState.patternSeqVisible = false;
        }
    }
    else if (panel == &mixerView || panel == &sessionLauncher || panel == &pianoRollView)
    {
        layoutState.browserVisible = false;
        layoutState.flagshipVisible = false;
        layoutState.inspectorVisible = false;
        layoutState.patternSeqVisible = false;
        if (panel == &mixerView)
        {
            layoutState.sessionVisible = false;
            layoutState.pianoRollVisible = false;
        }
        else if (panel == &sessionLauncher)
        {
            layoutState.mixerVisible = false;
            layoutState.pianoRollVisible = false;
        }
        else // pianoRollView
        {
            layoutState.mixerVisible = false;
            layoutState.sessionVisible = false;
        }
    }
    else if (panel == &arrangeView)
    {
        layoutState.browserVisible = false;
        layoutState.flagshipVisible = false;
        layoutState.inspectorVisible = false;
        layoutState.patternSeqVisible = false;
        layoutState.mixerVisible = false;
        layoutState.sessionVisible = false;
    }

    updateLayoutConstraints();
    applyLayout(true);
    updatePanelTabs();
}

void MainView::restorePanels()
{
    if (!isMaximized)
        return;

    layoutState = savedLayoutState;
    isMaximized = false;
    maximizedPanel = nullptr;

    updateLayoutConstraints();
    applyLayout(true);
    updatePanelTabs();
}

void MainView::showPanelContextMenu(juce::Component* panel, const juce::Point<int>& position)
{
    juce::PopupMenu menu;

    menu.addItem(1, "Maximize Panel", true, isMaximized && maximizedPanel == panel);
    menu.addItem(2, "Restore Panels", isMaximized);
    menu.addSeparator();

    if (panel == &browserPanel || panel == &flagshipPanel)
    {
        menu.addItem(10, "Toggle Browser", true, layoutState.browserVisible);
        menu.addItem(11, "Toggle Flagship", true, layoutState.flagshipVisible);
    }
    else if (panel == &inspectorPanel || panel == &patternSequencer || panel == &drumMachine)
    {
        menu.addItem(20, "Toggle Inspector", true, layoutState.inspectorVisible);
        menu.addItem(21, "Toggle Pattern Sequencer", true, layoutState.patternSeqVisible);
        menu.addItem(22, "Toggle Drum Machine", true, layoutState.drumMachineVisible);
    }
    else if (panel == &mixerView || panel == &sessionLauncher || panel == &pianoRollView)
    {
        menu.addItem(30, "Toggle Mixer", true, layoutState.mixerVisible);
        menu.addItem(31, "Toggle Session", true, layoutState.sessionVisible);
        menu.addItem(32, "Toggle Piano Roll", true, layoutState.pianoRollVisible);
    }

    menu.addSeparator();
    menu.addItem(100, "Export Layout...");
    menu.addItem(101, "Import Layout...");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(
        juce::Rectangle<int>(position.x, position.y, 1, 1)),
        [this, panel](int result)
        {
            switch (result)
            {
                case 1: maximizePanel(panel); break;
                case 2: restorePanels(); break;
                case 10: toggleBrowser(); break;
                case 11: toggleFlagship(); break;
                case 20: toggleInspector(); break;
                case 21: togglePattern(); break;
                case 22: toggleDrumMachine(); break;
                case 30: toggleMixer(); break;
                case 31: toggleSession(); break;
                case 100:
                {
                    auto chooser = std::make_shared<juce::FileChooser>("Export Layout", juce::File(), "*.xml");
                    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                        [this, chooser](const juce::FileChooser& fc)
                        {
                            if (!fc.getResults().isEmpty())
                                exportLayoutToFile(fc.getResult());
                        });
                    break;
                }
                case 101:
                {
                    auto chooser = std::make_shared<juce::FileChooser>("Import Layout", juce::File(), "*.xml");
                    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                        [this, chooser](const juce::FileChooser& fc)
                        {
                            if (!fc.getResults().isEmpty())
                                importLayoutFromFile(fc.getResult());
                        });
                    break;
                }
                default: break;
            }
        });
}

void MainView::setupKeyFocus()
{
    setWantsKeyboardFocus(true);
    addKeyListener(this);
}

//=========================== PanelTab Implementation =================

MainView::PanelTab::PanelTab(const juce::String& name, bool isActive, std::function<void()> onClick)
    : tabName(name)
    , active(isActive)
    , clickHandler(std::move(onClick))
{
    setSize(60, 24);
}

void MainView::PanelTab::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    auto bg = active ? juce::Colour(Colors::surfaceElevated) : juce::Colour(Colors::surface);
    auto text = active ? juce::Colour(Colors::text) : juce::Colour(Colors::textSecondary);
    auto accent = juce::Colour(Colors::accent);

    g.fillAll(bg);

    if (isMouseOver())
    {
        g.setColour(accent.withAlpha(0.2f));
        g.fillAll();
    }

    if (active)
    {
        g.setColour(accent);
        g.fillRect(0, getHeight() - 2, getWidth(), 2);
    }

    g.setColour(text);
    g.setFont(Typography::bodySmall);
    g.drawText(tabName, getLocalBounds(), juce::Justification::centred);
}

void MainView::PanelTab::mouseDown(const juce::MouseEvent&)
{
    if (clickHandler)
        clickHandler();
}

//=========================== Panel Tabs Setup =======================

void MainView::setupPanelTabs()
{
    leftTabBrowser = std::make_unique<PanelTab>("Browser", true, [this] {
        if (!layoutState.browserVisible) toggleBrowser();
    });
    leftTabFlagship = std::make_unique<PanelTab>("Flagship", false, [this] {
        if (!layoutState.flagshipVisible) toggleFlagship();
    });
    rightTabInspector = std::make_unique<PanelTab>("Inspector", true, [this] {
        if (!layoutState.inspectorVisible) toggleInspector();
    });
    rightTabPattern = std::make_unique<PanelTab>("Pattern", false, [this] {
        if (!layoutState.patternSeqVisible) togglePattern();
    });
    bottomTabSession = std::make_unique<PanelTab>("Session", false, [this] {
        if (!layoutState.sessionVisible) toggleSession();
    });
    bottomTabPianoRoll = std::make_unique<PanelTab>("Piano", false, [this] {
        if (!layoutState.pianoRollVisible) togglePianoRoll();
    });
    bottomTabMixer = std::make_unique<PanelTab>("Mixer", true, [this] {
        if (!layoutState.mixerVisible) toggleMixer();
    });

    // Add tabs to containers (will be positioned in updatePanelTabs)
    leftContainer.addChildComponent(leftTabBrowser.get());
    leftContainer.addChildComponent(leftTabFlagship.get());
    rightContainer.addChildComponent(rightTabInspector.get());
    rightContainer.addChildComponent(rightTabPattern.get());
    bottomContainer.addChildComponent(bottomTabSession.get());
    bottomContainer.addChildComponent(bottomTabPianoRoll.get());
    bottomContainer.addChildComponent(bottomTabMixer.get());
}

void MainView::updatePanelTabs()
{
    // Show tabs when panels are collapsed
    const bool showLeftTabs = !layoutState.browserVisible && !layoutState.flagshipVisible;
    const bool showRightTabs = !layoutState.inspectorVisible && !layoutState.patternSeqVisible;
    const bool showBottomTabs = !layoutState.sessionVisible && !layoutState.pianoRollVisible && !layoutState.mixerVisible;

    if (showLeftTabs && leftTabBrowser && leftTabFlagship)
    {
        leftTabBrowser->setVisible(true);
        leftTabFlagship->setVisible(true);
        leftTabBrowser->setActive(false);
        leftTabFlagship->setActive(false);

        auto bounds = leftContainer.getLocalBounds();
        leftTabBrowser->setBounds(bounds.removeFromTop(24).withWidth(60));
        leftTabFlagship->setBounds(bounds.removeFromTop(24).withWidth(60));
    }
    else
    {
        if (leftTabBrowser) leftTabBrowser->setVisible(false);
        if (leftTabFlagship) leftTabFlagship->setVisible(false);
    }

    if (showRightTabs && rightTabInspector && rightTabPattern)
    {
        rightTabInspector->setVisible(true);
        rightTabPattern->setVisible(true);
        rightTabInspector->setActive(false);
        rightTabPattern->setActive(false);

        auto bounds = rightContainer.getLocalBounds();
        rightTabInspector->setBounds(bounds.removeFromTop(24).withWidth(60));
        rightTabPattern->setBounds(bounds.removeFromTop(24).withWidth(60));
    }
    else
    {
        if (rightTabInspector) rightTabInspector->setVisible(false);
        if (rightTabPattern) rightTabPattern->setVisible(false);
    }

    if (showBottomTabs && bottomTabSession && bottomTabPianoRoll && bottomTabMixer)
    {
        bottomTabSession->setVisible(true);
        bottomTabPianoRoll->setVisible(true);
        bottomTabMixer->setVisible(true);
        bottomTabSession->setActive(false);
        bottomTabPianoRoll->setActive(false);
        bottomTabMixer->setActive(false);

        auto bounds = bottomContainer.getLocalBounds();
        bottomTabSession->setBounds(bounds.removeFromLeft(60).withHeight(24));
        bottomTabPianoRoll->setBounds(bounds.removeFromLeft(60).withHeight(24));
        bottomTabMixer->setBounds(bounds.removeFromLeft(60).withHeight(24));
    }
    else
    {
        if (bottomTabSession) bottomTabSession->setVisible(false);
        if (bottomTabPianoRoll) bottomTabPianoRoll->setVisible(false);
        if (bottomTabMixer) bottomTabMixer->setVisible(false);
    }
}

void MainView::setupTooltips()
{
    // Set tooltips for keyboard shortcuts
    /*
    browserPanel.setTooltip("Browser Panel (Press 'B' to toggle)");
    flagshipPanel.setTooltip("Flagship Device Panel (Press 'F' to toggle)");
    arrangeView.setTooltip("Arrange View - Main timeline");
    inspectorPanel.setTooltip("Inspector Panel (Press 'I' to toggle)");
    patternSequencer.setTooltip("Pattern Sequencer (Press 'P' to toggle)");
    drumMachine.setTooltip("Drum Machine (Press 'D' to toggle)");
    sessionLauncher.setTooltip("Session Launcher (Press 'S' to toggle)");
    pianoRollView.setTooltip("Piano Roll Editor - MIDI note editing");
    mixerView.setTooltip("Mixer View (Press 'M' to toggle)");

    // Tooltips for layout presets
    transportBar.setTooltip("Transport Bar - Ctrl/Cmd+1/2/3/4 for layout presets");
    */
}

// ---------- Toggles with animation + persistence ----------

void MainView::toggleBrowser()
{
    layoutState.browserVisible = !layoutState.browserVisible;
    updateLayoutConstraints();
    applyLayout(true);
    updatePanelTabs();
    saveLayoutState();
}

void MainView::toggleInspector()
{
    layoutState.inspectorVisible = !layoutState.inspectorVisible;
    updateLayoutConstraints();
    applyLayout(true);
    updatePanelTabs();
    saveLayoutState();
}

void MainView::toggleMixer()
{
    layoutState.mixerVisible = !layoutState.mixerVisible;
    updateLayoutConstraints();
    applyLayout(true);
    updatePanelTabs();
    saveLayoutState();
}

void MainView::toggleSession()
{
    layoutState.sessionVisible = !layoutState.sessionVisible;
    updateLayoutConstraints();
    applyLayout(true);
    updatePanelTabs();
    saveLayoutState();
}

void MainView::toggleFlagship()
{
    layoutState.flagshipVisible = !layoutState.flagshipVisible;
    updateLayoutConstraints();
    applyLayout(true);
    updatePanelTabs();
    saveLayoutState();
}

void MainView::togglePattern()
{
    layoutState.patternSeqVisible = !layoutState.patternSeqVisible;
    updateLayoutConstraints();
    applyLayout(true);
    updatePanelTabs();
    saveLayoutState();
}

void MainView::togglePianoRoll()
{
    layoutState.pianoRollVisible = !layoutState.pianoRollVisible;
    updateLayoutConstraints();
    applyLayout(true);
    updatePanelTabs();
    saveLayoutState();
}

void MainView::toggleDrumMachine()
{
    layoutState.drumMachineVisible = !layoutState.drumMachineVisible;
    updateLayoutConstraints();
    applyLayout(true);
    updatePanelTabs();
    saveLayoutState();
}

// ---------- Persistence ----------

std::unique_ptr<juce::PropertiesFile> MainView::createLayoutPropsFile()
{
    juce::PropertiesFile::Options opts;
    opts.applicationName     = "NeuroDAW";
    opts.filenameSuffix      = ".settings";
    opts.osxLibrarySubFolder = "Application Support";
    opts.folderName          = "NeuroDAW";
    opts.commonToAllUsers    = false;
    opts.doNotSave           = false;
    opts.storageFormat       = juce::PropertiesFile::storeAsXML;
    return std::make_unique<juce::PropertiesFile>(opts);
}

void MainView::loadFromProps(juce::PropertiesFile& pf)
{
    auto b = pf.getValue("browserWidth",  juce::String(layoutState.browserWidth)).getIntValue();
    auto r = pf.getValue("inspectorWidth",juce::String(layoutState.inspectorWidth)).getIntValue();
    auto mh= pf.getValue("mixerHeight",   juce::String(layoutState.mixerHeight)).getIntValue();
    auto sh= pf.getValue("sessionHeight", juce::String(layoutState.sessionHeight)).getIntValue();

    layoutState.browserWidth   = juce::jlimit(static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kPanelMinWidth),  static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kPanelMaxWidth),  b);
    layoutState.inspectorWidth = juce::jlimit(static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kPanelMinWidth),  static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kPanelMaxWidth),  r);
    layoutState.mixerHeight    = juce::jlimit(static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kPanelMinHeight), static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kPanelMaxHeight), mh);
    layoutState.sessionHeight  = juce::jlimit(static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kPanelMinHeight), static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kPanelMaxHeight), sh);

    layoutState.leftSplitRatio  = pf.getDoubleValue("leftSplitRatio",  layoutState.leftSplitRatio);
    layoutState.rightSplitRatio = pf.getDoubleValue("rightSplitRatio", layoutState.rightSplitRatio);

    layoutState.browserVisible   = pf.getBoolValue("browserVisible",   layoutState.browserVisible);
    layoutState.flagshipVisible  = pf.getBoolValue("flagshipVisible",  layoutState.flagshipVisible);
    layoutState.inspectorVisible = pf.getBoolValue("inspectorVisible", layoutState.inspectorVisible);
    layoutState.patternSeqVisible= pf.getBoolValue("patternSeqVisible",layoutState.patternSeqVisible);
    layoutState.mixerVisible     = pf.getBoolValue("mixerVisible",     layoutState.mixerVisible);
    layoutState.sessionVisible   = pf.getBoolValue("sessionVisible",   layoutState.sessionVisible);
    layoutState.pianoRollVisible = pf.getBoolValue("pianoRollVisible", layoutState.pianoRollVisible);

    const int presetValue = pf.getIntValue("lastPreset", static_cast<int>(layoutState.lastPreset));
    layoutState.lastPreset = static_cast<LayoutPreset>(juce::jlimit(0, 3, presetValue));
}

void MainView::saveToProps(juce::PropertiesFile& pf)
{
    pf.setValue("browserWidth",    layoutState.browserWidth);
    pf.setValue("inspectorWidth",  layoutState.inspectorWidth);
    pf.setValue("mixerHeight",     layoutState.mixerHeight);
    pf.setValue("sessionHeight",   layoutState.sessionHeight);
    pf.setValue("leftSplitRatio",  layoutState.leftSplitRatio);
    pf.setValue("rightSplitRatio", layoutState.rightSplitRatio);

    pf.setValue("browserVisible",   layoutState.browserVisible);
    pf.setValue("flagshipVisible",  layoutState.flagshipVisible);
    pf.setValue("inspectorVisible", layoutState.inspectorVisible);
    pf.setValue("patternSeqVisible",layoutState.patternSeqVisible);
    pf.setValue("drumMachineVisible", layoutState.drumMachineVisible);
    pf.setValue("mixerVisible",     layoutState.mixerVisible);
    pf.setValue("sessionVisible",   layoutState.sessionVisible);
    pf.setValue("pianoRollVisible", layoutState.pianoRollVisible);
    pf.setValue("lastPreset",       static_cast<int>(layoutState.lastPreset));

    pf.saveIfNeeded();
}

void MainView::saveLayoutState()
{
    try
    {
        if (auto pf = createLayoutPropsFile())
            saveToProps(*pf);
    }
    catch (...)
    {
        // If properties file save fails, silently continue
    }
}

void MainView::restoreLayoutState()
{
    try
    {
        if (auto pf = createLayoutPropsFile())
            loadFromProps(*pf);
    }
    catch (...)
    {
        // If properties file fails, use default layout state
    }

    clampLayoutState();
    updateLayoutConstraints();
    applyLayout(false);
}

void MainView::clampLayoutState()
{
    layoutState.browserWidth   = juce::jlimit(static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kPanelMinWidth),  static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kPanelMaxWidth),  layoutState.browserWidth);
    layoutState.inspectorWidth = juce::jlimit(static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kPanelMinWidth),  static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kPanelMaxWidth),  layoutState.inspectorWidth);
    layoutState.mixerHeight    = juce::jlimit(static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kPanelMinHeight), static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kPanelMaxHeight), layoutState.mixerHeight);
    layoutState.sessionHeight   = juce::jlimit(static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kPanelMinHeight), static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kPanelMaxHeight), layoutState.sessionHeight);
    layoutState.leftSplitRatio  = juce::jlimit(0.1f, 0.9f, layoutState.leftSplitRatio);
    layoutState.rightSplitRatio = juce::jlimit(0.1f, 0.9f, layoutState.rightSplitRatio);
}

void MainView::setupCommandPalette()
{
    // Register all commands
    appCommands.registerCommand({
        "new-project", "New Project", "Create a new project", "Ctrl+N", {}
    });
    appCommands.registerCommand({
        "open-project", "Open Project", "Open an existing project", "Ctrl+O", {}
    });
    appCommands.registerCommand({
        "save-project", "Save Project", "Save the current project", "Ctrl+S", {}
    });
    appCommands.registerCommand({
        "save-project-as", "Save Project As", "Save the current project with a new name", "Ctrl+Shift+S", {}
    });
    appCommands.registerCommand({
        "undo", "Undo", "Undo the last action", "Ctrl+Z", {}
    });
    appCommands.registerCommand({
        "redo", "Redo", "Redo the last undone action", "Ctrl+Shift+Z", {}
    });
    appCommands.registerCommand({
        "toggle-browser", "Toggle Browser", "Show or hide the browser panel", "B", {}
    });
    appCommands.registerCommand({
        "toggle-inspector", "Toggle Inspector", "Show or hide the inspector panel", "I", {}
    });
    appCommands.registerCommand({
        "toggle-mixer", "Toggle Mixer", "Show or hide the mixer panel", "M", {}
    });
    appCommands.registerCommand({
        "toggle-pattern", "Toggle Pattern Sequencer", "Show or hide the pattern sequencer", "P", {}
    });
    appCommands.registerCommand({
        "toggle-flagship", "Toggle Flagship Device", "Show or hide the flagship device panel", "F", {}
    });
    appCommands.registerCommand({
        "toggle-drum-machine", "Toggle Drum Machine", "Show or hide the drum machine panel", "D", {}
    });
    appCommands.registerCommand({
        "toggle-session", "Toggle Session Launcher", "Show or hide the session launcher", "S", {}
    });
    appCommands.registerCommand({
        "play-stop", "Play / Stop", "Start or stop playback", "Space", {}
    });

    // Wire AppCommands executor to MainView methods
    appCommands.setCommandExecutor([this](const juce::String& commandId) -> bool {
        if (commandId == "new-project" && parentWindow != nullptr)
        {
            parentWindow->newProject();
            return true;
        }
        else if (commandId == "open-project" && parentWindow != nullptr)
        {
            parentWindow->openProject();
            return true;
        }
        else if (commandId == "save-project" && parentWindow != nullptr)
        {
            parentWindow->saveProject();
            return true;
        }
        else if (commandId == "save-project-as" && parentWindow != nullptr)
        {
            parentWindow->saveProjectAs();
            return true;
        }
        else if (commandId == "undo" && projectModel != nullptr && undoManager.canUndo())
        {
            undoManager.undo(*projectModel);
            refreshViews();
            return true;
        }
        else if (commandId == "redo" && projectModel != nullptr && undoManager.canRedo())
        {
            undoManager.redo(*projectModel);
            refreshViews();
            return true;
        }
        else if (commandId == "toggle-browser")
        {
            toggleBrowser();
            return true;
        }
        else if (commandId == "toggle-inspector")
        {
            toggleInspector();
            return true;
        }
        else if (commandId == "toggle-mixer")
        {
            toggleMixer();
            return true;
        }
        else if (commandId == "toggle-pattern")
        {
            togglePattern();
            return true;
        }
        else if (commandId == "toggle-flagship")
        {
            toggleFlagship();
            return true;
        }
        else if (commandId == "toggle-drum-machine")
        {
            toggleDrumMachine();
            return true;
        }
        else if (commandId == "toggle-session")
        {
            toggleSession();
            return true;
        }
        else if (commandId == "play-stop" && engineContext != nullptr)
        {
            if (engineContext->isPlaying())
                engineContext->stop();
            else
                engineContext->play();
            return true;
        }
        return false;
    });

    // Set commands to palette (with executor wired)
    commandPalette.setCommands(appCommands.getAllCommands());

    // Make command palette invisible initially
    commandPalette.setVisible(false);
    addChildComponent(&commandPalette);
}

void MainView::showCommandPalette()
{
    commandPalette.showModal(this);
}

void MainView::onResizerDoubleClick(ThemedResizerBar* resizer)
{
    if (resizer == leftResizer.get())
    {
        // Toggle left dock
        const bool shouldShow = !layoutState.browserVisible && !layoutState.flagshipVisible;
        layoutState.browserVisible = shouldShow;
        layoutState.flagshipVisible = shouldShow;
        updateLayoutConstraints();
        applyLayout(true);
        saveLayoutState();
    }
    else if (resizer == rightResizer.get())
    {
        // Toggle right dock
        const bool shouldShow = !layoutState.inspectorVisible && !layoutState.patternSeqVisible;
        layoutState.inspectorVisible = shouldShow;
        layoutState.patternSeqVisible = shouldShow;
        updateLayoutConstraints();
        applyLayout(true);
        saveLayoutState();
    }
    else if (resizer == rootResizer.get())
    {
        // Toggle bottom dock
        const bool shouldShow = !layoutState.mixerVisible && !layoutState.sessionVisible;
        layoutState.mixerVisible = shouldShow;
        layoutState.sessionVisible = shouldShow;
        updateLayoutConstraints();
        applyLayout(true);
        saveLayoutState();
    }
    else if (resizer == leftInnerResizer.get())
    {
        // Toggle between browser and flagship (show only one)
        if (layoutState.browserVisible && layoutState.flagshipVisible)
        {
            layoutState.flagshipVisible = false;
        }
        else if (layoutState.browserVisible)
        {
            layoutState.browserVisible = false;
            layoutState.flagshipVisible = true;
        }
        else
        {
            layoutState.browserVisible = true;
            layoutState.flagshipVisible = false;
        }
        updateLayoutConstraints();
        applyLayout(true);
        saveLayoutState();
    }
    else if (resizer == rightInnerResizer.get())
    {
        // Toggle between inspector and pattern (show only one)
        if (layoutState.inspectorVisible && layoutState.patternSeqVisible)
        {
            layoutState.patternSeqVisible = false;
        }
        else if (layoutState.inspectorVisible)
        {
            layoutState.inspectorVisible = false;
            layoutState.patternSeqVisible = true;
        }
        else
        {
            layoutState.inspectorVisible = true;
            layoutState.patternSeqVisible = false;
        }
        updateLayoutConstraints();
        applyLayout(true);
        saveLayoutState();
    }
    /*
    else if (resizer == bottomInnerResizer.get())
    {
        // Toggle between session and mixer (show only one)
        if (layoutState.sessionVisible && layoutState.mixerVisible)
        {
            layoutState.sessionVisible = false;
        }
        else if (layoutState.sessionVisible)
        {
            layoutState.sessionVisible = false;
            layoutState.mixerVisible = true;
        }
        else
        {
            layoutState.sessionVisible = true;
            layoutState.mixerVisible = false;
        }
        updateLayoutConstraints();
        applyLayout(true);
        saveLayoutState();
    }
    */
}

void MainView::applyLayoutPreset(LayoutPreset preset)
{
    layoutState.lastPreset = preset;

    switch (preset)
    {
        case LayoutPreset::Arrange:
            // Full arrangement view - show everything
            layoutState.browserVisible = true;
            layoutState.inspectorVisible = true;
            layoutState.mixerVisible = true;
            layoutState.sessionVisible = false;
            layoutState.flagshipVisible = false;
            layoutState.patternSeqVisible = false;
            layoutState.browserWidth = static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kTrackHeaderWidth) + 50; // FL Studio browser width
            layoutState.inspectorWidth = 320; // Standard inspector width for editing
            layoutState.mixerHeight = static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kMixerFaderHeight); // Professional mixer height
            break;

        case LayoutPreset::Mix:
            // Mixer-focused - maximize mixer, show browser for track selection
            layoutState.browserVisible = true;
            layoutState.inspectorVisible = true;
            layoutState.mixerVisible = true;
            layoutState.sessionVisible = false;
            layoutState.flagshipVisible = false;
            layoutState.patternSeqVisible = false;
            layoutState.browserWidth = static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kTrackHeaderWidth) + 30; // Compact browser for mixing
            layoutState.inspectorWidth = 300; // Compact inspector for mixing
            layoutState.mixerHeight = static_cast<int>(daw::ui::lookandfeel::DesignSystem::Layout::kMixerFaderHeight) * 2; // Expanded mixer for mixing mode
            break;

        case LayoutPreset::Edit:
            // Editing-focused - inspector, pattern sequencer, piano roll area
            layoutState.browserVisible = false;
            layoutState.inspectorVisible = true;
            layoutState.mixerVisible = false;
            layoutState.sessionVisible = false;
            layoutState.flagshipVisible = false;
            layoutState.patternSeqVisible = true;
            layoutState.inspectorWidth = 360;
            layoutState.rightSplitRatio = 0.65f; // More space for inspector
            break;

        case LayoutPreset::Live:
            // Live performance - session launcher, pattern sequencer, flagship
            layoutState.browserVisible = false;
            layoutState.inspectorVisible = false;
            layoutState.mixerVisible = true;
            layoutState.sessionVisible = true;
            layoutState.flagshipVisible = true;
            layoutState.patternSeqVisible = true;
            layoutState.drumMachineVisible = true;
            layoutState.mixerHeight = 180;
            layoutState.sessionHeight = 200;
            layoutState.leftSplitRatio = 0.4f; // More space for flagship
            layoutState.rightSplitRatio = 0.5f;
            break;
    }

    clampLayoutState();
    updateLayoutConstraints();
    applyLayout(true);
    updatePanelTabs();
    saveLayoutState();
}

void MainView::setProjectModel(std::shared_ptr<daw::project::ProjectModel> model)
{
    if (model != projectModel)
    {
        projectModel = model;

        // Update all components that depend on project model
        browserPanel.setProjectModel(model);
        // Note: arrangeView, inspectorPanel, mixerView are constructed with projectModel
        // and will automatically use the new shared_ptr when it's updated
        patternSequencer.setProjectModel(model);
        pianoRollView.setProjectModel(model);

        // Clear undo history when switching projects
        undoManager.clearHistory();

        refreshViews();
    }
}

void MainView::setProjectName(const juce::String& name)
{
    statusStrip.setProjectName(name);
}

void MainView::setInferenceEngine(std::shared_ptr<daw::ai::inference::InferenceEngine> engine)
{
    pianoRollView.setInferenceEngine(engine);
    patternSequencer.setInferenceEngine(engine);
}

void MainView::setAnimationService(std::shared_ptr<animation::AdaptiveAnimationService> service)
{
    animationService = std::move(service);
    if (animationService)
    {
        animationService->attachToComponent(*this);
    }
}

void MainView::setBoundsWithAnimation(juce::Component& comp, juce::Rectangle<int> bounds, bool animated)
{
    if (animated)
    {
        animationHelper.animateBounds(comp, bounds, 300);
    }
    else
    {
        comp.setBounds(bounds);
    }
}

void MainView::mouseDown(const juce::MouseEvent& e)
{
    // Handle mouse down on the main view background
    // For example, to clear focus or selection
    if (e.eventComponent == this)
    {
        grabKeyboardFocus();
        if (projectModel)
        {
            auto& selectionModel = projectModel->getSelectionModel();
            selectionModel.clearTrackSelection();
            selectionModel.clearClipSelection();
        }
    }

    juce::Component::mouseDown(e);
}

} // namespace daw::ui::views
