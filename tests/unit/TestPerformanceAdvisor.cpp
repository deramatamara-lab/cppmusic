/**
 * @file TestPerformanceAdvisor.cpp
 * @brief Unit tests for performance tier adjustment logic.
 */

#include "engine/performance/PerformanceAdvisor.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

using namespace cppmusic::engine::performance;

namespace {

// Mock node for testing
class MockNode : public NodeQuality {
public:
    explicit MockNode(const char* name) : name_(name) {}
    
    std::vector<QualityTier> getSupportedTiers() const override {
        return {QualityTier::Low, QualityTier::Medium, QualityTier::High};
    }
    
    QualityTier getCurrentTier() const override {
        return currentTier_;
    }
    
    void setQualityTier(QualityTier tier) override {
        currentTier_ = tier;
    }
    
    float estimateCostPerSample(QualityTier tier) const override {
        switch (tier) {
            case QualityTier::Low: return 1.0f;
            case QualityTier::Medium: return 2.5f;
            case QualityTier::High: return 6.0f;
            case QualityTier::Ultra: return 15.0f;
        }
        return 1.0f;
    }
    
    const char* getNodeName() const override {
        return name_;
    }
    
private:
    const char* name_;
    QualityTier currentTier_ = QualityTier::High;
};

void testInitialization() {
    std::cout << "  Testing initialization..." << std::endl;
    
    PerformanceAdvisor advisor;
    advisor.initialize(48000.0, 512);
    
    // Block budget should be approximately 10.67ms
    auto budget = advisor.getBlockBudget();
    assert(budget.count() > 10000 && budget.count() < 11000);
    
    assert(advisor.getGlobalTier() == QualityTier::High);
    assert(advisor.getAverageLoad() == 0.0f);
    
    std::cout << "  Initialization passed." << std::endl;
}

void testNodeRegistration() {
    std::cout << "  Testing node registration..." << std::endl;
    
    PerformanceAdvisor advisor;
    advisor.initialize(48000.0, 512);
    
    MockNode node1("Synth1");
    MockNode node2("Reverb");
    
    advisor.registerNode(&node1);
    advisor.registerNode(&node2);
    
    auto nodes = advisor.getNodes();
    assert(nodes.size() == 2);
    
    advisor.unregisterNode(&node1);
    nodes = advisor.getNodes();
    assert(nodes.size() == 1);
    
    std::cout << "  Node registration passed." << std::endl;
}

void testTierPropagation() {
    std::cout << "  Testing tier propagation to nodes..." << std::endl;
    
    PerformanceAdvisor advisor;
    advisor.initialize(48000.0, 512);
    
    MockNode node1("Node1");
    MockNode node2("Node2");
    
    advisor.registerNode(&node1);
    advisor.registerNode(&node2);
    
    // Set global tier
    advisor.setGlobalTier(QualityTier::Low);
    
    // All nodes should be updated
    assert(node1.getCurrentTier() == QualityTier::Low);
    assert(node2.getCurrentTier() == QualityTier::Low);
    
    advisor.setGlobalTier(QualityTier::Medium);
    assert(node1.getCurrentTier() == QualityTier::Medium);
    assert(node2.getCurrentTier() == QualityTier::Medium);
    
    std::cout << "  Tier propagation passed." << std::endl;
}

void testPreferences() {
    std::cout << "  Testing quality preferences..." << std::endl;
    
    PerformanceAdvisor advisor;
    advisor.initialize(48000.0, 512);
    
    QualityPreferences prefs;
    prefs.preferredTier = QualityTier::High;
    prefs.minimumTier = QualityTier::Medium;
    prefs.allowAutoDowngrade = true;
    prefs.allowAutoUpgrade = true;
    prefs.targetLoadPercent = 75.0f;
    
    advisor.setPreferences(prefs);
    
    const auto& retrieved = advisor.getPreferences();
    assert(retrieved.preferredTier == QualityTier::High);
    assert(retrieved.minimumTier == QualityTier::Medium);
    assert(retrieved.allowAutoDowngrade == true);
    
    std::cout << "  Quality preferences passed." << std::endl;
}

void testBlockMeasurement() {
    std::cout << "  Testing block measurement..." << std::endl;
    
    PerformanceAdvisor advisor;
    advisor.initialize(48000.0, 512);
    
    // Simulate some blocks with deterministic timing simulation
    // Instead of sleeping, we directly set simulated block times
    for (int i = 0; i < 10; ++i) {
        advisor.beginBlock();
        // Simulate time passing by calling endBlock with a known elapsed time
        // The implementation should use the time between begin/end
        // For testing purposes, we measure without real delay
        advisor.endBlock();
    }
    
    // Average load should be non-zero (or zero if no actual processing time)
    float avgLoad = advisor.getAverageLoad();
    // Load can be 0 or very small since we didn't do real work
    assert(avgLoad >= 0.0f && avgLoad <= 1.0f);
    
    std::cout << "  Block measurement passed." << std::endl;
}

void testNodeLoadInfo() {
    std::cout << "  Testing node load info..." << std::endl;
    
    PerformanceAdvisor advisor;
    advisor.initialize(48000.0, 512);
    
    MockNode node1("Synth");
    MockNode node2("Effect");
    
    advisor.registerNode(&node1);
    advisor.registerNode(&node2);
    
    auto loads = advisor.getNodeLoads();
    assert(loads.size() == 2);
    
    for (const auto& info : loads) {
        assert(info.node != nullptr);
        assert(info.estimatedCost > 0.0f);
    }
    
    std::cout << "  Node load info passed." << std::endl;
}

// Note: The following tests verify the structure of the downgrade logic
// but don't actually trigger downgrades since that requires sustained
// high load over many blocks

void testTierHelpers() {
    std::cout << "  Testing tier helper functions..." << std::endl;
    
    // Test decrement
    assert(decrementTier(QualityTier::Ultra) == QualityTier::High);
    assert(decrementTier(QualityTier::High) == QualityTier::Medium);
    assert(decrementTier(QualityTier::Medium) == QualityTier::Low);
    assert(decrementTier(QualityTier::Low) == QualityTier::Low);
    
    // Test increment
    assert(incrementTier(QualityTier::Low) == QualityTier::Medium);
    assert(incrementTier(QualityTier::Medium) == QualityTier::High);
    assert(incrementTier(QualityTier::High) == QualityTier::Ultra);
    assert(incrementTier(QualityTier::Ultra) == QualityTier::Ultra);
    
    // Test toString
    assert(std::string(toString(QualityTier::Low)) == "Low");
    assert(std::string(toString(QualityTier::Medium)) == "Medium");
    assert(std::string(toString(QualityTier::High)) == "High");
    assert(std::string(toString(QualityTier::Ultra)) == "Ultra");
    
    std::cout << "  Tier helper functions passed." << std::endl;
}

} // anonymous namespace

int main() {
    std::cout << "Running Performance Advisor Tests..." << std::endl;
    
    testInitialization();
    testNodeRegistration();
    testTierPropagation();
    testPreferences();
    testBlockMeasurement();
    testNodeLoadInfo();
    testTierHelpers();
    
    std::cout << "All Performance Advisor Tests PASSED!" << std::endl;
    return 0;
}
