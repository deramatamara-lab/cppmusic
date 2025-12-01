# Sandbox Plugin Execution & Differential Analyzer

This document describes the sandbox process model, IPC buffers, and differential latency analyzer.

## Overview

The plugin sandbox system provides:
- Process isolation for third-party plugins
- Secure IPC for audio/MIDI data exchange
- Latency measurement and reporting
- Crash recovery without DAW restart

## Sandbox Process Model

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Plugin Sandbox Architecture              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                    DAW Process                       │   │
│  │                                                      │   │
│  │  ┌──────────────┐    ┌──────────────────────────┐   │   │
│  │  │ Audio Thread │◄──►│ SandboxBridge            │   │   │
│  │  │ (RT-safe)    │    │ - Lock-free ring buffers │   │   │
│  │  └──────────────┘    │ - Shared memory          │   │   │
│  │                      └──────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────┘   │
│               │                          │                  │
│               │ Shared Memory            │ Control Socket   │
│               ▼                          ▼                  │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              Sandbox Process (per plugin)           │   │
│  │                                                      │   │
│  │  ┌──────────────┐    ┌──────────────────────────┐   │   │
│  │  │ SandboxRunner│◄──►│ Plugin Instance          │   │   │
│  │  │ - IPC handler│    │ - VST/AU/LV2             │   │   │
│  │  └──────────────┘    │ - Isolated memory        │   │   │
│  │                      └──────────────────────────┘   │   │
│  │                                                      │   │
│  │  Restrictions:                                       │   │
│  │  - Limited filesystem access                         │   │
│  │  - No network access                                │   │
│  │  - Memory limit enforced                            │   │
│  │  - CPU time monitored                               │   │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### SandboxRunner

Manages sandboxed plugin processes:

```cpp
namespace cppmusic::platform::sandbox {

class SandboxRunner {
public:
    // Spawn a sandboxed plugin process
    SandboxId spawn(const PluginInfo& plugin, const SandboxConfig& config);
    
    // Terminate a sandbox
    void terminate(SandboxId id);
    
    // Check sandbox health
    SandboxStatus getStatus(SandboxId id) const;
    
    // Get IPC channel for communication
    IPCChannel* getChannel(SandboxId id);
    
    // Watchdog timeout configuration
    void setWatchdogTimeout(std::chrono::milliseconds timeout);
};

struct SandboxConfig {
    size_t maxMemoryMB = 512;
    std::chrono::milliseconds processTimeout = std::chrono::seconds(5);
    std::vector<std::filesystem::path> allowedPaths;
    bool allowGPUAccess = false;
};

enum class SandboxStatus {
    Starting,
    Running,
    Unresponsive,
    Crashed,
    Terminated
};

}
```

### Process Spawning (Stub)

```cpp
SandboxId SandboxRunner::spawn(const PluginInfo& plugin, 
                                const SandboxConfig& config) {
    SandboxId id = generateSandboxId();
    
    // Create shared memory region for audio/MIDI
    auto sharedMem = createSharedMemory(config.maxMemoryMB);
    
    // Fork/spawn child process
    #ifdef _WIN32
    // Use CreateProcessAsUser with restricted token
    // TODO: Implement actual Windows sandbox
    #else
    // Use fork() + seccomp/namespaces
    // TODO: Implement actual Linux sandbox
    #endif
    
    // For stub: just track as "running"
    sandboxes_[id] = {
        .status = SandboxStatus::Running,
        .plugin = plugin,
        .config = config,
        .sharedMem = std::move(sharedMem)
    };
    
    return id;
}
```

## IPC Buffers

### Shared Memory Layout

```cpp
struct SharedAudioBuffer {
    // Lock-free ring buffer for audio
    std::atomic<size_t> writePos;
    std::atomic<size_t> readPos;
    size_t bufferSize;
    float data[];  // Flexible array member
    
    // Non-blocking write
    bool write(const float* samples, size_t numSamples) {
        size_t currentWrite = writePos.load(std::memory_order_relaxed);
        size_t currentRead = readPos.load(std::memory_order_acquire);
        
        size_t available = bufferSize - (currentWrite - currentRead);
        if (numSamples > available) return false;
        
        // Copy samples with wrap-around
        for (size_t i = 0; i < numSamples; ++i) {
            data[(currentWrite + i) % bufferSize] = samples[i];
        }
        
        writePos.store(currentWrite + numSamples, std::memory_order_release);
        return true;
    }
    
    // Non-blocking read
    size_t read(float* samples, size_t maxSamples) {
        size_t currentRead = readPos.load(std::memory_order_relaxed);
        size_t currentWrite = writePos.load(std::memory_order_acquire);
        
        size_t available = currentWrite - currentRead;
        size_t toRead = std::min(available, maxSamples);
        
        for (size_t i = 0; i < toRead; ++i) {
            samples[i] = data[(currentRead + i) % bufferSize];
        }
        
        readPos.store(currentRead + toRead, std::memory_order_release);
        return toRead;
    }
};
```

### Control Messages

