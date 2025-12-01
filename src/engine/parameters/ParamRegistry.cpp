/**
 * @file ParamRegistry.cpp
 * @brief Implementation of the parameter registry.
 */

#include "ParamRegistry.hpp"
#include <algorithm>
#include <queue>

namespace cppmusic::engine::parameters {

ParamRegistry::ParamRegistry() = default;
ParamRegistry::~ParamRegistry() = default;

ParamId ParamRegistry::registerParam(const ParamSpec& spec) {
    ParamId id = nextId_++;
    
    auto param = std::make_unique<ParamSignal>(id, spec);
    params_[id] = std::move(param);
    dependencies_[id] = {};
    
    return id;
}

bool ParamRegistry::unregisterParam(ParamId id) {
    auto it = params_.find(id);
    if (it == params_.end()) {
        return false;
    }
    
    // Remove all dependencies involving this parameter
    dependencies_.erase(id);
    for (auto& [paramId, deps] : dependencies_) {
        deps.erase(id);
    }
    
    params_.erase(it);
    return true;
}

ParamSignal* ParamRegistry::getParam(ParamId id) {
    auto it = params_.find(id);
    if (it != params_.end()) {
        return it->second.get();
    }
    return nullptr;
}

const ParamSignal* ParamRegistry::getParam(ParamId id) const {
    auto it = params_.find(id);
    if (it != params_.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::size_t ParamRegistry::getParamCount() const noexcept {
    return params_.size();
}

void ParamRegistry::forEachParam(const std::function<void(ParamSignal&)>& fn) {
    for (auto& [id, param] : params_) {
        fn(*param);
    }
}

void ParamRegistry::forEachParam(const std::function<void(const ParamSignal&)>& fn) const {
    for (const auto& [id, param] : params_) {
        fn(*param);
    }
}

bool ParamRegistry::addDependency(ParamId source, ParamId target) {
    // Check both parameters exist
    if (params_.find(source) == params_.end() ||
        params_.find(target) == params_.end()) {
        return false;
    }
    
    // Check if this would create a cycle
    if (wouldCreateCycle(source, target)) {
        return false;
    }
    
    dependencies_[source].insert(target);
    return true;
}

bool ParamRegistry::removeDependency(ParamId source, ParamId target) {
    auto it = dependencies_.find(source);
    if (it == dependencies_.end()) {
        return false;
    }
    
    return it->second.erase(target) > 0;
}

bool ParamRegistry::wouldCreateCycle(ParamId source, ParamId target) const {
    // Adding source -> target would create a cycle if there's already
    // a path from target to source
    
    std::unordered_set<ParamId> visited;
    std::queue<ParamId> toVisit;
    toVisit.push(target);
    
    while (!toVisit.empty()) {
        ParamId current = toVisit.front();
        toVisit.pop();
        
        if (current == source) {
            return true;  // Found path from target to source
        }
        
        if (visited.count(current) > 0) {
            continue;
        }
        visited.insert(current);
        
        auto it = dependencies_.find(current);
        if (it != dependencies_.end()) {
            for (ParamId dep : it->second) {
                toVisit.push(dep);
            }
        }
    }
    
    return false;
}

bool ParamRegistry::hasCycleFromNode(ParamId start,
                                      std::unordered_set<ParamId>& visited,
                                      std::unordered_set<ParamId>& recursionStack) const {
    visited.insert(start);
    recursionStack.insert(start);
    
    auto it = dependencies_.find(start);
    if (it != dependencies_.end()) {
        for (ParamId neighbor : it->second) {
            if (visited.find(neighbor) == visited.end()) {
                if (hasCycleFromNode(neighbor, visited, recursionStack)) {
                    return true;
                }
            } else if (recursionStack.count(neighbor) > 0) {
                return true;  // Back edge found
            }
        }
    }
    
    recursionStack.erase(start);
    return false;
}

bool ParamRegistry::hasCycle() const {
    std::unordered_set<ParamId> visited;
    std::unordered_set<ParamId> recursionStack;
    
    for (const auto& [id, param] : params_) {
        if (visited.find(id) == visited.end()) {
            if (hasCycleFromNode(id, visited, recursionStack)) {
                return true;
            }
        }
    }
    
    return false;
}

std::vector<ParamId> ParamRegistry::getTopologicalOrder() const {
    std::vector<ParamId> result;
    
    if (params_.empty()) {
        return result;
    }
    
    // Build in-degree map
    std::unordered_map<ParamId, std::size_t> inDegree;
    for (const auto& [id, param] : params_) {
        inDegree[id] = 0;
    }
    
    for (const auto& [source, targets] : dependencies_) {
        for (ParamId target : targets) {
            ++inDegree[target];
        }
    }
    
    // Start with nodes that have no incoming edges
    std::queue<ParamId> queue;
    for (const auto& [id, degree] : inDegree) {
        if (degree == 0) {
            queue.push(id);
        }
    }
    
    // Process nodes
    while (!queue.empty()) {
        ParamId current = queue.front();
        queue.pop();
        result.push_back(current);
        
        auto it = dependencies_.find(current);
        if (it != dependencies_.end()) {
            for (ParamId neighbor : it->second) {
                --inDegree[neighbor];
                if (inDegree[neighbor] == 0) {
                    queue.push(neighbor);
                }
            }
        }
    }
    
    // If we didn't process all nodes, there's a cycle
    if (result.size() != params_.size()) {
        return {};  // Empty indicates cycle
    }
    
    return result;
}

} // namespace cppmusic::engine::parameters
