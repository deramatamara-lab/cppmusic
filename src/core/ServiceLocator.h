#pragma once

#include <memory>
#include <unordered_map>
#include <typeindex>
#include <functional>
#include <mutex>

namespace daw::core {

/**
 * Lightweight service locator for dependency injection and feature flag management.
 *
 * This provides a centralized registry for services to avoid ad-hoc instantiation
 * and enable controlled feature toggles (AI, GPU acceleration, etc.).
 *
 * Key principles:
 * - Services are registered once at startup
 * - Thread-safe service lookup with minimal overhead
 * - Support for feature flags and conditional service registration
 * - Interface-based design for easy mocking in tests
 */
class ServiceLocator
{
public:
    /**
     * Feature flags for optional systems
     */
    struct FeatureFlags
    {
        bool aiEnabled = true;
        bool gpuAcceleration = true;
        bool adaptiveAnimation = true;
        bool advancedDSP = true;
        bool performanceMonitoring = true;
        bool pluginSandboxing = false; // Default off until implemented
        bool cloudSync = false;        // Default off until implemented

        // Debug/development flags
        bool verboseLogging = false;
        bool mockServices = false;     // Use mocks instead of real services
        bool benchmarkMode = false;    // Enable continuous benchmarking
    };

    /**
     * Service registration interface
     */
    template<typename ServiceType>
    void registerService(std::shared_ptr<ServiceType> service)
    {
        std::lock_guard<std::mutex> lock(servicesMutex);
        const auto typeIndex = std::type_index(typeid(ServiceType));
        services[typeIndex] = service;
    }

    /**
     * Service lookup with type safety
     */
    template<typename ServiceType>
    [[nodiscard]] std::shared_ptr<ServiceType> getService() const
    {
        std::lock_guard<std::mutex> lock(servicesMutex);
        const auto typeIndex = std::type_index(typeid(ServiceType));

        auto it = services.find(typeIndex);
        if (it != services.end())
        {
            return std::static_pointer_cast<ServiceType>(it->second);
        }

        return nullptr;
    }

    /**
     * Check if a service is registered
     */
    template<typename ServiceType>
    [[nodiscard]] bool hasService() const
    {
        return getService<ServiceType>() != nullptr;
    }

    /**
     * Unregister a service (primarily for testing)
     */
    template<typename ServiceType>
    void unregisterService()
    {
        std::lock_guard<std::mutex> lock(servicesMutex);
        const auto typeIndex = std::type_index(typeid(ServiceType));
        services.erase(typeIndex);
    }

    /**
     * Clear all services (shutdown)
     */
    void clearAllServices()
    {
        std::lock_guard<std::mutex> lock(servicesMutex);
        services.clear();
    }

    /**
     * Get feature flags (thread-safe)
     */
    const FeatureFlags& getFeatureFlags() const
    {
        std::lock_guard<std::mutex> lock(flagsMutex);
        return featureFlags;
    }

    /**
     * Update feature flags (thread-safe)
     */
    void setFeatureFlags(const FeatureFlags& flags)
    {
        std::lock_guard<std::mutex> lock(flagsMutex);
        featureFlags = flags;
    }

    /**
     * Update a single feature flag
     */
    void setFeatureFlag(const std::string& flagName, bool enabled)
    {
        std::lock_guard<std::mutex> lock(flagsMutex);

        if (flagName == "ai") featureFlags.aiEnabled = enabled;
        else if (flagName == "gpu") featureFlags.gpuAcceleration = enabled;
        else if (flagName == "animation") featureFlags.adaptiveAnimation = enabled;
        else if (flagName == "advanced_dsp") featureFlags.advancedDSP = enabled;
        else if (flagName == "performance_monitoring") featureFlags.performanceMonitoring = enabled;
        else if (flagName == "plugin_sandboxing") featureFlags.pluginSandboxing = enabled;
        else if (flagName == "cloud_sync") featureFlags.cloudSync = enabled;
        else if (flagName == "verbose_logging") featureFlags.verboseLogging = enabled;
        else if (flagName == "mock_services") featureFlags.mockServices = enabled;
        else if (flagName == "benchmark_mode") featureFlags.benchmarkMode = enabled;
    }

