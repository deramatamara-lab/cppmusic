# Mega Integration Overview

This document provides a cross-system interaction map for the advanced subsystems integrated into the cppmusic DAW.

## Feature Domains

1. [Reactive Parameter & Modulation Graph](parameters.md)
2. [Spectral Piano Roll & Harmonic Analyzer](pianoroll_spectral.md)
3. [Hybrid CPU/GPU DSP Offload Framework](gpu_offload.md)
4. [Automation Curves 2.0](automation_layers.md)
5. [Collaborative Live Session Skeleton](collaboration.md)
6. [Intelligent Asset Browser & Smart Tagging](assets.md)
7. [Adaptive Performance Mode & Load Balancer](performance_adaptive.md)
8. [Project Integrity & Time-Travel Undo](undo_integrity.md)
9. [AI Arrangement Assistant](ai_arrangement.md)
10. [Sandbox Plugin Execution & Differential Analyzer](plugin_sandbox.md)

## Thread Topology

The DAW uses a strict thread separation model to ensure real-time audio performance:

```
┌─────────────────────────────────────────────────────────────────┐
│                        Thread Topology                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────┐    Lock-Free     ┌─────────────────────────┐  │
│  │ Audio Thread│◄────Queues──────►│ AsyncDispatcher         │  │
│  │ (RT-safe)   │                  │ (MPSC param changes)    │  │
│  └─────────────┘                  └───────────┬─────────────┘  │
│                                               │                 │
│  ┌─────────────┐                              │                 │
│  │ GUI Thread  │◄────────────────────────────►│                 │
│  │ (Event loop)│                              │                 │
│  └─────────────┘                              │                 │
│                                               │                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    Worker Pool                          │   │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐ │   │
│  │  │ AI Worker│  │ Analysis │  │ Tagging  │  │  Export  │ │   │
│  │  └──────────┘  └──────────┘  └──────────┘  └──────────┘ │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ┌─────────────┐                  ┌─────────────────────────┐  │
│  │ NetSession  │                  │ GPU Queue               │  │
│  │ (CRDT sync) │                  │ (Offload dispatcher)    │  │
│  └─────────────┘                  └─────────────────────────┘  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Thread Responsibilities

| Thread | Responsibilities | Constraints |
|--------|-----------------|-------------|
| Audio | DSP processing, buffer management | No allocations, no locks, no syscalls |
| GUI | User interaction, rendering | May allocate, responsive to events |
| WorkerPool | AI inference, analysis, export | Background tasks, interruptible |
| NetSession | CRDT synchronization, network I/O | Async I/O, conflict resolution |
| GPUQueue | Compute shader dispatch | Batched execution, latency budgeting |

## Cross-System Interaction Map

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│  ParamRegistry  │◄───►│   ModMatrix     │◄───►│ AutomationClip  │
│  (Signal graph) │     │ (Mod sources)   │     │ (Layer stack)   │
└────────┬────────┘     └────────┬────────┘     └────────┬────────┘
         │                       │                       │
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────────────────────────────────────────────────────┐
│                        Audio Graph                              │
│  (Processes nodes in topological order)                         │
└─────────────────────────────────────────────────────────────────┘
         │                                               │
         ▼                                               ▼
┌─────────────────┐                            ┌─────────────────┐
│ PerformanceAdv. │                            │  OffloadManager │
│ (Quality tiers) │                            │  (GPU/CPU)      │
└─────────────────┘                            └─────────────────┘

┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│  PatternCRDT    │◄───►│ SessionServer   │◄───►│ CollabOverlay   │
│  (Note ops)     │     │ (Network sync)  │     │ (UI cursors)    │
└─────────────────┘     └─────────────────┘     └─────────────────┘

┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│ HarmonicAnalyzer│◄───►│FeatureExtractor │◄───►│  TagClassifier  │
│ (Pitch classes) │     │ (Spectral data) │     │  (ML inference) │
└─────────────────┘     └─────────────────┘     └─────────────────┘
         │                                               │
         ▼                                               ▼
┌─────────────────┐                            ┌─────────────────┐
│ArrangementAnalyz│                            │     AssetDB     │
│ (Structure)     │                            │ (Sample library)│
└─────────────────┘                            └─────────────────┘

┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   UndoService   │◄───►│  StateHasher    │◄───►│VersionStore    │
│ (Command stack) │     │ (Integrity)     │     │ (Snapshots)     │
└─────────────────┘     └─────────────────┘     └─────────────────┘

┌─────────────────┐     ┌─────────────────┐
│ SandboxRunner   │◄───►│ PluginInspector │
│ (Process isol.) │     │ (Latency diff)  │
└─────────────────┘     └─────────────────┘
```

## Risk Ledger

| Risk | Impact | Mitigation |
|------|--------|------------|
| Large PR size | Review complexity | Clear module separation, stub status documentation |
| Feature creep | Partial complexity merging | Explicit stub markers, phased implementation |
| Compile time increase | Developer productivity | Modular targets, interface libraries |
| GPU availability | Runtime failures | Graceful CPU fallback, feature detection |
| Network latency | Collaboration lag | Optimistic local updates, CRDT convergence |
| Plugin crashes | DAW instability | Process isolation, watchdog timeouts |
| AI model size | Memory pressure | Lazy loading, quality tier negotiation |

## Build Configuration

The mega integration introduces optional feature flags:

| Flag | Default | Description |
|------|---------|-------------|
| `ENABLE_GPU` | OFF | Enable Vulkan-based GPU DSP offload |
| `ENABLE_COLLAB` | OFF | Enable collaborative session features |
| `ENABLE_AI` | OFF | Enable AI-powered features |
| `ENABLE_SANDBOX` | OFF | Enable plugin sandboxing |
| `ENABLE_PERF_ADAPTIVE` | ON | Enable adaptive performance mode |

When features are disabled, stub implementations return graceful no-ops or defaults.

## Acceptance Criteria Summary

- [ ] Compilation succeeds with all ENABLE_* permutations
- [ ] Unit tests pass locally and in CI
- [ ] No blocking allocations in audio path (code inspection/runtime asserts)
- [ ] Parameter graph prevents cycles
- [ ] Automation layer merge is deterministic
- [ ] CRDT operations yield consistent ordering
- [ ] Undo operations restore project snapshot equivalence
- [ ] PerformanceAdvisor triggers downgrade at >75% block budget

## Non-Goals

- Full production-ready GPU kernels (scaffolding only)
- Real network server (mock/loopback only)
- Complete ML models (placeholders)
- Final visual polish (functional prototypes only)
