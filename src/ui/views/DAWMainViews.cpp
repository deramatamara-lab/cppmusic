#include "DAWMainViews.hpp"
#include <juce_gui_extra/juce_gui_extra.h>

namespace daw::ui::views
{

// ============================================================================
// ArrangeView Implementation
// ============================================================================

// Timeline ruler component
class ArrangeView::TimelineRuler : public juce::Component
{
public:
    TimelineRuler()
    {
        setSize(800, 32);
    }

    void paint(juce::Graphics& g) override
    {
        const auto& t = ultra::tokens();
        auto bounds = getLocalBounds().toFloat();

        // Background
        g.setColour(t.color.bg1);
        g.fillRect(bounds);
        g.setColour(t.color.panelBorder);
        g.drawHorizontalLine(bounds.getBottom() - 1, bounds.getX(), bounds.getRight());

        // Beat markers - every 16th note
        g.setColour(t.color.textSecondary);
        g.setFont(juce::Font(t.font.familyMono, t.font.size12, juce::Font::plain));

        const float beatsPerView = 16.0f;
        const float beatWidth = bounds.getWidth() / beatsPerView;

        for (int i = 0; i <= beatsPerView; ++i)
        {
            float x = bounds.getX() + i * beatWidth;
            bool isMainBeat = (i % 4) == 0;

            g.setColour(isMainBeat ? t.color.textPrimary : t.color.textSecondary);
            g.drawVerticalLine(x, bounds.getY(), bounds.getBottom() - (isMainBeat ? 0 : 8));

            if (isMainBeat)
            {
                g.drawText(juce::String(i / 4 + 1), juce::Rectangle<float>(x + 4, bounds.getY(), 30, 16),
                          juce::Justification::centredLeft, true);
            }
        }
    }
};

// Individual track lane
class ArrangeView::TrackLane : public juce::Component
{
public:
    TrackLane(const juce::String& trackName) : name(trackName)
    {
        setSize(800, 48);
    }

    void paint(juce::Graphics& g) override
    {
        const auto& t = ultra::tokens();
        auto bounds = getLocalBounds().toFloat();

        // Alternating track colors
        static bool alternate = false;
        alternate = !alternate;
        g.setColour(alternate ? t.color.bg1 : t.color.bg2);
        g.fillRect(bounds);

        // Track name area (left 120px)
        auto nameArea = bounds.removeFromLeft(120);
        g.setColour(t.color.bg2.brighter(0.1f));
        g.fillRect(nameArea);
        g.setColour(t.color.panelBorder);
        g.drawVerticalLine(nameArea.getRight(), nameArea.getY(), nameArea.getBottom());

        // Track name
        g.setColour(t.color.textPrimary);
        g.setFont(juce::Font(t.font.familyBase, t.font.size14, juce::Font::plain));
        g.drawText(name, nameArea.reduced(8, 4), juce::Justification::centredLeft, true);

        // Clip areas (demo clips)
        drawClip(g, juce::Rectangle<float>(140, bounds.getY() + 4, 120, bounds.getHeight() - 8),
                t.color.accentPrimary, "Clip 1");
        drawClip(g, juce::Rectangle<float>(280, bounds.getY() + 4, 80, bounds.getHeight() - 8),
                t.color.accentSecondary, "Clip 2");

        // Bottom border
        g.setColour(t.color.panelBorder);
        g.drawHorizontalLine(bounds.getBottom(), bounds.getX(), bounds.getRight());
    }

private:
    juce::String name;