    /**
     * Get a single feature flag value
     */
    [[nodiscard]] bool getFeatureFlag(const std::string& flagName) const
    {
        std::lock_guard<std::mutex> lock(flagsMutex);

        if (flagName == "ai") return featureFlags.aiEnabled;
        else if (flagName == "gpu") return featureFlags.gpuAcceleration;
        else if (flagName == "animation") return featureFlags.adaptiveAnimation;
        else if (flagName == "advanced_dsp") return featureFlags.advancedDSP;
        else if (flagName == "performance_monitoring") return featureFlags.performanceMonitoring;
        else if (flagName == "plugin_sandboxing") return featureFlags.pluginSandboxing;
        else if (flagName == "cloud_sync") return featureFlags.cloudSync;
        else if (flagName == "verbose_logging") return featureFlags.verboseLogging;
        else if (flagName == "mock_services") return featureFlags.mockServices;
        else if (flagName == "benchmark_mode") return featureFlags.benchmarkMode;

        return false;
    }

    /**
     * Singleton access (thread-safe)
     */
    static ServiceLocator& getInstance()
    {
        static ServiceLocator instance;
        return instance;
    }

    /**
     * Initialize services based on feature flags
     * This is called once at application startup
     */
    void initializeServices();

    /**
     * Shutdown all services
     * This is called once at application shutdown
     */
    void shutdownServices();

    /**
     * Get service registration statistics
     */
    struct ServiceStats
    {
        size_t registeredServices;
        size_t enabledFeatures;
        bool initialized;
    };

    [[nodiscard]] ServiceStats getServiceStats() const
    {
        std::lock_guard<std::mutex> lock(servicesMutex);

        size_t enabledFeatures = 0;
        {
            std::lock_guard<std::mutex> flagsLock(flagsMutex);
            if (featureFlags.aiEnabled) ++enabledFeatures;
            if (featureFlags.gpuAcceleration) ++enabledFeatures;
            if (featureFlags.adaptiveAnimation) ++enabledFeatures;
            if (featureFlags.advancedDSP) ++enabledFeatures;
            if (featureFlags.performanceMonitoring) ++enabledFeatures;
            if (featureFlags.pluginSandboxing) ++enabledFeatures;
            if (featureFlags.cloudSync) ++enabledFeatures;
        }

        return ServiceStats{
            services.size(),
            enabledFeatures,
            initialized
        };
    }

private:
    mutable std::mutex servicesMutex;
    mutable std::mutex flagsMutex;

    std::unordered_map<std::type_index, std::shared_ptr<void>> services;
    FeatureFlags featureFlags;

    bool initialized = false;

    // Private constructor for singleton
    ServiceLocator() = default;
    ~ServiceLocator() = default;

    // Non-copyable
    ServiceLocator(const ServiceLocator&) = delete;
    ServiceLocator& operator=(const ServiceLocator&) = delete;
};

/**
 * Convenience macros for service access
 */
#define GET_SERVICE(ServiceType) \
    daw::core::ServiceLocator::getInstance().getService<ServiceType>()

#define HAS_SERVICE(ServiceType) \
    daw::core::ServiceLocator::getInstance().hasService<ServiceType>()

#define REGISTER_SERVICE(ServiceType, instance) \
    daw::core::ServiceLocator::getInstance().registerService<ServiceType>(instance)

#define GET_FEATURE_FLAG(flagName) \
    daw::core::ServiceLocator::getInstance().getFeatureFlag(flagName)

#define SET_FEATURE_FLAG(flagName, enabled) \
    daw::core::ServiceLocator::getInstance().setFeatureFlag(flagName, enabled)

} // namespace daw::core
