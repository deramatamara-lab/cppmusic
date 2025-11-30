#include "ComponentLibrary.hpp"
#include <juce_gui_extra/juce_gui_extra.h>

namespace daw::ui::showcase
{

// ============================================================================
// Color Palette Display
// ============================================================================
class ComponentLibraryShowcase::TokensSection::ColorPalette : public juce::Component
{
public:
    ColorPalette()
    {
        setSize(800, 400);
    }

    void paint(juce::Graphics& g) override
    {
        const auto& t = ultra::tokens();
        auto bounds = getLocalBounds().toFloat();

        // Section title
        g.setColour(t.color.textPrimary);
        g.setFont(juce::Font(t.font.familyBase, t.font.size24, juce::Font::bold));
        g.drawText("Color Tokens", bounds.removeFromTop(40), juce::Justification::centredLeft, true);

        // Color swatches in organized groups
        drawColorGroup(g, bounds.removeFromTop(80), "Backgrounds", {
            {"bg/0", t.color.bg0},
            {"bg/1", t.color.bg1},
            {"bg/2", t.color.bg2},
            {"panel/border", t.color.panelBorder}
        });

        drawColorGroup(g, bounds.removeFromTop(80), "Text", {
            {"text/primary", t.color.textPrimary},
            {"text/secondary", t.color.textSecondary}
        });

        drawColorGroup(g, bounds.removeFromTop(80), "Accents", {
            {"accent/primary", t.color.accentPrimary},
            {"accent/secondary", t.color.accentSecondary},
            {"accent/warn", t.color.warn},
            {"accent/danger", t.color.danger}
        });

        drawColorGroup(g, bounds.removeFromTop(80), "Meters", {
            {"meter/ok", t.color.meterOK},
            {"meter/hot", t.color.meterHot},
            {"meter/clip", t.color.meterClip}
        });

        drawColorGroup(g, bounds, "Grids & Effects", {
            {"graph/grid", t.color.graphGrid},
            {"graph/gridSubtle", t.color.graphGridSubtle},
            {"shadow/soft", t.color.shadowSoft}
        });
    }

private:
    void drawColorGroup(juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title,
                       const std::vector<std::pair<juce::String, juce::Colour>>& colors)
    {
        const auto& t = ultra::tokens();

        // Group title
        g.setColour(t.color.textSecondary);
        g.setFont(juce::Font(t.font.familyBase, t.font.size16, juce::Font::bold));
        auto titleArea = area.removeFromTop(24);
        g.drawText(title, titleArea, juce::Justification::centredLeft, true);

        // Color swatches
        const float swatchSize = 48.0f;
        const float spacing = 16.0f;
        float x = area.getX();

        for (const auto& [name, color] : colors)
        {
            auto swatchBounds = juce::Rectangle<float>(x, area.getY(), swatchSize, swatchSize);

            // Swatch background
            g.setColour(color);
            g.fillRoundedRectangle(swatchBounds, t.radius.s);
            g.setColour(t.color.panelBorder);
            g.drawRoundedRectangle(swatchBounds, t.radius.s, 1.0f);

            // Color name below swatch
            g.setColour(t.color.textSecondary);
            g.setFont(juce::Font(t.font.familyMono, t.font.size12, juce::Font::plain));
            g.drawText(name, juce::Rectangle<float>(x, area.getY() + swatchSize + 4, swatchSize + 40, 16),
                      juce::Justification::centredLeft, true);

            // Hex value
            g.drawText(color.toString(), juce::Rectangle<float>(x, area.getY() + swatchSize + 20, swatchSize + 40, 16),
                      juce::Justification::centredLeft, true);

            x += swatchSize + spacing + 40;
        }
    }
};

// ============================================================================
// Typography Scale Display
// ============================================================================
class ComponentLibraryShowcase::TokensSection::TypographyScale : public juce::Component
{
public:
    TypographyScale()
    {
        setSize(800, 300);
    }

