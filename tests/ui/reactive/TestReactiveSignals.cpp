/**
 * @file TestReactiveSignals.cpp
 * @brief Tests for reactive signal system
 */

#include "ui/core/reactive/Signal.hpp"
#include "ui/core/reactive/ParameterSignal.hpp"
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>

using namespace daw::ui::reactive;

namespace
{

void testBasicSignal()
{
    std::cout << "Testing basic Signal<T>..." << std::endl;
    
    Signal<int> signal(0);
    int callCount = 0;
    int lastValue = 0;
    
    auto sub = signal.subscribe([&](int value) {
        callCount++;
        lastValue = value;
    });
    
    // Set value
    signal.set(42);
    assert(signal.isDirty() && "Signal should be dirty after set");
    assert(callCount == 0 && "Callback should not fire before flush");
    
    // Flush
    signal.flush();
    assert(!signal.isDirty() && "Signal should not be dirty after flush");
    assert(callCount == 1 && "Callback should fire once after flush");
    assert(lastValue == 42 && "Callback should receive new value");
    
    // Set same value should not trigger callback
    signal.set(42);
    signal.flush();
    assert(callCount == 1 && "Callback should not fire for same value");
    
    std::cout << "  PASSED" << std::endl;
}

void testSignalCoalescing()
{
    std::cout << "Testing signal coalescing..." << std::endl;
    
    Signal<int> signal(0);
    int callCount = 0;
    int lastValue = 0;
    
    auto sub = signal.subscribe([&](int value) {
        callCount++;
        lastValue = value;
    });
    
    // Multiple updates before flush
    for (int i = 1; i <= 50; ++i) {
        signal.set(i);
    }
    
    // Only last value should be delivered
    signal.flush();
    assert(callCount == 1 && "Only one callback for coalesced updates");
    assert(lastValue == 50 && "Should receive final coalesced value");
    
    std::cout << "  PASSED" << std::endl;
}

void testSignalAggregator()
{
    std::cout << "Testing SignalAggregator..." << std::endl;
    
    SignalAggregator aggregator;
    Signal<int> signal1(0);
    Signal<float> signal2(0.0f);
    
    aggregator.registerSignal(&signal1);
    aggregator.registerSignal(&signal2);
    
    int count1 = 0, count2 = 0;
    auto sub1 = signal1.subscribe([&](int) { count1++; });
    auto sub2 = signal2.subscribe([&](float) { count2++; });
    
    signal1.set(1);
    signal2.set(1.0f);
    
    // Single flush for all signals
    aggregator.flush();
    
    assert(count1 == 1 && "Signal 1 should fire");
    assert(count2 == 1 && "Signal 2 should fire");
    
    // Unregister and verify
    aggregator.unregisterSignal(&signal1);
    signal1.set(2);
    aggregator.flush();
    
    assert(count1 == 1 && "Unregistered signal should not flush via aggregator");
    
    // Manual flush still works
    signal1.flush();
    assert(count1 == 2 && "Manual flush should work after unregister");
    
    std::cout << "  PASSED" << std::endl;
}

void testSubscriptionLifetime()
{
    std::cout << "Testing subscription lifetime..." << std::endl;
    
    Signal<int> signal(0);
    int callCount = 0;
    
    {
        auto sub = signal.subscribe([&](int) { callCount++; });
        signal.set(1);
        signal.flush();
        assert(callCount == 1 && "Callback fires with active subscription");
    }
    // Subscription destroyed
    
    signal.set(2);
    signal.flush();
    assert(callCount == 1 && "Callback should not fire after subscription destroyed");
    
    std::cout << "  PASSED" << std::endl;
}

void testThreadSafety()
{
    std::cout << "Testing thread safety..." << std::endl;
    
    Signal<int> signal(0);
    std::atomic<int> callCount{0};
    
    auto sub = signal.subscribe([&](int) { callCount++; });
    
    // Writer thread
    std::thread writer([&]() {
        for (int i = 0; i < 1000; ++i) {
            signal.set(i);
        }
    });
    
    // Flush thread
    std::thread flusher([&]() {
        for (int i = 0; i < 100; ++i) {
            signal.flush();
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });
    
    writer.join();
    flusher.join();
    signal.flush();  // Final flush
    
    // Should not crash, exact call count depends on timing
    assert(callCount >= 1 && "At least one callback should fire");
    
    std::cout << "  PASSED (no crashes, " << callCount << " callbacks)" << std::endl;
}

void testLockFreeQueue()
{
    std::cout << "Testing LockFreeQueue..." << std::endl;
    
    LockFreeQueue<int, 64> queue;
    
    // Basic push/pop
    assert(queue.tryPush(1) && "Push should succeed");
    assert(queue.tryPush(2) && "Push should succeed");
    
    auto v1 = queue.tryPop();
    assert(v1.has_value() && *v1 == 1 && "Pop should return 1");
    
    auto v2 = queue.tryPop();
    assert(v2.has_value() && *v2 == 2 && "Pop should return 2");
    
    auto v3 = queue.tryPop();
    assert(!v3.has_value() && "Pop from empty should return nullopt");
    
    // Fill to capacity
    for (int i = 0; i < 63; ++i) {
        assert(queue.tryPush(i) && "Push should succeed until full");
    }
    assert(!queue.tryPush(999) && "Push to full queue should fail");
    
    // Drain
    while (queue.tryPop().has_value()) {}
    assert(queue.empty() && "Queue should be empty after drain");
    
    std::cout << "  PASSED" << std::endl;
}

void testNoteCollectionSignal()
{
    std::cout << "Testing NoteCollectionSignal..." << std::endl;
    
    NoteCollectionSignal notes;
    int callCount = 0;
    
    auto sub = notes.subscribe([&](const auto&) { callCount++; });
    
    // Add notes
    notes.addNote({.pitch = 60, .startBeats = 0, .lengthBeats = 1});
    notes.addNote({.pitch = 64, .startBeats = 1, .lengthBeats = 1});
    notes.addNote({.pitch = 67, .startBeats = 2, .lengthBeats = 1});
    
    notes.flush();
    assert(callCount == 1 && "One callback for batch adds");
    assert(notes.size() == 3 && "Should have 3 notes");
    
    // Query visible
    auto visible = notes.getVisibleNotes(0.5, 1.5, 60, 70);
    assert(visible.size() == 2 && "Should see 2 notes in range");
    
    // Update note
    auto allNotes = notes.getNotes();
    notes.updateNote({.id = allNotes[0].id, .pitch = 61, .startBeats = 0, .lengthBeats = 1});
    notes.flush();
    assert(notes.getNotes()[0].pitch == 61 && "Note should be updated");
    
    // Remove note
    notes.removeNote(allNotes[1].id);
    notes.flush();
    assert(notes.size() == 2 && "Should have 2 notes after removal");
    
    std::cout << "  PASSED" << std::endl;
}

void testParameterSignal()
{
    std::cout << "Testing ParameterSignal..." << std::endl;
    
    ParameterSignal param("volume", "Volume", 0.0f, 1.0f, 0.8f);
    
    assert(std::abs(param.getValue() - 0.8f) < 0.01f && "Default value should be 0.8");
    
    param.setValue(0.5f);
    param.flush();
    assert(std::abs(param.getValue() - 0.5f) < 0.01f && "Value should be 0.5");
    
    // Clamping
    param.setValue(1.5f);
    param.flush();
    assert(std::abs(param.getValue() - 1.0f) < 0.01f && "Value should clamp to 1.0");
    
    param.setValue(-0.5f);
    param.flush();
    assert(std::abs(param.getValue() - 0.0f) < 0.01f && "Value should clamp to 0.0");
    
    // Normalized
    param.setNormalized(0.5f);
    param.flush();
    assert(std::abs(param.getValue() - 0.5f) < 0.01f && "Normalized 0.5 should be 0.5");
    assert(std::abs(param.getNormalized() - 0.5f) < 0.01f && "getNormalized should return 0.5");
    
    std::cout << "  PASSED" << std::endl;
}

void testCoalescingPerformance()
{
    std::cout << "Testing coalescing performance..." << std::endl;
    
    Signal<int> signal(0);
    int callCount = 0;
    
    auto sub = signal.subscribe([&](int) { callCount++; });
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Simulate 50+ updates per frame
    for (int frame = 0; frame < 100; ++frame) {
        for (int update = 0; update < 60; ++update) {
            signal.set(frame * 60 + update);
        }
        signal.flush();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    // 6000 updates coalesced into 100 callbacks
    assert(callCount == 100 && "Should have exactly 100 callbacks (one per frame)");
    
    std::cout << "  PASSED (6000 updates -> 100 callbacks in " 
              << durationUs << "us)" << std::endl;
}

} // anonymous namespace

int main()
{
    std::cout << "=== Reactive Signal Tests ===" << std::endl;
    
    try {
        testBasicSignal();
        testSignalCoalescing();
        testSignalAggregator();
        testSubscriptionLifetime();
        testThreadSafety();
        testLockFreeQueue();
        testNoteCollectionSignal();
        testParameterSignal();
        testCoalescingPerformance();
        
        std::cout << "\n=== All tests PASSED ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test FAILED with exception: " << e.what() << std::endl;
        return 1;
    }
}
