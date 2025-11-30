#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_data_structures/juce_data_structures.h>
#if JUCE_MODULE_AVAILABLE_juce_animation
#include <juce_animation/juce_animation.h>
#endif
#include <functional>
#include <atomic>
#include <memory>
#include <cmath>
#include "../../core/ServiceLocator.h"
#include "../animation/AdaptiveAnimationService.h"

namespace ultra
{

// ============================================================================
// TOKENS (JSON-overridable) - Exact PLAN.md specification
// ============================================================================
struct Tokens
{
    struct Colors
    {
        // Exact color tokens from PLAN.md
        // Tuned towards a modern FL Studio–style dark neon palette
        juce::Colour bg0{ 0xFF101015 };        // "bg/0": deep charcoal
        juce::Colour bg1{ 0xFF161821 };        // "bg/1": surface 1
        juce::Colour bg2{ 0xFF1F222C };        // "bg/2": surface 2
        juce::Colour panelBorder{ 0xFF303544 }; // "panel/border"
        juce::Colour textPrimary{ 0xFFE8ECF7 }; // "text/primary"
        juce::Colour textSecondary{ 0xFFA2A8BC }; // "text/secondary"
        juce::Colour accentPrimary{ 0xFFFFA726 }; // "accent/primary": FL‑style orange
        juce::Colour accentSecondary{ 0xFF4ADE80 }; // "accent/secondary": neon green
        juce::Colour warn{ 0xFFFFB020 };       // "accent/warn": "#FFB020"
        juce::Colour danger{ 0xFFFF4D4D };     // "accent/danger": "#FF4D4D"
        juce::Colour graphGrid{ 0xFF2A2F3A };  // "graph/grid"
        juce::Colour graphGridSubtle{ 0xFF1C2029 }; // "graph/gridSubtle"
        juce::Colour meterOK{ 0xFF22D39B };    // "meter/ok"
        juce::Colour meterHot{ 0xFFFFC857 };   // "meter/hot": "#FFC857"
        juce::Colour meterClip{ 0xFFFF4D4D };  // "meter/clip": "#FF4D4D"
        juce::Colour shadowSoft{ 0x59000000 }; // "shadow/soft": "rgba(0,0,0,0.35)"
    } color;

    struct Fonts
    {
        // Exact font tokens from PLAN.md
        float size12 = 12.f, size14 = 14.f, size16 = 16.f, size18 = 18.f, size24 = 24.f, size32 = 32.f;
        juce::String familyBase = "Inter, SF Pro, Segoe UI, Roboto"; // "family/base"
        juce::String familyMono = "JetBrains Mono, ui-monospace";    // "family/mono"
    } font;

    struct Spacing
    {
        // Exact spacing tokens from PLAN.md
        int s2 = 2, s4 = 4, s6 = 6, s8 = 8, s12 = 12, s16 = 16, s24 = 24, s32 = 32;
        // Convenience aliases used by legacy code
        int small = 6, medium = 12, large = 16, xlarge = 24;
    } space;

    struct Radius
    {
        // Exact radius tokens from PLAN.md
        float s = 8.f, m = 12.f, l = 16.f, xl = 22.f;
    } radius;

    struct Anim
    {
        // Exact animation tokens from PLAN.md
        int msFast = 120, msMed = 220, msSlow = 360;
        // Cubic bezier values for animation curves
        struct EasingCurve { float p0, p1, p2, p3; };
        EasingCurve easeStandard{0.22f, 1.0f, 0.36f, 1.0f}; // "ease/standard"
        EasingCurve easeInOut{0.4f, 0.0f, 0.2f, 1.0f};      // "ease/inOut"

        // Spring parameters for knob animations
        struct SpringParams { float stiffness, damping, mass; };
        SpringParams springKnob{360.0f, 26.0f, 1.0f}; // "spring/knob"
    } anim;
};

namespace detail
{
    inline std::atomic<unsigned int>& tokenRevisionCounter()
    {
        static std::atomic<unsigned int> revision { 1u };
        return revision;
    }

    inline void incrementTokenRevision()
    {
        tokenRevisionCounter().fetch_add (1u, std::memory_order_relaxed);
    }
}

inline unsigned int currentTokenRevision()
{
    return detail::tokenRevisionCounter().load (std::memory_order_relaxed);
}

void tokensDidChange();

// Singleton-style access (thread-safe initialization)
// Accessors & mutation helpers
const Tokens& tokens();               // read-only view (preferred)
Tokens& mutableTokens();              // mutable access for theme overrides
void resetTokensToDefaults();         // restore factory defaults
void applyTokenOverrides(const std::function<void(Tokens&)>& updater);

// Load tokens from JSON (overrides defaults)
void loadTokensFromJSON (const juce::String& json);

// ============================================================================
// ICONS (inline SVG path data → juce::Path) - Complete DAW icon set
// ============================================================================
struct Icons
{
    // Transport controls
    static juce::Path play()
    {
        juce::Path p;
        p.addTriangle(0.0f, 0.0f, 0.0f, 20.0f, 18.0f, 10.0f);
        return p;
    }

    static juce::Path stop()
    {
        juce::Path p;
        p.addRectangle(0.0f, 0.0f, 18.0f, 18.0f);
        return p;
    }

    static juce::Path record()
    {
        juce::Path p;
        p.addEllipse(0.0f, 0.0f, 18.0f, 18.0f);
        return p;
    }

    static juce::Path pause()
    {
        juce::Path p;
        p.addRectangle(0.0f, 0.0f, 6.0f, 18.0f);
        p.addRectangle(12.0f, 0.0f, 6.0f, 18.0f);
        return p;
    }

    // Navigation and utility
    static juce::Path settings()
    {
        juce::Path p;
        p.addStar(juce::Point<float>(9.0f, 9.0f), 8, 6.0f, 9.0f, 0.0f);
        p.addEllipse(5.0f, 5.0f, 8.0f, 8.0f);  // inner circle
        return p;
    }

    static juce::Path search()
    {
        juce::Path p;
        p.addEllipse(0.0f, 0.0f, 14.0f, 14.0f);
        p.startNewSubPath(12.0f, 12.0f);
        p.lineTo(18.0f, 18.0f);
        return p;
    }

    static juce::Path menu()
    {
        juce::Path p;
        p.addRectangle(0.0f, 2.0f, 18.0f, 2.0f);
        p.addRectangle(0.0f, 8.0f, 18.0f, 2.0f);
        p.addRectangle(0.0f, 14.0f, 18.0f, 2.0f);
        return p;
    }

    // Audio controls
    static juce::Path volumeUp()
    {
        juce::Path p;
        p.addRectangle(0.0f, 6.0f, 4.0f, 6.0f);
        p.addTriangle(4.0f, 6.0f, 4.0f, 12.0f, 10.0f, 9.0f);
        // Sound waves
        p.startNewSubPath(12.0f, 4.0f);
        p.quadraticTo(16.0f, 9.0f, 12.0f, 14.0f);
        p.startNewSubPath(14.0f, 2.0f);
        p.quadraticTo(20.0f, 9.0f, 14.0f, 16.0f);
        return p;
    }

    static juce::Path volumeDown()
    {
        juce::Path p;
        p.addRectangle(0.0f, 6.0f, 4.0f, 6.0f);
        p.addTriangle(4.0f, 6.0f, 4.0f, 12.0f, 10.0f, 9.0f);
        // Single sound wave
        p.startNewSubPath(12.0f, 6.0f);
        p.quadraticTo(15.0f, 9.0f, 12.0f, 12.0f);
        return p;
    }

