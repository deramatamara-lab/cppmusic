/**
 * @file SuggestionPanel.cpp
 * @brief AI arrangement suggestion panel (stub)
 */

#include "SuggestionPanel.h"

namespace daw::ui::arrangement {

class SuggestionPanel::Impl {
public:
    struct Suggestion {
        juce::String id;
        juce::String type;  // pattern, transition, fill, variation
        juce::String description;
        float confidence{0.0f};
        double startTime{0.0};
        double duration{0.0};
        int trackIndex{0};
        bool applied{false};
    };
    
    std::vector<Suggestion> suggestions;
    int selectedIndex{-1};
    bool autoRefresh{true};
    bool showLowConfidence{false};
    float minConfidenceThreshold{0.5f};
};

SuggestionPanel::SuggestionPanel()
    : impl_(std::make_unique<Impl>()) {
}

SuggestionPanel::~SuggestionPanel() = default;

void SuggestionPanel::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    // Header
    auto header = bounds.removeFromTop(35);
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(header);
    
    g.setColour(juce::Colours::white);
    g.drawText("AI Suggestions", header.reduced(10, 0), juce::Justification::centredLeft);
    
    // AI indicator
    auto aiIndicator = header.removeFromRight(60);
    g.setColour(juce::Colour(0xff40ff80));
    g.fillRoundedRectangle(aiIndicator.reduced(5), 5.0f);
    g.setColour(juce::Colours::black);
    g.drawText("AI", aiIndicator, juce::Justification::centred);
    
    bounds.removeFromTop(5);
    
    // Filter suggestions
    auto visibleSuggestions = impl_->suggestions;
    if (!impl_->showLowConfidence) {
        visibleSuggestions.erase(
            std::remove_if(visibleSuggestions.begin(), visibleSuggestions.end(),
                [this](const Impl::Suggestion& s) {
                    return s.confidence < impl_->minConfidenceThreshold;
                }),
            visibleSuggestions.end());
    }
    
    // Suggestions list
    if (visibleSuggestions.empty()) {
        g.setColour(juce::Colour(0xff808080));
        g.drawText("No suggestions available\nAnalyze your arrangement to get AI suggestions",
                   bounds, juce::Justification::centred);
        return;
    }
    
    int itemHeight = 70;
    for (size_t i = 0; i < visibleSuggestions.size(); ++i) {
        if (bounds.getHeight() < itemHeight) break;
        
        auto row = bounds.removeFromTop(itemHeight);
        const auto& suggestion = visibleSuggestions[i];
        
        bool isSelected = static_cast<int>(i) == impl_->selectedIndex;
        
        // Card background
        g.setColour(isSelected ? juce::Colour(0xff303040) : juce::Colour(0xff252525));
        g.fillRoundedRectangle(row.reduced(5), 5.0f);
        
        auto content = row.reduced(10);
        
        // Type badge
        juce::Colour typeColor = 
            suggestion.type == "pattern" ? juce::Colour(0xff4080ff) :
            suggestion.type == "transition" ? juce::Colour(0xff40ff80) :
            suggestion.type == "fill" ? juce::Colour(0xffffaa40) :
            juce::Colour(0xffff4080);
        
        auto badge = content.removeFromLeft(70).removeFromTop(20);
        g.setColour(typeColor);
        g.fillRoundedRectangle(badge, 3.0f);
        g.setColour(juce::Colours::black);
        g.drawText(suggestion.type, badge, juce::Justification::centred);
        
        content.removeFromLeft(10);
        
        // Description
        g.setColour(juce::Colours::white);
        g.drawText(suggestion.description, content.removeFromTop(25),
                   juce::Justification::centredLeft);
        
        // Confidence bar
        auto confBar = content.removeFromTop(15);
        g.setColour(juce::Colour(0xff404040));
        g.fillRoundedRectangle(confBar.withWidth(100), 3.0f);
        
        juce::Colour confColor = 
            suggestion.confidence > 0.8f ? juce::Colour(0xff40ff40) :
            suggestion.confidence > 0.5f ? juce::Colour(0xffffaa40) :
            juce::Colour(0xffff4040);
        g.setColour(confColor);
        g.fillRoundedRectangle(confBar.withWidth(100 * suggestion.confidence), 3.0f);
        
        g.setColour(juce::Colour(0xff808080));
        g.drawText(juce::String(suggestion.confidence * 100, 0) + "% confidence",
                   confBar.withX(confBar.getX() + 110), juce::Justification::centredLeft);
        
        // Apply button
        auto applyBtn = row.removeFromRight(60).reduced(10);
        g.setColour(suggestion.applied ? juce::Colour(0xff40ff40) : juce::Colour(0xff4080ff));
        g.fillRoundedRectangle(applyBtn, 3.0f);
        g.setColour(juce::Colours::black);
        g.drawText(suggestion.applied ? "✓" : "Apply", applyBtn, juce::Justification::centred);
        
        bounds.removeFromTop(5);
    }
}

void SuggestionPanel::resized() {
    // Layout handled in paint
}

void SuggestionPanel::refresh() {
    // TODO: Query AI for new suggestions
    repaint();
}

int SuggestionPanel::getSuggestionCount() const {
    return static_cast<int>(impl_->suggestions.size());
}

void SuggestionPanel::setSelectedIndex(int index) {
    impl_->selectedIndex = index;
    repaint();
}

int SuggestionPanel::getSelectedIndex() const {
    return impl_->selectedIndex;
}

void SuggestionPanel::applySelected() {
    if (impl_->selectedIndex >= 0 && 
        impl_->selectedIndex < static_cast<int>(impl_->suggestions.size())) {
        impl_->suggestions[impl_->selectedIndex].applied = true;
        repaint();
    }
}

void SuggestionPanel::setAutoRefresh(bool enabled) {
    impl_->autoRefresh = enabled;
}

bool SuggestionPanel::isAutoRefreshEnabled() const {
    return impl_->autoRefresh;
}

void SuggestionPanel::setShowLowConfidence(bool show) {
    impl_->showLowConfidence = show;
    repaint();
}

bool SuggestionPanel::isShowingLowConfidence() const {
    return impl_->showLowConfidence;
}

void SuggestionPanel::setMinConfidenceThreshold(float threshold) {
    impl_->minConfidenceThreshold = juce::jlimit(0.0f, 1.0f, threshold);
    repaint();
}

float SuggestionPanel::getMinConfidenceThreshold() const {
    return impl_->minConfidenceThreshold;
}

}  // namespace daw::ui::arrangement
