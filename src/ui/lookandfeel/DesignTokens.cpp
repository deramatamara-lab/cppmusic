#include "DesignTokens.h"
#include <cmath>
#include <mutex>

namespace daw::ui::lookandfeel
{

// ------------------ Perceptual color helpers (OKLab) -------------------------

namespace
{
    using F = float;

    inline F srgbToLinear(F c) noexcept
    {
        return (c <= 0.04045f) ? (c / 12.92f) : std::pow((c + 0.055f) / 1.055f, 2.4f);
    }
    inline F linearToSrgb(F c) noexcept
    {
        return (c <= 0.0031308f) ? (12.92f * c) : (1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f);
    }
    inline F clamp01(F v) noexcept { return juce::jlimit(0.0f, 1.0f, v); }

    struct OKLab { F L, a, b; };

    // sRGB (0..1) -> OKLab
    inline OKLab toOKLab(F R, F G, F B) noexcept
    {
        const F r = srgbToLinear(R), g = srgbToLinear(G), b = srgbToLinear(B);
        const F l = 0.4122214708f*r + 0.5363325363f*g + 0.0514459929f*b;
        const F m = 0.2119034982f*r + 0.6806995451f*g + 0.1073969566f*b;
        const F s = 0.0883024619f*r + 0.2817188376f*g + 0.6299787005f*b;

        const F l_ = std::cbrt(l);
        const F m_ = std::cbrt(m);
        const F s_ = std::cbrt(s);

        OKLab o;
        o.L = 0.2104542553f*l_ + 0.7936177850f*m_ - 0.0040720468f*s_;
        o.a = 1.9779984951f*l_ - 2.4285922050f*m_ + 0.4505937099f*s_;
        o.b = 0.0259040371f*l_ + 0.7827717662f*m_ - 0.8086757660f*s_;
        return o;
    }

    // OKLab -> sRGB (0..1)
    inline void fromOKLab(OKLab o, F& R, F& G, F& B) noexcept
    {
        const F l_ = o.L + 0.3963377774f*o.a + 0.2158037573f*o.b;
        const F m_ = o.L - 0.1055613458f*o.a - 0.0638541728f*o.b;
        const F s_ = o.L - 0.0894841775f*o.a - 1.2914855480f*o.b;

        const F l = l_*l_*l_;
        const F m = m_*m_*m_;
        const F s = s_*s_*s_;

        const F r =  4.0767416621f*l - 3.3077115913f*m + 0.2309699292f*s;
        const F g = -1.2684380046f*l + 2.6097574011f*m - 0.3413193965f*s;
        const F b = -0.0041960863f*l - 0.7034186147f*m + 1.7076147010f*s;

        R = clamp01(linearToSrgb(r));
        G = clamp01(linearToSrgb(g));
        B = clamp01(linearToSrgb(b));
    }

    inline juce::Colour oklabAdjustLightness(juce::Colour c, F deltaL) noexcept
    {
        F R = c.getFloatRed(), G = c.getFloatGreen(), B = c.getFloatBlue();
        auto o = toOKLab(R, G, B);
        o.L = clamp01(o.L + deltaL);
        fromOKLab(o, R, G, B);
        return juce::Colour::fromFloatRGBA(R, G, B, c.getFloatAlpha());
    }

    inline juce::Colour oklabMix(juce::Colour a, juce::Colour b, F t) noexcept
    {
        F ar=a.getFloatRed(), ag=a.getFloatGreen(), ab=a.getFloatBlue();
        F br=b.getFloatRed(), bg=b.getFloatGreen(), bb=b.getFloatBlue();
        auto oa = toOKLab(ar, ag, ab);
        auto ob = toOKLab(br, bg, bb);
        OKLab o { juce::jmap(t, oa.L, ob.L),
                  juce::jmap(t, oa.a, ob.a),
                  juce::jmap(t, oa.b, ob.b) };
        fromOKLab(o, ar, ag, ab);
        return juce::Colour::fromFloatRGBA(ar, ag, ab, juce::jmap(t, a.getFloatAlpha(), b.getFloatAlpha()));
    }

    // WCAG 2.1 relative luminance + contrast ratio
    inline float relativeLuminance(juce::Colour c) noexcept
    {
        const float r = srgbToLinear(c.getFloatRed());
        const float g = srgbToLinear(c.getFloatGreen());
        const float b = srgbToLinear(c.getFloatBlue());
        return 0.2126f*r + 0.7152f*g + 0.0722f*b;
    }