    static juce::Path mute()
    {
        juce::Path p;
        p.addRectangle(0.0f, 6.0f, 4.0f, 6.0f);
        p.addTriangle(4.0f, 6.0f, 4.0f, 12.0f, 10.0f, 9.0f);
        // X mark
        p.startNewSubPath(12.0f, 6.0f);
        p.lineTo(16.0f, 12.0f);
        p.startNewSubPath(16.0f, 6.0f);
        p.lineTo(12.0f, 12.0f);
        return p;
    }

    // File operations
    static juce::Path save()
    {
        juce::Path p;
        p.addRectangle(2.0f, 0.0f, 14.0f, 18.0f);
        p.addRectangle(2.0f, 0.0f, 14.0f, 5.0f); // floppy top
        p.addRectangle(6.0f, 0.0f, 6.0f, 3.0f);  // label area
        p.addRectangle(4.0f, 10.0f, 10.0f, 6.0f); // disk area
        return p;
    }

    static juce::Path load()
    {
        juce::Path p;
        p.addRectangle(2.0f, 0.0f, 12.0f, 18.0f);
        p.addTriangle(14.0f, 0.0f, 14.0f, 6.0f, 20.0f, 6.0f); // folder tab
        return p;
    }

    // Edit controls
    static juce::Path cut()
    {
        juce::Path p;
        // Scissors shape
        p.addEllipse(2.0f, 2.0f, 4.0f, 4.0f);
        p.addEllipse(2.0f, 12.0f, 4.0f, 4.0f);
        p.startNewSubPath(6.0f, 4.0f);
        p.lineTo(16.0f, 9.0f);
        p.startNewSubPath(6.0f, 14.0f);
        p.lineTo(16.0f, 9.0f);
        return p;
    }

    static juce::Path copy()
    {
        juce::Path p;
        p.addRectangle(2.0f, 2.0f, 12.0f, 12.0f);
        p.addRectangle(6.0f, 6.0f, 12.0f, 12.0f);
        return p;
    }

    static juce::Path paste()
    {
        juce::Path p;
        p.addRectangle(4.0f, 4.0f, 12.0f, 14.0f);
        p.addRectangle(6.0f, 0.0f, 8.0f, 6.0f); // clipboard top
        return p;
    }

    // Arrow directions
    static juce::Path arrowLeft()
    {
        juce::Path p;
        p.addTriangle(12.0f, 4.0f, 12.0f, 14.0f, 4.0f, 9.0f);
        return p;
    }

    static juce::Path arrowRight()
    {
        juce::Path p;
        p.addTriangle(6.0f, 4.0f, 6.0f, 14.0f, 14.0f, 9.0f);
        return p;
    }

    static juce::Path arrowUp()
    {
        juce::Path p;
        p.addTriangle(4.0f, 12.0f, 14.0f, 12.0f, 9.0f, 4.0f);
        return p;
    }

    static juce::Path arrowDown()
    {
        juce::Path p;
        p.addTriangle(4.0f, 6.0f, 14.0f, 6.0f, 9.0f, 14.0f);
        return p;
    }

    // Zoom and view
    static juce::Path zoomIn()
    {
        juce::Path p;
        p.addEllipse(0.0f, 0.0f, 14.0f, 14.0f);
        // Plus sign
        p.addRectangle(6.0f, 3.0f, 2.0f, 8.0f);
        p.addRectangle(3.0f, 6.0f, 8.0f, 2.0f);
        // Handle
        p.startNewSubPath(12.0f, 12.0f);
        p.lineTo(18.0f, 18.0f);
        return p;
    }

    static juce::Path zoomOut()
    {
        juce::Path p;
        p.addEllipse(0.0f, 0.0f, 14.0f, 14.0f);
        // Minus sign
        p.addRectangle(3.0f, 6.0f, 8.0f, 2.0f);
        // Handle
        p.startNewSubPath(12.0f, 12.0f);
        p.lineTo(18.0f, 18.0f);
        return p;
    }

    // Status indicators
    static juce::Path warning()
    {
        juce::Path p;
        p.addTriangle(9.0f, 2.0f, 2.0f, 16.0f, 16.0f, 16.0f);
        p.addEllipse(8.0f, 12.0f, 2.0f, 2.0f); // dot
        p.addRectangle(8.0f, 6.0f, 2.0f, 4.0f); // exclamation
        return p;
    }

    static juce::Path error()
    {
        juce::Path p;
        p.addEllipse(0.0f, 0.0f, 18.0f, 18.0f);
        // X mark
        p.startNewSubPath(5.0f, 5.0f);
        p.lineTo(13.0f, 13.0f);
        p.startNewSubPath(13.0f, 5.0f);
        p.lineTo(5.0f, 13.0f);
        return p;
    }

    static juce::Path success()
    {
        juce::Path p;
        p.addEllipse(0.0f, 0.0f, 18.0f, 18.0f);
        // Check mark
        p.startNewSubPath(5.0f, 9.0f);
        p.lineTo(8.0f, 12.0f);
        p.lineTo(13.0f, 6.0f);
        return p;
    }
};

// ============================================================================
// LOOKANDFEEL — main product skin
// ============================================================================
class MainLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MainLookAndFeel();
    ~MainLookAndFeel() override = default;

    // Global application of this LAF
    static void applyGlobalLookAndFeel();
    static void resetGlobalLookAndFeel();
    static void refreshGlobalLookAndFeel();

    // Typography
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;

    // Widgets
    void drawLabel (juce::Graphics&, juce::Label&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;

private:
    // Cached fonts to avoid allocation in paint paths
    juce::Font labelFont, buttonFont, comboFont, popupFont;
    unsigned int tokenRevisionSnapshot = 0;

    // Helpers
    static std::unique_ptr<MainLookAndFeel>& globalInstance();
    void refreshFromTokens();
    void ensureLookAndFeelFresh();
    void updateFonts();
    void applyTokenColours();
    void drawPanelBackground (juce::Graphics& g, juce::Rectangle<float> bounds) const;
    void drawOuterGlow (juce::Graphics& g, juce::Rectangle<float> bounds, float radius, float alpha) const;
};

// ============================================================================
// WIDGETS
// ============================================================================

// Segmented ring rotary (Pitch-Monster-style)
class RingSlider : public juce::Slider
{
public:
    RingSlider();
    ~RingSlider() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::String valueText;
    void updateValueText();
    friend class UltraDesignSystemTests;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RingSlider)
};

// Pill toggle (chip-style buttons for SNAP/WIDE/etc.)
class PillToggle : public juce::ToggleButton
{
public:
    explicit PillToggle (const juce::String& buttonText);
    ~PillToggle() override = default;

    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    std::weak_ptr<daw::ui::animation::AdaptiveAnimationService> animationService;
    float hoverAmount { 0.0f };
    float pressAmount { 0.0f };
    uint32_t hoverAnimationId { 0 };
    uint32_t pressAnimationId { 0 };

    void animateState (float target,
                       float durationMs,
                       float& storage,
                       uint32_t& handle);
    void cancelAnimation (uint32_t& handle);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PillToggle)
};

// Peak/RMS meter pair with clip indicators and hold
class PeakRmsMeter : public juce::Component, private juce::Timer
{
public:
    PeakRmsMeter();
    ~PeakRmsMeter() override;

    // Set levels in [0..1]
    void setLevels (float peak, float rms);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    float peakLevel = 0.0f;
    float rmsLevel = 0.0f;
    float peakHold = 0.0f;
    int peakHoldTimer = 0;