    void drawClip(juce::Graphics& g, juce::Rectangle<float> clipBounds, juce::Colour color, const juce::String& clipName)
    {
        const auto& t = ultra::tokens();

        // Clip background with gradient
        juce::ColourGradient gradient(color.withAlpha(0.3f), clipBounds.getTopLeft(),
                                    color.withAlpha(0.6f), clipBounds.getBottomRight(), false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(clipBounds, t.radius.s);

        // Clip border
        g.setColour(color);
        g.drawRoundedRectangle(clipBounds, t.radius.s, 1.0f);

        // Waveform preview (stylized)
        g.setColour(color.withAlpha(0.8f));
        auto waveArea = clipBounds.reduced(4);
        for (int i = 0; i < waveArea.getWidth(); i += 3)
        {
            float height = juce::Random::getSystemRandom().nextFloat() * waveArea.getHeight() * 0.6f;
            g.drawVerticalLine(waveArea.getX() + i, waveArea.getCentreY() - height/2,
                              waveArea.getCentreY() + height/2);
        }

        // Clip name
        g.setColour(t.color.textPrimary);
        g.setFont(juce::Font(t.font.familyBase, t.font.size12, juce::Font::plain));
        g.drawText(clipName, clipBounds.reduced(4), juce::Justification::topLeft, true);
    }
};

ArrangeView::ArrangeView()
{
    timelineRuler = std::make_unique<TimelineRuler>();
    addAndMakeVisible(*timelineRuler);

    tracksViewport = std::make_unique<juce::Viewport>();
    addAndMakeVisible(*tracksViewport);

    createTrackLanes();
    setupLayout();
}

void ArrangeView::createTrackLanes()
{
    juce::StringArray trackNames = {
        "Master", "Drums", "Bass", "Synth Lead", "Synth Pad",
        "Vocal", "Guitar", "FX", "Aux 1", "Aux 2"
    };

    auto container = std::make_unique<juce::Component>();
    container->setSize(800, trackNames.size() * 48);

    for (int i = 0; i < trackNames.size(); ++i)
    {
        auto track = std::make_unique<TrackLane>(trackNames[i]);
        track->setBounds(0, i * 48, 800, 48);
        container->addAndMakeVisible(*track);
        trackLanes.add(track.release());
    }

    tracksViewport->setViewedComponent(container.release(), true);
    tracksViewport->setScrollBarsShown(true, true);
}

void ArrangeView::setupLayout()
{
    // Layout will be finalized in resized()
}

void ArrangeView::paint(juce::Graphics& g)
{
    const auto& t = ultra::tokens();
    g.fillAll(t.color.bg0);
}

void ArrangeView::resized()
{
    auto bounds = getLocalBounds();

    // Timeline ruler at top
    rulerArea = bounds.removeFromTop(32);
    timelineRuler->setBounds(rulerArea);

    // Main tracks area
    tracksArea = bounds;
    tracksViewport->setBounds(tracksArea);
}

// ============================================================================
// MixerView Implementation
// ============================================================================

class MixerView::ChannelStrip : public juce::Component, public juce::Timer
{
public:
    ChannelStrip(int channelNumber) : channelNum(channelNumber)
    {
        setSize(channelWidth, 400);

        // Create fader
        fader = std::make_unique<juce::Slider>();
        fader->setSliderStyle(juce::Slider::LinearVertical);
        fader->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        fader->setRange(-60.0, 12.0, 0.1);
        fader->setValue(0.0);
        addAndMakeVisible(*fader);

        // Create meter
        meter = std::make_unique<ultra::PeakRmsMeter>();
        addAndMakeVisible(*meter);

        // Set random levels for demo
        startTimer(100);
    }

