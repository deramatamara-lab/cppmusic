/**
 * @file TestPattern.cpp
 * @brief Unit tests for cppmusic::model::Pattern note storage and length computation.
 *
 * Uses a minimal standalone main() for CTest integration.
 */

#include "model/Pattern.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

namespace {

constexpr double kEpsilon = 1e-9;

bool approxEqual(double a, double b, double epsilon = kEpsilon) {
    return std::abs(a - b) < epsilon;
}

void testDefaultConstruction() {
    std::cout << "  Testing default construction..." << std::endl;
    
    cppmusic::model::Pattern pattern;
    
    assert(pattern.getName() == "Untitled");
    assert(approxEqual(pattern.getLengthBeats(), 16.0));
    assert(pattern.isEmpty());
    assert(pattern.getNoteCount() == 0);
    assert(approxEqual(pattern.computeContentLength(), 16.0));
    
    std::cout << "    PASSED" << std::endl;
}

void testNamedConstruction() {
    std::cout << "  Testing named construction..." << std::endl;
    
    cppmusic::model::Pattern pattern("My Pattern", 2, 4);
    
    assert(pattern.getName() == "My Pattern");
    assert(approxEqual(pattern.getLengthBeats(), 8.0)); // 2 bars * 4 beats
    
    std::cout << "    PASSED" << std::endl;
}

void testAddNote() {
    std::cout << "  Testing addNote..." << std::endl;
    
    cppmusic::model::Pattern pattern;
    
    cppmusic::model::NoteEvent note1;
    note1.pitch = 60;
    note1.velocity = 100;
    note1.startBeat = 0.0;
    note1.durationBeats = 1.0;
    
    pattern.addNote(note1);
    
    assert(!pattern.isEmpty());
    assert(pattern.getNoteCount() == 1);
    
    const auto* retrieved = pattern.getNote(0);
    assert(retrieved != nullptr);
    assert(retrieved->pitch == 60);
    assert(retrieved->velocity == 100);
    assert(approxEqual(retrieved->startBeat, 0.0));
    assert(approxEqual(retrieved->durationBeats, 1.0));
    
    std::cout << "    PASSED" << std::endl;
}

void testNotesSortedByStartBeat() {
    std::cout << "  Testing notes sorted by start beat..." << std::endl;
    
    cppmusic::model::Pattern pattern;
    
    cppmusic::model::NoteEvent note1;
    note1.pitch = 60;
    note1.startBeat = 2.0;
    
    cppmusic::model::NoteEvent note2;
    note2.pitch = 62;
    note2.startBeat = 0.5;
    
    cppmusic::model::NoteEvent note3;
    note3.pitch = 64;
    note3.startBeat = 1.0;
    
    // Add notes out of order
    pattern.addNote(note1);
    pattern.addNote(note2);
    pattern.addNote(note3);
    
    assert(pattern.getNoteCount() == 3);
    
    const auto& notes = pattern.getNotes();
    assert(approxEqual(notes[0].startBeat, 0.5));
    assert(approxEqual(notes[1].startBeat, 1.0));
    assert(approxEqual(notes[2].startBeat, 2.0));
    
    std::cout << "    PASSED" << std::endl;
}

void testRemoveNote() {
    std::cout << "  Testing removeNote..." << std::endl;
    
    cppmusic::model::Pattern pattern;
    
    cppmusic::model::NoteEvent note1;
    note1.pitch = 60;
    note1.startBeat = 0.0;
    
    cppmusic::model::NoteEvent note2;
    note2.pitch = 62;
    note2.startBeat = 1.0;
    
    pattern.addNote(note1);
    pattern.addNote(note2);
    
    assert(pattern.getNoteCount() == 2);
    
    // Remove first note
    bool removed = pattern.removeNote(0);
    assert(removed);
    assert(pattern.getNoteCount() == 1);
    assert(pattern.getNotes()[0].pitch == 62);
    
    // Try to remove out of bounds
    removed = pattern.removeNote(10);
    assert(!removed);
    assert(pattern.getNoteCount() == 1);
    
    std::cout << "    PASSED" << std::endl;
}

void testClearNotes() {
    std::cout << "  Testing clearNotes..." << std::endl;
    
    cppmusic::model::Pattern pattern;
    
    for (int i = 0; i < 5; ++i) {
        cppmusic::model::NoteEvent note;
        note.pitch = static_cast<std::uint8_t>(60 + i);
        note.startBeat = static_cast<double>(i);
        pattern.addNote(note);
    }
    
    assert(pattern.getNoteCount() == 5);
    
    pattern.clearNotes();
    
    assert(pattern.isEmpty());
    assert(pattern.getNoteCount() == 0);
    
    std::cout << "    PASSED" << std::endl;
}

void testComputeContentLength() {
    std::cout << "  Testing computeContentLength..." << std::endl;
    
    cppmusic::model::Pattern pattern("Test", 4, 4); // 16 beats
    
    // Empty pattern should return pattern length
    assert(approxEqual(pattern.computeContentLength(), 16.0));
    
    // Add a note that ends before pattern length
    cppmusic::model::NoteEvent note1;
    note1.startBeat = 0.0;
    note1.durationBeats = 2.0;
    pattern.addNote(note1);
    
    // Content length should still be pattern length
    assert(approxEqual(pattern.computeContentLength(), 16.0));
    
    // Add a note that extends beyond pattern length
    cppmusic::model::NoteEvent note2;
    note2.startBeat = 15.0;
    note2.durationBeats = 4.0; // Ends at beat 19
    pattern.addNote(note2);
    
    // Content length should now be 19
    assert(approxEqual(pattern.computeContentLength(), 19.0));
    
    std::cout << "    PASSED" << std::endl;
}

void testGetNotesInRange() {
    std::cout << "  Testing getNotesInRange..." << std::endl;
    
    cppmusic::model::Pattern pattern;
    
    // Add notes at various positions
    for (int i = 0; i < 8; ++i) {
        cppmusic::model::NoteEvent note;
        note.pitch = static_cast<std::uint8_t>(60 + i);
        note.startBeat = static_cast<double>(i * 2);
        note.durationBeats = 1.0;
        pattern.addNote(note);
    }
    
    // Query range [3.0, 7.0) - should include notes starting at 2, 4, 6
    auto notesInRange = pattern.getNotesInRange(3.0, 7.0);
    
    // Note at beat 2 (ends at 3) should NOT be included (ends exactly at range start)
    // Note at beat 4 (starts at 4, ends at 5) should be included
    // Note at beat 6 (starts at 6, ends at 7) should be included
    assert(notesInRange.size() == 2);
    
    std::cout << "    PASSED" << std::endl;
}

void testNoteEventEndBeat() {
    std::cout << "  Testing NoteEvent::getEndBeat..." << std::endl;
    
    cppmusic::model::NoteEvent note;
    note.startBeat = 5.0;
    note.durationBeats = 2.5;
    
    assert(approxEqual(note.getEndBeat(), 7.5));
    
    std::cout << "    PASSED" << std::endl;
}

void testNoteEventOverlapsRange() {
    std::cout << "  Testing NoteEvent::overlapsRange..." << std::endl;
    
    cppmusic::model::NoteEvent note;
    note.startBeat = 2.0;
    note.durationBeats = 2.0; // Spans [2, 4)
    
    // Completely before
    assert(!note.overlapsRange(0.0, 1.0));
    
    // Touching start (no overlap)
    assert(!note.overlapsRange(0.0, 2.0));
    
    // Overlapping start
    assert(note.overlapsRange(1.0, 3.0));
    
    // Completely inside
    assert(note.overlapsRange(2.5, 3.5));
    
    // Overlapping end
    assert(note.overlapsRange(3.0, 5.0));
    
    // Touching end (no overlap)
    assert(!note.overlapsRange(4.0, 5.0));
    
    // Completely after
    assert(!note.overlapsRange(5.0, 6.0));
    
    // Encompassing note
    assert(note.overlapsRange(0.0, 10.0));
    
    std::cout << "    PASSED" << std::endl;
}

void testPatternCopy() {
    std::cout << "  Testing Pattern copy..." << std::endl;
    
    cppmusic::model::Pattern pattern1("Original", 2, 4);
    
    cppmusic::model::NoteEvent note;
    note.pitch = 60;
    note.startBeat = 0.0;
    pattern1.addNote(note);
    
    // Copy construct
    cppmusic::model::Pattern pattern2(pattern1);
    
    assert(pattern2.getName() == "Original");
    assert(pattern2.getNoteCount() == 1);
    
    // Modify copy
    pattern2.setName("Copy");
    pattern2.clearNotes();
    
    // Original should be unchanged
    assert(pattern1.getName() == "Original");
    assert(pattern1.getNoteCount() == 1);
    
    std::cout << "    PASSED" << std::endl;
}

void testPatternSetLength() {
    std::cout << "  Testing Pattern setLengthBeats..." << std::endl;
    
    cppmusic::model::Pattern pattern;
    
    pattern.setLengthBeats(32.0);
    assert(approxEqual(pattern.getLengthBeats(), 32.0));
    
    // Negative length should be clamped to 0
    pattern.setLengthBeats(-5.0);
    assert(approxEqual(pattern.getLengthBeats(), 0.0));
    
    std::cout << "    PASSED" << std::endl;
}

} // anonymous namespace

int main() {
    std::cout << "Running Pattern unit tests..." << std::endl;
    
    testDefaultConstruction();
    testNamedConstruction();
    testAddNote();
    testNotesSortedByStartBeat();
    testRemoveNote();
    testClearNotes();
    testComputeContentLength();
    testGetNotesInRange();
    testNoteEventEndBeat();
    testNoteEventOverlapsRange();
    testPatternCopy();
    testPatternSetLength();
    
    std::cout << "\nAll Pattern tests PASSED!" << std::endl;
    return 0;
}
