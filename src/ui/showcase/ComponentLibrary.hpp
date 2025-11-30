#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/UltraDesignSystem.hpp"
#include <memory>

namespace daw::ui::showcase
{

// ============================================================================
// Component Library Showcase - Demonstrates all design tokens and components
// ============================================================================
class ComponentLibraryShowcase : public juce::Component
{
public:
    ComponentLibraryShowcase();
    ~ComponentLibraryShowcase() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class TokensSection;
    class ComponentsSection;
    class AnimationSection;

    std::unique_ptr<juce::Viewport> mainViewport;
    std::unique_ptr<juce::Component> contentContainer;
    std::unique_ptr<TokensSection> tokensSection;
    std::unique_ptr<ComponentsSection> componentsSection;
    std::unique_ptr<AnimationSection> animationSection;

    void setupSections();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComponentLibraryShowcase)
};

// ============================================================================
// Design Tokens Showcase Section
// ============================================================================
class ComponentLibraryShowcase::TokensSection : public juce::Component
{
public:
    TokensSection();
    ~TokensSection() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class ColorPalette;
    class TypographyScale;
    class SpacingGrid;
    class RadiusExamples;

    std::unique_ptr<ColorPalette> colorPalette;
    std::unique_ptr<TypographyScale> typography;
    std::unique_ptr<SpacingGrid> spacing;
    std::unique_ptr<RadiusExamples> radii;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TokensSection)
};

// ============================================================================
// Components Showcase Section
// ============================================================================
class ComponentLibraryShowcase::ComponentsSection : public juce::Component
{
public:
    ComponentsSection();
    ~ComponentsSection() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class KnobsAndSliders;
    class ButtonsAndToggles;
    class MetersAndIndicators;
    class NavigationElements;

    std::unique_ptr<KnobsAndSliders> knobsSliders;
    std::unique_ptr<ButtonsAndToggles> buttons;
    std::unique_ptr<MetersAndIndicators> meters;
    std::unique_ptr<NavigationElements> navigation;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComponentsSection)
};

// ============================================================================
// Animation & Micro-interactions Section
// ============================================================================
class ComponentLibraryShowcase::AnimationSection : public juce::Component
{
public:
    AnimationSection();
    ~AnimationSection() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class KnobAnimations;
    class TabTransitions;
    class MeterAnimations;
    class HoverEffects;

    std::unique_ptr<KnobAnimations> knobAnimations;
    std::unique_ptr<TabTransitions> tabTransitions;
    std::unique_ptr<MeterAnimations> meterAnimations;
    std::unique_ptr<HoverEffects> hoverEffects;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimationSection)
};

} // namespace daw::ui::showcase
