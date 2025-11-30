#include "AnimationHelper.h"

namespace daw::ui::utils
{

AnimationHelper::AnimationHelper()
{
}

void AnimationHelper::animateBounds(juce::Component& component, juce::Rectangle<int> targetBounds,
                                  int durationMs, std::function<void()> onComplete)
{
    animator.animateComponent(&component, targetBounds, 1.0f, durationMs, false, 1.0, 1.0);

    if (onComplete)
    {
        juce::MessageManager::callAsync([onComplete]() {
            onComplete();
        });
    }
}

void AnimationHelper::animateOpacity(juce::Component& component, float targetAlpha,
                                    int durationMs, std::function<void()> onComplete)
{
    const auto currentAlpha = component.getAlpha();
    const auto startAlpha = currentAlpha;
    // const auto delta = targetAlpha - startAlpha;

    // Use timer for smooth opacity animation
    class OpacityAnimator : public juce::Timer
    {
    public:
        OpacityAnimator(juce::Component& comp, float start, float target, int duration, std::function<void()> cb)
            : component(comp), startAlpha(start), targetAlpha(target), durationMs(duration), callback(cb)
        {
            startTime = juce::Time::getMillisecondCounterHiRes();
            startTimer(16); // ~60fps
        }

        void timerCallback() override
        {
            const auto elapsed = static_cast<float>(juce::Time::getMillisecondCounterHiRes() - startTime);
            const auto progress = juce::jmin(1.0f, elapsed / static_cast<float>(durationMs));

            // Ease out cubic
            const auto eased = 1.0f - std::pow(1.0f - progress, 3.0f);
            const auto currentAlpha = startAlpha + (targetAlpha - startAlpha) * eased;

            component.setAlpha(currentAlpha);

            if (progress >= 1.0f)
            {
                stopTimer();
                if (callback)
                    callback();
                delete this;
            }
        }

    private:
        juce::Component& component;
        float startAlpha;
        float targetAlpha;
        int durationMs;
        std::function<void()> callback;
        double startTime;
    };

    new OpacityAnimator(component, startAlpha, targetAlpha, durationMs, onComplete);
}

void AnimationHelper::animateScale(juce::Component& component, float targetScale,
                                  int durationMs, std::function<void()> onComplete)
{
    const auto currentBounds = component.getBounds();
    // const auto center = currentBounds.getCentre();
    const auto targetBounds = currentBounds.withSizeKeepingCentre(
        static_cast<int>(currentBounds.getWidth() * targetScale),
        static_cast<int>(currentBounds.getHeight() * targetScale));

    animateBounds(component, targetBounds, durationMs, onComplete);
}

void AnimationHelper::fadeIn(juce::Component& component, int durationMs)
{
    component.setAlpha(0.0f);
    component.setVisible(true);
    animateOpacity(component, 1.0f, durationMs);
}

void AnimationHelper::fadeOut(juce::Component& component, int durationMs, std::function<void()> onComplete)
{
    animateOpacity(component, 0.0f, durationMs, [&component, onComplete]() {
        component.setVisible(false);
        if (onComplete)
            onComplete();
    });
}

void AnimationHelper::pulse(juce::Component& component, int durationMs, int repeatCount)
{
    class PulseAnimator : public juce::Timer
    {
    public:
        PulseAnimator(juce::Component& comp, int duration, int repeats)
            : component(comp), durationMs(duration), repeats(repeats)
        {
            startTime = juce::Time::getMillisecondCounterHiRes();
            startTimer(16); // ~60fps
        }

        void timerCallback() override
        {
            const auto elapsed = static_cast<float>(juce::Time::getMillisecondCounterHiRes() - startTime);
            const auto cycleTime = static_cast<float>(durationMs);
            const auto cycleProgress = std::fmod(elapsed, cycleTime) / cycleTime;

            // Pulse: 0 -> 1 -> 0
            const auto pulseValue = std::sin(cycleProgress * juce::MathConstants<float>::twoPi) * 0.5f + 0.5f;
            const auto scale = 1.0f + pulseValue * 0.1f; // 10% scale variation

            const auto currentBounds = component.getBounds();
            // const auto center = currentBounds.getCentre();
            component.setBounds(currentBounds.withSizeKeepingCentre(
                static_cast<int>(currentBounds.getWidth() * scale),
                static_cast<int>(currentBounds.getHeight() * scale)));

            const auto cycles = static_cast<int>(elapsed / cycleTime);
            if (cycles >= repeats)
            {
                stopTimer();
                // Reset to original size
                component.setBounds(currentBounds);
                delete this;
            }
        }

    private:
        juce::Component& component;
        int durationMs;
        int repeats;
        double startTime;
    };

    new PulseAnimator(component, durationMs, repeatCount);
}

} // namespace daw::ui::utils