    void paint(juce::Graphics& g) override
    {
        const auto& t = ultra::tokens();
        auto bounds = getLocalBounds().toFloat();

        // Channel strip background
        g.setColour(t.color.bg2);
        g.fillRoundedRectangle(bounds, t.radius.m);
        g.setColour(t.color.panelBorder);
        g.drawRoundedRectangle(bounds, t.radius.m, 1.0f);

        // Channel number at bottom
        g.setColour(t.color.textSecondary);
        g.setFont(juce::Font(t.font.familyBase, t.font.size12, juce::Font::bold));
        g.drawText(juce::String(channelNum), bounds.removeFromBottom(20), juce::Justification::centred, true);

        // EQ thumbnail (placeholder)
        auto eqArea = juce::Rectangle<float>(bounds.getX() + 4, bounds.getY() + 8, bounds.getWidth() - 8, 60);
        g.setColour(t.color.bg0);
        g.fillRoundedRectangle(eqArea, t.radius.s);
        g.setColour(t.color.accentPrimary.withAlpha(0.6f));

        // Draw mini EQ curve
        juce::Path eqCurve;
        eqCurve.startNewSubPath(eqArea.getX(), eqArea.getCentreY());
        for (float x = 0; x <= eqArea.getWidth(); x += 4)
        {
            float y = eqArea.getCentreY() + std::sin(x * 0.1f) * 12.0f;
            eqCurve.lineTo(eqArea.getX() + x, y);
        }
        g.strokePath(eqCurve, juce::PathStrokeType(2.0f));
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(80);  // EQ area
        bounds.removeFromBottom(20); // Channel label

        // Meter on the right
        auto meterArea = bounds.removeFromRight(16);
        meter->setBounds(meterArea);

        // Fader takes remaining space
        fader->setBounds(bounds.reduced(4));
    }

    void timerCallback() override
    {
        // Simulate random meter levels
        float peak = juce::Random::getSystemRandom().nextFloat() * 0.8f;
        float rms = peak * 0.7f;
        meter->setLevels(peak, rms);
    }

private:
    int channelNum;
    std::unique_ptr<juce::Slider> fader;
    std::unique_ptr<ultra::PeakRmsMeter> meter;
};

class MixerView::MasterSection : public juce::Component, public juce::Timer
{
public:
    MasterSection()
    {
        setSize(120, 400);

        masterFader = std::make_unique<juce::Slider>();
        masterFader->setSliderStyle(juce::Slider::LinearVertical);
        masterFader->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        masterFader->setRange(-60.0, 12.0, 0.1);
        masterFader->setValue(0.0);
        addAndMakeVisible(*masterFader);

        masterMeter = std::make_unique<ultra::PeakRmsMeter>();
        addAndMakeVisible(*masterMeter);

        startTimer(50);
    }

    void paint(juce::Graphics& g) override
    {
        const auto& t = ultra::tokens();
        auto bounds = getLocalBounds().toFloat();

        // Master section background
        g.setColour(t.color.bg1);
        g.fillRoundedRectangle(bounds, t.radius.l);
        g.setColour(t.color.accentPrimary);
        g.drawRoundedRectangle(bounds, t.radius.l, 2.0f);

        // "MASTER" label
        g.setColour(t.color.textPrimary);
        g.setFont(juce::Font(t.font.familyBase, t.font.size16, juce::Font::bold));
        g.drawText("MASTER", bounds.removeFromBottom(30), juce::Justification::centred, true);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromBottom(30); // Master label

        auto meterArea = bounds.removeFromRight(24);
        masterMeter->setBounds(meterArea);

        masterFader->setBounds(bounds.reduced(8));
    }

    void timerCallback() override
    {
        // Master bus levels
        float peak = juce::Random::getSystemRandom().nextFloat() * 0.9f;
        float rms = peak * 0.8f;
        masterMeter->setLevels(peak, rms);
    }

private:
    std::unique_ptr<juce::Slider> masterFader;
    std::unique_ptr<ultra::PeakRmsMeter> masterMeter;
};

MixerView::MixerView()
{
    setupMixer();
}

void MixerView::setupMixer()
{
    channelsViewport = std::make_unique<juce::Viewport>();
    addAndMakeVisible(*channelsViewport);

    createChannelStrips();

    masterSection = std::make_unique<MasterSection>();
    addAndMakeVisible(*masterSection);
}

void MixerView::createChannelStrips()
{
    auto container = std::make_unique<juce::Component>();
    container->setSize(maxChannels * channelWidth, 400);

    for (int i = 0; i < maxChannels; ++i)
    {
        auto strip = std::make_unique<ChannelStrip>(i + 1);
        strip->setBounds(i * channelWidth, 0, channelWidth, 400);
        container->addAndMakeVisible(*strip);
        channelStrips.add(strip.release());
    }

    channelsViewport->setViewedComponent(container.release(), true);
    channelsViewport->setScrollBarsShown(true, false);
}

void MixerView::paint(juce::Graphics& g)
{
    const auto& t = ultra::tokens();
    g.fillAll(t.color.bg0);
}

void MixerView::resized()
{
    auto bounds = getLocalBounds();

    // Master section on the right
    auto masterArea = bounds.removeFromRight(120);
    masterSection->setBounds(masterArea);

    // Channel strips viewport
    channelsViewport->setBounds(bounds);
}

// ============================================================================
// PianoRollView Implementation
// ============================================================================

class PianoRollView::PianoKeyboard : public juce::Component
{
public:
    PianoKeyboard()
    {
        setSize(60, 8 * 12 * 16); // 8 octaves * 12 keys * 16px height
    }

