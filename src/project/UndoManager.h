#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <cstdint>
#include <juce_graphics/juce_graphics.h>

namespace daw::project
{

class ProjectModel;

/**
 * @brief Command interface for undo/redo operations
 *
 * Base class for all undoable commands.
 * Follows DAW_DEV_RULES: semantic undo steps.
 */
class UndoableCommand
{
public:
    explicit UndoableCommand(const std::string& description);
    virtual ~UndoableCommand() = default;

    // Non-copyable, movable
    UndoableCommand(const UndoableCommand&) = delete;
    UndoableCommand& operator=(const UndoableCommand&) = delete;
    UndoableCommand(UndoableCommand&&) noexcept = default;
    UndoableCommand& operator=(UndoableCommand&&) noexcept = default;

    /**
     * @brief Execute the command
     * @param model Project model to operate on
     * @return true if successful
     */
    [[nodiscard]] virtual bool execute(ProjectModel& model) = 0;

    /**
     * @brief Undo the command
     * @param model Project model to operate on
     * @return true if successful
     */
    [[nodiscard]] virtual bool undo(ProjectModel& model) = 0;

    /**
     * @brief Get command description
     */
    [[nodiscard]] const std::string& getDescription() const noexcept { return description; }

protected:
    std::string description;
};

/**
 * @brief Undo/Redo manager
 *
 * Manages command history and provides undo/redo functionality.
 * Follows DAW_DEV_RULES: semantic undo steps, bounded history.
 */
class UndoManager
{
public:
    UndoManager();
    ~UndoManager() = default;

    // Non-copyable, movable
    UndoManager(const UndoManager&) = delete;
    UndoManager& operator=(const UndoManager&) = delete;
    UndoManager(UndoManager&&) noexcept = default;
    UndoManager& operator=(UndoManager&&) noexcept = default;

    /**
     * @brief Execute a command and add to undo stack
     * @param command Command to execute
     * @param model Project model
     * @return true if successful
     */
    bool executeCommand(std::unique_ptr<UndoableCommand> command, ProjectModel& model);

    /**
     * @brief Undo last command
     * @param model Project model
     * @return true if successful
     */
    bool undo(ProjectModel& model);

    /**
     * @brief Redo last undone command
     * @param model Project model
     * @return true if successful
     */
    bool redo(ProjectModel& model);

    /**
     * @brief Check if undo is available
     */
    [[nodiscard]] bool canUndo() const noexcept { return !undoStack.empty(); }

    /**
     * @brief Check if redo is available
     */
    [[nodiscard]] bool canRedo() const noexcept { return !redoStack.empty(); }

    /**
     * @brief Get description of next undo operation
     */
    [[nodiscard]] std::string getUndoDescription() const;

    /**
     * @brief Get description of next redo operation
     */
    [[nodiscard]] std::string getRedoDescription() const;

    /**
     * @brief Clear undo/redo history
     */
    void clearHistory();

    /**
     * @brief Set maximum history size
     */
    void setMaxHistorySize(size_t maxSize) { maxHistorySize = maxSize; }

    /**
     * @brief Get current history size
     */
    [[nodiscard]] size_t getHistorySize() const { return undoStack.size() + redoStack.size(); }

    // Listener support
    using HistoryChangedCallback = std::function<void()>;
    void addHistoryListener(HistoryChangedCallback callback);
    void removeHistoryListener(HistoryChangedCallback callback);

private:
    std::vector<std::unique_ptr<UndoableCommand>> undoStack;
    std::vector<std::unique_ptr<UndoableCommand>> redoStack;
    size_t maxHistorySize{100}; // Maximum number of undo steps
    std::vector<HistoryChangedCallback> listeners;
    bool isPerformingUndoRedo{false};

    void notifyListeners();
    void trimHistory();
};

// ========== Concrete Command Implementations ==========

/**
 * @brief Command to add a track
 */
class AddTrackCommand : public UndoableCommand
{
public:
    AddTrackCommand(const std::string& name, juce::Colour color);
    [[nodiscard]] bool execute(ProjectModel& model) override;
    [[nodiscard]] bool undo(ProjectModel& model) override;

private:
    std::string trackName;
    juce::Colour trackColor;
    uint32_t createdTrackId{0};
};

/**
 * @brief Command to remove a track
 */
class RemoveTrackCommand : public UndoableCommand
{
public:
    explicit RemoveTrackCommand(uint32_t trackId);
    [[nodiscard]] bool execute(ProjectModel& model) override;
    [[nodiscard]] bool undo(ProjectModel& model) override;

private:
    uint32_t trackId;
    std::string trackName;
    juce::Colour trackColor;
    float trackGain{0.0f};
    float trackPan{0.0f};
    bool trackMuted{false};
    bool trackSoloed{false};
};

/**
 * @brief Command to add a clip
 */
class AddClipCommand : public UndoableCommand
{
public:
    AddClipCommand(uint32_t trackId, double startBeats, double lengthBeats, const std::string& label);
    [[nodiscard]] bool execute(ProjectModel& model) override;
    [[nodiscard]] bool undo(ProjectModel& model) override;

private:
    uint32_t trackId;
    double startBeats;
    double lengthBeats;
    std::string label;
    uint32_t createdClipId{0};
};

// Forward declaration - RemoveClipCommand is defined in commands/RemoveClipCommand.h

/**
 * @brief Command to rename a track
 */
class RenameTrackCommand : public UndoableCommand
{
public:
    RenameTrackCommand(uint32_t trackId, const std::string& newName);
    [[nodiscard]] bool execute(ProjectModel& model) override;
    [[nodiscard]] bool undo(ProjectModel& model) override;

private:
    uint32_t trackId;
    std::string newName;
    std::string oldName;
};

} // namespace daw::project

