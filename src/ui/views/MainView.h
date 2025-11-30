#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>
#include <memory>

#include "../lookandfeel/DesignSystem.h"
#include "../../audio/engine/EngineContext.h"
#include "../../project/ProjectModel.h"
#include "../../project/UndoManager.h"
#include "../../ai/inference/InferenceEngine.h"
#include "../../ai/config/AIConfig.h"
#include "TransportBar.h"
#include "BrowserPanel.h"
#include "ArrangeView.h"
#include "InspectorPanel.h"
#include "MixerView.h"
#include "PianoRollView.h"
#include "../components/FlagshipDevicePanel.h"
#include "../components/PatternSequencerPanel.h"
#include "../components/SessionLauncherView.h"
#include "../components/StatusStrip.h"
#include "../components/CommandPalette.h"
#include "../components/DrumMachine.h"
#include "../components/AppCommands.h"
#include "../core/AnimationHelper.h"

namespace daw::ui::animation {
class AdaptiveAnimationService;
}

namespace daw::ui {
class MainWindow;
}

namespace daw::ui::views
{

// Forward declaration
class MainView;

//=========================== Themed Resizer ==========================

class ThemedResizerBar : public juce::StretchableLayoutResizerBar
{
public:
    ThemedResizerBar(juce::StretchableLayoutManager* layoutToUse,
                     int itemIndexInLayout,
                     bool isBarVertical,
                     MainView* parentView = nullptr);

    void paint(juce::Graphics& g) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

private:
    MainView* mainView;
    bool isBarVerticalMember;

    [[nodiscard]] bool isVertical() const noexcept
    {
        return isBarVerticalMember;
    }

    friend class MainView;
};

//=========================== Main View ==============================

class MainView : public juce::Component, public juce::KeyListener
{
public:
    explicit MainView(std::shared_ptr<daw::audio::engine::EngineContext> engineContext);
    ~MainView() override;

    // Access to arrange view for transport integration
    ArrangeView& getArrangeView() { return arrangeView; }

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
    void modifierKeysChanged(const juce::ModifierKeys& modifiers) override;
    void mouseDown(const juce::MouseEvent& e) override;

    // Set parent window for project operations
    void setParentWindow(daw::ui::MainWindow* window) { parentWindow = window; }

    // Get undo manager for command execution
    [[nodiscard]] daw::project::UndoManager* getUndoManager() { return &undoManager; }

    void refreshViews();
    void updatePatternSequencerFromSelection();

    // Access to child panels (for MainWindow integration)
    [[nodiscard]] BrowserPanel* getBrowserPanel() { return &browserPanel; }

    // Project model access
    [[nodiscard]] std::shared_ptr<daw::project::ProjectModel> getProjectModel() const noexcept { return projectModel; }
    void setProjectModel(std::shared_ptr<daw::project::ProjectModel> model);

    // Project name (for status strip)
    void setProjectName(const juce::String& name);

    // AI inference engine
    void setInferenceEngine(std::shared_ptr<daw::ai::inference::InferenceEngine> engine);

    // Adaptive animation service
    void setAnimationService(std::shared_ptr<animation::AdaptiveAnimationService> service);

    // Programmatic toggles (also bound to keys)
    void toggleBrowser();
    void toggleInspector();
    void toggleMixer();
    void toggleSession();
    void toggleFlagship();
    void togglePattern();
    void togglePianoRoll();
    void toggleDrumMachine();

    // Layout presets (workspaces)
    enum class LayoutPreset
    {
        Arrange,  // Full arrangement view
        Mix,      // Mixer-focused
        Edit,     // Editing-focused (piano roll, inspector)
        Live      // Live performance (session launcher, pattern sequencer)
    };
    void applyLayoutPreset(LayoutPreset preset);
    [[nodiscard]] LayoutPreset getCurrentPreset() const noexcept { return layoutState.lastPreset; }
    [[nodiscard]] juce::String getPresetName(LayoutPreset preset) const;

    // Layout export/import
    bool exportLayoutToFile(const juce::File& file) const;
    bool importLayoutFromFile(const juce::File& file);

    // Professional DAW layout dimensions (using design system constants)
    // Note: Access via daw::ui::lookandfeel::DesignSystem::Layout::
    // kTransportHeight = 54px, kStatusStripHeight = 24px
    // kPanelMinWidth/Height, kPanelMaxWidth/Height
    // kTrackHeight = 40px, kTrackHeaderWidth = 200px

private:
    // ---------- Dependencies ----------
    std::shared_ptr<daw::audio::engine::EngineContext> engineContext;
    std::shared_ptr<daw::project::ProjectModel> projectModel;
    daw::project::UndoManager undoManager;
    std::shared_ptr<daw::ai::inference::InferenceEngine> inferenceEngine;
    std::shared_ptr<daw::ai::config::AIConfig> aiConfig;
    std::shared_ptr<animation::AdaptiveAnimationService> animationService;
    daw::ui::MainWindow* parentWindow{nullptr}; // For project save/load operations

    // ---------- Primary child components ----------
    TransportBar transportBar;
    daw::ui::components::StatusStrip statusStrip;
    juce::Component bodyContainer;   // holds center + root resizer + bottom
    juce::Component centerContainer; // holds left dock + resizer + arrange + resizer + right dock
    juce::Component leftContainer;   // browser + resizer + flagship
    juce::Component rightContainer;  // inspector + resizer + pattern
    juce::Component bottomContainer; // session + resizer + mixer

