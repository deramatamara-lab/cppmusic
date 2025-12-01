/**
 * @file ModMatrix.cpp
 * @brief Implementation of the modulation matrix.
 */

#include "ModMatrix.hpp"
#include "ParamRegistry.hpp"

namespace cppmusic::engine::parameters {

float ModSource::getValue(const ParamRegistry* registry) const {
    switch (type) {
        case Type::Parameter:
            if (registry) {
                if (const auto* param = registry->getParam(paramId)) {
                    return param->getValueNormalized();
                }
            }
            return 0.0f;
            
        case Type::LFO:
        case Type::Envelope:
        case Type::External:
            // Placeholder: return 0.5 (center) for non-parameter sources
            // Real implementation would query the appropriate signal generator
            return 0.5f;
    }
    return 0.0f;
}

ModMatrix::ModMatrix(ParamRegistry* registry)
    : registry_(registry) {
}

ModMatrix::~ModMatrix() = default;

ModSlotId ModMatrix::connect(const ModSource& source, ParamId target,
                              float amount, BlendMode mode) {
    // Verify target exists
    if (!registry_ || !registry_->getParam(target)) {
        return InvalidModSlotId;
    }
    
    ModSlotId id = nextSlotId_++;
    
    ModSlot slot;
    slot.id = id;
    slot.source = source;
    slot.target = target;
    slot.amount = amount;
    slot.blendMode = mode;
    slot.enabled = true;
    
    slots_[id] = slot;
    
    // If source is a parameter, add dependency in registry
    if (source.type == ModSource::Type::Parameter && registry_) {
        registry_->addDependency(source.paramId, target);
    }
    
    return id;
}

bool ModMatrix::disconnect(ModSlotId slot) {
    auto it = slots_.find(slot);
    if (it == slots_.end()) {
        return false;
    }
    
    // Remove dependency if it was a parameter source
    const ModSlot& modSlot = it->second;
    if (modSlot.source.type == ModSource::Type::Parameter && registry_) {
        registry_->removeDependency(modSlot.source.paramId, modSlot.target);
    }
    
    slots_.erase(it);
    return true;
}

void ModMatrix::setAmount(ModSlotId slot, float amount) {
    auto it = slots_.find(slot);
    if (it != slots_.end()) {
        it->second.amount = amount;
    }
}

void ModMatrix::setBlendMode(ModSlotId slot, BlendMode mode) {
    auto it = slots_.find(slot);
    if (it != slots_.end()) {
        it->second.blendMode = mode;
    }
}

void ModMatrix::setEnabled(ModSlotId slot, bool enabled) {
    auto it = slots_.find(slot);
    if (it != slots_.end()) {
        it->second.enabled = enabled;
    }
}

const ModSlot* ModMatrix::getSlot(ModSlotId slot) const {
    auto it = slots_.find(slot);
    if (it != slots_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<const ModSlot*> ModMatrix::getSlotsForTarget(ParamId target) const {
    std::vector<const ModSlot*> result;
    for (const auto& [id, slot] : slots_) {
        if (slot.target == target) {
            result.push_back(&slot);
        }
    }
    return result;
}

std::size_t ModMatrix::getSlotCount() const noexcept {
    return slots_.size();
}

void ModMatrix::clearModulations() noexcept {
    if (!registry_) return;
    
    registry_->forEachParam([](ParamSignal& param) {
        param.setModulationAmount(0.0f);
    });
}

void ModMatrix::applyModulation(const ModSlot& slot,
                                 std::unordered_map<ParamId, float>& modAmounts) noexcept {
    if (!slot.enabled) return;
    
    float sourceValue = slot.source.getValue(registry_);
    float modValue = 0.0f;
    
    switch (slot.blendMode) {
        case BlendMode::Add:
            modValue = sourceValue * slot.amount;
            break;
            
        case BlendMode::Multiply:
            // For multiply, we accumulate a multiplier offset
            // The actual multiplication happens when applied
            modValue = sourceValue * slot.amount;
            break;
            
        case BlendMode::Replace:
            modValue = sourceValue * slot.amount;
            break;
            
        case BlendMode::Bipolar:
            // Convert 0-1 to -1 to +1
            modValue = (sourceValue - 0.5f) * 2.0f * slot.amount;
            break;
    }
    
    // Accumulate modulation for this target
    modAmounts[slot.target] += modValue;
}

void ModMatrix::process() noexcept {
    if (!registry_) return;
    
    // Clear previous modulations
    clearModulations();
    
    // Accumulate modulations per target
    std::unordered_map<ParamId, float> modAmounts;
    
    // Get processing order to ensure dependencies are processed first
    auto order = registry_->getTopologicalOrder();
    
    // Apply modulations in topological order
    for (const auto& [slotId, slot] : slots_) {
        applyModulation(slot, modAmounts);
    }
    
    // Apply accumulated modulations to parameters
    for (const auto& [paramId, modAmount] : modAmounts) {
        if (auto* param = registry_->getParam(paramId)) {
            param->setModulationAmount(modAmount);
        }
    }
}

} // namespace cppmusic::engine::parameters
