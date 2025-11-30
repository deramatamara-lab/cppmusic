#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <typeindex>
#include <type_traits>
#include <mutex>
#include <functional>
#include <optional>

namespace daw::core {

/**
 * @brief Service registry with dependency injection support
 *
 * Provides centralized service management with:
 * - Runtime service registration and resolution
 * - Mock injection for testing
 * - Thread-safe access
 * - Service lifecycle management
 * - Interface-based service discovery
 *
 * Follows DAW_DEV_RULES: no allocations in hot paths, thread-safe, testable.
 */
class ServiceRegistry
{
public:
    ServiceRegistry() = default;
    ~ServiceRegistry();

    // Non-copyable, movable
    ServiceRegistry(const ServiceRegistry&) = delete;
    ServiceRegistry& operator=(const ServiceRegistry&) = delete;
    ServiceRegistry(ServiceRegistry&&) noexcept = default;
    ServiceRegistry& operator=(ServiceRegistry&&) noexcept = default;

    /**
     * @brief Register a service instance
     * @tparam ServiceType The service interface/class type
     * @param instance Unique pointer to service instance
     * @param serviceName Optional human-readable name for debugging
     */
    template<typename ServiceType>
    void registerService(std::unique_ptr<ServiceType> instance, std::string serviceName = "")
    {
        static_assert(std::is_polymorphic_v<ServiceType>, "ServiceType must be polymorphic");

        std::lock_guard<std::mutex> lock(mutex_);

        const auto typeIndex = std::type_index(typeid(ServiceType));
        services_[typeIndex] = ServiceEntry{
            std::move(instance),
            typeIndex,
            std::move(serviceName)
        };
    }

    /**
     * @brief Register a service factory function
     * @tparam ServiceType The service interface/class type
     * @param factory Function that creates service instances
     * @param serviceName Optional human-readable name for debugging
     */
    template<typename ServiceType>
    void registerServiceFactory(std::function<std::unique_ptr<ServiceType>()> factory,
                               std::string serviceName = "")
    {
        static_assert(std::is_polymorphic_v<ServiceType>, "ServiceType must be polymorphic");

        std::lock_guard<std::mutex> lock(mutex_);

        const auto typeIndex = std::type_index(typeid(ServiceType));
        factories_[typeIndex] = FactoryEntry{
            [factory = std::move(factory)]() -> std::unique_ptr<void> {
                return factory();
            },
            typeIndex,
            std::move(serviceName)
        };
    }

    /**
     * @brief Resolve a service instance
     * @tparam ServiceType The service interface/class type
     * @return Pointer to service instance, or nullptr if not registered
     */
    template<typename ServiceType>
    ServiceType* resolve()
    {
        static_assert(std::is_polymorphic_v<ServiceType>, "ServiceType must be polymorphic");

        std::lock_guard<std::mutex> lock(mutex_);

        const auto typeIndex = std::type_index(typeid(ServiceType));

        // Check for registered instance first
        if (auto it = services_.find(typeIndex); it != services_.end())
        {
            return static_cast<ServiceType*>(it->second.instance.get());
        }

        // Check for factory and create instance
        if (auto it = factories_.find(typeIndex); it != factories_.end())
        {
            auto instance = it->second.factory();
            auto* rawPtr = static_cast<ServiceType*>(instance.get());
            services_[typeIndex] = ServiceEntry{
                std::move(instance),
                typeIndex,
                it->second.serviceName
            };
            return rawPtr;
        }

        return nullptr;
    }

    /**
     * @brief Check if a service is registered
     * @tparam ServiceType The service interface/class type
     * @return true if service is available
     */
    template<typename ServiceType>
    bool hasService() const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        const auto typeIndex = std::type_index(typeid(ServiceType));
        return services_.count(typeIndex) > 0 || factories_.count(typeIndex) > 0;
    }

