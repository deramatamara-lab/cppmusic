/**
 * @file UndoTimeTravelWidget.h
 * @brief Time-travel undo history visualization header
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace daw::ui::undo {

/**
 * @brief Time-travel undo history widget
 *
 * Features:
 * - Visual undo timeline
 * - Jump to any point in history
 * - Checkpoint markers
 * - State hash verification
 */
class UndoTimeTravelWidget : public juce::Component {
public:
    UndoTimeTravelWidget();
    ~UndoTimeTravelWidget() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // History navigation
    [[nodiscard]] int getHistorySize() const;
    [[nodiscard]] int getCurrentIndex() const;
    void setSelectedIndex(int index);
    [[nodiscard]] int getSelectedIndex() const;

    // Display options
    void setShowHashes(bool show);
    [[nodiscard]] bool isShowingHashes() const;

    // Actions
    void travelToSelected();
    void createCheckpoint(const juce::String& name);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UndoTimeTravelWidget)
};

}  // namespace daw::ui::undo