    void paint(juce::Graphics& g) override
    {
        const auto& t = ultra::tokens();
        auto bounds = getLocalBounds().toFloat();

        g.setColour(t.color.bg1);
        g.fillRect(bounds);

        // Draw piano keys
        const float keyHeight = 16.0f;
        const int totalKeys = 8 * 12; // 8 octaves

        for (int i = 0; i < totalKeys; ++i)
        {
            auto keyBounds = juce::Rectangle<float>(0, bounds.getHeight() - (i + 1) * keyHeight,
                                                  bounds.getWidth(), keyHeight);
            int noteInOctave = i % 12;
            bool isBlackKey = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
                              noteInOctave ==8 || noteInOctave == 10);

            g.setColour(isBlackKey ? t.color.bg0 : t.color.bg2);
            g.fillRect(keyBounds);
            g.setColour(t.color.panelBorder);
            g.drawHorizontalLine(keyBounds.getY(), keyBounds.getX(), keyBounds.getRight());

            // Note name for C notes
            if (noteInOctave == 0)
            {
                g.setColour(t.color.textSecondary);
                g.setFont(juce::Font(t.font.familyBase, 10.0f, juce::Font::plain));
                g.drawText("C" + juce::String(i / 12), keyBounds.reduced(2),
                          juce::Justification::centredLeft, true);
            }
        }
    }
};

class PianoRollView::NoteGrid : public juce::Component
{
public:
    NoteGrid()
    {
        setSize(1600, 8 * 12 * 16); // Wide grid for 4 bars at high resolution
    }

    void paint(juce::Graphics& g) override
    {
        const auto& t = ultra::tokens();
        auto bounds = getLocalBounds();

        g.setColour(t.color.bg0);
        g.fillRect(bounds);

        // Draw cyan grid as specified in PLAN.md
        drawCyanGrid(g, bounds);

        // Draw some demo notes
        drawDemoNotes(g, bounds);
    }

private:
    void drawCyanGrid(juce::Graphics& g, juce::Rectangle<int> area)
    {
        const auto& t = ultra::tokens();
        g.setColour(t.color.accentSecondary.withAlpha(0.25f)); // Cyan grid at 25% opacity

        const float noteHeight = 16.0f;
        const float beatWidth = area.getWidth() / 16.0f; // 16 beats visible

        // Horizontal lines (note separators)
        for (int i = 0; i <= area.getHeight() / noteHeight; ++i)
        {
            float y = area.getY() + i * noteHeight;
            g.drawHorizontalLine(y, area.getX(), area.getRight());
        }

        // Vertical lines (beat grid)
        for (int i = 0; i <= 16; ++i)
        {
            float x = area.getX() + i * beatWidth;
            bool isMainBeat = (i % 4) == 0;
            g.setColour(isMainBeat ? t.color.accentSecondary.withAlpha(0.5f) :
                                   t.color.accentSecondary.withAlpha(0.25f));
            g.drawVerticalLine(x, area.getY(), area.getBottom());
        }
    }

