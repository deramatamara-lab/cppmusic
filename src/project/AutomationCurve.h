#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace daw::project
{

/**
 * @brief Pre-rendered automation curve
 * 
 * Stores automation data as pre-rendered samples for efficient playback.
 * Supports cubic interpolation for smooth curves.
 * Follows DAW_DEV_RULES: real-time safe access.
 */
class AutomationCurve
{
public:
    struct Point
    {
        double timeBeats;
        float value;
    };

    AutomationCurve() noexcept = default;
    ~AutomationCurve() = default;

    // Non-copyable, movable
    AutomationCurve(const AutomationCurve&) = delete;
    AutomationCurve& operator=(const AutomationCurve&) = delete;
    AutomationCurve(AutomationCurve&&) noexcept = default;
    AutomationCurve& operator=(AutomationCurve&&) noexcept = default;

    /**
     * @brief Add automation point
     */
    void addPoint(double timeBeats, float value) noexcept;

    /**
     * @brief Remove point at index
     */
    void removePoint(size_t index) noexcept;

    /**
     * @brief Clear all points
     */
    void clear() noexcept;

    /**
     * @brief Render curve to sample buffer
     * @param sampleRate Sample rate
     * @param tempoBpm Tempo in BPM
     * @param startBeat Start beat position
     * @param numSamples Number of samples to render
     * @param output Output buffer (must be pre-allocated)
     */
    void renderToBuffer(double sampleRate, double tempoBpm, double startBeat, int numSamples, float* output) const noexcept;

    /**
     * @brief Get value at specific beat position (with interpolation)
     */
    [[nodiscard]] float getValueAtBeat(double timeBeats) const noexcept;

    /**
     * @brief Get number of points
     */
    [[nodiscard]] size_t getNumPoints() const noexcept { return points.size(); }

    /**
     * @brief Get point at index
     */
    [[nodiscard]] const Point& getPoint(size_t index) const noexcept { return points[index]; }

private:
    std::vector<Point> points;
    
    [[nodiscard]] float interpolateCubic(const Point& p0, const Point& p1, const Point& p2, const Point& p3, double t) const noexcept;
    [[nodiscard]] size_t findPointIndex(double timeBeats) const noexcept;
};

} // namespace daw::project

