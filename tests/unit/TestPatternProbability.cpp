/**
 * @file TestPatternProbability.cpp
 * @brief Unit tests for Pattern probability, conditions, and swing evaluation.
 */

#include "model/Pattern.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

namespace {

constexpr double kEpsilon = 1e-6;

[[maybe_unused]]
bool approxEqual(double a, double b, double epsilon = kEpsilon) {
    return std::abs(a - b) < epsilon;
}

void testConditionAlways() {
    std::cout << "  Testing NoteCondition::Always..." << std::endl;
    
    cppmusic::model::NoteEvent note;
    note.probability = 1.0f;
    note.condition = cppmusic::model::NoteCondition::Always;
    
    // Should always play
    for (uint32_t i = 0; i < 10; ++i) {
        assert(cppmusic::model::Pattern::evaluateNoteCondition(note, i, 12345));
    }
    
    std::cout << "    PASSED" << std::endl;
}

void testConditionFirstOnly() {
    std::cout << "  Testing NoteCondition::FirstOnly..." << std::endl;
    
    cppmusic::model::NoteEvent note;
    note.probability = 1.0f;
    note.condition = cppmusic::model::NoteCondition::FirstOnly;
    
    // Should play only on first iteration (0)
    assert(cppmusic::model::Pattern::evaluateNoteCondition(note, 0, 12345));
    assert(!cppmusic::model::Pattern::evaluateNoteCondition(note, 1, 12345));
    assert(!cppmusic::model::Pattern::evaluateNoteCondition(note, 2, 12345));
    
    std::cout << "    PASSED" << std::endl;
}

void testConditionNth() {
    std::cout << "  Testing NoteCondition::Nth (every Nth)..." << std::endl;
    
    cppmusic::model::NoteEvent note;
    note.probability = 1.0f;
    note.condition = cppmusic::model::NoteCondition::Nth;
    note.conditionParam = 3;  // Every 3rd iteration
    
    // Should play on iterations 2, 5, 8, ... (3rd, 6th, 9th)
    assert(!cppmusic::model::Pattern::evaluateNoteCondition(note, 0, 12345));  // 1st
    assert(!cppmusic::model::Pattern::evaluateNoteCondition(note, 1, 12345));  // 2nd
    assert(cppmusic::model::Pattern::evaluateNoteCondition(note, 2, 12345));   // 3rd
    assert(!cppmusic::model::Pattern::evaluateNoteCondition(note, 3, 12345));  // 4th
    assert(!cppmusic::model::Pattern::evaluateNoteCondition(note, 4, 12345));  // 5th
    assert(cppmusic::model::Pattern::evaluateNoteCondition(note, 5, 12345));   // 6th
    
    std::cout << "    PASSED" << std::endl;
}

void testConditionEveryN() {
    std::cout << "  Testing NoteCondition::EveryN (every N starting from 0)..." << std::endl;
    
    cppmusic::model::NoteEvent note;
    note.probability = 1.0f;
    note.condition = cppmusic::model::NoteCondition::EveryN;
    note.conditionParam = 2;  // Every 2nd iteration starting from 0
    
    // Should play on iterations 0, 2, 4, 6, ...
    assert(cppmusic::model::Pattern::evaluateNoteCondition(note, 0, 12345));
    assert(!cppmusic::model::Pattern::evaluateNoteCondition(note, 1, 12345));
    assert(cppmusic::model::Pattern::evaluateNoteCondition(note, 2, 12345));
    assert(!cppmusic::model::Pattern::evaluateNoteCondition(note, 3, 12345));
    assert(cppmusic::model::Pattern::evaluateNoteCondition(note, 4, 12345));
    
    std::cout << "    PASSED" << std::endl;
}

void testConditionSkipM() {
    std::cout << "  Testing NoteCondition::SkipM..." << std::endl;
    
    cppmusic::model::NoteEvent note;
    note.probability = 1.0f;
    note.condition = cppmusic::model::NoteCondition::SkipM;
    note.conditionParam = 3;  // Skip first 3 iterations
    
    // Should not play on iterations 0, 1, 2
    assert(!cppmusic::model::Pattern::evaluateNoteCondition(note, 0, 12345));
    assert(!cppmusic::model::Pattern::evaluateNoteCondition(note, 1, 12345));
    assert(!cppmusic::model::Pattern::evaluateNoteCondition(note, 2, 12345));
    // Should play from iteration 3 onwards
    assert(cppmusic::model::Pattern::evaluateNoteCondition(note, 3, 12345));
    assert(cppmusic::model::Pattern::evaluateNoteCondition(note, 4, 12345));
    
    std::cout << "    PASSED" << std::endl;
}

void testProbabilityDeterministic() {
    std::cout << "  Testing deterministic probability evaluation..." << std::endl;
    
    cppmusic::model::NoteEvent note;
    note.pitch = 60;
    note.startBeat = 0.0;
    note.probability = 0.5f;
    note.condition = cppmusic::model::NoteCondition::Always;
    
    // Same seed should produce same result
    uint64_t seed = 12345;
    bool result1 = cppmusic::model::Pattern::evaluateNoteCondition(note, 0, seed);
    bool result2 = cppmusic::model::Pattern::evaluateNoteCondition(note, 0, seed);
    assert(result1 == result2);
    
    // Same seed with same iteration should always produce same result
    for (int i = 0; i < 5; ++i) {
        bool r = cppmusic::model::Pattern::evaluateNoteCondition(note, 0, seed);
        assert(r == result1);
    }
    
    std::cout << "    PASSED" << std::endl;
}

void testProbabilityZero() {
    std::cout << "  Testing zero probability..." << std::endl;
    
    cppmusic::model::NoteEvent note;
    note.probability = 0.0f;
    note.condition = cppmusic::model::NoteCondition::Always;
    
    // Should never play
    for (uint32_t i = 0; i < 10; ++i) {
        assert(!cppmusic::model::Pattern::evaluateNoteCondition(note, i, i * 1000));
    }
    
    std::cout << "    PASSED" << std::endl;
}

void testSwingAdjustment() {
    std::cout << "  Testing swing adjustment..." << std::endl;
    
    cppmusic::model::Pattern pattern;
    pattern.setSwingAmount(0.5f);
    pattern.setSwingResolution(0.5);  // 8th notes
    
    // On-beat note (beat 0) - no swing
    cppmusic::model::NoteEvent onBeat;
    onBeat.startBeat = 0.0;
    double adjusted = pattern.getSwingAdjustedBeat(onBeat);
    assert(approxEqual(adjusted, 0.0));
    
    // Off-beat note (beat 0.5) - should be swung late
    cppmusic::model::NoteEvent offBeat;
    offBeat.startBeat = 0.5;
    adjusted = pattern.getSwingAdjustedBeat(offBeat);
    // With 50% swing on 8th notes (0.5 beat resolution), offset should be 0.125 beats
    assert(adjusted > 0.5);  // Should be later than original
    
    std::cout << "    PASSED" << std::endl;
}

void testSwingOverride() {
    std::cout << "  Testing per-note swing override..." << std::endl;
    
    cppmusic::model::Pattern pattern;
    pattern.setSwingAmount(0.5f);  // Pattern swing
    pattern.setSwingResolution(0.5);
    
    // Note with its own swing override
    cppmusic::model::NoteEvent note;
    note.startBeat = 0.5;  // Off-beat
    note.swingAmount = -0.5f;  // Opposite swing
    
    double adjusted = pattern.getSwingAdjustedBeat(note);
    // Should use note's swing, not pattern's
    assert(adjusted < 0.5);  // Should be earlier than original
    
    std::cout << "    PASSED" << std::endl;
}

void testGetPlayableNotes() {
    std::cout << "  Testing getPlayableNotes with conditions..." << std::endl;
    
    cppmusic::model::Pattern pattern;
    
    // Add note that always plays
    cppmusic::model::NoteEvent note1;
    note1.pitch = 60;
    note1.startBeat = 0.0;
    note1.condition = cppmusic::model::NoteCondition::Always;
    pattern.addNote(note1);
    
    // Add note that plays first only
    cppmusic::model::NoteEvent note2;
    note2.pitch = 64;
    note2.startBeat = 1.0;
    note2.condition = cppmusic::model::NoteCondition::FirstOnly;
    pattern.addNote(note2);
    
    // First iteration - both should play
    auto playable = pattern.getPlayableNotes(0, 12345);
    assert(playable.size() == 2);
    
    // Second iteration - only first note should play
    playable = pattern.getPlayableNotes(1, 12345);
    assert(playable.size() == 1);
    assert(playable[0].pitch == 60);
    
    std::cout << "    PASSED" << std::endl;
}

} // anonymous namespace

int main() {
    std::cout << "Running Pattern probability/conditions unit tests..." << std::endl;
    
    testConditionAlways();
    testConditionFirstOnly();
    testConditionNth();
    testConditionEveryN();
    testConditionSkipM();
    testProbabilityDeterministic();
    testProbabilityZero();
    testSwingAdjustment();
    testSwingOverride();
    testGetPlayableNotes();
    
    std::cout << "\nAll probability/conditions tests PASSED!" << std::endl;
    return 0;
}