    void drawDemoNotes(juce::Graphics& g, juce::Rectangle<int> area)
    {
        const auto& t = ultra::tokens();
        const float noteHeight = 16.0f;
        const float beatWidth = area.getWidth() / 16.0f;

        // Draw some demo MIDI notes
        struct Note { int key; float start; float length; float velocity; };
        std::vector<Note> notes = {
            {60, 0.0f, 1.0f, 0.8f},   // C4
            {64, 1.0f, 0.5f, 0.6f},   // E4
            {67, 1.5f, 0.5f, 0.7f},   // G4
            {72, 2.0f, 2.0f, 0.9f},   // C5
            {60, 4.0f, 0.25f, 0.5f},  // C4 ghost
            {62, 4.25f, 0.25f, 0.4f}, // D4 ghost
            {64, 4.5f, 1.5f, 0.8f},   // E4
        };

        for (const auto& note : notes)
        {
            auto noteBounds = juce::Rectangle<float>(
                area.getX() + note.start * beatWidth,
                area.getBottom() - (note.key - 36) * noteHeight - noteHeight, // MIDI note 36 = C2
                note.length * beatWidth,
                noteHeight - 1
            );

            // Note color based on velocity
            auto noteColor = note.velocity > 0.5f ? t.color.accentPrimary : t.color.textSecondary;
            if (note.velocity < 0.5f) noteColor = noteColor.withAlpha(0.35f); // Ghost notes at 35%

            g.setColour(noteColor.withAlpha(0.8f));
            g.fillRoundedRectangle(noteBounds, t.radius.s);
            g.setColour(noteColor);
            g.drawRoundedRectangle(noteBounds, t.radius.s, 1.0f);
        }
    }
};

class PianoRollView::VelocityLane : public juce::Component
{
public:
    VelocityLane()
    {
        setSize(1600, 80);
    }

    void paint(juce::Graphics& g) override
    {
        const auto& t = ultra::tokens();
        auto bounds = getLocalBounds().toFloat();

        g.setColour(t.color.bg1);
        g.fillRect(bounds);
        g.setColour(t.color.panelBorder);
        g.drawHorizontalLine(bounds.getY(), bounds.getX(), bounds.getRight());

        // Draw velocity bars with rounded tops
        const float beatWidth = bounds.getWidth() / 16.0f;
        std::vector<float> velocities = {0.8f, 0.6f, 0.7f, 0.9f, 0.5f, 0.4f, 0.8f};

        for (int i = 0; i < (int)velocities.size(); ++i)
        {
            auto barBounds = juce::Rectangle<float>(
                bounds.getX() + i * beatWidth + 2,
                bounds.getBottom() - bounds.getHeight() * velocities[i],
                beatWidth - 4,
                bounds.getHeight() * velocities[i]
            );

            g.setColour(t.color.accentSecondary.withAlpha(0.8f));
            g.fillRoundedRectangle(barBounds, t.radius.s);
        }
    }
};

PianoRollView::PianoRollView()
{
    setupPianoRoll();
}

void PianoRollView::setupPianoRoll()
{
    pianoKeys = std::make_unique<PianoKeyboard>();
    addAndMakeVisible(*pianoKeys);

    noteGrid = std::make_unique<NoteGrid>();

    gridViewport = std::make_unique<juce::Viewport>();
    gridViewport->setViewedComponent(noteGrid.get(), false);
    gridViewport->setScrollBarsShown(true, true);
    addAndMakeVisible(*gridViewport);

    velocityLane = std::make_unique<VelocityLane>();
    addAndMakeVisible(*velocityLane);
}

void PianoRollView::paint(juce::Graphics& g)
{
    const auto& t = ultra::tokens();
    g.fillAll(t.color.bg0);
}

void PianoRollView::resized()
{
    auto bounds = getLocalBounds();

    // Velocity lane at bottom
    auto velocityArea = bounds.removeFromBottom(80);
    auto pianoVelArea = velocityArea.removeFromLeft(60);
    velocityLane->setBounds(velocityArea);

    // Piano keys on left
    auto pianoArea = bounds.removeFromLeft(60);
    pianoKeys->setBounds(pianoArea);

    // Note grid viewport
    gridViewport->setBounds(bounds);
}

void PianoRollView::drawCyanGrid(juce::Graphics& /*g*/, juce::Rectangle<int> /*area*/)
{
    // Implementation moved to NoteGrid::drawCyanGrid
}

// ============================================================================
// DevicesView Implementation
// ============================================================================

class DevicesView::DeviceBrowser : public juce::Component, public juce::ListBoxModel
{
public:
    DeviceBrowser()
    {
        // Create browser tabs
        browserTabs = std::make_unique<ultra::TabBarPro>();
        browserTabs->setTabs({"INSTRUMENTS", "EFFECTS", "SAMPLES"});
        addAndMakeVisible(*browserTabs);

        // Create list
        deviceList = std::make_unique<juce::ListBox>("Devices", this);
        addAndMakeVisible(*deviceList);

        populateDeviceList();
    }