```cpp
enum class SandboxMessageType : uint32_t {
    // Plugin lifecycle
    Initialize,
    Prepare,
    ProcessBlock,
    Release,
    
    // Parameter control
    SetParameter,
    GetParameter,
    
    // State management
    GetState,
    SetState,
    
    // Response messages
    Acknowledge,
    Error,
    LatencyReport
};

struct SandboxMessage {
    SandboxMessageType type;
    uint32_t sequenceId;
    uint32_t payloadSize;
    uint8_t payload[];  // Variable-length data
};
```

## PluginInspector

Analyzes plugin behavior:

```cpp
namespace cppmusic::platform::plugins {

class PluginInspector {
public:
    // Measure plugin latency
    PluginLatencyReport measureLatency(SandboxId id);
    
    // Monitor resource usage
    PluginResourceUsage getResourceUsage(SandboxId id);
    
    // Differential latency analysis
    LatencyDiff computeLatencyDiff(const PluginLatencyReport& before,
                                    const PluginLatencyReport& after);
    
    // Get plugin health status
    PluginHealthStatus getHealthStatus(SandboxId id);
};

struct PluginLatencyReport {
    std::chrono::microseconds reportedLatency;   // Plugin-reported
    std::chrono::microseconds measuredLatency;   // Actual measured
    std::chrono::microseconds jitter;            // Variation
    size_t samplesMeasured;
};

struct PluginResourceUsage {
    float cpuPercent;
    size_t memoryMB;
    size_t peakMemoryMB;
    uint32_t audioDropouts;
};

}
```

## Differential Latency Analyzer

Compares plugin latency across conditions:

```cpp
struct LatencyDiff {
    std::chrono::microseconds baselineLatency;
    std::chrono::microseconds currentLatency;
    std::chrono::microseconds difference;
    float percentChange;
    
    enum class Significance {
        None,       // < 5% change
        Minor,      // 5-15% change
        Major,      // > 15% change
        Critical    // Exceeds block budget
    };
    
    Significance significance;
};

LatencyDiff PluginInspector::computeLatencyDiff(
    const PluginLatencyReport& before,
    const PluginLatencyReport& after) {
    
    LatencyDiff diff;
    diff.baselineLatency = before.measuredLatency;
    diff.currentLatency = after.measuredLatency;
    diff.difference = after.measuredLatency - before.measuredLatency;
    diff.percentChange = static_cast<float>(diff.difference.count()) / 
                         static_cast<float>(before.measuredLatency.count()) * 100.0f;
    
    if (std::abs(diff.percentChange) < 5.0f) {
        diff.significance = LatencyDiff::Significance::None;
    } else if (std::abs(diff.percentChange) < 15.0f) {
        diff.significance = LatencyDiff::Significance::Minor;
    } else if (after.measuredLatency > blockBudget_) {
        diff.significance = LatencyDiff::Significance::Critical;
    } else {
        diff.significance = LatencyDiff::Significance::Major;
    }
    
    return diff;
}
```

## Crash Recovery

### Watchdog

Monitors sandbox health and recovers from crashes:

```cpp
void SandboxRunner::watchdogLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        for (auto& [id, sandbox] : sandboxes_) {
            if (!isProcessAlive(sandbox.processId)) {
                sandbox.status = SandboxStatus::Crashed;
                
                LOG_ERROR("Plugin sandbox {} crashed: {}",
                          id, sandbox.plugin.name);
                
                // Notify listeners
                for (auto* listener : crashListeners_) {
                    listener->onSandboxCrash(id, sandbox.plugin);
                }
                
                // Auto-restart if configured
                if (sandbox.config.autoRestart) {
                    respawn(id);
                }
            }
        }
    }
}
```

### Logging

Sandbox events are logged with rejection reason codes:

| Code | Reason |
|------|--------|
| `SB001` | Plugin failed to initialize |
| `SB002` | Plugin exceeded memory limit |
| `SB003` | Plugin exceeded CPU time |
| `SB004` | Plugin attempted forbidden syscall |
| `SB005` | Plugin unresponsive (watchdog timeout) |
| `SB006` | Plugin crashed |

## File Layout

```
src/platform/sandbox/
├── SandboxRunner.hpp/.cpp
├── SharedAudioBuffer.hpp
└── CMakeLists.txt

src/platform/plugins/
├── PluginInspector.hpp/.cpp
└── CMakeLists.txt

src/ui/plugins/
├── PluginHealthWidget.cpp
└── CMakeLists.txt
```

## Security Hardening

### Syscall Filtering (Linux)

```cpp
// Example seccomp filter (stub)
void SandboxRunner::installSeccompFilter() {
    // Allow: read, write, mmap, mprotect, futex, clock_gettime
    // Deny: fork, exec, socket, open (except whitelisted)
    // TODO: Implement actual seccomp rules
}
```

### Memory Isolation

- Each sandbox has isolated address space
- Shared memory regions are mapped read-only where appropriate
- Stack canaries enabled
- ASLR enforced

## Testing

Key test cases:
- Sandbox spawns and terminates cleanly
- Audio data flows through IPC correctly
- Crash is detected and reported
- Latency measurement is accurate
- Memory limits are enforced