    /**
     * @brief Unregister a service
     * @tparam ServiceType The service interface/class type
     */
    template<typename ServiceType>
    void unregisterService()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        const auto typeIndex = std::type_index(typeid(ServiceType));
        services_.erase(typeIndex);
        factories_.erase(typeIndex);
    }

    /**
     * @brief Clear all services
     */
    void clearAllServices()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        services_.clear();
        factories_.clear();
    }

    /**
     * @brief Get service information for debugging
     * @return Vector of service info strings
     */
    std::vector<std::string> getServiceInfo() const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<std::string> info;
        info.reserve(services_.size() + factories_.size());

        for (const auto& [typeIndex, entry] : services_)
        {
            info.push_back("Service: " + entry.serviceName +
                          " (" + typeIndex.name() + ") [instance]");
        }

        for (const auto& [typeIndex, entry] : factories_)
        {
            info.push_back("Service: " + entry.serviceName +
                          " (" + typeIndex.name() + ") [factory]");
        }

        return info;
    }

    /**
     * @brief Get singleton instance (Meyers singleton)
     */
    static ServiceRegistry& getInstance()
    {
        static ServiceRegistry instance;
        return instance;
    }

private:
    struct ServiceEntry
    {
        std::unique_ptr<void> instance;
        std::type_index typeIndex;
        std::string serviceName;
    };

    struct FactoryEntry
    {
        std::function<std::unique_ptr<void>()> factory;
        std::type_index typeIndex;
        std::string serviceName;
    };

    mutable std::mutex mutex_;
    std::unordered_map<std::type_index, ServiceEntry> services_;
    std::unordered_map<std::type_index, FactoryEntry> factories_;
};

/**
 * @brief Service locator helper class
 *
 * Provides convenient access to the global service registry.
 * Used throughout the codebase for dependency resolution.
 */
class ServiceLocator
{
public:
    /**
     * @brief Get a service instance
     * @tparam ServiceType The service interface/class type
     * @return Pointer to service instance, or nullptr if not available
     */
    template<typename ServiceType>
    static ServiceType* get()
    {
        return ServiceRegistry::getInstance().resolve<ServiceType>();
    }

    /**
     * @brief Check if a service is available
     * @tparam ServiceType The service interface/class type
     * @return true if service is available
     */
    template<typename ServiceType>
    static bool has()
    {
        return ServiceRegistry::getInstance().hasService<ServiceType>();
    }

    /**
     * @brief Register a service instance
     * @tparam ServiceType The service interface/class type
     * @param instance Unique pointer to service instance
     * @param serviceName Optional human-readable name for debugging
     */
    template<typename ServiceType>
    static void registerService(std::unique_ptr<ServiceType> instance,
                               std::string serviceName = "")
    {
        ServiceRegistry::getInstance().registerService(std::move(instance),
                                                      std::move(serviceName));
    }

    /**
     * @brief Register a service factory
     * @tparam ServiceType The service interface/class type
     * @param factory Function that creates service instances
     * @param serviceName Optional human-readable name for debugging
     */
    template<typename ServiceType>
    static void registerServiceFactory(std::function<std::unique_ptr<ServiceType>()> factory,
                                      std::string serviceName = "")
    {
        ServiceRegistry::getInstance().registerServiceFactory(std::move(factory),
                                                             std::move(serviceName));
    }

    /**
     * @brief Require a service (throws if not available)
     * @tparam ServiceType The service interface/class type
     * @return Reference to service instance
     * @throws std::runtime_error if service not available
     */
    template<typename ServiceType>
    static ServiceType& require()
    {
        if (auto* service = get<ServiceType>(); service != nullptr)
        {
            return *service;
        }

        throw std::runtime_error("Required service not available: " +
                                std::string(typeid(ServiceType).name()));
    }

    /**
     * @brief Clear all services (primarily for testing)
     */
    static void clearAllServices()
    {
        ServiceRegistry::getInstance().clearAllServices();
    }

    /**
     * @brief Get service information for debugging
     */
    static std::vector<std::string> getServiceInfo()
    {
        return ServiceRegistry::getInstance().getServiceInfo();
    }

private:
    ServiceLocator() = delete;
    ~ServiceLocator() = delete;
};

} // namespace daw::core
