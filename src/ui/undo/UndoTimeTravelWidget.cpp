/**
 * @file UndoTimeTravelWidget.cpp
 * @brief Time-travel undo history visualization (stub)
 */

#include "UndoTimeTravelWidget.h"

namespace daw::ui::undo {

class UndoTimeTravelWidget::Impl {
public:
    struct HistoryEntry {
        juce::String id;
        juce::String description;
        juce::Time timestamp;
        juce::String hash;  // State hash for integrity verification
        bool isCheckpoint{false};
    };
    
    std::vector<HistoryEntry> history;
    int currentIndex{-1};
    int selectedIndex{-1};
    bool showHashes{false};
};

UndoTimeTravelWidget::UndoTimeTravelWidget()
    : impl_(std::make_unique<Impl>()) {
}

UndoTimeTravelWidget::~UndoTimeTravelWidget() = default;

void UndoTimeTravelWidget::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    // Title
    auto header = bounds.removeFromTop(30);
    g.setColour(juce::Colours::white);
    g.drawText("Undo History (Time Travel)", header, juce::Justification::centredLeft);
    
    // Timeline
    auto timeline = bounds.removeFromTop(40);
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(timeline);
    
    if (!impl_->history.empty()) {
        float itemWidth = timeline.getWidth() / impl_->history.size();
        
        for (size_t i = 0; i < impl_->history.size(); ++i) {
            float x = i * itemWidth;
            bool isCurrent = static_cast<int>(i) == impl_->currentIndex;
            bool isSelected = static_cast<int>(i) == impl_->selectedIndex;
            bool isCheckpoint = impl_->history[i].isCheckpoint;
            
            juce::Colour dotColor = isCurrent ? juce::Colour(0xff40ff40) :
                                    isSelected ? juce::Colour(0xff4080ff) :
                                    isCheckpoint ? juce::Colour(0xffffaa40) :
                                    juce::Colour(0xff606060);
            
            g.setColour(dotColor);
            float dotSize = isCheckpoint ? 12.0f : 8.0f;
            g.fillEllipse(x + itemWidth/2 - dotSize/2, 
                         timeline.getCentreY() - dotSize/2,
                         dotSize, dotSize);
        }
        
        // Connection line
        g.setColour(juce::Colour(0xff404040));
        g.drawLine(0, timeline.getCentreY(), timeline.getWidth(), timeline.getCentreY(), 2.0f);
    }
    
    bounds.removeFromTop(10);
    
    // History list
    int itemHeight = 35;
    for (int i = static_cast<int>(impl_->history.size()) - 1; i >= 0; --i) {
        if (bounds.getHeight() < itemHeight) break;
        
        auto row = bounds.removeFromTop(itemHeight);
        const auto& entry = impl_->history[i];
        
        bool isCurrent = i == impl_->currentIndex;
        bool isSelected = i == impl_->selectedIndex;
        
        if (isCurrent) {
            g.setColour(juce::Colour(0xff40ff40).withAlpha(0.2f));
            g.fillRect(row);
        } else if (isSelected) {
            g.setColour(juce::Colour(0xff4080ff).withAlpha(0.2f));
            g.fillRect(row);
        }
        
        // Entry indicator
        g.setColour(entry.isCheckpoint ? juce::Colour(0xffffaa40) : juce::Colour(0xff606060));
        g.fillEllipse(row.getX() + 10, row.getCentreY() - 4, 8, 8);
        
        // Description
        g.setColour(juce::Colours::white);
        g.drawText(entry.description, row.withTrimmedLeft(30), 
                   juce::Justification::centredLeft);
        
        // Timestamp
        g.setColour(juce::Colour(0xff808080));
        g.drawText(entry.timestamp.toString(false, true), 
                   row.removeFromRight(80), juce::Justification::centredRight);
        
        // Hash (if enabled)
        if (impl_->showHashes) {
            g.drawText(entry.hash.substring(0, 8) + "...", 
                       row.removeFromRight(100), juce::Justification::centredRight);
        }
    }
    
    // Empty state
    if (impl_->history.empty()) {
        g.setColour(juce::Colour(0xff808080));
        g.drawText("No undo history\nMake changes to see history here",
                   bounds, juce::Justification::centred);
    }
}

void UndoTimeTravelWidget::resized() {
    // Layout handled in paint
}

int UndoTimeTravelWidget::getHistorySize() const {
    return static_cast<int>(impl_->history.size());
}

int UndoTimeTravelWidget::getCurrentIndex() const {
    return impl_->currentIndex;
}

void UndoTimeTravelWidget::setSelectedIndex(int index) {
    impl_->selectedIndex = juce::jlimit(-1, static_cast<int>(impl_->history.size()) - 1, index);
    repaint();
}

int UndoTimeTravelWidget::getSelectedIndex() const {
    return impl_->selectedIndex;
}

void UndoTimeTravelWidget::setShowHashes(bool show) {
    impl_->showHashes = show;
    repaint();
}

bool UndoTimeTravelWidget::isShowingHashes() const {
    return impl_->showHashes;
}

void UndoTimeTravelWidget::travelToSelected() {
    // TODO: Implement state restoration
    if (impl_->selectedIndex >= 0) {
        impl_->currentIndex = impl_->selectedIndex;
        repaint();
    }
}

void UndoTimeTravelWidget::createCheckpoint(const juce::String& name) {
    // TODO: Create checkpoint in undo service
    (void)name;
}

}  // namespace daw::ui::undo
