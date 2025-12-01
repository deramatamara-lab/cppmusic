# Adaptive Performance Mode & Load Balancer

This document describes quality tiers, predictive scheduling, and downgrade triggers.

## Overview

The adaptive performance system provides:
- Quality tier negotiation for DSP nodes
- Predictive load estimation
- Automatic downgrade under high CPU load
- User-configurable quality preferences

## Quality Tiers

### NodeQuality Interface

DSP nodes implement quality tiers:

```cpp
namespace cppmusic::engine::performance {

enum class QualityTier {
    Low,      // Minimal processing, lowest latency
    Medium,   // Balanced quality/performance
    High,     // Full quality, higher CPU usage
    Ultra     // Maximum quality, may exceed budget
};

class NodeQuality {
public:
    virtual ~NodeQuality() = default;
    
    // Get supported tiers
    virtual std::vector<QualityTier> getSupportedTiers() const = 0;
    
    // Get current tier
    virtual QualityTier getCurrentTier() const = 0;
    
    // Set quality tier
    virtual void setQualityTier(QualityTier tier) = 0;
    
    // Estimate CPU cost per sample at given tier
    virtual float estimateCostPerSample(QualityTier tier) const = 0;
};

}
```

### Tier Characteristics

| Tier | Oversampling | Filter Order | Lookahead | CPU Factor |
|------|--------------|--------------|-----------|------------|
| Low | 1x | 2 | None | 1.0x |
| Medium | 2x | 4 | Partial | 2.5x |
| High | 4x | 8 | Full | 6.0x |
| Ultra | 8x | 16 | Extended | 15.0x |

## PerformanceAdvisor

### Architecture

```cpp
class PerformanceAdvisor {
public:
    // Initialize with audio settings
    void initialize(double sampleRate, size_t blockSize);
    
    // Register nodes for management
    void registerNode(NodeQuality* node);
    void unregisterNode(NodeQuality* node);
    
    // Called each block to measure and adjust
    void beginBlock();
    void endBlock();
    
    // Manual tier override
    void setGlobalTier(QualityTier tier);
    QualityTier getGlobalTier() const;
    
    // Statistics
    float getAverageLoad() const;
    float getPeakLoad() const;
    std::vector<NodeLoadInfo> getNodeLoads() const;
};
```

### Load Measurement

```cpp
struct BlockMetrics {
    std::chrono::microseconds totalDuration;
    std::chrono::microseconds budgetMicros;
    float loadFactor;  // duration / budget
    
    bool isOverloaded() const {
        return loadFactor > 0.75f;  // >75% budget used
    }
    
    bool isCritical() const {
        return loadFactor > 0.95f;  // Near dropout
    }
};
```

## Predictive Scheduling Model

### Load Prediction

Uses exponential moving average with spike detection:

```cpp
class LoadPredictor {
public:
    void recordSample(float load) {
        // EMA for smooth trend
        emaLoad_ = alpha_ * load + (1 - alpha_) * emaLoad_;
        
        // Track recent maximum for spike detection
        recentMax_ = std::max(recentMax_ * decay_, load);
    }
    
    float predict() const {
        // Predict with headroom for spikes
        return std::max(emaLoad_, recentMax_ * 0.8f);
    }
    
private:
    float emaLoad_ = 0.0f;
    float recentMax_ = 0.0f;
    float alpha_ = 0.1f;   // EMA smoothing
    float decay_ = 0.95f;  // Spike decay
};
```

### Cost Estimation

```cpp
float PerformanceAdvisor::estimateTotalCost(QualityTier tier) const {
    float totalCost = 0.0f;
    
    for (const auto* node : nodes_) {
        totalCost += node->estimateCostPerSample(tier) * blockSize_;
    }
    
    return totalCost / budgetMicros_;  // As fraction of budget
}
```

## Downgrade Triggers

### Trigger Conditions