    void paint(juce::Graphics& g) override
    {
        const auto& t = ultra::tokens();
        auto bounds = getLocalBounds().toFloat();

        // Section title
        g.setColour(t.color.textPrimary);
        g.setFont(juce::Font(t.font.familyBase, t.font.size24, juce::Font::bold));
        g.drawText("Typography Scale", bounds.removeFromTop(40), juce::Justification::centredLeft, true);

        // Typography examples
        const std::vector<std::tuple<juce::String, float, juce::String>> typeExamples = {
            {"size/32", t.font.size32, "Display - The quick brown fox"},
            {"size/24", t.font.size24, "Heading 1 - The quick brown fox"},
            {"size/18", t.font.size18, "Heading 2 - The quick brown fox"},
            {"size/16", t.font.size16, "Title - The quick brown fox"},
            {"size/14", t.font.size14, "Body - The quick brown fox jumps over the lazy dog"},
            {"size/12", t.font.size12, "Caption - The quick brown fox jumps over the lazy dog"}
        };

        float y = bounds.getY();
        for (const auto& [name, size, text] : typeExamples)
        {
            // Size label
            g.setColour(t.color.textSecondary);
            g.setFont(juce::Font(t.font.familyMono, t.font.size12, juce::Font::plain));
            g.drawText(name, juce::Rectangle<float>(bounds.getX(), y, 80, 20),
                      juce::Justification::centredLeft, true);

            // Text example
            g.setColour(t.color.textPrimary);
            g.setFont(juce::Font(t.font.familyBase, size, juce::Font::plain));
            g.drawText(text, juce::Rectangle<float>(bounds.getX() + 100, y, bounds.getWidth() - 100, size + 8),
                      juce::Justification::centredLeft, true);

            y += size + 16;
        }

        // Font families
        y += 20;
        g.setColour(t.color.textSecondary);
        g.setFont(juce::Font(t.font.familyBase, t.font.size14, juce::Font::bold));
        g.drawText("Font Families:", juce::Rectangle<float>(bounds.getX(), y, 200, 20),
                  juce::Justification::centredLeft, true);

        y += 24;
        g.setFont(juce::Font(t.font.familyBase, t.font.size14, juce::Font::plain));
        g.drawText("Base: " + t.font.familyBase, juce::Rectangle<float>(bounds.getX(), y, 400, 20),
                  juce::Justification::centredLeft, true);

        y += 20;
        g.setFont(juce::Font(t.font.familyMono, t.font.size14, juce::Font::plain));
        g.drawText("Mono: " + t.font.familyMono, juce::Rectangle<float>(bounds.getX(), y, 400, 20),
                  juce::Justification::centredLeft, true);
    }
};

// ============================================================================
// Spacing Grid Display
// ============================================================================
class ComponentLibraryShowcase::TokensSection::SpacingGrid : public juce::Component
{
public:
    SpacingGrid()
    {
        setSize(800, 200);
    }

