/**
 * @file TestPatternCRDT.cpp
 * @brief Unit tests for CRDT consistency and ordering.
 */

#include "services/collab/PatternCRDT.hpp"
#include <cassert>
#include <iostream>

using namespace cppmusic::services::collab;
using namespace cppmusic::model;

namespace {

void testNoteInsert() {
    std::cout << "  Testing note insertion..." << std::endl;
    
    PatternCRDT crdt(1);  // Peer 1
    
    NoteEvent note;
    note.midiNote = 60;
    note.velocity = 100;
    note.startBeat = 0.0;
    note.durationBeats = 1.0;
    
    NoteId id = crdt.insertNote(note);
    assert(id.peerId == 1);
    assert(id.timestamp > 0);
    
    assert(crdt.getNoteCount() == 1);
    
    auto retrieved = crdt.getNote(id);
    assert(retrieved.has_value());
    assert(retrieved->midiNote == 60);
    assert(retrieved->velocity == 100);
    
    std::cout << "  Note insertion passed." << std::endl;
}

void testNoteDelete() {
    std::cout << "  Testing note deletion..." << std::endl;
    
    PatternCRDT crdt(1);
    
    NoteEvent note;
    note.midiNote = 64;
    
    NoteId id = crdt.insertNote(note);
    assert(crdt.getNoteCount() == 1);
    
    assert(crdt.deleteNote(id));
    assert(crdt.getNoteCount() == 0);
    
    // Deleted note should return nullopt
    assert(!crdt.getNote(id).has_value());
    
    // Deleting again should return false
    assert(!crdt.deleteNote(id));
    
    std::cout << "  Note deletion passed." << std::endl;
}

void testMergeCommutativity() {
    std::cout << "  Testing merge commutativity..." << std::endl;
    
    PatternCRDT crdt1(1);
    PatternCRDT crdt2(2);
    
    NoteEvent note1;
    note1.midiNote = 60;
    note1.startBeat = 0.0;
    crdt1.insertNote(note1);
    
    NoteEvent note2;
    note2.midiNote = 64;
    note2.startBeat = 1.0;
    crdt2.insertNote(note2);
    
    // Merge in both orders
    PatternCRDT copy1(crdt1);
    PatternCRDT copy2(crdt2);
    
    copy1.merge(crdt2);
    copy2.merge(crdt1);
    
    // Both should have same notes
    auto notes1 = copy1.getCanonicalNotes();
    auto notes2 = copy2.getCanonicalNotes();
    
    assert(notes1.size() == 2);
    assert(notes2.size() == 2);
    
    // Canonical ordering should be the same
    assert(notes1[0].midiNote == notes2[0].midiNote);
    assert(notes1[1].midiNote == notes2[1].midiNote);
    
    std::cout << "  Merge commutativity passed." << std::endl;
}

void testMergeIdempotency() {
    std::cout << "  Testing merge idempotency..." << std::endl;
    
    PatternCRDT crdt1(1);
    PatternCRDT crdt2(2);
    
    NoteEvent note;
    note.midiNote = 60;
    crdt1.insertNote(note);
    
    crdt2.merge(crdt1);
    std::size_t countAfterFirst = crdt2.getNoteCount();
    
    // Merge same state again
    crdt2.merge(crdt1);
    std::size_t countAfterSecond = crdt2.getNoteCount();
    
    assert(countAfterFirst == countAfterSecond);
    assert(countAfterFirst == 1);
    
    std::cout << "  Merge idempotency passed." << std::endl;
}

void testMergeAssociativity() {
    std::cout << "  Testing merge associativity..." << std::endl;
    
    PatternCRDT crdt1(1);
    PatternCRDT crdt2(2);
    PatternCRDT crdt3(3);
    
    NoteEvent note1{};
    note1.midiNote = 60;
    crdt1.insertNote(note1);
    
    NoteEvent note2{};
    note2.midiNote = 64;
    crdt2.insertNote(note2);
    
    NoteEvent note3{};
    note3.midiNote = 67;
    crdt3.insertNote(note3);
    
    // (A merge B) merge C
    PatternCRDT result1(crdt1);
    result1.merge(crdt2);
    result1.merge(crdt3);
    
    // A merge (B merge C)
    PatternCRDT temp(crdt2);
    temp.merge(crdt3);
    PatternCRDT result2(crdt1);
    result2.merge(temp);
    
    auto notes1 = result1.getCanonicalNotes();
    auto notes2 = result2.getCanonicalNotes();
    
    assert(notes1.size() == 3);
    assert(notes2.size() == 3);
    
    // Same canonical ordering
    for (std::size_t i = 0; i < 3; ++i) {
        assert(notes1[i].midiNote == notes2[i].midiNote);
    }
    
    std::cout << "  Merge associativity passed." << std::endl;
}

void testConcurrentInsert() {
    std::cout << "  Testing concurrent insert resolution..." << std::endl;
    
    PatternCRDT crdt1(1);
    PatternCRDT crdt2(2);
    
    // Both insert at same beat position
    NoteEvent note1{};
    note1.midiNote = 60;
    note1.startBeat = 0.0;
    crdt1.insertNote(note1);
    
    NoteEvent note2{};
    note2.midiNote = 64;
    note2.startBeat = 0.0;
    crdt2.insertNote(note2);
    
    // Merge
    crdt1.merge(crdt2);
    
    // Both notes should exist
    assert(crdt1.getNoteCount() == 2);
    
    // Canonical ordering should be deterministic
    auto notes = crdt1.getCanonicalNotes();
    assert(notes.size() == 2);
    
    std::cout << "  Concurrent insert resolution passed." << std::endl;
}

void testDeleteWinsOverUpdate() {
    std::cout << "  Testing delete wins over update..." << std::endl;
    
    PatternCRDT crdt1(1);
    PatternCRDT crdt2(2);
    
    NoteEvent note{};
    note.midiNote = 60;
    NoteId id = crdt1.insertNote(note);
    
    // Sync to crdt2
    crdt2.merge(crdt1);
    
    // crdt1 deletes
    crdt1.deleteNote(id);
    
    // crdt2 updates (before receiving delete)
    note.velocity = 127;
    crdt2.updateNote(id, note);
    
    // Merge: delete should win
    crdt2.merge(crdt1);
    
    assert(crdt2.getNoteCount() == 0);
    assert(!crdt2.getNote(id).has_value());
    
    std::cout << "  Delete wins over update passed." << std::endl;
}

void testCanonicalOrdering() {
    std::cout << "  Testing canonical ordering stability..." << std::endl;
    
    PatternCRDT crdt(1);
    
    // Insert notes in non-sorted order
    NoteEvent note3{};
    note3.midiNote = 67;
    note3.startBeat = 4.0;
    crdt.insertNote(note3);
    
    NoteEvent note1{};
    note1.midiNote = 60;
    note1.startBeat = 0.0;
    crdt.insertNote(note1);
    
    NoteEvent note2{};
    note2.midiNote = 64;
    note2.startBeat = 2.0;
    crdt.insertNote(note2);
    
    // Get canonical notes
    auto notes = crdt.getCanonicalNotes();
    
    // Should be sorted by start beat
    assert(notes[0].startBeat == 0.0);
    assert(notes[1].startBeat == 2.0);
    assert(notes[2].startBeat == 4.0);
    
    // Get again - should be identical
    auto notes2 = crdt.getCanonicalNotes();
    for (std::size_t i = 0; i < 3; ++i) {
        assert(notes[i].startBeat == notes2[i].startBeat);
        assert(notes[i].midiNote == notes2[i].midiNote);
    }
    
    std::cout << "  Canonical ordering stability passed." << std::endl;
}

} // anonymous namespace

int main() {
    std::cout << "Running Pattern CRDT Tests..." << std::endl;
    
    testNoteInsert();
    testNoteDelete();
    testMergeCommutativity();
    testMergeIdempotency();
    testMergeAssociativity();
    testConcurrentInsert();
    testDeleteWinsOverUpdate();
    testCanonicalOrdering();
    
    std::cout << "All Pattern CRDT Tests PASSED!" << std::endl;
    return 0;
}
