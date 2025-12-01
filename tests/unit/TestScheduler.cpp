/**
 * @file TestScheduler.cpp
 * @brief Unit tests for the Scheduler class including warp mapping and beat-to-frame conversion.
 */

#include "engine/Scheduler.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

namespace {

constexpr double kEpsilon = 1e-6;

[[maybe_unused]]
bool approxEqual(double a, double b, double epsilon = kEpsilon) {
    return std::abs(a - b) < epsilon;
}

void testDefaultConfiguration() {
    std::cout << "  Testing default configuration..." << std::endl;
    
    cppmusic::engine::Scheduler scheduler;
    
    assert(approxEqual(scheduler.getSampleRate(), 44100.0));
    assert(approxEqual(scheduler.getTempo(), 120.0));
    assert(scheduler.getTimeSignatureNumerator() == 4);
    assert(scheduler.getTimeSignatureDenominator() == 4);
    
    std::cout << "    PASSED" << std::endl;
}

void testBeatsToFrames() {
    std::cout << "  Testing beatsToFrames..." << std::endl;
    
    cppmusic::engine::Scheduler scheduler;
    scheduler.setSampleRate(44100.0);
    scheduler.setTempo(120.0);
    
    // At 120 BPM, 1 beat = 0.5 seconds = 22050 samples
    assert(scheduler.beatsToFrames(1.0) == 22050);
    
    // 4 beats = 2 seconds = 88200 samples
    assert(scheduler.beatsToFrames(4.0) == 88200);
    
    // 0.5 beats
    assert(scheduler.beatsToFrames(0.5) == 11025);
    
    std::cout << "    PASSED" << std::endl;
}

void testFramesToBeats() {
    std::cout << "  Testing framesToBeats..." << std::endl;
    
    cppmusic::engine::Scheduler scheduler;
    scheduler.setSampleRate(44100.0);
    scheduler.setTempo(120.0);
    
    // 22050 samples = 1 beat at 120 BPM
    assert(approxEqual(scheduler.framesToBeats(22050), 1.0));
    
    // 88200 samples = 4 beats
    assert(approxEqual(scheduler.framesToBeats(88200), 4.0));
    
    std::cout << "    PASSED" << std::endl;
}

void testTempoChange() {
    std::cout << "  Testing tempo changes..." << std::endl;
    
    cppmusic::engine::Scheduler scheduler;
    scheduler.setSampleRate(44100.0);
    
    // At 60 BPM, 1 beat = 1 second = 44100 samples
    scheduler.setTempo(60.0);
    assert(scheduler.beatsToFrames(1.0) == 44100);
    
    // At 240 BPM, 1 beat = 0.25 seconds = 11025 samples
    scheduler.setTempo(240.0);
    assert(scheduler.beatsToFrames(1.0) == 11025);
    
    std::cout << "    PASSED" << std::endl;
}

void testPatternBeat() {
    std::cout << "  Testing getPatternBeat for polymeter..." << std::endl;
    
    // Pattern of 8 beats
    constexpr double patternLength = 8.0;
    
    // Global beat 0 -> pattern beat 0
    if (!approxEqual(cppmusic::engine::Scheduler::getPatternBeat(0.0, patternLength), 0.0)) {
        std::cerr << "FAILED: getPatternBeat(0.0, 8.0)" << std::endl;
        std::abort();
    }
    
    // Global beat 4 -> pattern beat 4
    if (!approxEqual(cppmusic::engine::Scheduler::getPatternBeat(4.0, patternLength), 4.0)) {
        std::cerr << "FAILED: getPatternBeat(4.0, 8.0)" << std::endl;
        std::abort();
    }
    
    // Global beat 8 -> pattern beat 0 (looped)
    if (!approxEqual(cppmusic::engine::Scheduler::getPatternBeat(8.0, patternLength), 0.0)) {
        std::cerr << "FAILED: getPatternBeat(8.0, 8.0)" << std::endl;
        std::abort();
    }
    
    // Global beat 10 -> pattern beat 2 (looped)
    if (!approxEqual(cppmusic::engine::Scheduler::getPatternBeat(10.0, patternLength), 2.0)) {
        std::cerr << "FAILED: getPatternBeat(10.0, 8.0)" << std::endl;
        std::abort();
    }
    
    // Global beat 17.5 -> pattern beat 1.5
    if (!approxEqual(cppmusic::engine::Scheduler::getPatternBeat(17.5, patternLength), 1.5)) {
        std::cerr << "FAILED: getPatternBeat(17.5, 8.0)" << std::endl;
        std::abort();
    }
    
    std::cout << "    PASSED" << std::endl;
}

void testLoopIteration() {
    std::cout << "  Testing getLoopIteration..." << std::endl;
    
    constexpr double patternLength = 8.0;
    
    // Beat 0 -> iteration 0
    if (cppmusic::engine::Scheduler::getLoopIteration(0.0, patternLength) != 0) {
        std::cerr << "FAILED: getLoopIteration(0.0, 8.0)" << std::endl;
        std::abort();
    }
    
    // Beat 7 -> iteration 0
    if (cppmusic::engine::Scheduler::getLoopIteration(7.0, patternLength) != 0) {
        std::cerr << "FAILED: getLoopIteration(7.0, 8.0)" << std::endl;
        std::abort();
    }
    
    // Beat 8 -> iteration 1
    if (cppmusic::engine::Scheduler::getLoopIteration(8.0, patternLength) != 1) {
        std::cerr << "FAILED: getLoopIteration(8.0, 8.0)" << std::endl;
        std::abort();
    }
    
    // Beat 24 -> iteration 3
    if (cppmusic::engine::Scheduler::getLoopIteration(24.0, patternLength) != 3) {
        std::cerr << "FAILED: getLoopIteration(24.0, 8.0)" << std::endl;
        std::abort();
    }
    
    std::cout << "    PASSED" << std::endl;
}

void testWarpMapLinear() {
    std::cout << "  Testing WarpMap with linear mapping..." << std::endl;
    
    cppmusic::engine::WarpMap warpMap;
    
    // Create 1:1 linear mapping
    warpMap.addMarker({0.0, 0.0});
    warpMap.addMarker({8.0, 8.0});
    
    assert(warpMap.isActive());
    
    // Source beat should equal target beat
    assert(approxEqual(warpMap.sourceToTarget(0.0), 0.0));
    assert(approxEqual(warpMap.sourceToTarget(4.0), 4.0));
    assert(approxEqual(warpMap.sourceToTarget(8.0), 8.0));
    
    std::cout << "    PASSED" << std::endl;
}

void testWarpMapStretched() {
    std::cout << "  Testing WarpMap with time stretch..." << std::endl;
    
    cppmusic::engine::WarpMap warpMap;
    
    // Create 2x time stretch (8 source beats -> 16 target beats)
    warpMap.addMarker({0.0, 0.0});
    warpMap.addMarker({8.0, 16.0});
    
    // Source beat 0 -> target beat 0
    assert(approxEqual(warpMap.sourceToTarget(0.0), 0.0));
    
    // Source beat 4 -> target beat 8 (half speed)
    assert(approxEqual(warpMap.sourceToTarget(4.0), 8.0));
    
    // Source beat 8 -> target beat 16
    assert(approxEqual(warpMap.sourceToTarget(8.0), 16.0));
    
    std::cout << "    PASSED" << std::endl;
}

void testWarpMapPiecewise() {
    std::cout << "  Testing WarpMap with piecewise mapping..." << std::endl;
    
    cppmusic::engine::WarpMap warpMap;
    
    // Create non-linear mapping with speed change at beat 4
    warpMap.addMarker({0.0, 0.0});
    warpMap.addMarker({4.0, 2.0});  // First half at 2x speed
    warpMap.addMarker({8.0, 8.0});  // Second half at normal speed
    
    // Source beat 0 -> target beat 0
    assert(approxEqual(warpMap.sourceToTarget(0.0), 0.0));
    
    // Source beat 2 -> target beat 1 (half of first segment)
    assert(approxEqual(warpMap.sourceToTarget(2.0), 1.0));
    
    // Source beat 4 -> target beat 2
    assert(approxEqual(warpMap.sourceToTarget(4.0), 2.0));
    
    // Source beat 6 -> target beat 5 (midpoint of second segment)
    assert(approxEqual(warpMap.sourceToTarget(6.0), 5.0));
    
    // Source beat 8 -> target beat 8
    assert(approxEqual(warpMap.sourceToTarget(8.0), 8.0));
    
    std::cout << "    PASSED" << std::endl;
}

void testWarpMapInverse() {
    std::cout << "  Testing WarpMap inverse mapping..." << std::endl;
    
    cppmusic::engine::WarpMap warpMap;
    
    // Create 2x time stretch
    warpMap.addMarker({0.0, 0.0});
    warpMap.addMarker({8.0, 16.0});
    
    // Target beat 0 -> source beat 0
    assert(approxEqual(warpMap.targetToSource(0.0), 0.0));
    
    // Target beat 8 -> source beat 4
    assert(approxEqual(warpMap.targetToSource(8.0), 4.0));
    
    // Target beat 16 -> source beat 8
    assert(approxEqual(warpMap.targetToSource(16.0), 8.0));
    
    std::cout << "    PASSED" << std::endl;
}

void testMicroTiming() {
    std::cout << "  Testing applyMicroTiming..." << std::endl;
    
    // Positive offset
    assert(cppmusic::engine::Scheduler::applyMicroTiming(1000, 50) == 1050);
    
    // Negative offset
    assert(cppmusic::engine::Scheduler::applyMicroTiming(1000, -50) == 950);
    
    // Negative offset clamped to zero
    assert(cppmusic::engine::Scheduler::applyMicroTiming(30, -50) == 0);
    
    std::cout << "    PASSED" << std::endl;
}

} // anonymous namespace

int main() {
    std::cout << "Running Scheduler unit tests..." << std::endl;
    
    testDefaultConfiguration();
    testBeatsToFrames();
    testFramesToBeats();
    testTempoChange();
    testPatternBeat();
    testLoopIteration();
    testWarpMapLinear();
    testWarpMapStretched();
    testWarpMapPiecewise();
    testWarpMapInverse();
    testMicroTiming();
    
    std::cout << "\nAll Scheduler tests PASSED!" << std::endl;
    return 0;
}