    void paint(juce::Graphics& g) override
    {
        const auto& t = ultra::tokens();
        auto bounds = getLocalBounds().toFloat();

        // Section title
        g.setColour(t.color.textPrimary);
        g.setFont(juce::Font(t.font.familyBase, t.font.size24, juce::Font::bold));
        g.drawText("Spacing Tokens", bounds.removeFromTop(40), juce::Justification::centredLeft, true);

        // Spacing examples
        const std::vector<std::pair<juce::String, int>> spacingTokens = {
            {"2px", t.space.s2},
            {"4px", t.space.s4},
            {"6px", t.space.s6},
            {"8px", t.space.s8},
            {"12px", t.space.s12},
            {"16px", t.space.s16},
            {"24px", t.space.s24},
            {"32px", t.space.s32}
        };

        float x = bounds.getX();
        const float maxBarHeight = 60.0f;

        for (const auto& [name, value] : spacingTokens)
        {
            // Spacing bar (visual representation)
            float barHeight = (float)value / 32.0f * maxBarHeight;
            auto barBounds = juce::Rectangle<float>(x, bounds.getBottom() - barHeight - 40, 20, barHeight);

            g.setColour(t.color.accentPrimary.withAlpha(0.6f));
            g.fillRoundedRectangle(barBounds, t.radius.s);
            g.setColour(t.color.accentPrimary);
            g.drawRoundedRectangle(barBounds, t.radius.s, 1.0f);

            // Token name
            g.setColour(t.color.textSecondary);
            g.setFont(juce::Font(t.font.familyMono, t.font.size12, juce::Font::plain));
            g.drawText(name, juce::Rectangle<float>(x - 10, bounds.getBottom() - 30, 40, 16),
                      juce::Justification::centred, true);

            // Pixel value
            g.drawText(juce::String(value), juce::Rectangle<float>(x - 10, bounds.getBottom() - 16, 40, 16),
                      juce::Justification::centred, true);

            x += 60;
        }
    }
};

// ============================================================================
// Radius Examples Display
// ============================================================================
class ComponentLibraryShowcase::TokensSection::RadiusExamples : public juce::Component
{
public:
    RadiusExamples()
    {
        setSize(800, 150);
    }

    void paint(juce::Graphics& g) override
    {
        const auto& t = ultra::tokens();
        auto bounds = getLocalBounds().toFloat();

        // Section title
        g.setColour(t.color.textPrimary);
        g.setFont(juce::Font(t.font.familyBase, t.font.size24, juce::Font::bold));
        g.drawText("Radius Tokens", bounds.removeFromTop(40), juce::Justification::centredLeft, true);

        // Radius examples
        const std::vector<std::pair<juce::String, float>> radiusTokens = {
            {"s (8px)", t.radius.s},
            {"m (12px)", t.radius.m},
            {"l (16px)", t.radius.l},
            {"xl (22px)", t.radius.xl}
        };

        float x = bounds.getX();
        const float rectSize = 80.0f;
        const float spacing = 120.0f;

        for (const auto& [name, radius] : radiusTokens)
        {
            // Example rectangle with radius
            auto rectBounds = juce::Rectangle<float>(x, bounds.getY() + 20, rectSize, rectSize);

            g.setColour(t.color.bg2);
            g.fillRoundedRectangle(rectBounds, radius);
            g.setColour(t.color.accentPrimary);
            g.drawRoundedRectangle(rectBounds, radius, 2.0f);

            // Token name
            g.setColour(t.color.textSecondary);
            g.setFont(juce::Font(t.font.familyMono, t.font.size12, juce::Font::plain));
            g.drawText(name, juce::Rectangle<float>(x, bounds.getY() + rectSize + 30, rectSize, 16),
                      juce::Justification::centred, true);

            x += spacing;
        }
    }
};

// ============================================================================
// Knobs and Sliders Showcase
// ============================================================================
class ComponentLibraryShowcase::ComponentsSection::KnobsAndSliders : public juce::Component
{
public:
    KnobsAndSliders()
    {
        setSize(800, 300);

        // Create various knob examples
        for (int i = 0; i < 4; ++i)
        {
            auto knob = std::make_unique<ultra::RingSlider>();
            knob->setRange(-12.0, 12.0, 0.01);
            knob->setValue((i - 2) * 3.0); // Different values for demo
            addAndMakeVisible(*knob);
            knobs.add(knob.release());
        }

        // Create XY pad
        xyPad = std::make_unique<ultra::XYPad>();
        xyPad->setValue(0.3f, 0.7f);
        addAndMakeVisible(*xyPad);
    }