    void paint(juce::Graphics& g) override
    {
        const auto& t = ultra::tokens();
        g.fillAll(t.color.bg1);
        g.setColour(t.color.panelBorder);
        g.drawVerticalLine(getWidth() - 1, 0, getHeight());
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        browserTabs->setBounds(bounds.removeFromTop(36));
        deviceList->setBounds(bounds.reduced(8));
    }

    // ListBoxModel implementation
    int getNumRows() override { return devices.size(); }

    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override
    {
        const auto& t = ultra::tokens();

        if (rowIsSelected)
        {
            g.setColour(t.color.accentPrimary.withAlpha(0.3f));
            g.fillRect(0, 0, width, height);
        }

        if (rowNumber < devices.size())
        {
            g.setColour(rowIsSelected ? t.color.textPrimary : t.color.textSecondary);
            g.setFont(juce::Font(t.font.familyBase, t.font.size14, juce::Font::plain));
            g.drawText(devices[rowNumber], juce::Rectangle<int>(8, 0, width - 16, height),
                      juce::Justification::centredLeft, true);
        }
    }

private:
    std::unique_ptr<ultra::TabBarPro> browserTabs;
    std::unique_ptr<juce::ListBox> deviceList;
    juce::StringArray devices;

    void populateDeviceList()
    {
        devices.add("Bass Synthesizer");
        devices.add("Lead Synthesizer");
        devices.add("Drum Machine");
        devices.add("String Ensemble");
        devices.add("Analog Filter");
        devices.add("Vintage Delay");
        devices.add("Hall Reverb");
        devices.add("Compressor");
        devices.add("EQ Eight");
        devices.add("Spectrum Analyzer");
    }
};

class DevicesView::DeviceRack : public juce::Component
{
public:
    DeviceRack()
    {
        // Create some demo device slots
        createDemoDevices();
    }

    void paint(juce::Graphics& g) override
    {
        const auto& t = ultra::tokens();
        g.fillAll(t.color.bg0);

        // Draw device chain flow
        g.setColour(t.color.panelBorder);
        const int slotHeight = 120;
        for (int i = 0; i < deviceSlots.size() - 1; ++i)
        {
            int centerY = (i + 1) * slotHeight - slotHeight/2;
            g.drawVerticalLine(getWidth()/2, centerY + 40, centerY + 80);
        }
    }

    void resized() override
    {
        const int slotHeight = 120;
        for (int i = 0; i < deviceSlots.size(); ++i)
        {
            deviceSlots[i]->setBounds(8, i * slotHeight + 8, getWidth() - 16, slotHeight - 16);
        }
    }

private:
    juce::OwnedArray<juce::Component> deviceSlots;

    void createDemoDevices()
    {
        juce::StringArray deviceNames = {"Bass Synth", "Analog Filter", "Vintage Delay", "Hall Reverb"};

        for (const auto& name : deviceNames)
        {
            auto device = std::make_unique<DeviceSlot>(name);
            addAndMakeVisible(*device);
            deviceSlots.add(device.release());
        }
    }

