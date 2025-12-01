/**
 * @file BenchmarkUIPipeline.cpp
 * @brief Performance benchmarks for UI pipeline
 */

#include "ui/core/reactive/Signal.hpp"
#include "ui/core/reactive/ParameterSignal.hpp"
#include "ui/core/diagnostics/DiagnosticsOverlay.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

using namespace daw::ui::reactive;
using namespace daw::ui::diagnostics;

namespace
{

// Performance targets
constexpr float TARGET_MEAN_FRAME_MS = 4.0f;
constexpr float TARGET_P99_FRAME_MS = 12.0f;

struct BenchmarkResult
{
    float meanMs{0.0f};
    float p99Ms{0.0f};
    float minMs{0.0f};
    float maxMs{0.0f};
    int samples{0};
};

BenchmarkResult runBenchmark(const std::string& name, std::function<void()> workload, int iterations = 500)
{
    std::vector<float> times;
    times.reserve(static_cast<std::size_t>(iterations));
    
    // Warmup
    for (int i = 0; i < 10; ++i) {
        workload();
    }
    
    // Measure
    for (int i = 0; i < iterations; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        workload();
        auto end = std::chrono::high_resolution_clock::now();
        float ms = std::chrono::duration<float, std::milli>(end - start).count();
        times.push_back(ms);
    }
    
    // Calculate statistics
    std::sort(times.begin(), times.end());
    
    BenchmarkResult result;
    result.samples = iterations;
    result.minMs = times.front();
    result.maxMs = times.back();
    
    float sum = 0.0f;
    for (float t : times) sum += t;
    result.meanMs = sum / static_cast<float>(times.size());
    
    std::size_t p99Index = static_cast<std::size_t>(times.size() * 0.99);
    result.p99Ms = times[p99Index];
    
    std::cout << "  " << name << ":\n"
              << "    Mean: " << result.meanMs << " ms\n"
              << "    P99:  " << result.p99Ms << " ms\n"
              << "    Min:  " << result.minMs << " ms\n"
              << "    Max:  " << result.maxMs << " ms\n";
    
    return result;
}

void benchmarkSignalFlush()
{
    std::cout << "\n=== Signal Flush Benchmark ===" << std::endl;
    
    SignalAggregator aggregator;
    std::vector<std::unique_ptr<Signal<float>>> signals;
    std::vector<Subscription> subscriptions;  // Keep subscriptions alive
    
    // Create 100 signals
    for (int i = 0; i < 100; ++i) {
        auto sig = std::make_unique<Signal<float>>(0.0f);
        subscriptions.push_back(sig->subscribe([](float) {}));  // Keep subscription
        aggregator.registerSignal(sig.get());
        signals.push_back(std::move(sig));
    }
    
    auto result = runBenchmark("100 signals flush", [&]() {
        // Dirty half the signals
        for (std::size_t i = 0; i < signals.size(); i += 2) {
            signals[i]->set(static_cast<float>(i));
        }
        aggregator.flush();
    });
    
    (void)result;  // Suppress warning when NDEBUG
    assert(result.meanMs < 1.0f && "Signal flush should be < 1ms");
}

void benchmarkNoteVirtualization()
{
    std::cout << "\n=== Note Virtualization Benchmark ===" << std::endl;
    
    NoteCollectionSignal notes;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> pitchDist(36, 96);
    std::uniform_real_distribution<double> beatDist(0, 400);
    
    // Create 100,000 notes
    std::cout << "  Creating 100,000 notes..." << std::endl;
    for (int i = 0; i < 100000; ++i) {
        notes.addNote({
            .pitch = pitchDist(rng),
            .startBeats = beatDist(rng),
            .lengthBeats = 0.5
        });
    }
    notes.flush();
    
    auto result = runBenchmark("Query visible (4 bars)", [&]() {
        auto visible = notes.getVisibleNotes(0, 16, 48, 72);
        // Simulate drawing
        volatile std::size_t count = visible.size();
        (void)count;
    });
    
    (void)result;  // Suppress warning when NDEBUG
    assert(result.meanMs < 2.0f && "Visible query should be < 2ms");
}

void benchmarkLargeDatasetScroll()
{
    std::cout << "\n=== Large Dataset Scroll Benchmark ===" << std::endl;
    
    // Create 100 patterns with 10,000 notes each
    std::vector<NoteCollectionSignal> patterns(100);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> pitchDist(36, 96);
    
    std::cout << "  Creating 100 patterns x 10,000 notes..." << std::endl;
    for (auto& pattern : patterns) {
        for (int i = 0; i < 10000; ++i) {
            pattern.addNote({
                .pitch = pitchDist(rng),
                .startBeats = static_cast<double>(i) * 0.25,
                .lengthBeats = 0.5
            });
        }
        pattern.flush();
    }
    
    // Simulate scrolling through patterns
    double scrollPos = 0;
    
    auto result = runBenchmark("Scroll simulation", [&]() {
        // Query 3 visible patterns
        int startPattern = static_cast<int>(scrollPos) % 97;
        std::size_t totalVisible = 0;
        
        for (int p = startPattern; p < startPattern + 3; ++p) {
            auto visible = patterns[static_cast<std::size_t>(p)].getVisibleNotes(0, 16, 48, 72);
            totalVisible += visible.size();
        }
        
        scrollPos += 0.1;
        volatile std::size_t count = totalVisible;
        (void)count;
    });
    
    (void)result;  // Suppress warning when NDEBUG
    assert(result.p99Ms < TARGET_P99_FRAME_MS && "Scroll P99 should meet target");
}

void benchmarkFullFrameSimulation()
{
    std::cout << "\n=== Full Frame Simulation ===" << std::endl;
    
    // Setup systems
    SignalAggregator aggregator;
    DiagnosticsManager diagnostics;
    
    // Create signals for transport, mixer, etc.
    Signal<double> playheadSignal(0.0);
    Signal<float> bpmSignal(120.0f);
    std::vector<std::unique_ptr<Signal<float>>> meterSignals;
    
    for (int i = 0; i < 32; ++i) {
        auto signal = std::make_unique<Signal<float>>(0.0f);
        aggregator.registerSignal(signal.get());
        meterSignals.push_back(std::move(signal));
    }
    
    aggregator.registerSignal(&playheadSignal);
    aggregator.registerSignal(&bpmSignal);
    
    // Create note data
    NoteCollectionSignal notes;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> pitchDist(36, 96);
    
    for (int i = 0; i < 10000; ++i) {
        notes.addNote({
            .pitch = pitchDist(rng),
            .startBeats = static_cast<double>(i) * 0.25,
            .lengthBeats = 0.5
        });
    }
    notes.flush();
    
    double playhead = 0;
    
    auto result = runBenchmark("Full frame simulation", [&]() {
        diagnostics.beginFrame();
        
        // Update signals (simulating audio thread updates)
        playheadSignal.set(playhead);
        for (auto& meter : meterSignals) {
            meter->set(static_cast<float>(rand()) / RAND_MAX);
        }
        
        // Flush all signals
        aggregator.flush();
        notes.flush();
        
        // Query visible notes
        auto visible = notes.getVisibleNotes(playhead, playhead + 16, 48, 72);
        
        // Simulate draw calls
        volatile std::size_t drawCount = visible.size() + meterSignals.size();
        (void)drawCount;
        
        diagnostics.setDrawCalls(static_cast<int>(drawCount));
        diagnostics.setVisibleNotes(static_cast<int>(visible.size()));
        
        diagnostics.endFrame();
        
        playhead += 0.01;
    });
    
    std::cout << "\n  PERFORMANCE TARGETS:\n"
              << "    Mean < " << TARGET_MEAN_FRAME_MS << " ms: " 
              << (result.meanMs < TARGET_MEAN_FRAME_MS ? "PASS" : "FAIL") << "\n"
              << "    P99 < " << TARGET_P99_FRAME_MS << " ms: "
              << (result.p99Ms < TARGET_P99_FRAME_MS ? "PASS" : "FAIL") << "\n";
    
    assert(result.meanMs < TARGET_MEAN_FRAME_MS && "Mean frame time should meet target");
    assert(result.p99Ms < TARGET_P99_FRAME_MS && "P99 frame time should meet target");
}

void benchmarkVirtualizationAccuracy()
{
    std::cout << "\n=== Virtualization Accuracy Test ===" << std::endl;
    
    NoteCollectionSignal notes;
    std::mt19937 rng(42);
    
    // Create uniform distribution of notes
    for (int bar = 0; bar < 100; ++bar) {
        for (int note = 0; note < 100; ++note) {
            notes.addNote({
                .pitch = 48 + (note % 24),
                .startBeats = static_cast<double>(bar * 4 + (note % 4)),
                .lengthBeats = 0.5
            });
        }
    }
    notes.flush();
    
    std::size_t totalNotes = notes.size();
    
    // Query 4-bar window (4% of total)
    auto visible = notes.getVisibleNotes(0, 16, 48, 72);
    std::size_t visibleCount = visible.size();
    
    // Expected: ~400 notes visible (4% of 10000)
    // Allow 1% margin = +/- 100 notes
    double expectedRatio = 4.0 / 100.0;
    double actualRatio = static_cast<double>(visibleCount) / static_cast<double>(totalNotes);
    double marginRatio = std::abs(actualRatio - expectedRatio);
    
    std::cout << "  Total notes: " << totalNotes << "\n"
              << "  Visible notes: " << visibleCount << "\n"
              << "  Expected ratio: " << (expectedRatio * 100) << "%\n"
              << "  Actual ratio: " << (actualRatio * 100) << "%\n"
              << "  Margin: " << (marginRatio * 100) << "%\n"
              << "  Result: " << (marginRatio < 0.01 ? "PASS" : "FAIL") << "\n";
    
    assert(marginRatio < 0.01 && "Virtualization margin should be < 1%");
}

} // anonymous namespace

int main()
{
    std::cout << "=== UI Pipeline Benchmarks ===" << std::endl;
    std::cout << "Targets: Mean < " << TARGET_MEAN_FRAME_MS << "ms, P99 < " 
              << TARGET_P99_FRAME_MS << "ms" << std::endl;
    
    try {
        benchmarkSignalFlush();
        benchmarkNoteVirtualization();
        benchmarkLargeDatasetScroll();
        benchmarkFullFrameSimulation();
        benchmarkVirtualizationAccuracy();
        
        std::cout << "\n=== All benchmarks PASSED ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Benchmark FAILED with exception: " << e.what() << std::endl;
        return 1;
    }
}