    void paint(juce::Graphics& g) override
    {
        const auto& t = ultra::tokens();
        auto bounds = getLocalBounds().toFloat();

        // Section title
        g.setColour(t.color.textPrimary);
        g.setFont(juce::Font(t.font.familyBase, t.font.size24, juce::Font::bold));
        g.drawText("Knobs & Controllers", bounds.removeFromTop(40), juce::Justification::centredLeft, true);

        // Component labels
        g.setColour(t.color.textSecondary);
        g.setFont(juce::Font(t.font.familyBase, t.font.size14, juce::Font::plain));

        // Ring slider labels
        const juce::StringArray knobLabels = {"Frequency", "Resonance", "Drive", "Mix"};
        for (int i = 0; i < knobs.size(); ++i)
        {
            g.drawText(knobLabels[i], juce::Rectangle<float>(20 + i * 140, 200, 120, 20),
                      juce::Justification::centred, true);
        }

        // XY pad label
        g.drawText("XY Modulation", juce::Rectangle<float>(580, 50, 160, 20),
                  juce::Justification::centred, true);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(40); // Title space

        // Position knobs in a row
        const int knobSize = 120;
        const int knobSpacing = 140;
        for (int i = 0; i < knobs.size(); ++i)
        {
            knobs[i]->setBounds(20 + i * knobSpacing, 50, knobSize, knobSize);
        }

        // XY pad on the right
        xyPad->setBounds(580, 70, 160, 160);
    }

private:
    juce::OwnedArray<ultra::RingSlider> knobs;
    std::unique_ptr<ultra::XYPad> xyPad;
};

// ============================================================================
// Buttons and Toggles Showcase
// ============================================================================
class ComponentLibraryShowcase::ComponentsSection::ButtonsAndToggles : public juce::Component
{
public:
    ButtonsAndToggles()
    {
        setSize(800, 200);

        // Create pill toggles
        const juce::StringArray toggleLabels = {"SNAP", "LOOP", "GRID", "SYNC"};
        for (const auto& label : toggleLabels)
        {
            auto toggle = std::make_unique<ultra::PillToggle>(label);
            toggle->setToggleState(juce::Random::getSystemRandom().nextBool(), juce::dontSendNotification);
            addAndMakeVisible(*toggle);
            pillToggles.add(toggle.release());
        }

        // Create tab bar
        tabBar = std::make_unique<ultra::TabBarPro>();
        tabBar->setTabs({"NORMAL", "MIDI", "CHORD"});
        addAndMakeVisible(*tabBar);

        // Create regular buttons
        playButton = std::make_unique<juce::TextButton>("Play");
        stopButton = std::make_unique<juce::TextButton>("Stop");
        recordButton = std::make_unique<juce::TextButton>("Record");

        addAndMakeVisible(*playButton);
        addAndMakeVisible(*stopButton);
        addAndMakeVisible(*recordButton);
    }

