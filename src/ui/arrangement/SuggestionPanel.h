/**
 * @file SuggestionPanel.h
 * @brief AI arrangement suggestion panel header
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace daw::ui::arrangement {

/**
 * @brief AI-powered arrangement suggestion panel
 *
 * Features:
 * - Pattern placement suggestions
 * - Transition recommendations
 * - Fill suggestions
 * - Confidence scoring
 */
class SuggestionPanel : public juce::Component {
public:
    SuggestionPanel();
    ~SuggestionPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Suggestions
    void refresh();
    [[nodiscard]] int getSuggestionCount() const;
    void setSelectedIndex(int index);
    [[nodiscard]] int getSelectedIndex() const;
    void applySelected();

    // Settings
    void setAutoRefresh(bool enabled);
    [[nodiscard]] bool isAutoRefreshEnabled() const;
    void setShowLowConfidence(bool show);
    [[nodiscard]] bool isShowingLowConfidence() const;
    void setMinConfidenceThreshold(float threshold);
    [[nodiscard]] float getMinConfidenceThreshold() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SuggestionPanel)
};

}  // namespace daw::ui::arrangement
