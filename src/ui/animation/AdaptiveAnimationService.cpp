#include "AdaptiveAnimationService.h"
#include "AdaptiveAnimationManager.h"
#include "../../core/ServiceLocator.h"
#include <juce_core/juce_core.h>

namespace daw::ui::animation
{

AdaptiveAnimationService::AdaptiveAnimationService()
    : manager_(std::make_unique<AdaptiveAnimationManager>())
{
}

AdaptiveAnimationService::~AdaptiveAnimationService()
{
    shutdown();
}

bool AdaptiveAnimationService::initialize(juce::Component* hostComponent)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_.load())
        return true;

    if (!manager_->initialize())
    {
        juce::Logger::writeToLog("AdaptiveAnimationManager failed to initialize; animations disabled");
        return false;
    }

    if (hostComponent != nullptr)
    {
        manager_->attachToComponent(*hostComponent);
        attachedComponent_ = hostComponent;
    }

    initialized_.store(true);
    juce::Logger::writeToLog("AdaptiveAnimationService initialized");
    return true;
}

void AdaptiveAnimationService::shutdown()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.load())
        return;

    if (attachedComponent_ != nullptr)
    {
        manager_->detachFromComponent(*attachedComponent_);
        attachedComponent_ = nullptr;
    }

    manager_->shutdown();
    initialized_.store(false);
    juce::Logger::writeToLog("AdaptiveAnimationService shutdown complete");
}

void AdaptiveAnimationService::attachToComponent(juce::Component& component)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.load())
    {
        initialize(&component);
        return;
    }

    manager_->attachToComponent(component);
    attachedComponent_ = &component;
}

void AdaptiveAnimationService::detachFromComponent(juce::Component& component)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (attachedComponent_ == &component)
    {
        manager_->detachFromComponent(component);
        attachedComponent_ = nullptr;
    }
}

AdaptiveAnimationManager* AdaptiveAnimationService::getManager() const noexcept
{
    return manager_.get();
}

uint32_t AdaptiveAnimationService::animateFloat(float startValue,
                                                float endValue,
                                                float durationMs,
                                                std::function<void(float)> onValue,
                                                std::function<void()> onComplete)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.load())
        return 0;

    const auto animationId = manager_->createAnimation(startValue, endValue, durationMs);
    if (animationId == 0)
        return 0;

    if (onValue)
        manager_->setAnimationCallback(animationId, std::move(onValue));

    if (onComplete)
        manager_->setCompletionCallback(animationId, std::move(onComplete));

    if (!manager_->startAnimation(animationId))
        return 0;

    return animationId;
}

bool AdaptiveAnimationService::cancelAnimation(uint32_t animationId)
{
    if (animationId == 0)
        return false;

    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.load())
        return false;

    return manager_->cancelAnimation(animationId);
}

} // namespace daw::ui::animation