    class DeviceSlot : public juce::Component
    {
    public:
        DeviceSlot(const juce::String& deviceName) : name(deviceName)
        {
            // Create some demo knobs
            for (int i = 0; i < 4; ++i)
            {
                auto knob = std::make_unique<ultra::RingSlider>();
                knob->setValue(juce::Random::getSystemRandom().nextFloat());
                addAndMakeVisible(*knob);
                knobs.add(knob.release());
            }
        }

        void paint(juce::Graphics& g) override
        {
            const auto& t = ultra::tokens();
            auto bounds = getLocalBounds().toFloat();

            // Device background
            g.setColour(t.color.bg2);
            g.fillRoundedRectangle(bounds, t.radius.l);
            g.setColour(t.color.panelBorder);
            g.drawRoundedRectangle(bounds, t.radius.l, 1.0f);

            // Device name
            g.setColour(t.color.textPrimary);
            g.setFont(juce::Font(t.font.familyBase, t.font.size16, juce::Font::bold));
            g.drawText(name, bounds.removeFromTop(24).reduced(8), juce::Justification::centredLeft, true);
        }

        void resized() override
        {
            auto bounds = getLocalBounds();
            bounds.removeFromTop(24); // Title area

            const float knobSize = 64.0f;
            const float spacing = 8.0f;
            float totalWidth = knobs.size() * knobSize + (knobs.size() - 1) * spacing;
            float startX = (bounds.getWidth() - totalWidth) / 2.0f;

            for (int i = 0; i < knobs.size(); ++i)
            {
                knobs[i]->setBounds(startX + i * (knobSize + spacing),
                                   bounds.getCentreY() - knobSize/2, knobSize, knobSize);
            }
        }

    private:
        juce::String name;
        juce::OwnedArray<ultra::RingSlider> knobs;
    };
};

class DevicesView::DeviceInspector : public juce::Component
{
public:
    DeviceInspector()
    {
        // Inspector tabs
        inspectorTabs = std::make_unique<ultra::TabBarPro>();
        inspectorTabs->setTabs({"DEVICE", "CHAIN", "BROWSER"});
        addAndMakeVisible(*inspectorTabs);

        // XY pad for device modulation
        xyPad = std::make_unique<ultra::XYPad>();
        addAndMakeVisible(*xyPad);
    }