    juce::Rectangle<float> peakBounds, rmsBounds;

    friend class UltraDesignSystemTests;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PeakRmsMeter)
};

// XY Pad for two-parameter control with grid + glow handle
class XYPad : public juce::Component
{
public:
    XYPad();
    ~XYPad() override = default;

    std::function<void (float x, float y)> onChange;
    void setValue (float x, float y);
    std::pair<float, float> getValue() const { return {xValue, yValue}; }

    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    float xValue = 0.5f, yValue = 0.5f;
    void updateFromMouse (juce::Point<float> position);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XYPad)
};

// Header toolbar with transport cluster + CPU meter
class HeaderToolbar : public juce::Component
{
public:
    HeaderToolbar();
    ~HeaderToolbar() override = default;

    std::function<void()> onPlay, onStop, onRecord, onSettings;
    void setCPULevel (float percentage);
    void setBPM (double bpm);
    void setTimeDisplay (const juce::String& timeText);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    float cpuLevel = 0.0f;
    double currentBPM = 120.0;
    juce::String timeDisplay = "00:00.000";

    juce::Rectangle<float> playBounds, stopBounds, recordBounds, settingsBounds, cpuBounds;

    static void drawIconButton (juce::Graphics& g, juce::Rectangle<float> bounds,
                                const juce::Path& icon, juce::Colour color);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderToolbar)
};

// Tab bar for mode switching (NORMAL/MIDI/CHORD style)
class TabBarPro : public juce::Component
{
public:
    TabBarPro();
    ~TabBarPro() override = default;

    void setTabs (const juce::StringArray& tabNames);
    int getSelectedTab() const { return selectedIndex; }
    void setSelectedTab (int index);

    std::function<void(int)> onChange;

    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::StringArray tabs{"NORMAL", "MIDI", "CHORD"};
    int selectedIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabBarPro)
};

} // namespace ultra

// ============================================================================
// IMPLEMENTATION (inline for single-header)
// ============================================================================

inline ultra::Tokens& ultra::mutableTokens()
{
    static Tokens storage;
    return storage;
}

inline const ultra::Tokens& ultra::tokens()
{
    return mutableTokens();
}

inline void ultra::resetTokensToDefaults()
{
    mutableTokens() = Tokens{};
    tokensDidChange();
}

inline void ultra::applyTokenOverrides(const std::function<void(ultra::Tokens&)>& updater)
{
    if (updater)
    {
        updater (mutableTokens());
        tokensDidChange();
    }
}

inline void ultra::loadTokensFromJSON (const juce::String& json)
{
    const juce::var parsed = juce::JSON::parse (json);
    if (! parsed.isObject())
        return;

    auto& t = mutableTokens();

    const auto get = [&] (const juce::String& path) -> juce::var
    {
        juce::StringArray segments;
        segments.addTokens (path, "/", {});
        juce::var node = parsed;
        for (int i = 0; i < segments.size(); ++i)
        {
            if (! node.isObject())
                return {};
            auto candidate = node.getProperty (segments[i], juce::var());

            if (candidate.isVoid())
            {
                juce::String joined = segments[i];
                for (int j = i + 1; j < segments.size(); ++j)
                {
                    joined << "/" << segments[j];
                    candidate = node.getProperty (joined, juce::var());
                    if (! candidate.isVoid())
                    {
                        i = j;
                        break;
                    }
                }
            }

            if (candidate.isVoid())
                return {};

            node = candidate;
        }
        return node;
    };

    const auto parseColour = [] (const juce::String& raw, juce::Colour& target) -> bool
    {
        auto text = raw.trim();
        if (text.isEmpty())
            return false;

        if (text.startsWithIgnoreCase ("rgba"))
        {
            const auto start = text.indexOfChar ('(');
            const auto end = text.lastIndexOfChar (')');
            if (start < 0 || end <= start)
                return false;

            auto inner = text.substring (start + 1, end);
            juce::StringArray comps;
            comps.addTokens (inner, ",", {});
            comps.trim();
            if (comps.size() != 4)
                return false;

            const auto r = juce::jlimit (0, 255, comps[0].trim().getIntValue());
            const auto g = juce::jlimit (0, 255, comps[1].trim().getIntValue());
            const auto b = juce::jlimit (0, 255, comps[2].trim().getIntValue());
            const auto alphaValue = comps[3].trim().getFloatValue();
            const auto a = juce::jlimit (0.0f, 1.0f, alphaValue);
            target = juce::Colour::fromFloatRGBA ((float) r / 255.0f, (float) g / 255.0f, (float) b / 255.0f, a);
            return true;
        }

        auto colourText = text;
        if (colourText.startsWithIgnoreCase ("0x"))
            colourText = colourText.substring (2);
        if (colourText.startsWithChar ('#'))
            colourText = colourText.substring (1);

        if (colourText.length() == 6)
        {
            const juce::uint32 rgb = (juce::uint32) colourText.getHexValue32();
            target = juce::Colour::fromRGB ((rgb >> 16) & 0xFFu, (rgb >> 8) & 0xFFu, rgb & 0xFFu);
            return true;
        }

        if (colourText.length() == 8)
        {
            const juce::uint32 argb = (juce::uint32) colourText.getHexValue32();
            target = juce::Colour ((juce::uint8) ((argb >> 24) & 0xFFu), (juce::uint8) ((argb >> 16) & 0xFFu),
                                   (juce::uint8) ((argb >> 8) & 0xFFu), (juce::uint8) (argb & 0xFFu));
            return true;
        }

        return false;
    };

    const auto assignColour = [&] (juce::Colour& target, const juce::String& path)
    {
        if (auto value = get (path); value.isString())
        {
            juce::Colour parsedColour;
            if (parseColour (value.toString(), parsedColour))
                target = parsedColour;
        }
    };

    const auto assignFloat = [&] (float& target, const juce::String& path)
    {
        if (auto value = get (path); value.isDouble() || value.isInt())
            target = (float) value;
    };

    const auto assignInt = [&] (int& target, const juce::String& path)
    {
        if (auto value = get (path); value.isInt() || value.isDouble())
            target = (int) value;
    };

    const auto assignSpring = [&] (Tokens::Anim::SpringParams& spring, const juce::String& path)
    {
        if (auto value = get (path); value.isObject())
        {
            auto stiffness = value.getProperty ("stiffness", {});
            auto damping   = value.getProperty ("damping", {});
            auto mass      = value.getProperty ("mass", {});

            if (stiffness.isDouble() || stiffness.isInt()) spring.stiffness = (float) stiffness;
            if (damping.isDouble()   || damping.isInt())   spring.damping   = (float) damping;
            if (mass.isDouble()      || mass.isInt())      spring.mass      = (float) mass;
        }
    };

    const auto assignCubic = [&] (Tokens::Anim::EasingCurve& curve, const juce::String& path)
    {
        if (auto value = get (path); value.isString())
        {
            auto text = value.toString().trim();
            if (text.startsWithIgnoreCase ("cubic-bezier"))
            {
                const auto start = text.indexOfChar ('(');
                const auto end = text.lastIndexOfChar (')');
                if (start >= 0 && end > start)
                {
                    auto inner = text.substring (start + 1, end);
                    juce::StringArray comps;
                    comps.addTokens (inner, ",", {});
                    comps.trim();
                    if (comps.size() == 4)
                    {
                        curve.p0 = comps[0].getFloatValue();
                        curve.p1 = comps[1].getFloatValue();
                        curve.p2 = comps[2].getFloatValue();
                        curve.p3 = comps[3].getFloatValue();
                    }
                }
            }
        }
    };

    // Colours
    assignColour (t.color.bg0, "color/bg/0");
    assignColour (t.color.bg1, "color/bg/1");
    assignColour (t.color.bg2, "color/bg/2");
    assignColour (t.color.panelBorder, "color/panel/border");
    assignColour (t.color.textPrimary, "color/text/primary");
    assignColour (t.color.textSecondary, "color/text/secondary");
    assignColour (t.color.accentPrimary, "color/accent/primary");
    assignColour (t.color.accentSecondary, "color/accent/secondary");
    assignColour (t.color.warn, "color/accent/warn");
    assignColour (t.color.danger, "color/accent/danger");
    assignColour (t.color.graphGrid, "color/graph/grid");
    assignColour (t.color.graphGridSubtle, "color/graph/gridSubtle");
    assignColour (t.color.meterOK, "color/meter/ok");
    assignColour (t.color.meterHot, "color/meter/hot");
    assignColour (t.color.meterClip, "color/meter/clip");
    assignColour (t.color.shadowSoft, "color/shadow/soft");

    // Fonts
    if (auto family = get ("font/family/base"); family.isString())
        t.font.familyBase = family.toString();
    if (auto familyMono = get ("font/family/mono"); familyMono.isString())
        t.font.familyMono = familyMono.toString();

    assignFloat (t.font.size12, "font/size/12");
    assignFloat (t.font.size14, "font/size/14");
    assignFloat (t.font.size16, "font/size/16");
    assignFloat (t.font.size18, "font/size/18");
    assignFloat (t.font.size24, "font/size/24");
    assignFloat (t.font.size32, "font/size/32");

    // Spacing
    assignInt (t.space.s2,  "space/2");
    assignInt (t.space.s4,  "space/4");
    assignInt (t.space.s6,  "space/6");
    assignInt (t.space.s8,  "space/8");
    assignInt (t.space.s12, "space/12");
    assignInt (t.space.s16, "space/16");
    assignInt (t.space.s24, "space/24");
    assignInt (t.space.s32, "space/32");

    t.space.small  = t.space.s6;
    t.space.medium = t.space.s12;
    t.space.large  = t.space.s16;
    t.space.xlarge = t.space.s24;

    // Radius
    assignFloat (t.radius.s,  "radius/s");
    assignFloat (t.radius.m,  "radius/m");
    assignFloat (t.radius.l,  "radius/l");
    assignFloat (t.radius.xl, "radius/xl");

    // Animation timings
    assignInt (t.anim.msFast, "anim/ms/fast");
    assignInt (t.anim.msMed,  "anim/ms/med");
    assignInt (t.anim.msSlow, "anim/ms/slow");
    assignCubic (t.anim.easeStandard, "anim/ease/standard");
    assignCubic (t.anim.easeInOut,    "anim/ease/inOut");
    assignSpring (t.anim.springKnob,  "anim/spring/knob");

    tokensDidChange();
}

