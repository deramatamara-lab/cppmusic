#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/UltraDesignSystem.hpp"
#include <memory>

namespace daw::ui::views
{

// ============================================================================
// Base DAW View Interface
// ============================================================================
class DAWViewBase : public juce::Component
{
public:
    DAWViewBase() = default;
    virtual ~DAWViewBase() = default;

    virtual void activate() {}
    virtual void deactivate() {}
    virtual juce::String getViewName() const = 0;

protected:
    bool isActive = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DAWViewBase)
};

// ============================================================================
// Arrange View - Central timeline with tracks and clips
// ============================================================================
class ArrangeView : public DAWViewBase
{
public:
    ArrangeView();
    ~ArrangeView() override = default;

    juce::String getViewName() const override { return "Arrange"; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class TimelineRuler;
    class TrackLane;
    class ClipEditor;

    std::unique_ptr<TimelineRuler> timelineRuler;
    juce::OwnedArray<TrackLane> trackLanes;
    std::unique_ptr<juce::Viewport> tracksViewport;

    // Layout areas
    juce::Rectangle<int> rulerArea, tracksArea, bottomPanel;

    void setupLayout();
    void createTrackLanes();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArrangeView)
};

// ============================================================================
// Mixer View - Multi-channel mixing console
// ============================================================================
class MixerView : public DAWViewBase
{
public:
    MixerView();
    ~MixerView() override = default;

    juce::String getViewName() const override { return "Mixer"; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class ChannelStrip;
    class MasterSection;

    juce::OwnedArray<ChannelStrip> channelStrips;
    std::unique_ptr<MasterSection> masterSection;
    std::unique_ptr<juce::Viewport> channelsViewport;

    static constexpr int maxChannels = 32;
    static constexpr int channelWidth = 76;

    void setupMixer();
    void createChannelStrips();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerView)
};

// ============================================================================
// Piano Roll View - MIDI note editor with cyan grid
// ============================================================================
class PianoRollView : public DAWViewBase
{
public:
    PianoRollView();
    ~PianoRollView() override = default;

    juce::String getViewName() const override { return "Piano Roll"; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class PianoKeyboard;
    class NoteGrid;
    class VelocityLane;

    std::unique_ptr<PianoKeyboard> pianoKeys;
    std::unique_ptr<NoteGrid> noteGrid;
    std::unique_ptr<VelocityLane> velocityLane;
    std::unique_ptr<juce::Viewport> gridViewport;

    // Grid parameters
    int keysPerOctave = 12;
    int visibleOctaves = 8;
    float noteHeight = 16.0f;

    void setupPianoRoll();
    void drawCyanGrid (juce::Graphics& g, juce::Rectangle<int> area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollView)
};

// ============================================================================
// Devices View - Plugin host and device chains
// ============================================================================
class DevicesView : public DAWViewBase
{
public:
    DevicesView();
    ~DevicesView() override = default;

    juce::String getViewName() const override { return "Devices"; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class DeviceRack;
    class DeviceBrowser;
    class DeviceInspector;

    std::unique_ptr<DeviceBrowser> deviceBrowser;
    std::unique_ptr<DeviceRack> deviceRack;
    std::unique_ptr<DeviceInspector> deviceInspector;

    // Layout proportions (12-column grid as per PLAN.md)
    static constexpr float browserWidth = 0.25f;  // 3 columns
    static constexpr float rackWidth = 0.50f;     // 6 columns
    static constexpr float inspectorWidth = 0.25f; // 3 columns

    void setupDevicesLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DevicesView)
};

// ============================================================================
// Main DAW Container - Manages all views and global navigation
// ============================================================================
class DAWMainContainer : public juce::Component
{
public:
    DAWMainContainer();
    ~DAWMainContainer() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

    void showView (const juce::String& viewName);
    void showArrangeView() { showView("Arrange"); }
    void showMixerView() { showView("Mixer"); }
    void showPianoRollView() { showView("Piano Roll"); }
    void showDevicesView() { showView("Devices"); }

private:
    // Header with transport and navigation
    std::unique_ptr<ultra::HeaderToolbar> headerToolbar;
    std::unique_ptr<ultra::TabBarPro> viewTabs;

    // Main views
    std::unique_ptr<ArrangeView> arrangeView;
    std::unique_ptr<MixerView> mixerView;
    std::unique_ptr<PianoRollView> pianoRollView;
    std::unique_ptr<DevicesView> devicesView;

    DAWViewBase* currentView = nullptr;

    // Layout constants from PLAN.md
    static constexpr int headerHeight = 64;
    static constexpr int tabBarHeight = 48;
    static constexpr int gutterSize = 16;

    void setupLayout();
    void setupCallbacks();
    void switchToView (DAWViewBase* newView);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DAWMainContainer)
};

} // namespace daw::ui::views
