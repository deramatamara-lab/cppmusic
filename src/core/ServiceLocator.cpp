#include "ServiceLocator.h"

namespace daw::core {

void ServiceLocator::initializeServices()
{
    if (initialized)
        return;

    // Service initialization is handled by the application startup code
    // This method serves as a hook for any cross-service initialization

    initialized = true;
}

void ServiceLocator::shutdownServices()
{
    if (!initialized)
        return;

    // Clear all services (their destructors will handle cleanup)
    clearAllServices();

    initialized = false;
}

} // namespace daw::core