    inline float contrastRatio(juce::Colour fg, juce::Colour bg) noexcept
    {
        const float L1 = relativeLuminance(fg);
        const float L2 = relativeLuminance(bg);
        const float hi = juce::jmax(L1, L2);
        const float lo = juce::jmin(L1, L2);
        return (hi + 0.05f) / (lo + 0.05f);
    }

    // Choose readable "on" color for a background; tries white/black then falls back to secondary accent if needed.
    inline juce::Colour pickOnColor(juce::Colour background, juce::Colour accentSecondary) noexcept
    {
        constexpr float kMin = 4.5f; // body text
        const auto white = juce::Colours::white;
        const auto black = juce::Colours::black;

        if (contrastRatio(white, background) >= kMin) return white;
        if (contrastRatio(black, background) >= kMin) return black;

        // Blend towards the more readable pole while keeping some brand hue
        const auto toward = (relativeLuminance(background) < 0.5f) ? white : black;
        auto mixed = oklabMix(accentSecondary, toward, 0.75f).withAlpha(1.0f);
        // Ensure final contrast; if still low, snap to pole
        return (contrastRatio(mixed, background) >= kMin) ? mixed : toward;
    }

    inline ColorTokens buildDarkColors(ColorTokens c) noexcept
    {
        // Ensure consistent shadow tint & focus ring
        c.panelShadow = juce::Colours::black;
        c.focusRing   = c.accentSecondary.withAlpha(1.0f);

        // Derive interaction states perceptually
        c.accentPrimaryHover  = oklabAdjustLightness(c.accentPrimary, +0.08f);
        c.accentPrimaryActive = oklabAdjustLightness(c.accentPrimary, -0.10f);

        // Readable on-accent text
        c.onAccent = pickOnColor(c.accentPrimary, c.accentSecondary);
        return c;
    }

    inline ColorTokens buildLightColors(ColorTokens c) noexcept
    {
        // Light theme base (use your preferred palette; this is harmonious with your brand hues)
        c.background       = juce::Colour(0xfff6f5ff);
        c.backgroundAlt    = juce::Colour(0xffefedfb);
        c.panelBackground  = juce::Colour(0xffffffff);
        c.panelHighlight   = juce::Colour(0xffefeaff);
        c.panelBorder      = juce::Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f, 0.12f);
        c.panelShadow      = juce::Colours::black.withAlpha(1.0f);

        c.textPrimary      = juce::Colour(0xff1b1833);
        c.textSecondary    = juce::Colour(0xff5a5670);
        c.textDisabled     = juce::Colour(0xffa6a3b6);

        // Derive interaction states
        c.accentPrimaryHover  = oklabAdjustLightness(c.accentPrimary, -0.04f); // go slightly darker on light
        c.accentPrimaryActive = oklabAdjustLightness(c.accentPrimary, -0.12f);

        c.focusRing       = c.accentSecondary;
        c.onAccent        = pickOnColor(c.accentPrimary, c.accentSecondary);
        return c;
    }
}

// ------------------ Public API ------------------

const DesignTokens& getDesignTokens(Theme theme) noexcept
{
    static std::once_flag onceDark, onceLight;
    static DesignTokens dark, light;

    if (theme == Theme::Dark)
    {
        std::call_once(onceDark, [] {
            DesignTokens t;
            t.theme = Theme::Dark;
            t.colours = buildDarkColors(t.colours);
            dark = t;
        });
        return dark;
    }
    else
    {
        std::call_once(onceLight, [] {
            DesignTokens t;
            t.theme = Theme::Light;
            t.colours = buildLightColors(t.colours);
            light = t;
        });
        return light;
    }
}

} // namespace daw::ui::lookandfeel

// ------------------ Minimal verification (optional in Release) ---------------

#if JUCE_UNIT_TESTS
namespace daw::ui::lookandfeel
{
struct TokenContrastTest final : public juce::UnitTest
{
    TokenContrastTest() : juce::UnitTest("DesignTokens Accessibility/Derivation") {}

    void runTest() override
    {
        beginTest("On-accent contrast ≥ 4.5");
        for (auto th : { Theme::Dark, Theme::Light })
        {
            const auto& d = getDesignTokens(th);
            const float cr = contrastRatio(d.colours.onAccent, d.colours.accentPrimary);
            expect(cr >= 4.5f, juce::String("contrast=") + juce::String(cr));
        }

        beginTest("Hover/Active are distinct from base");
        for (auto th : { Theme::Dark, Theme::Light })
        {
            const auto& c = getDesignTokens(th).colours;
            expect(! c.accentPrimaryHover.equals(c.accentPrimary));
            expect(! c.accentPrimaryActive.equals(c.accentPrimary));
        }
    }
};
static TokenContrastTest tokenContrastTest;
} // namespace daw::ui::lookandfeel
#endif
