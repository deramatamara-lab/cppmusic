/**
 * @file TestParamGraph.cpp
 * @brief Unit tests for parameter graph and cycle detection.
 */

#include "engine/parameters/ParamRegistry.hpp"
#include "engine/parameters/ModMatrix.hpp"
#include <cassert>
#include <iostream>

using namespace cppmusic::engine::parameters;

namespace {

void testRegisterParams() {
    std::cout << "  Testing parameter registration..." << std::endl;
    
    ParamRegistry registry;
    
    ParamSpec spec1{"Volume", 0.0f, 1.0f, 0.75f};
    ParamSpec spec2{"Pan", -1.0f, 1.0f, 0.0f};
    
    ParamId id1 = registry.registerParam(spec1);
    ParamId id2 = registry.registerParam(spec2);
    
    assert(id1 != InvalidParamId);
    assert(id2 != InvalidParamId);
    assert(id1 != id2);
    assert(registry.getParamCount() == 2);
    
    auto* param1 = registry.getParam(id1);
    auto* param2 = registry.getParam(id2);
    
    assert(param1 != nullptr);
    assert(param2 != nullptr);
    assert(param1->getName() == "Volume");
    assert(param2->getName() == "Pan");
    assert(param1->getDefaultValue() == 0.75f);
    assert(param2->getDefaultValue() == 0.0f);
    
    std::cout << "  Parameter registration passed." << std::endl;
}

void testParamValues() {
    std::cout << "  Testing parameter value access..." << std::endl;
    
    ParamRegistry registry;
    ParamSpec spec{"Cutoff", 20.0f, 20000.0f, 1000.0f};
    ParamId id = registry.registerParam(spec);
    
    auto* param = registry.getParam(id);
    assert(param != nullptr);
    
    // Test initial value
    assert(param->getValue() == 1000.0f);
    
    // Test setValue
    param->setValue(5000.0f);
    assert(param->getValue() == 5000.0f);
    
    // Test clamping
    param->setValue(50000.0f);
    assert(param->getValue() == 20000.0f);
    
    param->setValue(10.0f);
    assert(param->getValue() == 20.0f);
    
    // Test normalized access
    param->setValueNormalized(0.5f);
    float expected = 20.0f + 0.5f * (20000.0f - 20.0f);
    assert(std::abs(param->getValue() - expected) < 0.1f);
    
    std::cout << "  Parameter value access passed." << std::endl;
}

void testNoCycleSimple() {
    std::cout << "  Testing no cycle in simple chain..." << std::endl;
    
    ParamRegistry registry;
    
    ParamId a = registry.registerParam({"A", 0.0f, 1.0f, 0.5f});
    ParamId b = registry.registerParam({"B", 0.0f, 1.0f, 0.5f});
    ParamId c = registry.registerParam({"C", 0.0f, 1.0f, 0.5f});
    
    // A -> B -> C (no cycle)
    assert(registry.addDependency(a, b));
    assert(registry.addDependency(b, c));
    assert(!registry.hasCycle());
    
    auto order = registry.getTopologicalOrder();
    assert(order.size() == 3);
    
    std::cout << "  No cycle test passed." << std::endl;
}

void testCycleDetection() {
    std::cout << "  Testing cycle detection..." << std::endl;
    
    ParamRegistry registry;
    
    ParamId a = registry.registerParam({"A", 0.0f, 1.0f, 0.5f});
    ParamId b = registry.registerParam({"B", 0.0f, 1.0f, 0.5f});
    ParamId c = registry.registerParam({"C", 0.0f, 1.0f, 0.5f});
    
    // A -> B -> C
    assert(registry.addDependency(a, b));
    assert(registry.addDependency(b, c));
    
    // Try to add C -> A (would create cycle)
    assert(registry.wouldCreateCycle(c, a));
    assert(!registry.addDependency(c, a));
    assert(!registry.hasCycle());
    
    std::cout << "  Cycle detection passed." << std::endl;
}

void testModulationRouting() {
    std::cout << "  Testing modulation routing..." << std::endl;
    
    ParamRegistry registry;
    
    ParamId source = registry.registerParam({"LFO", 0.0f, 1.0f, 0.5f});
    ParamId target = registry.registerParam({"Cutoff", 0.0f, 1.0f, 0.5f});
    
    ModMatrix matrix(&registry);
    
    ModSource modSource;
    modSource.type = ModSource::Type::Parameter;
    modSource.paramId = source;
    
    ModSlotId slot = matrix.connect(modSource, target, 0.5f, BlendMode::Add);
    assert(slot != InvalidModSlotId);
    assert(matrix.getSlotCount() == 1);
    
    // Verify slot
    const ModSlot* slotInfo = matrix.getSlot(slot);
    assert(slotInfo != nullptr);
    assert(slotInfo->target == target);
    assert(slotInfo->amount == 0.5f);
    
    std::cout << "  Modulation routing passed." << std::endl;
}

void testModulationProcess() {
    std::cout << "  Testing modulation processing..." << std::endl;
    
    ParamRegistry registry;
    
    ParamId source = registry.registerParam({"Source", 0.0f, 1.0f, 0.8f});
    ParamId target = registry.registerParam({"Target", 0.0f, 1.0f, 0.5f});
    
    ModMatrix matrix(&registry);
    
    ModSource modSource;
    modSource.type = ModSource::Type::Parameter;
    modSource.paramId = source;
    
    matrix.connect(modSource, target, 0.25f, BlendMode::Add);
    
    // Process modulation
    matrix.process();
    
    // Check modulation was applied
    auto* targetParam = registry.getParam(target);
    assert(targetParam != nullptr);
    
    // Modulated value should be base + (source_normalized * amount)
    // = 0.5 + (0.8 * 0.25) = 0.7
    float modulated = targetParam->getModulatedValue();
    assert(std::abs(modulated - 0.7f) < 0.01f);
    
    std::cout << "  Modulation processing passed." << std::endl;
}

} // anonymous namespace

int main() {
    std::cout << "Running Parameter Graph Tests..." << std::endl;
    
    testRegisterParams();
    testParamValues();
    testNoCycleSimple();
    testCycleDetection();
    testModulationRouting();
    testModulationProcess();
    
    std::cout << "All Parameter Graph Tests PASSED!" << std::endl;
    return 0;
}
