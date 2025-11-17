#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/DesignSystem.h"

namespace daw::ui::components
{

/**
 * @brief Base panel component
 * 
 * Panel with rounded corners, soft glow, gradient fill, optional header.
 * Follows DAW_DEV_RULES: uses design system, no magic numbers.
 */
class Panel : public juce::Component
{
public:
    Panel();
    ~Panel() override = default;

    void paint(juce::Graphics& g) override;

    /**
     * @brief Set panel title
     */
    void setTitle(const juce::String& title);

    /**
     * @brief Get panel title
     */
    [[nodiscard]] juce::String getTitle() const noexcept { return title; }

    /**
     * @brief Show/hide header
     */
    void setShowHeader(bool show);

    /**
     * @brief Set elevated appearance
     */
    void setElevated(bool elevated);

protected:
    juce::String title;
    bool showHeader{true};
    bool isElevated{false};

private:
    [[nodiscard]] juce::Rectangle<int> getHeaderBounds() const;
    [[nodiscard]] juce::Rectangle<int> getContentBounds() const;
};

} // namespace daw::ui::components

