#pragma once

#include <memory>
#include <atomic>
#include <mutex>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

namespace daw::ui {
class AdaptiveAnimationManager;
}

namespace daw::ui::animation {

/**
 * AdaptiveAnimationService
 *
 * High-level service that wraps AdaptiveAnimationManager and exposes
 * lifecycle hooks compatible with the ServiceLocator + dependency injection
 * architecture. Handles GPU attachment, feature-flag gating, and provides
 * a stable API for UI consumers to request animations.
 */
class AdaptiveAnimationService final
{
public:
    AdaptiveAnimationService();
    ~AdaptiveAnimationService();

    AdaptiveAnimationService(const AdaptiveAnimationService&) = delete;
    AdaptiveAnimationService& operator=(const AdaptiveAnimationService&) = delete;
    AdaptiveAnimationService(AdaptiveAnimationService&&) noexcept = delete;
    AdaptiveAnimationService& operator=(AdaptiveAnimationService&&) noexcept = delete;

    /**
     * Initialize the service.
     * @param hostComponent Optional component used for GPU attachment.
     */
    bool initialize(juce::Component* hostComponent = nullptr);

    /**
     * Shutdown service and release resources.
     */
    void shutdown();

    /**
     * Attach to host component (GPU context).
     */
    void attachToComponent(juce::Component& component);

    /**
     * Detach from host component.
     */
    void detachFromComponent(juce::Component& component);

    /**
     * Access the underlying manager (read-only).
     */
    [[nodiscard]] AdaptiveAnimationManager* getManager() const noexcept;

    /**
     * Utility for animating a float value.
     */
    uint32_t animateFloat(float startValue,
                          float endValue,
                          float durationMs,
                          std::function<void(float)> onValue,
                          std::function<void()> onComplete = nullptr);

    /** Cancel a previously created animation. */
    bool cancelAnimation(uint32_t animationId);

    /**
     * Expose initialization state.
     */
    [[nodiscard]] bool isInitialized() const noexcept { return initialized_.load(); }

private:
    std::unique_ptr<AdaptiveAnimationManager> manager_;
    std::atomic<bool> initialized_ { false };
    juce::Component* attachedComponent_ { nullptr };
    std::mutex mutex_;
};

} // namespace daw::ui::animation