| Trigger | Threshold | Action |
|---------|-----------|--------|
| Sustained High Load | >75% for 100 blocks | Downgrade one tier |
| Critical Load | >95% for 3 blocks | Immediate downgrade |
| Audio Dropout | Buffer underrun | Emergency Low tier |
| Recovery | <50% for 500 blocks | Upgrade one tier |

### State Machine

```
┌─────────────────────────────────────────────────────────────┐
│                    Quality State Machine                    │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────┐   sustained >75%   ┌─────────┐               │
│  │  Ultra  │──────────────────►│  High   │               │
│  └─────────┘                    └─────────┘               │
│       ▲                              │                     │
│       │ <50% for 500 blocks          │ sustained >75%     │
│       │                              ▼                     │
│  ┌─────────┐                    ┌─────────┐               │
│  │  High   │◄───────────────────│ Medium  │               │
│  └─────────┘   <50% for 500     └─────────┘               │
│       ▲                              │                     │
│       │                              │ sustained >75%     │
│       │                              ▼                     │
│  ┌─────────┐                    ┌─────────┐               │
│  │ Medium  │◄───────────────────│   Low   │               │
│  └─────────┘   <50% for 500     └─────────┘               │
│                                      │                     │
│                                      │ dropout             │
│                                      ▼                     │
│                                 ┌──────────┐              │
│                                 │ EMERGENCY│              │
│                                 │ (all Low)│              │
│                                 └──────────┘              │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Implementation

```cpp
void PerformanceAdvisor::endBlock() {
    auto duration = measureBlockDuration();
    float load = static_cast<float>(duration.count()) / budgetMicros_;
    
    loadPredictor_.recordSample(load);
    
    // Check for critical load
    if (load > 0.95f) {
        criticalCount_++;
        if (criticalCount_ >= 3) {
            triggerDowngrade(DowngradeReason::CriticalLoad);
        }
    } else {
        criticalCount_ = 0;
    }
    
    // Check for sustained high load
    if (load > 0.75f) {
        highLoadCount_++;
        if (highLoadCount_ >= 100) {
            triggerDowngrade(DowngradeReason::SustainedHighLoad);
        }
    } else {
        highLoadCount_ = 0;
    }
    
    // Check for recovery
    if (load < 0.50f) {
        lowLoadCount_++;
        if (lowLoadCount_ >= 500) {
            triggerUpgrade();
        }
    } else {
        lowLoadCount_ = 0;
    }
}
```

## Logging

All quality changes are logged with reason codes:

```cpp
enum class DowngradeReason {
    SustainedHighLoad,  // DG001
    CriticalLoad,       // DG002
    AudioDropout,       // DG003
    UserRequest         // DG004
};

enum class UpgradeReason {
    SustainedLowLoad,   // UG001
    UserRequest         // UG002
};

void PerformanceAdvisor::triggerDowngrade(DowngradeReason reason) {
    QualityTier newTier = decrementTier(currentTier_);
    
    LOG_INFO("Performance downgrade: {} -> {} (reason: {})",
             toString(currentTier_), toString(newTier), toString(reason));
    
    applyTier(newTier);
}
```

## User Configuration

### Quality Preferences

```cpp
struct QualityPreferences {
    QualityTier preferredTier = QualityTier::High;
    QualityTier minimumTier = QualityTier::Medium;
    bool allowAutoDowngrade = true;
    bool allowAutoUpgrade = true;
    float targetLoadPercent = 75.0f;
};
```

## File Layout

```
src/engine/performance/
├── PerformanceAdvisor.hpp/.cpp
├── NodeQuality.hpp/.cpp
└── CMakeLists.txt

src/ui/performance/
├── PerfDashboard.cpp
└── CMakeLists.txt
```

## Testing

Key test cases:
- Tier adjustment triggers at >75% load
- Emergency downgrade at dropout
- Recovery upgrade at sustained low load
- Cost estimation accuracy
- State machine transitions are correct
