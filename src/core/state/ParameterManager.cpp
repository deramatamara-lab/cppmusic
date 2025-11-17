#include "ParameterManager.h"

namespace daw::core::state
{

ParameterManager::ParameterManager()
{
}

void ParameterManager::setParameter(int id, float value) noexcept
{
    parameters[id].store(value, std::memory_order_release);
}

float ParameterManager::getParameter(int id) const noexcept
{
    auto it = parameters.find(id);
    if (it != parameters.end())
    {
        return it->second.load(std::memory_order_acquire);
    }
    return 0.0f;
}

} // namespace daw::core::state