    void paint(juce::Graphics& g) override
    {
        const auto& t = ultra::tokens();
        auto bounds = getLocalBounds().toFloat();

        // Section title
        g.setColour(t.color.textPrimary);
        g.setFont(juce::Font(t.font.familyBase, t.font.size24, juce::Font::bold));
        g.drawText("Buttons & Navigation", bounds.removeFromTop(40), juce::Justification::centredLeft, true);

        // Component labels
        g.setColour(t.color.textSecondary);
        g.setFont(juce::Font(t.font.familyBase, t.font.size14, juce::Font::plain));
        g.drawText("Pill Toggles:", juce::Rectangle<float>(20, 50, 120, 20),
                  juce::Justification::centredLeft, true);
        g.drawText("Tab Bar:", juce::Rectangle<float>(20, 100, 120, 20),
                  juce::Justification::centredLeft, true);
        g.drawText("Transport:", juce::Rectangle<float>(20, 150, 120, 20),
                  juce::Justification::centredLeft, true);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(40); // Title space

        // Pill toggles
        int x = 150;
        for (auto* toggle : pillToggles)
        {
            toggle->setBounds(x, 45, 80, 28);
            x += 90;
        }

        // Tab bar
        tabBar->setBounds(150, 95, 300, 36);

        // Transport buttons
        x = 150;
        playButton->setBounds(x, 145, 60, 32); x += 70;
        stopButton->setBounds(x, 145, 60, 32); x += 70;
        recordButton->setBounds(x, 145, 60, 32);
    }

private:
    juce::OwnedArray<ultra::PillToggle> pillToggles;
    std::unique_ptr<ultra::TabBarPro> tabBar;
    std::unique_ptr<juce::TextButton> playButton, stopButton, recordButton;
};

// ============================================================================
// Meters and Indicators Showcase
// ============================================================================
class ComponentLibraryShowcase::ComponentsSection::MetersAndIndicators : public juce::Component,
                                                                          private juce::Timer
{
public:
    MetersAndIndicators()
    {
        setSize(800, 250);

        // Create meters
        for (int i = 0; i < 6; ++i)
        {
            auto meter = std::make_unique<ultra::PeakRmsMeter>();
            addAndMakeVisible(*meter);
            meters.add(meter.release());
        }

        startTimer(50); // Update meter levels
    }

    void paint(juce::Graphics& g) override
    {
        const auto& t = ultra::tokens();
        auto bounds = getLocalBounds().toFloat();

        // Section title
        g.setColour(t.color.textPrimary);
        g.setFont(juce::Font(t.font.familyBase, t.font.size24, juce::Font::bold));
        g.drawText("Meters & Indicators", bounds.removeFromTop(40), juce::Justification::centredLeft, true);

        // Component labels
        g.setColour(t.color.textSecondary);
        g.setFont(juce::Font(t.font.familyBase, t.font.size12, juce::Font::plain));

        const juce::StringArray meterLabels = {"L", "R", "AUX1", "AUX2", "FX", "MASTER"};
        for (int i = 0; i < meters.size(); ++i)
        {
            g.drawText(meterLabels[i], juce::Rectangle<float>(50 + i * 80, 190, 24, 16),
                      juce::Justification::centred, true);
        }

        // Level indicators
        g.drawText("Peak/RMS Meters with Hold and Clipping", juce::Rectangle<float>(50, 50, 400, 20),
                  juce::Justification::centredLeft, true);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(40); // Title space

        // Position meters in a row
        for (int i = 0; i < meters.size(); ++i)
        {
            meters[i]->setBounds(50 + i * 80, 70, 24, 120);
        }
    }

    void timerCallback() override
    {
        // Animate meter levels
        for (int i = 0; i < meters.size(); ++i)
        {
            float phase = juce::Time::getMillisecondCounter() * 0.003f + i * 0.7f;
            float peak = (std::sin(phase) * 0.4f + 0.5f) * 0.9f;
            float rms = peak * 0.7f;

            // Occasionally add some hot/clip levels
            if (juce::Random::getSystemRandom().nextFloat() < 0.1f)
                peak = juce::jmin(1.0f, peak + 0.3f);

            meters[i]->setLevels(peak, rms);
        }
    }

private:
    juce::OwnedArray<ultra::PeakRmsMeter> meters;
};

// ============================================================================
// Navigation Elements Showcase
// ============================================================================
class ComponentLibraryShowcase::ComponentsSection::NavigationElements : public juce::Component
{
public:
    NavigationElements()
    {
        setSize(800, 150);

        // Create header toolbar
        headerToolbar = std::make_unique<ultra::HeaderToolbar>();
        headerToolbar->setCPULevel(0.42f);
        headerToolbar->setBPM(128.0);
        headerToolbar->setTimeDisplay("02:15.840");
        addAndMakeVisible(*headerToolbar);
    }