    // Actual user-facing panels
    BrowserPanel browserPanel;
    daw::ui::components::FlagshipDevicePanel flagshipPanel;
    ArrangeView arrangeView;
    InspectorPanel inspectorPanel;
    daw::ui::components::PatternSequencerPanel patternSequencer;
    daw::ui::components::SessionLauncherView sessionLauncher;
    PianoRollView pianoRollView;
    MixerView mixerView;
    daw::ui::components::DrumMachine drumMachine;
    daw::ui::components::CommandPalette commandPalette;
    daw::ui::AppCommands appCommands;
    juce::AudioDeviceManager* drumMachineDeviceManager { nullptr };
    utils::AnimationHelper animationHelper;

    // ---------- Stretchable layout managers ----------
    juce::StretchableLayoutManager bodyLayout;   // vertical: center | bar | bottom
    juce::StretchableLayoutManager centerLayout; // horizontal: left | bar | arrange | bar | right
    juce::StretchableLayoutManager leftLayout;   // vertical: browser | bar | flagship
    juce::StretchableLayoutManager rightLayout;  // vertical: inspector | bar | pattern
    juce::StretchableLayoutManager bottomLayout; // vertical: session | bar | mixer

    // ---------- Resizer bars ----------
    std::unique_ptr<ThemedResizerBar> rootResizer;     // between center and bottom
    std::unique_ptr<ThemedResizerBar> leftResizer;     // between left dock and arrange
    std::unique_ptr<ThemedResizerBar> rightResizer;    // between arrange and right dock
    std::unique_ptr<ThemedResizerBar> leftInnerResizer;   // inside left dock
    std::unique_ptr<ThemedResizerBar> rightInnerResizer;  // inside right dock
    std::unique_ptr<ThemedResizerBar> bottomInnerResizer1; // inside bottom dock (session/piano roll)
    std::unique_ptr<ThemedResizerBar> bottomInnerResizer2; // inside bottom dock (piano roll/mixer)

    // ---------- Layout state ----------
    struct LayoutState
    {
        int browserWidth = 300;
        int inspectorWidth = 320;
        int mixerHeight = 240;
        int sessionHeight = 160; // height of session area inside bottom dock
        float leftSplitRatio = 0.60f;   // browser vs flagship (vertical)
        float rightSplitRatio = 0.55f;  // inspector vs pattern (vertical)
        bool browserVisible = true;
        bool inspectorVisible = true;
        bool mixerVisible = true;
        bool sessionVisible = true;
        bool flagshipVisible = true;
        bool patternSeqVisible = true;
        bool drumMachineVisible = false; // DrumMachine panel (hidden by default)
        bool pianoRollVisible = false; // Hidden by default, shown when clip with pattern is selected
        LayoutPreset lastPreset = LayoutPreset::Arrange;
    };
    LayoutState layoutState;
    LayoutState savedLayoutState; // For restore after maximization

    // ---------- Panel tabs (when collapsed) ----------
    class PanelTab : public juce::Component
    {
    public:
        PanelTab(const juce::String& name, bool isActive, std::function<void()> onClick);
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent&) override;
        void setActive(bool activeState) { active = activeState; repaint(); }

    private:
        juce::String tabName;
        bool active;
        std::function<void()> clickHandler;
    };

    std::unique_ptr<PanelTab> leftTabBrowser;
    std::unique_ptr<PanelTab> leftTabFlagship;
    std::unique_ptr<PanelTab> rightTabInspector;
    std::unique_ptr<PanelTab> rightTabPattern;
    std::unique_ptr<PanelTab> bottomTabSession;
    std::unique_ptr<PanelTab> bottomTabMixer;
    std::unique_ptr<PanelTab> bottomTabPianoRoll;

    void updatePanelTabs();
    void setupPanelTabs();

    // ---------- Maximization state ----------
    bool isMaximized = false;
    juce::Component* maximizedPanel = nullptr;

    // ---------- Persistence ----------
    static std::unique_ptr<juce::PropertiesFile> createLayoutPropsFile();
    void loadFromProps(juce::PropertiesFile&);
    void saveToProps(juce::PropertiesFile&);
    void saveLayoutState();
    void restoreLayoutState();

    // ---------- Layout helpers ----------
    void setupUI();
    void setupLayouts();           // create resizers, initialise layout managers
    void updateLayoutConstraints();// map LayoutState -> StretchableLayoutManager constraints
    void applyLayout(bool animated);
    void clampLayoutState();       // Ensure all values are within valid ranges
    void onResizerDoubleClick(ThemedResizerBar* resizer); // Handle double-click to collapse/expand
    void showPanelContextMenu(juce::Component* panel, const juce::Point<int>& position);
    void maximizePanel(juce::Component* panel); // Maximize a panel (hide others temporarily)
    void restorePanels(); // Restore from maximized state

    void setBoundsWithAnimation(juce::Component& comp, juce::Rectangle<int> bounds, bool animated);

    // ---------- Key bindings ----------
    void setupKeyFocus();
    void setupCommandPalette();
    void showCommandPalette();

    // ---------- Tooltips ----------
    void setupTooltips();

    friend class ThemedResizerBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainView)
};

} // namespace daw::ui::views