inline ultra::MainLookAndFeel::MainLookAndFeel()
{
    refreshFromTokens();
}

inline std::unique_ptr<ultra::MainLookAndFeel>& ultra::MainLookAndFeel::globalInstance()
{
    static std::unique_ptr<MainLookAndFeel> instance;
    return instance;
}

inline void ultra::MainLookAndFeel::applyGlobalLookAndFeel()
{
    auto& instance = globalInstance();
    instance = std::make_unique<MainLookAndFeel>();
    juce::LookAndFeel::setDefaultLookAndFeel (instance.get());
}

inline void ultra::MainLookAndFeel::resetGlobalLookAndFeel()
{
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    globalInstance().reset();
}

inline void ultra::MainLookAndFeel::refreshGlobalLookAndFeel()
{
    if (auto& instance = globalInstance(); instance)
        instance->refreshFromTokens();
}

inline void ultra::MainLookAndFeel::refreshFromTokens()
{
    updateFonts();
    applyTokenColours();
    tokenRevisionSnapshot = currentTokenRevision();
}

inline void ultra::MainLookAndFeel::ensureLookAndFeelFresh()
{
    const auto revision = currentTokenRevision();
    if (revision != tokenRevisionSnapshot)
        refreshFromTokens();
}

inline juce::Font ultra::MainLookAndFeel::getLabelFont (juce::Label&)
{
    ensureLookAndFeelFresh();
    return labelFont;
}

inline juce::Font ultra::MainLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    ensureLookAndFeelFresh();
    return buttonFont.withHeight (buttonHeight > 0 ? juce::jmin ((float)buttonHeight * 0.6f, 18.0f) : 14.0f);
}

inline juce::Font ultra::MainLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    ensureLookAndFeelFresh();
    return comboFont;
}

inline juce::Font ultra::MainLookAndFeel::getPopupMenuFont()
{
    ensureLookAndFeelFresh();
    return popupFont;
}

inline void ultra::MainLookAndFeel::updateFonts()
{
    const auto& f = tokens().font;
    labelFont = juce::Font (f.familyBase, f.size14, juce::Font::plain);
    buttonFont = juce::Font (f.familyBase, f.size14, juce::Font::plain);
    comboFont = juce::Font (f.familyBase, f.size14, juce::Font::plain);
    popupFont = juce::Font (f.familyBase, f.size14, juce::Font::plain);
}

inline void ultra::MainLookAndFeel::applyTokenColours()
{
    const auto& c = tokens().color;
    setColour (juce::DocumentWindow::backgroundColourId, c.bg0);
    setColour (juce::TextButton::buttonColourId, c.accentPrimary);
    setColour (juce::TextButton::buttonOnColourId, c.accentSecondary);
    setColour (juce::TextButton::textColourOffId, c.textPrimary);
    setColour (juce::TextButton::textColourOnId, c.textPrimary);
    setColour (juce::Slider::thumbColourId, c.accentPrimary);
    setColour (juce::Slider::trackColourId, c.accentSecondary.withAlpha (0.3f));
    setColour (juce::Label::textColourId, c.textPrimary);
    setColour (juce::PopupMenu::backgroundColourId, c.bg1);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, c.accentSecondary.withAlpha (0.2f));
    setColour (juce::PopupMenu::textColourId, c.textPrimary);
    setColour (juce::PopupMenu::highlightedTextColourId, c.textPrimary);
    setColour (juce::ComboBox::backgroundColourId, c.bg2);
    setColour (juce::ComboBox::outlineColourId, c.panelBorder);
}

inline void ultra::tokensDidChange()
{
    detail::incrementTokenRevision();
    MainLookAndFeel::refreshGlobalLookAndFeel();
}

inline void ultra::MainLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    ensureLookAndFeelFresh();

    if (label.isBeingEdited())
    {
        LookAndFeel_V4::drawLabel (g, label);
        return;
    }

    const auto& t = tokens();
    g.setFont (getLabelFont (label));
    g.setColour (label.isEnabled() ? t.color.textPrimary : t.color.textSecondary);
    g.drawFittedText (label.getText(), label.getLocalBounds().toNearestInt(), label.getJustificationType(), 1);
}