    void paint(juce::Graphics& g) override
    {
        const auto& t = ultra::tokens();
        auto bounds = getLocalBounds().toFloat();

        // Section title
        g.setColour(t.color.textPrimary);
        g.setFont(juce::Font(t.font.familyBase, t.font.size24, juce::Font::bold));
        g.drawText("Navigation & Transport", bounds.removeFromTop(40), juce::Justification::centredLeft, true);

        // Component description
        g.setColour(t.color.textSecondary);
        g.setFont(juce::Font(t.font.familyBase, t.font.size14, juce::Font::plain));
        g.drawText("Transport controls, time display, BPM, CPU meter, and settings",
                  juce::Rectangle<float>(20, 50, 600, 20), juce::Justification::centredLeft, true);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(40); // Title space
        bounds.removeFromTop(30); // Description space

        headerToolbar->setBounds(20, bounds.getY(), getWidth() - 40, 64);
    }

private:
    std::unique_ptr<ultra::HeaderToolbar> headerToolbar;
};

// ============================================================================
// Main TokensSection Implementation
// ============================================================================
ComponentLibraryShowcase::TokensSection::TokensSection()
{
    colorPalette = std::make_unique<ColorPalette>();
    addAndMakeVisible(*colorPalette);

    typography = std::make_unique<TypographyScale>();
    addAndMakeVisible(*typography);

    spacing = std::make_unique<SpacingGrid>();
    addAndMakeVisible(*spacing);

    radii = std::make_unique<RadiusExamples>();
    addAndMakeVisible(*radii);
}

ComponentLibraryShowcase::TokensSection::~TokensSection() = default;

void ComponentLibraryShowcase::TokensSection::paint(juce::Graphics& g)
{
    const auto& t = ultra::tokens();
    g.setColour(t.color.textPrimary);
    g.setFont(juce::Font(t.font.familyBase, t.font.size32, juce::Font::bold));
    g.drawText("Design System Tokens", 20, 20, getWidth() - 40, 40, juce::Justification::centredLeft, true);
}

void ComponentLibraryShowcase::TokensSection::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(60); // Main title space

    colorPalette->setBounds(bounds.removeFromTop(400));
    typography->setBounds(bounds.removeFromTop(300));
    spacing->setBounds(bounds.removeFromTop(200));
    radii->setBounds(bounds.removeFromTop(150));
}

// ============================================================================
// Main ComponentsSection Implementation
// ============================================================================
ComponentLibraryShowcase::ComponentsSection::ComponentsSection()
{
    knobsSliders = std::make_unique<KnobsAndSliders>();
    addAndMakeVisible(*knobsSliders);

    buttons = std::make_unique<ButtonsAndToggles>();
    addAndMakeVisible(*buttons);

    meters = std::make_unique<MetersAndIndicators>();
    addAndMakeVisible(*meters);

    navigation = std::make_unique<NavigationElements>();
    addAndMakeVisible(*navigation);
}

ComponentLibraryShowcase::ComponentsSection::~ComponentsSection() = default;

void ComponentLibraryShowcase::ComponentsSection::paint(juce::Graphics& g)
{
    const auto& t = ultra::tokens();
    g.setColour(t.color.textPrimary);
    g.setFont(juce::Font(t.font.familyBase, t.font.size32, juce::Font::bold));
    g.drawText("UI Components", 20, 20, getWidth() - 40, 40, juce::Justification::centredLeft, true);
}

void ComponentLibraryShowcase::ComponentsSection::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(60); // Main title space

    knobsSliders->setBounds(bounds.removeFromTop(300));
    buttons->setBounds(bounds.removeFromTop(200));
    meters->setBounds(bounds.removeFromTop(250));
    navigation->setBounds(bounds.removeFromTop(150));
}

// ============================================================================
// Animation Section (Placeholder - would need JUCE animation framework)
// ============================================================================

class ComponentLibraryShowcase::AnimationSection::KnobAnimations : public juce::Component {};
class ComponentLibraryShowcase::AnimationSection::TabTransitions : public juce::Component {};
class ComponentLibraryShowcase::AnimationSection::MeterAnimations : public juce::Component {};
class ComponentLibraryShowcase::AnimationSection::HoverEffects : public juce::Component {};

ComponentLibraryShowcase::AnimationSection::AnimationSection()
{
    // Note: Full animation implementation would require JUCE 8 animation module
    // This is a placeholder showing the structure for animation demos
}

