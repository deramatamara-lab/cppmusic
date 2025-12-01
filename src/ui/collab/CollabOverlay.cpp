/**
 * @file CollabOverlay.cpp
 * @brief Collaboration session overlay (stub)
 */

#include "CollabOverlay.h"

namespace daw::ui::collab {

class CollabOverlay::Impl {
public:
    struct User {
        juce::String id;
        juce::String name;
        juce::Colour color;
        bool isOnline{true};
        double cursorTime{0.0};
        int cursorTrack{0};
    };
    
    std::vector<User> users;
    bool sessionActive{false};
    juce::String sessionId;
    juce::String localUserId;
};

CollabOverlay::CollabOverlay()
    : impl_(std::make_unique<Impl>()) {
}

CollabOverlay::~CollabOverlay() = default;

void CollabOverlay::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    if (!impl_->sessionActive) {
        // Not in session - transparent
        return;
    }
    
    // Draw user cursors and selections (stub)
    for (const auto& user : impl_->users) {
        if (!user.isOnline || user.id == impl_->localUserId) continue;
        
        // Draw cursor indicator
        g.setColour(user.color);
        float x = static_cast<float>(user.cursorTime * 50.0);  // Stub mapping
        g.drawLine(x, 0, x, bounds.getHeight(), 2.0f);
        
        // User name label
        g.setColour(user.color.darker());
        g.fillRoundedRectangle(x, 0, 80, 20, 3.0f);
        g.setColour(juce::Colours::white);
        g.drawText(user.name, juce::Rectangle<float>(x, 0, 80, 20),
                   juce::Justification::centred);
    }
    
    // Session indicator
    g.setColour(juce::Colour(0xff40ff80));
    g.fillRoundedRectangle(bounds.getWidth() - 120, 5, 115, 25, 5.0f);
    g.setColour(juce::Colours::black);
    g.drawText("Session Active", juce::Rectangle<float>(bounds.getWidth() - 120, 5, 115, 25),
               juce::Justification::centred);
}

void CollabOverlay::resized() {
    // Overlay takes full parent bounds
}

void CollabOverlay::setSessionActive(bool active) {
    impl_->sessionActive = active;
    repaint();
}

bool CollabOverlay::isSessionActive() const {
    return impl_->sessionActive;
}

void CollabOverlay::setSessionId(const juce::String& id) {
    impl_->sessionId = id;
}

juce::String CollabOverlay::getSessionId() const {
    return impl_->sessionId;
}

void CollabOverlay::setLocalUserId(const juce::String& id) {
    impl_->localUserId = id;
}

int CollabOverlay::getOnlineUserCount() const {
    int count = 0;
    for (const auto& user : impl_->users) {
        if (user.isOnline) ++count;
    }
    return count;
}

}  // namespace daw::ui::collab
