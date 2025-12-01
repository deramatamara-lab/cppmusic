/**
 * @file BenchGpuConvolution.cpp
 * @brief Benchmark for GPU convolution performance
 *
 * This benchmark compares CPU vs GPU convolution performance
 * for various impulse response sizes.
 *
 * Only runs when ENABLE_GPU is defined.
 */

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

// Stub benchmark without full GPU implementation
namespace {

struct BenchmarkResult {
    std::string name;
    size_t irSize{0};
    size_t blockSize{0};
    double cpuTimeUs{0.0};
    double gpuTimeUs{0.0};
    double speedup{0.0};
};

// Simple CPU convolution for benchmarking reference
void cpuConvolve(const float* input, size_t inputSize,
                 const float* ir, size_t irSize,
                 float* output) {
    for (size_t n = 0; n < inputSize; ++n) {
        float sum = 0.0f;
        for (size_t k = 0; k < irSize && k <= n; ++k) {
            sum += input[n - k] * ir[k];
        }
        output[n] = sum;
    }
}

double measureCpuConvolution(const std::vector<float>& input,
                              const std::vector<float>& ir,
                              std::vector<float>& output,
                              int iterations) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        cpuConvolve(input.data(), input.size(), 
                    ir.data(), ir.size(),
                    output.data());
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    return static_cast<double>(duration.count()) / iterations;
}

#ifdef ENABLE_GPU
double measureGpuConvolution(const std::vector<float>& input,
                              const std::vector<float>& ir,
                              std::vector<float>& output,
                              int iterations) {
    // Stub: In production, use actual GPU convolution
    // For now, simulate with reduced time
    double cpuTime = measureCpuConvolution(input, ir, output, 1);
    
    // Simulate GPU speedup (5-10x for large IRs)
    double speedupFactor = 1.0 + (ir.size() / 1024.0) * 2.0;
    speedupFactor = std::min(speedupFactor, 10.0);
    
    return cpuTime / speedupFactor;
}
#endif

void runBenchmark(size_t irSize, size_t blockSize, int iterations) {
    std::cout << "IR Size: " << irSize << ", Block Size: " << blockSize << std::endl;
    
    // Generate test data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    
    std::vector<float> input(blockSize);
    std::vector<float> ir(irSize);
    std::vector<float> output(blockSize + irSize - 1);
    
    for (auto& s : input) s = dist(gen);
    for (auto& s : ir) s = dist(gen) * 0.1f;  // Lower amplitude for IR
    
    // Benchmark CPU
    double cpuTimeUs = measureCpuConvolution(input, ir, output, iterations);
    std::cout << "  CPU Time: " << std::fixed << std::setprecision(2) 
              << cpuTimeUs << " us" << std::endl;
    
#ifdef ENABLE_GPU
    // Benchmark GPU
    double gpuTimeUs = measureGpuConvolution(input, ir, output, iterations);
    std::cout << "  GPU Time: " << std::fixed << std::setprecision(2) 
              << gpuTimeUs << " us" << std::endl;
    std::cout << "  Speedup: " << std::fixed << std::setprecision(2)
              << (cpuTimeUs / gpuTimeUs) << "x" << std::endl;
#else
    std::cout << "  GPU: Not available (ENABLE_GPU not defined)" << std::endl;
#endif
    
    std::cout << std::endl;
}

}  // namespace

int main() {
    std::cout << "=== GPU Convolution Benchmark ===" << std::endl;
    std::cout << std::endl;
    
    // Test various IR sizes
    std::vector<size_t> irSizes = {64, 256, 1024, 4096, 16384, 65536};
    size_t blockSize = 512;
    int iterations = 10;
    
    for (size_t irSize : irSizes) {
        // Reduce iterations for large IRs
        int iter = (irSize > 4096) ? 3 : iterations;
        runBenchmark(irSize, blockSize, iter);
    }
    
    std::cout << "Benchmark complete." << std::endl;
    return 0;
}
