/**
 * @file CollabOverlay.h
 * @brief Collaboration session overlay header
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace daw::ui::collab {

/**
 * @brief Overlay showing collaboration session state
 *
 * Features:
 * - Remote user cursors
 * - Selection indicators
 * - User presence list
 * - Session status
 */
class CollabOverlay : public juce::Component {
public:
    CollabOverlay();
    ~CollabOverlay() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Session state
    void setSessionActive(bool active);
    [[nodiscard]] bool isSessionActive() const;
    void setSessionId(const juce::String& id);
    [[nodiscard]] juce::String getSessionId() const;
    void setLocalUserId(const juce::String& id);

    // Users
    [[nodiscard]] int getOnlineUserCount() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CollabOverlay)
};

}  // namespace daw::ui::collab