    void paint(juce::Graphics& g) override
    {
        const auto& t = ultra::tokens();
        g.fillAll(t.color.bg1);
        g.setColour(t.color.panelBorder);
        g.drawVerticalLine(0, 0, getHeight());

        // XY Pad label
        g.setColour(t.color.textSecondary);
        g.setFont(juce::Font(t.font.familyBase, t.font.size14, juce::Font::plain));
        g.drawText("Mod Matrix", 16, 80, 120, 20, juce::Justification::centredLeft, true);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        inspectorTabs->setBounds(bounds.removeFromTop(36));

        // XY pad in the middle
        auto padArea = bounds.removeFromTop(200).reduced(16);
        padArea.removeFromTop(20); // Space for label
        xyPad->setBounds(padArea.withSizeKeepingCentre(160, 160));
    }

private:
    std::unique_ptr<ultra::TabBarPro> inspectorTabs;
    std::unique_ptr<ultra::XYPad> xyPad;
};

DevicesView::DevicesView()
{
    setupDevicesLayout();
}

void DevicesView::setupDevicesLayout()
{
    deviceBrowser = std::make_unique<DeviceBrowser>();
    addAndMakeVisible(*deviceBrowser);

    deviceRack = std::make_unique<DeviceRack>();
    addAndMakeVisible(*deviceRack);

    deviceInspector = std::make_unique<DeviceInspector>();
    addAndMakeVisible(*deviceInspector);
}

void DevicesView::paint(juce::Graphics& g)
{
    const auto& t = ultra::tokens();
    g.fillAll(t.color.bg0);
}

void DevicesView::resized()
{
    auto bounds = getLocalBounds();

    // 12-column grid layout as per PLAN.md
    const float totalWidth = bounds.getWidth();
    const float browserW = totalWidth * browserWidth;
    const float inspectorW = totalWidth * inspectorWidth;

    deviceBrowser->setBounds(bounds.removeFromLeft(browserW));
    deviceInspector->setBounds(bounds.removeFromRight(inspectorW));
    deviceRack->setBounds(bounds); // Remaining space
}

// ============================================================================
// DAWMainContainer Implementation
// ============================================================================

DAWMainContainer::DAWMainContainer()
{
    setupLayout();
    setupCallbacks();

    // Show arrange view by default
    showArrangeView();
}

void DAWMainContainer::setupLayout()
{
    // Header toolbar
    headerToolbar = std::make_unique<ultra::HeaderToolbar>();
    addAndMakeVisible(*headerToolbar);

    // View tabs
    viewTabs = std::make_unique<ultra::TabBarPro>();
    viewTabs->setTabs({"ARRANGE", "MIXER", "PIANO ROLL", "DEVICES"});
    addAndMakeVisible(*viewTabs);

    // Create all views
    arrangeView = std::make_unique<ArrangeView>();
    addChildComponent(*arrangeView);

    mixerView = std::make_unique<MixerView>();
    addChildComponent(*mixerView);

    pianoRollView = std::make_unique<PianoRollView>();
    addChildComponent(*pianoRollView);

    devicesView = std::make_unique<DevicesView>();
    addChildComponent(*devicesView);
}

void DAWMainContainer::setupCallbacks()
{
    viewTabs->onChange = [this](int index)
    {
        switch (index)
        {
            case 0: showArrangeView(); break;
            case 1: showMixerView(); break;
            case 2: showPianoRollView(); break;
            case 3: showDevicesView(); break;
        }
    };

    // Transport callbacks
    headerToolbar->onPlay = []() { DBG("Play pressed"); };
    headerToolbar->onStop = []() { DBG("Stop pressed"); };
    headerToolbar->onRecord = []() { DBG("Record pressed"); };
    headerToolbar->onSettings = []() { DBG("Settings pressed"); };

    // Set demo values
    headerToolbar->setCPULevel(0.37f);
    headerToolbar->setBPM(120.0);
    headerToolbar->setTimeDisplay("01:23.456");
}

void DAWMainContainer::paint(juce::Graphics& g)
{
    const auto& t = ultra::tokens();
    g.fillAll(t.color.bg0);
}

void DAWMainContainer::resized()
{
    auto bounds = getLocalBounds();

    // Header at top
    headerToolbar->setBounds(bounds.removeFromTop(headerHeight));

    // Tabs below header
    viewTabs->setBounds(bounds.removeFromTop(tabBarHeight));

    // Main content area with gutters
    auto contentArea = bounds.reduced(gutterSize);

    // Set bounds for all views
    arrangeView->setBounds(contentArea);
    mixerView->setBounds(contentArea);
    pianoRollView->setBounds(contentArea);
    devicesView->setBounds(contentArea);
}

void DAWMainContainer::showView(const juce::String& viewName)
{
    DAWViewBase* newView = nullptr;
    int tabIndex = 0;

    if (viewName == "Arrange") { newView = arrangeView.get(); tabIndex = 0; }
    else if (viewName == "Mixer") { newView = mixerView.get(); tabIndex = 1; }
    else if (viewName == "Piano Roll") { newView = pianoRollView.get(); tabIndex = 2; }
    else if (viewName == "Devices") { newView = devicesView.get(); tabIndex = 3; }

    if (newView != nullptr)
    {
        switchToView(newView);
        viewTabs->setSelectedTab(tabIndex);
    }
}

void DAWMainContainer::switchToView(DAWViewBase* newView)
{
    if (currentView == newView) return;

    // Hide current view
    if (currentView != nullptr)
    {
        currentView->setVisible(false);
        currentView->deactivate();
    }

    // Show new view
    currentView = newView;
    if (currentView != nullptr)
    {
        currentView->setVisible(true);
        currentView->activate();
        currentView->toFront(true);
    }
}

} // namespace daw::ui::views
