#include <juce_core/juce_core.h>
#include <project/ProjectModel.h>
#include <project/Track.h>
#include <project/Clip.h>
#include <project/Pattern.h>
#include <cmath>

class ProjectModelTest : public juce::UnitTest
{
public:
    ProjectModelTest() : juce::UnitTest("ProjectModel Test") {}

    void runTest() override
    {
        beginTest("Model creation");
        auto model = std::make_shared<daw::project::ProjectModel>();
        expect(model != nullptr, "Model should be created");
        expectEquals(model->getTracks().size(), size_t(0), "Should have no tracks initially");
        expectEquals(model->getPatterns().size(), size_t(0), "Should have no patterns initially");

        beginTest("Track management");
        const auto track1Ptr = model->addTrack("Track 1", juce::Colours::red);
        expect(track1Ptr != nullptr, "Track should be created");
        const auto trackId1 = track1Ptr != nullptr ? track1Ptr->getId() : 0u;
        expect(trackId1 > 0u, "Track ID should be valid");
        expectEquals(model->getTracks().size(), size_t(1), "Should have 1 track");

        const auto* track1 = model->getTrack(trackId1);
        expect(track1 != nullptr, "Track should be retrievable");
        expectEquals(track1->getName(), std::string("Track 1"), "Track name should match");
        expect(track1->getColor() == juce::Colours::red, "Track color should match");

        const auto track2Ptr = model->addTrack("Track 2", juce::Colours::blue);
        expect(track2Ptr != nullptr, "Second track should be created");
        const auto trackId2 = track2Ptr != nullptr ? track2Ptr->getId() : 0u;
        expectEquals(model->getTracks().size(), size_t(2), "Should have 2 tracks");

        beginTest("Track removal");
        model->removeTrack(trackId1);
        expectEquals(model->getTracks().size(), size_t(1), "Should have 1 track after removal");
        expect(model->getTrack(trackId1) == nullptr, "Removed track should be null");

        beginTest("Clip management");
        const auto clip1Ptr = model->addClip(trackId2, 0.0, 4.0, "Clip 1");
        expect(clip1Ptr != nullptr, "Clip should be created");
        const auto clipId1 = clip1Ptr != nullptr ? clip1Ptr->getId() : 0u;

        const auto* clip1 = model->getClip(clipId1);
        expect(clip1 != nullptr, "Clip should be retrievable");
        expectEquals(clip1->getStartBeats(), 0.0, "Clip start should be 0.0");
        expectWithinAbsoluteError(clip1->getLengthBeats(), 4.0, 0.01, "Clip length should be 4.0");
        expectEquals(clip1->getLabel(), std::string("Clip 1"), "Clip label should match");

        const auto clip2Ptr = model->addClip(trackId2, 4.0, 4.0, "Clip 2");
        expect(clip2Ptr != nullptr, "Second clip should be created");
        const auto clipId2 = clip2Ptr != nullptr ? clip2Ptr->getId() : 0u;
        expectEquals(model->getClips().size(), size_t(2), "Should have 2 clips");

        beginTest("Clip removal");
        model->removeClip(clipId1);
        expectEquals(model->getClips().size(), size_t(1), "Should have 1 clip after removal");
        expect(model->getClip(clipId1) == nullptr, "Removed clip should be null");

        beginTest("Pattern management");
        const auto pattern1Ptr = model->addPattern("Pattern 1");
        const auto patternId1 = pattern1Ptr != nullptr ? pattern1Ptr->getId() : 0u;
        expect(patternId1 > 0, "Pattern ID should be valid");
        expectEquals(model->getPatterns().size(), size_t(1), "Should have 1 pattern");

        auto* pattern1 = model->getPattern(patternId1);
        expect(pattern1 != nullptr, "Pattern should be retrievable");
        expectEquals(pattern1->getName(), std::string("Pattern 1"), "Pattern name should match");

        beginTest("Pattern notes");
        daw::project::Pattern::MIDINote note1;
        note1.note = 60; // C4
        note1.velocity = 100;
        note1.startBeat = 0.0;
        note1.lengthBeats = 1.0;
        note1.channel = 0;

        pattern1->addNote(note1);
        expectEquals(pattern1->getNotes().size(), size_t(1), "Pattern should have 1 note");

        const auto& notes = pattern1->getNotes();
        expectEquals((int)notes[0].note, 60, "Note pitch should be 60");
        expectEquals((int)notes[0].velocity, 100, "Note velocity should be 100");

        beginTest("Pattern removal");
        model->removePattern(patternId1);
        expectEquals(model->getPatterns().size(), size_t(0), "Should have 0 patterns after removal");
        expect(model->getPattern(patternId1) == nullptr, "Removed pattern should be null");

        beginTest("Clip-Pattern association");
        const auto pattern2Ptr = model->addPattern("Pattern 2");
        const auto patternId2 = pattern2Ptr != nullptr ? pattern2Ptr->getId() : 0u;
        const auto clip3Ptr = model->addClip(trackId2, 8.0, 4.0, "Clip 3");
        expect(clip3Ptr != nullptr, "Third clip should exist");
        const auto clipId3 = clip3Ptr != nullptr ? clip3Ptr->getId() : 0u;

        auto* clip3 = model->getClip(clipId3);
        clip3->setPatternId(patternId2);
        expectEquals((int)clip3->getPatternId(), (int)patternId2, "Clip should be associated with pattern");

        beginTest("Track mixer parameters");
        auto* track2 = model->getTrack(trackId2);
        track2->setGainDb(-6.0f);
        track2->setPan(0.5f);
        track2->setMuted(true);
        track2->setSoloed(false);

        expectWithinAbsoluteError(track2->getGainDb(), -6.0f, 0.01f, "Gain should be -6.0 dB");
        expectWithinAbsoluteError(track2->getPan(), 0.5f, 0.01f, "Pan should be 0.5");
        expect(track2->isMuted(), "Track should be muted");
        expect(!track2->isSoloed(), "Track should not be soloed");

        beginTest("Selection model");
        auto& selection = model->getSelectionModel();
        selection.selectTrack(trackId2);
        expect(selection.isTrackSelected(trackId2), "Track should be selected");

        selection.selectClip(clipId2);
        expect(selection.isClipSelected(clipId2), "Clip should be selected");

        selection.clearAll();
        expect(!selection.isTrackSelected(trackId2), "Track should not be selected after clear");
        expect(!selection.isClipSelected(clipId2), "Clip should not be selected after clear");
    }
};

static ProjectModelTest projectModelTest;

