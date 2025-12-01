#pragma once
/**
 * @file ParamRegistry.hpp
 * @brief Central registry for all parameters in the system.
 */

#include "ParamSignal.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cppmusic::engine::parameters {

/**
 * @brief Central registry managing all parameters in the system.
 * 
 * Provides:
 * - Unique ID assignment for parameters
 * - Parameter lookup by ID
 * - Dependency tracking for cycle detection
 * - Iteration over all parameters
 */
class ParamRegistry {
public:
    ParamRegistry();
    ~ParamRegistry();
    
    // Non-copyable, non-movable
    ParamRegistry(const ParamRegistry&) = delete;
    ParamRegistry& operator=(const ParamRegistry&) = delete;
    ParamRegistry(ParamRegistry&&) = delete;
    ParamRegistry& operator=(ParamRegistry&&) = delete;
    
    // =========================================================================
    // Parameter Management
    // =========================================================================
    
    /**
     * @brief Register a new parameter.
     * @param spec The parameter specification.
     * @return The unique ID assigned to the parameter, or InvalidParamId on failure.
     */
    ParamId registerParam(const ParamSpec& spec);
    
    /**
     * @brief Unregister and remove a parameter.
     * @param id The parameter ID to remove.
     * @return true if the parameter was removed, false if not found.
     */
    bool unregisterParam(ParamId id);
    
    /**
     * @brief Get a parameter by ID.
     * @param id The parameter ID.
     * @return Pointer to the parameter, or nullptr if not found.
     */
    [[nodiscard]] ParamSignal* getParam(ParamId id);
    [[nodiscard]] const ParamSignal* getParam(ParamId id) const;
    
    /**
     * @brief Get the number of registered parameters.
     */
    [[nodiscard]] std::size_t getParamCount() const noexcept;
    
    /**
     * @brief Iterate over all parameters.
     */
    void forEachParam(const std::function<void(ParamSignal&)>& fn);
    void forEachParam(const std::function<void(const ParamSignal&)>& fn) const;
    
    // =========================================================================
    // Dependency Management (for modulation routing)
    // =========================================================================
    
    /**
     * @brief Add a dependency from source to target.
     * @param source The source parameter ID.
     * @param target The target parameter ID.
     * @return true if the dependency was added, false if it would create a cycle.
     */
    bool addDependency(ParamId source, ParamId target);
    
    /**
     * @brief Remove a dependency.
     * @param source The source parameter ID.
     * @param target The target parameter ID.
     * @return true if the dependency was removed.
     */
    bool removeDependency(ParamId source, ParamId target);
    
    /**
     * @brief Check if adding a dependency would create a cycle.
     * @param source The source parameter ID.
     * @param target The target parameter ID.
     * @return true if adding this dependency would create a cycle.
     */
    [[nodiscard]] bool wouldCreateCycle(ParamId source, ParamId target) const;
    
    /**
     * @brief Check if the dependency graph currently has any cycles.
     * @return true if there are cycles in the graph.
     */
    [[nodiscard]] bool hasCycle() const;
    
    /**
     * @brief Get parameters in topological order (dependencies first).
     * @return Vector of parameter IDs in processing order, empty if there's a cycle.
     */
    [[nodiscard]] std::vector<ParamId> getTopologicalOrder() const;
    
private:
    /**
     * @brief Check for cycle using DFS from a given node.
     */
    bool hasCycleFromNode(ParamId start, 
                          std::unordered_set<ParamId>& visited,
                          std::unordered_set<ParamId>& recursionStack) const;
    
    std::unordered_map<ParamId, std::unique_ptr<ParamSignal>> params_;
    std::unordered_map<ParamId, std::unordered_set<ParamId>> dependencies_;
    ParamId nextId_ = 1;
};

} // namespace cppmusic::engine::parameters
