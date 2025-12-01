/**
 * @file BenchParamPropagation.cpp
 * @brief Benchmark for parameter change propagation performance
 *
 * Measures the performance of propagating parameter changes
 * through the modulation graph.
 */

#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace {

// Simplified parameter graph for benchmarking
class ParamGraphBench {
public:
    using ParamId = uint32_t;
    
    struct Connection {
        ParamId source;
        ParamId target;
        float amount{1.0f};
    };
    
    void addParam(ParamId id) {
        values_[id] = 0.0f;
        observers_[id] = {};
    }
    
    void connect(ParamId source, ParamId target, float amount = 1.0f) {
        connections_.push_back({source, target, amount});
        observers_[source].insert(target);
    }
    
    // Propagate a change and count updates
    int propagateChange(ParamId id, float newValue) {
        int updates = 0;
        
        values_[id] = newValue;
        ++updates;
        
        // BFS propagation
        std::vector<ParamId> queue;
        std::unordered_set<ParamId> visited;
        
        queue.push_back(id);
        visited.insert(id);
        
        while (!queue.empty()) {
            ParamId current = queue.front();
            queue.erase(queue.begin());
            
            for (ParamId observer : observers_[current]) {
                if (visited.find(observer) == visited.end()) {
                    visited.insert(observer);
                    queue.push_back(observer);
                    
                    // Update observer value
                    values_[observer] = values_[current] * 0.9f;  // Simplified
                    ++updates;
                }
            }
        }
        
        return updates;
    }
    
    size_t getParamCount() const { return values_.size(); }
    size_t getConnectionCount() const { return connections_.size(); }
    
private:
    std::unordered_map<ParamId, float> values_;
    std::unordered_map<ParamId, std::unordered_set<ParamId>> observers_;
    std::vector<Connection> connections_;
};

struct BenchmarkResult {
    std::string name;
    size_t paramCount{0};
    size_t connectionCount{0};
    double avgPropagationUs{0.0};
    int avgUpdates{0};
};

BenchmarkResult runBenchmark(const std::string& name,
                              size_t numParams,
                              size_t numConnections,
                              int iterations) {
    ParamGraphBench graph;
    
    // Add parameters
    for (size_t i = 0; i < numParams; ++i) {
        graph.addParam(static_cast<ParamGraphBench::ParamId>(i));
    }
    
    // Add connections (tree-like structure)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, numParams - 1);
    
    for (size_t i = 0; i < numConnections; ++i) {
        auto src = static_cast<ParamGraphBench::ParamId>(dist(gen));
        auto dst = static_cast<ParamGraphBench::ParamId>(dist(gen));
        if (src != dst) {
            graph.connect(src, dst, 0.5f);
        }
    }
    
    // Benchmark propagation
    std::uniform_real_distribution<float> valueDist(0.0f, 1.0f);
    
    auto start = std::chrono::high_resolution_clock::now();
    int totalUpdates = 0;
    
    for (int i = 0; i < iterations; ++i) {
        auto paramId = static_cast<ParamGraphBench::ParamId>(dist(gen));
        float newValue = valueDist(gen);
        totalUpdates += graph.propagateChange(paramId, newValue);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    BenchmarkResult result;
    result.name = name;
    result.paramCount = numParams;
    result.connectionCount = numConnections;
    result.avgPropagationUs = static_cast<double>(duration.count()) / iterations;
    result.avgUpdates = totalUpdates / iterations;
    
    return result;
}

void printResult(const BenchmarkResult& result) {
    std::cout << result.name << std::endl;
    std::cout << "  Parameters: " << result.paramCount << std::endl;
    std::cout << "  Connections: " << result.connectionCount << std::endl;
    std::cout << "  Avg propagation time: " << std::fixed << std::setprecision(2)
              << result.avgPropagationUs << " us" << std::endl;
    std::cout << "  Avg updates per change: " << result.avgUpdates << std::endl;
    std::cout << std::endl;
}

}  // namespace

int main() {
    std::cout << "=== Parameter Propagation Benchmark ===" << std::endl;
    std::cout << std::endl;
    
    int iterations = 1000;
    
    // Small graph
    printResult(runBenchmark("Small Graph", 10, 15, iterations));
    
    // Medium graph
    printResult(runBenchmark("Medium Graph", 100, 200, iterations));
    
    // Large graph
    printResult(runBenchmark("Large Graph", 1000, 2000, iterations));
    
    // Very large graph
    printResult(runBenchmark("Very Large Graph", 5000, 10000, iterations / 10));
    
    // Dense small graph
    printResult(runBenchmark("Dense Small Graph", 50, 500, iterations));
    
    // Sparse large graph
    printResult(runBenchmark("Sparse Large Graph", 2000, 500, iterations));
    
    std::cout << "Benchmark complete." << std::endl;
    return 0;
}