ComponentLibraryShowcase::AnimationSection::~AnimationSection() = default;

void ComponentLibraryShowcase::AnimationSection::paint(juce::Graphics& g)
{
    const auto& t = ultra::tokens();
    g.setColour(t.color.textPrimary);
    g.setFont(juce::Font(t.font.familyBase, t.font.size32, juce::Font::bold));
    g.drawText("Animations & Micro-interactions", 20, 20, getWidth() - 40, 40, juce::Justification::centredLeft, true);

    // Animation timing info
    g.setColour(t.color.textSecondary);
    g.setFont(juce::Font(t.font.familyBase, t.font.size16, juce::Font::plain));
    g.drawText("Animation Timings:", 20, 80, 200, 24, juce::Justification::centredLeft, true);

    auto& anim = t.anim;
    g.setFont(juce::Font(t.font.familyMono, t.font.size14, juce::Font::plain));
    g.drawText(juce::String("Fast: ") + juce::String(anim.msFast) + "ms", 20, 110, 200, 20, juce::Justification::centredLeft, true);
    g.drawText(juce::String("Medium: ") + juce::String(anim.msMed) + "ms", 20, 130, 200, 20, juce::Justification::centredLeft, true);
    g.drawText(juce::String("Slow: ") + juce::String(anim.msSlow) + "ms", 20, 150, 200, 20, juce::Justification::centredLeft, true);

    g.drawText("Easing Standard: cubic-bezier(0.22, 1, 0.36, 1)", 250, 110, 400, 20, juce::Justification::centredLeft, true);
    g.drawText("Easing In-Out: cubic-bezier(0.4, 0, 0.2, 1)", 250, 130, 400, 20, juce::Justification::centredLeft, true);
    g.drawText("Spring Knob: stiffness=360, damping=26, mass=1", 250, 150, 400, 20, juce::Justification::centredLeft, true);
}

void ComponentLibraryShowcase::AnimationSection::resized()
{
    // Animation demos would be positioned here
}

// ============================================================================
// Main ComponentLibraryShowcase Implementation
// ============================================================================
ComponentLibraryShowcase::ComponentLibraryShowcase()
{
    setupSections();
}

void ComponentLibraryShowcase::setupSections()
{
    // Create main viewport for scrolling
    mainViewport = std::make_unique<juce::Viewport>();
    addAndMakeVisible(*mainViewport);

    // Create content container
    contentContainer = std::make_unique<juce::Component>();

    // Create sections
    tokensSection = std::make_unique<TokensSection>();
    contentContainer->addAndMakeVisible(*tokensSection);

    componentsSection = std::make_unique<ComponentsSection>();
    contentContainer->addAndMakeVisible(*componentsSection);

    animationSection = std::make_unique<AnimationSection>();
    contentContainer->addAndMakeVisible(*animationSection);

    // Calculate total content height
    int totalHeight = 1050 + 900 + 300; // Approximate heights
    contentContainer->setSize(800, totalHeight);

    // Position sections
    tokensSection->setBounds(0, 0, 800, 1050);
    componentsSection->setBounds(0, 1050, 800, 900);
    animationSection->setBounds(0, 1950, 800, 300);

    mainViewport->setViewedComponent(contentContainer.get(), false);
    mainViewport->setScrollBarsShown(true, false);
}

void ComponentLibraryShowcase::paint(juce::Graphics& g)
{
    const auto& t = ultra::tokens();
    g.fillAll(t.color.bg0);
}

void ComponentLibraryShowcase::resized()
{
    mainViewport->setBounds(getLocalBounds());
    contentContainer->setSize(getWidth(), contentContainer->getHeight());

    // Update section widths
    tokensSection->setSize(getWidth(), tokensSection->getHeight());
    componentsSection->setSize(getWidth(), componentsSection->getHeight());
    animationSection->setSize(getWidth(), animationSection->getHeight());
}

} // namespace daw::ui::showcase
