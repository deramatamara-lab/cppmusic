// PatternTest exercises the Pattern model API for deterministic sequencing behavior.
#include <juce_core/juce_core.h>
#include <project/Pattern.h>

class PatternTest : public juce::UnitTest
{
public:
    PatternTest() : juce::UnitTest("Pattern Test") {}

    void runTest() override
    {
        using Pattern = daw::project::Pattern;
        using MIDINote = daw::project::Pattern::MIDINote;

        beginTest("Default construction");
        Pattern defaultPattern;
        expect(defaultPattern.getId() > 0, "Pattern ID should be generated");
        expectEquals(defaultPattern.getName(), std::string("Untitled Pattern"), "Default name should match");
        expectEquals(defaultPattern.getNumSteps(), 16, "Default step count should be 16");
        expectWithinAbsoluteError(defaultPattern.getSwing(), 0.0f, 1e-6f, "Default swing should be zero");
        expectWithinAbsoluteError(defaultPattern.getLengthBeats(), 16.0, 1e-6, "Empty pattern length equals steps");

        beginTest("Add note keeps deterministic ordering");
        Pattern pattern("Test Pattern", 8);
        MIDINote noteA{60, 100, 3.0, 0.5, 0};
        MIDINote noteB{62, 110, 1.0, 0.5, 0};
        pattern.addNote(noteA);
        pattern.addNote(noteB);
        const auto& sortedNotes = pattern.getNotes();
        expectEquals(sortedNotes.size(), size_t(2), "Pattern should store two notes");
        expectWithinAbsoluteError(sortedNotes.front().startBeat, 1.0, 1e-6, "Notes should be sorted by start beat");
        expectWithinAbsoluteError(sortedNotes.back().startBeat, 3.0, 1e-6, "Latest note should be last");

        beginTest("Remove and clear notes");
        pattern.removeNote(42); // out of range should be no-op
        expectEquals(pattern.getNotes().size(), size_t(2), "Invalid removal should not change notes");
        pattern.removeNote(0);
        expectEquals(pattern.getNotes().size(), size_t(1), "Removing valid index shrinks storage");
        pattern.clearNotes();
        expectEquals(pattern.getNotes().size(), size_t(0), "clearNotes removes all notes");

        beginTest("Set notes keeps deterministic order");
        std::vector<MIDINote> unsorted{
            MIDINote{65, 90, 4.0, 0.5, 0},
            MIDINote{64, 80, 2.0, 0.5, 0},
            MIDINote{63, 70, 3.0, 0.5, 0},
        };
        pattern.setNotes(unsorted);
        const auto& reordered = pattern.getNotes();
        expectEquals(reordered.size(), size_t(3), "setNotes should copy all notes");
        expectWithinAbsoluteError(reordered[0].startBeat, 2.0, 1e-6, "setNotes should sort by start beat (1)");
        expectWithinAbsoluteError(reordered[1].startBeat, 3.0, 1e-6, "setNotes should sort by start beat (2)");
        expectWithinAbsoluteError(reordered[2].startBeat, 4.0, 1e-6, "setNotes should sort by start beat (3)");

        beginTest("Get notes for step");
        const auto stepNotes = pattern.getNotesForStep(3);
        expectEquals(stepNotes.size(), size_t(1), "Exactly one note should be on step 3");
        expectWithinAbsoluteError(stepNotes.front().startBeat, 3.0, 1e-6, "Returned note should start within requested step");

        beginTest("Quantize and pattern length");
        MIDINote looseNote{70, 100, 1.33, 0.25, 0};
        MIDINote longNote{71, 90, 5.0, 2.0, 0};
        pattern.setNotes({looseNote, longNote});
        pattern.quantize(0.25);
        const auto& quantized = pattern.getNotes();
        expectWithinAbsoluteError(quantized[0].startBeat, 1.25, 1e-6, "Quantize should snap to nearest grid");
        expectWithinAbsoluteError(pattern.getLengthBeats(), 7.0, 1e-6, "Length should extend to farthest note end");

        beginTest("Swing clamping");
        pattern.setSwing(-0.5f);
        expectWithinAbsoluteError(pattern.getSwing(), 0.0f, 1e-6f, "Swing should clamp to minimum 0");
        pattern.setSwing(1.5f);
        expectWithinAbsoluteError(pattern.getSwing(), 1.0f, 1e-6f, "Swing should clamp to maximum 1");
        pattern.setSwing(0.35f);
        expectWithinAbsoluteError(pattern.getSwing(), 0.35f, 1e-6f, "Swing should store in-range values unchanged");
    }
};

static PatternTest patternTest;