inline void ultra::MainLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                          const juce::Colour& /*backgroundColour*/,
                                                          bool shouldDrawButtonAsHighlighted,
                                                          bool shouldDrawButtonAsDown)
{
    ensureLookAndFeelFresh();

    const auto& t = tokens();
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const auto radius = t.radius.m;

    auto base = t.color.accentPrimary;
    if (shouldDrawButtonAsDown) base = base.darker (0.3f);
    else if (shouldDrawButtonAsHighlighted) base = base.brighter (0.1f);

    juce::ColourGradient grad (base.brighter (0.2f), bounds.getTopLeft(),
                               base.darker (0.4f), bounds.getBottomRight(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (bounds, radius);

    if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
    {
        g.setColour (t.color.accentSecondary.withAlpha (0.6f));
        g.drawRoundedRectangle (bounds, radius, 1.5f);
        drawOuterGlow (g, bounds, 8.0f, 0.4f);
    }
}

inline void ultra::MainLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                                      bool shouldDrawButtonAsHighlighted,
                                                      bool shouldDrawButtonAsDown)
{
    ensureLookAndFeelFresh();
    drawButtonBackground (g, button, juce::Colours::transparentBlack, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
}

inline void ultra::MainLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                                  int buttonX, int buttonY, int buttonW, int buttonH,
                                                  juce::ComboBox& box)
{
    juce::ignoreUnused (isButtonDown);
    ensureLookAndFeelFresh();
    const auto& t = tokens();
    auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (0.5f);
    drawPanelBackground (g, bounds);

    juce::Path arrow;
    auto arrowBounds = juce::Rectangle<float> ((float)buttonX, (float)buttonY, (float)buttonW, (float)buttonH).reduced (4.0f);
    arrow.addTriangle (arrowBounds.getCentreX(), arrowBounds.getBottom() - arrowBounds.getHeight() * 0.25f,
                       arrowBounds.getX(), arrowBounds.getY() + arrowBounds.getHeight() * 0.25f,
                       arrowBounds.getRight(), arrowBounds.getY() + arrowBounds.getHeight() * 0.25f);
    g.setColour (t.color.textSecondary);
    g.fillPath (arrow);

    box.setColour (juce::ComboBox::textColourId, t.color.textPrimary);
}

inline void ultra::MainLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                                      float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                                      juce::Slider& slider)
{
    juce::ignoreUnused (slider);
    ensureLookAndFeelFresh();
    const auto& t = tokens();
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (6.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    const auto centre = bounds.getCentre();
    const auto knobRadius = radius * 0.75f;

    // Background halo
    g.setColour (t.color.accentPrimary.withAlpha (0.08f));
    g.fillEllipse (bounds);

    // Outer halo arc
    juce::Path haloArc;
    haloArc.addCentredArc (centre.x, centre.y, radius - 2.0f, radius - 2.0f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (t.color.accentPrimary.withAlpha (0.15f));
    g.strokePath (haloArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Knob body
    auto knobArea = juce::Rectangle<float> (knobRadius * 2.0f, knobRadius * 2.0f).withCentre (centre);
    juce::ColourGradient knobGrad (t.color.bg2.brighter (0.25f), knobArea.getTopLeft(),
                                   t.color.bg0.darker (0.3f), knobArea.getBottomRight(), false);
    g.setGradientFill (knobGrad);
    g.fillEllipse (knobArea);

    g.setColour (t.color.panelBorder.withAlpha (0.8f));
    g.drawEllipse (knobArea, 1.1f);

    // Value arc
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto arcRadius = knobRadius + 6.0f;
    juce::Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);

    juce::ColourGradient valueGrad (t.color.accentPrimary, centre.getPointOnCircumference (arcRadius, rotaryStartAngle),
                                    t.color.accentSecondary, centre.getPointOnCircumference (arcRadius, angle), false);
    g.setGradientFill (valueGrad);
    g.strokePath (valueArc, juce::PathStrokeType (2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Pointer
    const float pointerRadius = knobRadius * 0.8f;
    juce::Point<float> pointer (centre.x + std::cos (angle) * pointerRadius, centre.y + std::sin (angle) * pointerRadius);
    g.setColour (t.color.accentSecondary.withAlpha (0.5f));
    g.drawLine (centre.x, centre.y, pointer.x, pointer.y, 3.0f);
    g.setColour (t.color.accentSecondary);
    g.drawLine (centre.x, centre.y, pointer.x, pointer.y, 2.0f);

    drawOuterGlow (g, knobArea, 8.0f, 0.2f);
}

inline void ultra::MainLookAndFeel::drawPanelBackground (juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    const auto& t = tokens();
    const auto radius = t.radius.l;
    juce::Path panelPath;
    panelPath.addRoundedRectangle (bounds, radius);

    juce::ColourGradient gradient (t.color.bg1, bounds.getTopLeft(), t.color.bg2, bounds.getBottomRight(), false);
    g.setGradientFill (gradient);
    g.fillPath (panelPath);

    g.setColour (t.color.panelBorder);
    g.strokePath (panelPath, juce::PathStrokeType (1.0f));
}

inline void ultra::MainLookAndFeel::drawOuterGlow (juce::Graphics& g, juce::Rectangle<float> bounds, float radius, float alpha) const
{
    const auto& t = tokens();
    juce::Path glow;
    glow.addRoundedRectangle (bounds.expanded (2.0f), t.radius.l + 4.0f);
    g.setColour (t.color.accentPrimary.withAlpha (alpha));
    g.strokePath (glow, juce::PathStrokeType (radius * 0.05f));
}

// ============================================================================
// WIDGET IMPLEMENTATIONS
// ============================================================================

inline ultra::RingSlider::RingSlider()
{
    setSliderStyle (juce::Slider::Rotary);
    setRange (-12.0, 12.0, 0.01);
    onValueChange = [this]() { updateValueText(); };
    setValue (0.0);
    updateValueText();
}

inline void ultra::RingSlider::paint (juce::Graphics& g)
{
    const auto& t = tokens();
    auto bounds = getLocalBounds().toFloat();
    const auto centre = bounds.getCentre();
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f - 10.0f;

    // Ring parameters from PLAN.md spec
    const float ringThickness = 8.0f; // 8-12px ring thickness
    const int numSegments = 64;
    const float startAngle = juce::MathConstants<float>::pi * 1.25f; // 270 degree sweep
    const float endAngle = juce::MathConstants<float>::pi * 2.75f;
    const float proportion = juce::jlimit (0.0f, 1.0f, (float) valueToProportionOfLength (getValue()));
    const float currentAngle = juce::jmap (proportion, startAngle, endAngle);
    const int activeSegments = juce::jlimit (0, numSegments, (int) std::round (proportion * (float) numSegments));

    // Base disc with subtle shadow
    auto discBounds = bounds.reduced(ringThickness * 1.5f);
    g.setColour(t.color.shadowSoft);
    g.fillEllipse(discBounds.translated(0, 2)); // shadow offset
    g.setColour(t.color.bg2);
    g.fillEllipse(discBounds);
    g.setColour(t.color.panelBorder);
    g.drawEllipse(discBounds, 1.2f);

    // Segmented ring - exactly 64 segments as per PLAN.md
    for (int i = 0; i < numSegments; ++i)
    {
        const float segStart = juce::jmap((float)i / numSegments, startAngle, endAngle);
        const float segEnd = juce::jmap((float)(i + 1) / numSegments, startAngle, endAngle) - 0.02f; // small gap

        juce::Path segment;
        segment.addArc(centre.x - radius, centre.y - radius, radius * 2, radius * 2, segStart, segEnd, true);

        // Active segments get accent color with halo glow
        auto segmentColor = i < activeSegments ? t.color.accentPrimary : t.color.graphGrid.withAlpha(0.8f);
        g.setColour(segmentColor);
        g.strokePath(segment, juce::PathStrokeType(ringThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Add subtle glow to active segments
        if (i < activeSegments)
        {
            g.setColour(t.color.accentPrimary.withAlpha(0.3f));
            g.strokePath(segment, juce::PathStrokeType(ringThickness + 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    // Triangular needle/pointer
    juce::Path needle;
    needle.addRoundedRectangle(-2.0f, -discBounds.getHeight() * 0.28f, 4.0f, discBounds.getHeight() * 0.20f, 2.0f);
    g.setColour(t.color.accentPrimary);
    g.fillPath(needle, juce::AffineTransform::rotation(currentAngle).translated(centre.x, centre.y));

    // Big numeric value (32px as per PLAN.md)
    g.setColour(t.color.textPrimary);
    g.setFont(juce::Font(t.font.familyBase, t.font.size32, juce::Font::bold));
    g.drawText(valueText, discBounds.toNearestInt(), juce::Justification::centred, true);
}

inline void ultra::RingSlider::resized()
{
    // No-op
}

inline void ultra::RingSlider::updateValueText()
{
    auto value = (float) getValue();
    auto rounded = std::round (value * 10.0f) / 10.0f;

    if (std::abs (rounded) < 0.05f)
        rounded = 0.0f;

    juce::String text = juce::String (rounded, 1);
    if (text == "-0.0")
        text = "0.0";

    const auto prefix = rounded > 0.0f ? "+" : "";
    valueText = prefix + text + " dB";
}

// PillToggle implementation
inline ultra::PillToggle::PillToggle (const juce::String& buttonText) : juce::ToggleButton (buttonText)
{
    setClickingTogglesState (true);
    setRepaintsOnMouseActivity (true);

    if (auto service = daw::core::ServiceLocator::getInstance()
                           .getService<daw::ui::animation::AdaptiveAnimationService>())
    {
        animationService = service;
    }
}

inline void ultra::PillToggle::paint (juce::Graphics& g)
{
    const auto& t = tokens();
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    const auto radius = bounds.getHeight() / 2.0f;

    auto base = getToggleState() ? t.color.accentPrimary : t.color.bg2;

    const auto hoverBoost = juce::jlimit (0.0f, 0.30f, hoverAmount * 0.20f);
    const auto pressBoost = juce::jlimit (0.0f, 0.35f, pressAmount * 0.30f);
    if (hoverBoost > 0.0f)
        base = base.brighter (hoverBoost);
    if (pressBoost > 0.0f)
        base = base.brighter (pressBoost * 0.5f).contrasting (0.0f);

    g.setColour (base);
    g.fillRoundedRectangle (bounds, radius);

    g.setColour (t.color.panelBorder);
    g.drawRoundedRectangle (bounds, radius, 1.0f);

    g.setFont (juce::Font (t.font.familyBase, t.font.size14, juce::Font::bold));
    g.setColour (getToggleState() ? t.color.bg0 : t.color.textPrimary);
    g.drawText (getButtonText(), bounds, juce::Justification::centred, true);
}

inline void ultra::PillToggle::mouseEnter (const juce::MouseEvent& e)
{
    animateState (1.0f,  static_cast<float> (160.0f),
                  hoverAmount, hoverAnimationId);
    juce::ToggleButton::mouseEnter (e);
}

inline void ultra::PillToggle::mouseExit (const juce::MouseEvent& e)
{
    animateState (0.0f, static_cast<float> (220.0f),
                  hoverAmount, hoverAnimationId);
    juce::ToggleButton::mouseExit (e);
}

inline void ultra::PillToggle::mouseDown (const juce::MouseEvent& e)
{
    animateState (1.0f, static_cast<float> (140.0f),
                  pressAmount, pressAnimationId);
    juce::ToggleButton::mouseDown (e);
}

inline void ultra::PillToggle::mouseUp (const juce::MouseEvent& e)
{
    animateState (0.0f, static_cast<float> (220.0f),
                  pressAmount, pressAnimationId);
    juce::ToggleButton::mouseUp (e);
}

inline void ultra::PillToggle::animateState (float target,
                                             float durationMs,
                                             float& storage,
                                             uint32_t& handle)
{
    const auto current = storage;

    if (auto service = animationService.lock())
    {
        if (! service->isInitialized())
        {
            storage = target;
            repaint();
            return;
        }

        if (handle != 0)
            service->cancelAnimation (handle);

        auto self = juce::Component::SafePointer<PillToggle> (this);
        const auto id = service->animateFloat (current,
                                               target,
                                               durationMs,
                                               [self, storagePtr = &storage] (float value)
                                               {
                                                   if (self != nullptr)
                                                   {
                                                       *storagePtr = value;
                                                       self->repaint();
                                                   }
                                               },
                                               [self, handlePtr = &handle]
                                               {
                                                   if (self != nullptr)
                                                       *handlePtr = 0;
                                               });

        if (id == 0)
        {
            storage = target;
            repaint();
        }
        else
        {
            handle = id;
        }
    }
    else
    {
        storage = target;
        repaint();
    }
}

inline void ultra::PillToggle::cancelAnimation (uint32_t& handle)
{
    if (handle == 0)
        return;

    if (auto service = animationService.lock())
        service->cancelAnimation (handle);

    handle = 0;
}

// PeakRmsMeter implementation
inline ultra::PeakRmsMeter::PeakRmsMeter()
{
    startTimer (60); // 60 Hz updates
}

inline ultra::PeakRmsMeter::~PeakRmsMeter()
{
    stopTimer();
}

inline void ultra::PeakRmsMeter::setLevels (float peak, float rms)
{
    peakLevel = juce::jlimit (0.0f, 1.0f, peak);
    rmsLevel = juce::jlimit (0.0f, 1.0f, rms);
    if (peakLevel > peakHold)
    {
        peakHold = peakLevel;
        peakHoldTimer = 0;
    }
    repaint();
}

inline void ultra::PeakRmsMeter::paint (juce::Graphics& g)
{
    const auto& t = tokens();
    g.fillAll (t.color.bg0);

    auto rmsArea = rmsBounds;
    auto peakArea = peakBounds;

    const auto backgroundColour = t.color.graphGridSubtle.withAlpha (0.45f);
    g.setColour (backgroundColour);
    g.fillRect (rmsArea);
    g.fillRect (peakArea);

    const auto makePeakGradient = [&] (const juce::Rectangle<float>& area)
    {
        juce::ColourGradient gradient (t.color.meterOK, area.getBottomLeft(), t.color.meterClip, area.getTopLeft(), false);
        gradient.addColour (0.65, t.color.meterHot);
        gradient.addColour (0.9, t.color.meterClip);
        return gradient;
    };

    const auto makeRmsGradient = [&] (const juce::Rectangle<float>& area)
    {
        juce::ColourGradient gradient (t.color.meterOK.withAlpha (0.8f), area.getBottomLeft(),
                                       t.color.meterHot.withAlpha (0.9f), area.getTopLeft(), false);
        gradient.addColour (0.75, t.color.accentSecondary.withAlpha (0.8f));
        return gradient;
    };

    const auto drawMeter = [&] (const juce::Rectangle<float>& area, float level, juce::ColourGradient gradient)
    {
        if (level <= 0.0f)
            return;

        auto fill = area;
        fill = fill.removeFromBottom (fill.getHeight() * juce::jlimit (0.0f, 1.0f, level));
        if (fill.getHeight() <= 0.0f)
            return;

        gradient.point1 = { area.getBottomLeft().x, area.getBottom() };
        gradient.point2 = { area.getBottomLeft().x, area.getY() };
        g.setGradientFill (gradient);
        g.fillRect (fill);
    };

    drawMeter (rmsArea, rmsLevel, makeRmsGradient (rmsArea));
    drawMeter (peakArea, peakLevel, makePeakGradient (peakArea));

    // Peak hold
    if (peakHold > 0.01f)
    {
        auto holdY = peakArea.getBottom() - peakArea.getHeight() * peakHold;
        g.setColour (t.color.meterClip);
        g.drawHorizontalLine ((int)holdY, peakArea.getX(), peakArea.getRight());
    }

    // Grid lines
    g.setColour (t.color.graphGrid.withAlpha (0.3f));
    for (float i = 0.0f; i <= 1.0f; i += 0.1f)
    {
        float y = peakArea.getBottom() - peakArea.getHeight() * i;
        g.drawHorizontalLine ((int)y, peakArea.getX(), peakArea.getRight());
    }
}

inline void ultra::PeakRmsMeter::resized()
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    const float spacing = 4.0f;
    const float meterWidth = (bounds.getWidth() - spacing) / 2.0f;
    peakBounds = bounds.removeFromLeft (meterWidth);
    bounds.removeFromLeft (spacing);
    rmsBounds = bounds;
}

inline void ultra::PeakRmsMeter::timerCallback()
{
    if (peakHoldTimer++ > 1800) // 30 seconds at 60 Hz
        peakHold = 0.0f;
    repaint();
}

// ============================================================================
// XYPad implementation
// ============================================================================
inline ultra::XYPad::XYPad()
{
    setWantsKeyboardFocus(true);
}

inline void ultra::XYPad::setValue (float x, float y)
{
    xValue = juce::jlimit(0.0f, 1.0f, x);
    yValue = juce::jlimit(0.0f, 1.0f, y);
    repaint();
    if (onChange)
        onChange(xValue, yValue);
}

inline void ultra::XYPad::paint (juce::Graphics& g)
{
    const auto& t = tokens();
    auto bounds = getLocalBounds().toFloat();

    // Background with shadow
    g.setColour(t.color.shadowSoft);
    g.fillRoundedRectangle(bounds.translated(0, 2), t.radius.l + 2);
    g.setColour(t.color.bg2);
    g.fillRoundedRectangle(bounds, t.radius.l);
    g.setColour(t.color.panelBorder);
    g.drawRoundedRectangle(bounds, t.radius.l, 1.0f);

    auto inner = bounds.reduced(8.0f);

    // Grid lines (4x4 as per PLAN.md)
    g.setColour(t.color.graphGrid.withAlpha(0.4f));
    for (int i = 1; i < 4; ++i)
    {
        float x = inner.getX() + inner.getWidth() * i / 4.0f;
        float y = inner.getY() + inner.getHeight() * i / 4.0f;
        g.drawVerticalLine((int)x, inner.getY(), inner.getBottom());
        g.drawHorizontalLine((int)y, inner.getX(), inner.getRight());
    }

    // Handle position
    auto handlePos = juce::Point<float>(
        inner.getX() + inner.getWidth() * xValue,
        inner.getY() + inner.getHeight() * (1.0f - yValue)
    );

    // Handle glow
    const float handleRadius = 12.0f;
    g.setColour(t.color.accentPrimary.withAlpha(0.25f));
    g.fillEllipse(handlePos.x - handleRadius, handlePos.y - handleRadius,
                  handleRadius * 2, handleRadius * 2);

    // Handle outline
    g.setColour(t.color.accentPrimary);
    g.drawEllipse(handlePos.x - handleRadius, handlePos.y - handleRadius,
                  handleRadius * 2, handleRadius * 2, 2.0f);

    // Center dot
    g.fillEllipse(handlePos.x - 3, handlePos.y - 3, 6, 6);
}

inline void ultra::XYPad::mouseDrag (const juce::MouseEvent& e)
{
    updateFromMouse(e.position);
}

inline void ultra::XYPad::mouseDown (const juce::MouseEvent& e)
{
    updateFromMouse(e.position);
}

inline void ultra::XYPad::updateFromMouse (juce::Point<float> position)
{
    auto inner = getLocalBounds().toFloat().reduced(8.0f);
    float x = (position.x - inner.getX()) / inner.getWidth();
    float y = 1.0f - ((position.y - inner.getY()) / inner.getHeight());
    setValue(x, y);
}

// ============================================================================
// HeaderToolbar implementation
// ============================================================================
inline ultra::HeaderToolbar::HeaderToolbar()
{
}

inline void ultra::HeaderToolbar::setCPULevel (float percentage)
{
    cpuLevel = juce::jlimit(0.0f, 1.0f, percentage);
    repaint();
}

inline void ultra::HeaderToolbar::setBPM (double bpm)
{
    currentBPM = bpm;
    repaint();
}

inline void ultra::HeaderToolbar::setTimeDisplay (const juce::String& timeText)
{
    timeDisplay = timeText;
    repaint();
}

inline void ultra::HeaderToolbar::paint (juce::Graphics& g)
{
    const auto& t = tokens();
    auto bounds = getLocalBounds().toFloat();

    // Translucent panel background
    g.setColour(t.color.bg1.withAlpha(0.95f));
    g.fillRoundedRectangle(bounds, t.radius.m);
    g.setColour(t.color.panelBorder);
    g.drawRoundedRectangle(bounds, t.radius.m, 1.0f);

    auto leftSection = bounds.removeFromLeft(200);
    auto rightSection = bounds.removeFromRight(180);

    // Transport controls (left)
    const float buttonSize = 32.0f;
    const float buttonSpacing = 8.0f;
    auto transportArea = leftSection.reduced(8).withHeight(buttonSize);

    playBounds = transportArea.removeFromLeft(buttonSize);
    transportArea.removeFromLeft(buttonSpacing);
    stopBounds = transportArea.removeFromLeft(buttonSize);
    transportArea.removeFromLeft(buttonSpacing);
    recordBounds = transportArea.removeFromLeft(buttonSize);

    // Draw transport buttons
    drawIconButton(g, playBounds, Icons::play(), t.color.meterOK);
    drawIconButton(g, stopBounds, Icons::stop(), t.color.textSecondary);
    drawIconButton(g, recordBounds, Icons::record(), t.color.danger);

    // Center: Time display and BPM
    g.setColour(t.color.textPrimary);
    g.setFont(juce::Font(t.font.familyMono, t.font.size16, juce::Font::bold));
    g.drawText(timeDisplay, bounds.removeFromLeft(120), juce::Justification::centredLeft, true);

    g.setFont(juce::Font(t.font.familyBase, t.font.size14, juce::Font::plain));
    g.drawText(juce::String(currentBPM, 1) + " BPM", bounds.removeFromLeft(80), juce::Justification::centredLeft, true);

    // Right section: CPU meter and settings
    cpuBounds = rightSection.removeFromLeft(120).reduced(8, 12);
    settingsBounds = rightSection.removeFromRight(32).reduced(4);

    // CPU meter pill
    auto cpuArea = cpuBounds;
    g.setColour(t.color.accentSecondary.withAlpha(0.18f));
    g.fillRoundedRectangle(cpuArea, cpuArea.getHeight() / 2);
    g.setColour(t.color.panelBorder);
    g.drawRoundedRectangle(cpuArea, cpuArea.getHeight() / 2, 1.0f);

    auto cpuFill = cpuArea.withWidth(cpuArea.getWidth() * cpuLevel);
    g.setColour(cpuLevel > 0.8f ? t.color.danger : t.color.accentSecondary);
    g.fillRoundedRectangle(cpuFill, cpuArea.getHeight() / 2);

    g.setColour(t.color.textSecondary);
    g.setFont(juce::Font(t.font.familyBase, t.font.size12, juce::Font::plain));
    g.drawText(juce::String(juce::roundToInt(cpuLevel * 100)) + "% CPU",
               cpuArea, juce::Justification::centred, true);

    // Settings button
    drawIconButton(g, settingsBounds, Icons::settings(), t.color.textSecondary);
}

inline void ultra::HeaderToolbar::resized()
{
    // Bounds are calculated in paint()
}

inline void ultra::HeaderToolbar::mouseUp (const juce::MouseEvent& e)
{
    auto pos = e.position;
    if (playBounds.contains(pos) && onPlay) onPlay();
    else if (stopBounds.contains(pos) && onStop) onStop();
    else if (recordBounds.contains(pos) && onRecord) onRecord();
    else if (settingsBounds.contains(pos) && onSettings) onSettings();
}

inline void ultra::HeaderToolbar::drawIconButton (juce::Graphics& g, juce::Rectangle<float> bounds,
                                                  const juce::Path& icon, juce::Colour color)
{
    const auto& t = tokens();

    // Button background
    g.setColour(t.color.bg2);
    g.fillRoundedRectangle(bounds, bounds.getHeight() / 2);
    g.setColour(t.color.panelBorder);
    g.drawRoundedRectangle(bounds, bounds.getHeight() / 2, 1.0f);

    // Icon
    auto iconBounds = bounds.reduced(6);
    auto transform = juce::AffineTransform::scale(iconBounds.getWidth() / 18.0f, iconBounds.getHeight() / 18.0f)
                     .translated(iconBounds.getX(), iconBounds.getY());

    g.setColour(color);
    g.fillPath(icon, transform);
}

// ============================================================================
// TabBarPro implementation
// ============================================================================
inline ultra::TabBarPro::TabBarPro()
{
}

inline void ultra::TabBarPro::setTabs (const juce::StringArray& tabNames)
{
    tabs = tabNames;
    selectedIndex = juce::jlimit(0, tabs.size() - 1, selectedIndex);
    repaint();
}

inline void ultra::TabBarPro::setSelectedTab (int index)
{
    if (index >= 0 && index < tabs.size() && index != selectedIndex)
    {
        selectedIndex = index;
        repaint();
        if (onChange)
            onChange(selectedIndex);
    }
}

inline void ultra::TabBarPro::paint (juce::Graphics& g)
{
    const auto& t = tokens();
    auto bounds = getLocalBounds().toFloat();

    // Container background
    g.setColour(t.color.bg2);
    g.fillRoundedRectangle(bounds, t.radius.s);
    g.setColour(t.color.panelBorder);
    g.drawRoundedRectangle(bounds, t.radius.s, 1.0f);

    if (tabs.isEmpty()) return;

    const float tabWidth = bounds.getWidth() / (float)tabs.size();

    for (int i = 0; i < tabs.size(); ++i)
    {
        auto tabBounds = bounds.withX(bounds.getX() + i * tabWidth).withWidth(tabWidth).reduced(2);
        bool isSelected = (i == selectedIndex);

        // Tab background
        if (isSelected)
        {
            g.setColour(t.color.accentSecondary.withAlpha(0.22f));
            g.fillRoundedRectangle(tabBounds, t.radius.s);
            g.setColour(t.color.accentPrimary);
            g.drawRoundedRectangle(tabBounds, t.radius.s, 1.0f);
        }
        else
        {
            g.setColour(t.color.bg1);
            g.fillRoundedRectangle(tabBounds, t.radius.s);
        }

        // Tab text
        g.setColour(isSelected ? t.color.textPrimary : t.color.textSecondary);
        g.setFont(juce::Font(t.font.familyBase, t.font.size14, juce::Font::bold));
        g.drawText(tabs[i], tabBounds.toNearestInt(), juce::Justification::centred, true);
    }
}

inline void ultra::TabBarPro::mouseUp (const juce::MouseEvent& e)
{
    if (tabs.isEmpty()) return;

    const float tabWidth = (float)getWidth() / (float)tabs.size();
    int clickedTab = juce::jlimit(0, tabs.size() - 1, (int)(e.position.x / tabWidth));
    setSelectedTab(clickedTab);
}

// ============================================================================
// UNIT TESTS
// ============================================================================

#if JUCE_UNIT_TESTS

#include <juce_unit_tests/juce_unit_tests.h>

namespace ultra
{

class UltraDesignSystemTests final : public juce::UnitTest
{
public:
    UltraDesignSystemTests() : juce::UnitTest ("UltraDesignSystem", "Ultra") {}

    void runTest() override
    {
        beginTest ("Tokens defaults");
        {
            resetTokensToDefaults();
            const auto& t = tokens();
            expect (t.color.accentPrimary == juce::Colour (0xFF00D4FF));
            expect (t.font.size14 == 14.0f);
            expect (t.space.s8 == 8);
            expect (t.radius.m == 12.0f);
        }

        beginTest ("JSON token override");
        {
            resetTokensToDefaults();
            loadTokensFromJSON (R"JSON({
                "color": { "accent/primary": "#00FFC3" },
                "font": { "size/14": 15 }
            })JSON");
            const auto& t = tokens();
            expect (t.color.accentPrimary == juce::Colour::fromRGB (0x00, 0xFF, 0xC3));
            expect (t.font.size14 == 15.0f);
            resetTokensToDefaults();
        }

        beginTest ("RingSlider smoke test");
        {
            RingSlider slider;
            slider.setBounds (0, 0, 100, 100);
            slider.setValue (3.0);

            juce::Image img (juce::Image::ARGB, 100, 100, true);
            juce::Graphics g (img);
            slider.paint (g);

            // Should not crash, image should have content
            expect (img.getPixelAt (50, 50).getAlpha() > 0);
        }

        beginTest ("RingSlider value text updates");
        {
            RingSlider slider;
            slider.setBounds (0, 0, 100, 100);
            slider.setValue (6.5);
            expectEquals (slider.valueText, juce::String("+6.5 dB"));

            slider.setValue (-12.0);
            expectEquals (slider.valueText, juce::String("-12.0 dB"));

            slider.setValue (0.01);
            expectEquals (slider.valueText, juce::String("0.0 dB"));
        }

        beginTest ("PeakRmsMeter paint preserves bounds");
        {
            PeakRmsMeter meter;
            meter.setBounds (0, 0, 40, 160);
            meter.resized();

            const auto initialPeak = meter.peakBounds;
            const auto initialRms = meter.rmsBounds;

            meter.setLevels (0.8f, 0.3f);
            juce::Image img (juce::Image::ARGB, 40, 160, true);
            juce::Graphics g (img);
            meter.paint (g);

            expect (meter.peakBounds == initialPeak);
            expect (meter.rmsBounds == initialRms);

            const auto sampleX = (int) std::round (initialPeak.getCentreX());
            const auto bottomY = juce::jlimit (0, img.getHeight() - 1, (int) std::floor (initialPeak.getBottom() - 2.0f));
            const auto midY = juce::jlimit (0, img.getHeight() - 1, (int) std::floor (initialPeak.getBottom() - initialPeak.getHeight() * 0.6f));
            const auto bottomColour = img.getPixelAt (sampleX, bottomY);
            const auto midColour = img.getPixelAt (sampleX, midY);
            expect (bottomColour != midColour);
        }
    }
};

static UltraDesignSystemTests ultraDesignSystemTests;

} // namespace ultra

#endif
