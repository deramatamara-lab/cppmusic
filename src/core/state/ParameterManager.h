#pragma once

#include <atomic>
#include <unordered_map>
#include <string>

namespace daw::core::state
{

/**
 * @brief Parameter management system
 * 
 * Thread-safe parameter storage using atomics for audio thread access.
 * Lock-free reads from audio thread, safe writes from UI thread.
 */
class ParameterManager
{
public:
    ParameterManager();
    ~ParameterManager() = default;

    /**
     * @brief Set parameter value (UI thread)
     * @param id Parameter ID
     * @param value Parameter value
     */
    void setParameter(int id, float value) noexcept;

    /**
     * @brief Get parameter value (audio thread safe)
     * @param id Parameter ID
     * @return Parameter value
     */
    [[nodiscard]] float getParameter(int id) const noexcept;

private:
    std::unordered_map<int, std::atomic<float>> parameters;
};

} // namespace daw::core::state

