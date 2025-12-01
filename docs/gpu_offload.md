# Hybrid CPU/GPU DSP Offload Framework

This document describes the GPU offload heuristics, latency budgeting, and CPU fallback strategy.

## Overview

The hybrid DSP framework enables:
- Automatic offloading of compute-intensive DSP to GPU
- Latency-aware scheduling with budget management
- Seamless CPU fallback when GPU unavailable
- Vulkan compute shader integration (stub)

## Architecture

### GpuContext

Manages Vulkan instance, device, and command queues:

```cpp
namespace cppmusic::engine::dsp::offload {

class GpuContext {
public:
    // Initialize Vulkan context
    bool initialize();
    
    // Check if GPU compute is available
    bool isAvailable() const noexcept;
    
    // Get compute capabilities
    GpuCapabilities getCapabilities() const;
    
    // Submit compute work
    void submitBatch(const ComputeBatch& batch);
    
    // Wait for completion
    void waitIdle();
    
private:
    // Vulkan handles (stub - conditional compile)
    #if ENABLE_GPU
    VkInstance instance_;
    VkDevice device_;
    VkQueue computeQueue_;
    #endif
};

}
```

### OffloadManager

Heuristic scheduler deciding CPU vs GPU execution:

```cpp
class OffloadManager {
public:
    // Register a node for potential offload
    void registerNode(OffloadableNode* node);
    
    // Decide execution path for current block
    ExecutionPath getExecutionPath(OffloadableNode* node);
    
    // Update latency statistics
    void recordLatency(OffloadableNode* node, std::chrono::microseconds duration);
    
    enum class ExecutionPath {
        CPU,           // Execute on CPU
        GPU,           // Execute on GPU
        GPU_Fallback   // GPU requested but using CPU fallback
    };
};
```

## Offload Heuristics

### Decision Factors

| Factor | Weight | Description |
|--------|--------|-------------|
| Work Size | 0.4 | Larger workloads favor GPU |
| GPU Availability | 0.3 | GPU must be initialized |
| Historical Latency | 0.2 | Use measured performance |
| Block Budget | 0.1 | Time remaining in audio block |

### Offload Score Calculation

```cpp
float OffloadManager::computeOffloadScore(OffloadableNode* node) {
    float score = 0.0f;
    
    // Work size component
    size_t workSize = node->getWorkSize();
    score += 0.4f * std::min(1.0f, workSize / kGpuThreshold);
    
    // GPU availability
    if (gpuContext_.isAvailable()) {
        score += 0.3f;
    }
    
    // Historical latency comparison
    auto cpuLatency = getAverageCpuLatency(node);
    auto gpuLatency = getAverageGpuLatency(node);
    if (gpuLatency < cpuLatency) {
        score += 0.2f * (1.0f - gpuLatency / cpuLatency);
    }
    
    // Block budget component
    auto remaining = getBlockBudgetRemaining();
    auto estimated = node->estimateGpuLatency();
    if (remaining > estimated * 1.5f) {
        score += 0.1f;
    }
    
    return score;
}
```

### Threshold

- Score >= 0.6: GPU execution
- Score < 0.6: CPU execution

## Latency Budgeting

### Block Budget Model

For 48kHz sample rate with 512 sample buffer:
- Total budget: 512 / 48000 = 10.67ms
- Target utilization: 75% = 8.0ms
- Safety margin: 25% = 2.67ms

```cpp
struct LatencyBudget {
    std::chrono::microseconds totalBudget;
    std::chrono::microseconds targetBudget;
    std::chrono::microseconds consumed;
    
    bool canAllocate(std::chrono::microseconds request) const {
        return (consumed + request) <= targetBudget;
    }
};
```

### Budget Allocation

```
┌─────────────────────────────────────────────────────────────┐
│                    Block Budget (10.67ms)                   │
├─────────────────────────────────────────────────────────────┤
│ ████████████████████████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ │
│ │◄─── Target (8.0ms) ────►│◄─── Safety (2.67ms) ────►│    │
│                                                             │
│ Allocated:                                                  │
│ ▓▓▓▓ Node1 (1.2ms)                                         │
│     ▓▓▓▓▓▓ Node2 GPU (1.8ms)                               │
│           ▓▓▓▓▓▓▓▓▓▓ Node3 Conv (2.5ms)                    │
│                     ▓▓▓▓ Node4 (1.0ms)                     │
│                         ░░░░░░░░ Remaining (1.5ms)         │
└─────────────────────────────────────────────────────────────┘
```

## Fallback Strategy

### Graceful Degradation

When GPU is unavailable or overloaded:

1. **Detection**: Monitor GPU queue depth and latency
2. **Decision**: If queue > 3 or latency > budget, trigger fallback
3. **Execution**: Run CPU implementation
4. **Recovery**: Periodically retry GPU path

```cpp
ExecutionPath OffloadManager::getExecutionPath(OffloadableNode* node) {
    if (!gpuContext_.isAvailable()) {
        return ExecutionPath::CPU;
    }
    
    float score = computeOffloadScore(node);
    if (score < kOffloadThreshold) {
        return ExecutionPath::CPU;
    }
    
    // Check GPU health
    if (gpuQueueDepth_ > kMaxQueueDepth || 
        recentGpuStalls_ > kMaxStalls) {
        return ExecutionPath::GPU_Fallback;
    }
    
    return ExecutionPath::GPU;
}
```

### Logging

All fallback events are logged with reason codes:

| Code | Reason |
|------|--------|
| `FB001` | GPU context not initialized |
| `FB002` | Offload score below threshold |
| `FB003` | GPU queue depth exceeded |
| `FB004` | Recent GPU stalls detected |
| `FB005` | Insufficient block budget |

## GPU DSP Nodes

### GpuFFTNode

FFT processing via GPU compute:

```cpp
class GpuFFTNode : public OffloadableNode {
public:
    void prepare(size_t fftSize);
    void processBlock(const float* input, float* output, size_t numSamples);
    
    // OffloadableNode interface
    size_t getWorkSize() const override { return fftSize_; }
    std::chrono::microseconds estimateGpuLatency() const override;
};
```

### ConvolutionNode

IR-based convolution with GPU offload:

```cpp
class ConvolutionNode : public OffloadableNode {
public:
    bool loadIR(const std::filesystem::path& irPath);
    void processBlock(const float* input, float* output, size_t numSamples);
    
    size_t getWorkSize() const override { return irLength_ * blockSize_; }
};
```

## File Layout

```
src/engine/dsp/offload/
├── GpuContext.hpp/.cpp
├── OffloadManager.hpp/.cpp
└── CMakeLists.txt

src/engine/dsp/nodes/
├── GpuFFTNode.hpp/.cpp
├── ConvolutionNode.hpp/.cpp
└── CMakeLists.txt
```

## Conditional Compilation

```cmake
option(ENABLE_GPU "Enable Vulkan-based GPU DSP offload" OFF)

if(ENABLE_GPU)
    find_package(Vulkan REQUIRED)
    target_compile_definitions(dsp_offload PRIVATE ENABLE_GPU=1)
    target_link_libraries(dsp_offload PRIVATE Vulkan::Vulkan)
else()
    target_compile_definitions(dsp_offload PRIVATE ENABLE_GPU=0)
endif()
```
