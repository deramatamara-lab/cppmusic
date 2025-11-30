#include <juce_core/juce_core.h>
#include <project/ProjectModel.h>
#include <project/UndoManager.h>

class UndoManagerTest : public juce::UnitTest
{
public:
    UndoManagerTest() : juce::UnitTest("UndoManager Test") {}

    void runTest() override
    {
        beginTest("UndoManager creation");
        daw::project::UndoManager undoManager;
        auto model = std::make_shared<daw::project::ProjectModel>();

        expect(!undoManager.canUndo(), "Should not be able to undo initially");
        expect(!undoManager.canRedo(), "Should not be able to redo initially");

        beginTest("Add track command");
        // Create undo command
        auto addCmd = std::make_unique<daw::project::AddTrackCommand>("Test Track", juce::Colours::red);
        const bool executed = undoManager.executeCommand(std::move(addCmd), *model);
        expect(executed, "Command should execute successfully");
        expectEquals(model->getTracks().size(), size_t(1), "Should have 1 track after command");
        expect(undoManager.canUndo(), "Should be able to undo after command");

        beginTest("Undo operation");
        const auto undoDesc = undoManager.getUndoDescription();
        expect(undoDesc.length() > 0, "Undo description should not be empty");

        const bool undoResult = undoManager.undo(*model);
        expect(undoResult, "Undo should succeed");
        expect(undoManager.canRedo(), "Should be able to redo after undo");

        beginTest("Redo operation");
        const auto redoDesc = undoManager.getRedoDescription();
        expect(redoDesc.length() > 0, "Redo description should not be empty");

        const bool redoResult = undoManager.redo(*model);
        expect(redoResult, "Redo should succeed");
        expect(undoManager.canUndo(), "Should be able to undo after redo");

        beginTest("History limit");
        undoManager.setMaxHistorySize(3);

        // Add multiple commands
        for (int i = 0; i < 5; ++i)
        {
            auto cmd = std::make_unique<daw::project::AddTrackCommand>(("Track " + juce::String(i)).toStdString(), juce::Colours::blue);
            undoManager.executeCommand(std::move(cmd), *model);
        }

        // History should be limited
        expect(undoManager.canUndo(), "Should be able to undo");

        beginTest("Clear history");
        undoManager.clearHistory();
        expect(!undoManager.canUndo(), "Should not be able to undo after clear");
        expect(!undoManager.canRedo(), "Should not be able to redo after clear");
    }
};

static UndoManagerTest undoManagerTest;

