#pragma once

#include <cmath>
#include <algorithm>

namespace daw::core::utilities
{

/**
 * @brief Math utility functions
 * 
 * Real-time safe math operations for audio processing.
 */
class MathUtils
{
public:
    /**
     * @brief Convert linear gain to decibels
     * @param linear Linear gain value
     * @return Gain in decibels
     */
    [[nodiscard]] static float linearToDb(float linear) noexcept;

    /**
     * @brief Convert decibels to linear gain
     * @param db Gain in decibels
     * @return Linear gain value
     */
    [[nodiscard]] static float dbToLinear(float db) noexcept;

    /**
     * @brief Clamp value to range
     * @param value Value to clamp
     * @param min Minimum value
     * @param max Maximum value
     * @return Clamped value
     */
    template<typename T>
    [[nodiscard]] static constexpr T clamp(T value, T min, T max) noexcept
    {
        return (value < min) ? min : ((value > max) ? max : value);
    }
};

inline float MathUtils::linearToDb(float linear) noexcept
{
    return 20.0f * std::log10(std::max(linear, 1e-10f));
}

inline float MathUtils::dbToLinear(float db) noexcept
{
    return std::pow(10.0f, db / 20.0f);
}

} // namespace daw::core::utilities

