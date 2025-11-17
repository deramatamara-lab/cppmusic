#include "AutomationCurve.h"
#include <algorithm>
#include <cmath>

namespace daw::project
{

void AutomationCurve::addPoint(double timeBeats, float value) noexcept
{
    Point newPoint;
    newPoint.timeBeats = timeBeats;
    newPoint.value = value;
    
    // Insert in sorted order
    auto it = std::lower_bound(points.begin(), points.end(), newPoint,
        [](const Point& a, const Point& b) { return a.timeBeats < b.timeBeats; });
    
    points.insert(it, newPoint);
}

void AutomationCurve::removePoint(size_t index) noexcept
{
    if (index < points.size())
    {
        points.erase(points.begin() + static_cast<ptrdiff_t>(index));
    }
}

void AutomationCurve::clear() noexcept
{
    points.clear();
}

void AutomationCurve::renderToBuffer(double sampleRate, double tempoBpm, double startBeat, int numSamples, float* output) const noexcept
{
    if (points.empty())
    {
        std::fill(output, output + numSamples, 0.0f);
        return;
    }
    
    const auto samplesPerBeat = sampleRate * 60.0 / tempoBpm;
    
    for (int i = 0; i < numSamples; ++i)
    {
        const auto sampleBeat = startBeat + (i / samplesPerBeat);
        output[i] = getValueAtBeat(sampleBeat);
    }
}

float AutomationCurve::getValueAtBeat(double timeBeats) const noexcept
{
    if (points.empty())
        return 0.0f;
    
    if (points.size() == 1)
        return points[0].value;
    
    // Find surrounding points
    const auto index = findPointIndex(timeBeats);
    
    if (index == 0)
    {
        // Before first point - return first value
        return points[0].value;
    }
    
    if (index >= points.size())
    {
        // After last point - return last value
        return points.back().value;
    }
    
    // Linear interpolation between two points
    const auto& p0 = points[index - 1];
    const auto& p1 = points[index];
    
    if (p1.timeBeats == p0.timeBeats)
        return p0.value;
    
    const auto t = (timeBeats - p0.timeBeats) / (p1.timeBeats - p0.timeBeats);
    return p0.value + (p1.value - p0.value) * static_cast<float>(t);
}

float AutomationCurve::interpolateCubic(const Point& p0, const Point& p1, const Point& p2, const Point& p3, double t) const noexcept
{
    // Cubic Hermite interpolation
    const auto t2 = t * t;
    const auto t3 = t2 * t;
    
    const auto h1 = 2.0 * t3 - 3.0 * t2 + 1.0;
    const auto h2 = -2.0 * t3 + 3.0 * t2;
    const auto h3 = t3 - 2.0 * t2 + t;
    const auto h4 = t3 - t2;
    
    return static_cast<float>(h1 * p1.value + h2 * p2.value + h3 * (p2.value - p0.value) + h4 * (p3.value - p1.value));
}

size_t AutomationCurve::findPointIndex(double timeBeats) const noexcept
{
    auto it = std::lower_bound(points.begin(), points.end(), timeBeats,
        [](const Point& p, double beat) { return p.timeBeats < beat; });
    
    return static_cast<size_t>(std::distance(points.begin(), it));
}

} // namespace daw::project

