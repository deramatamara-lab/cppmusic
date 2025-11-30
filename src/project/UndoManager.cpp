#include "UndoManager.h"
#include "ProjectModel.h"
#include <algorithm>

namespace daw::project
{

// ========== UndoableCommand ==========

UndoableCommand::UndoableCommand(const std::string& description)
    : description(description)
{
}

// ========== UndoManager ==========

UndoManager::UndoManager()
    : maxHistorySize(100)
{
}

bool UndoManager::executeCommand(std::unique_ptr<UndoableCommand> command, ProjectModel& model)
{
    if (isPerformingUndoRedo || !command)
        return false;

    if (command->execute(model))
    {
        undoStack.push_back(std::move(command));
        redoStack.clear(); // Clear redo stack on new action
        trimHistory();
        notifyListeners();
        return true;
    }

    return false;
}

bool UndoManager::undo(ProjectModel& model)
{
    if (undoStack.empty() || isPerformingUndoRedo)
        return false;

    isPerformingUndoRedo = true;
    auto command = std::move(undoStack.back());
    undoStack.pop_back();

    const bool success = command->undo(model);
    if (success)
    {
        redoStack.push_back(std::move(command));
        notifyListeners();
    }
    else
    {
        // If undo failed, put command back
        undoStack.push_back(std::move(command));
    }

    isPerformingUndoRedo = false;
    return success;
}

bool UndoManager::redo(ProjectModel& model)
{
    if (redoStack.empty() || isPerformingUndoRedo)
        return false;

    isPerformingUndoRedo = true;
    auto command = std::move(redoStack.back());
    redoStack.pop_back();

    const bool success = command->execute(model);
    if (success)
    {
        undoStack.push_back(std::move(command));
        notifyListeners();
    }
    else
    {
        // If redo failed, put command back
        redoStack.push_back(std::move(command));
    }

    isPerformingUndoRedo = false;
    return success;
}

std::string UndoManager::getUndoDescription() const
{
    if (undoStack.empty())
        return "";
    return undoStack.back()->getDescription();
}

std::string UndoManager::getRedoDescription() const
{
    if (redoStack.empty())
        return "";
    return redoStack.back()->getDescription();
}

void UndoManager::clearHistory()
{
    undoStack.clear();
    redoStack.clear();
    notifyListeners();
}

void UndoManager::addHistoryListener(HistoryChangedCallback callback)
{
    listeners.push_back(callback);
}

void UndoManager::removeHistoryListener(HistoryChangedCallback callback)
{
    listeners.erase(
        std::remove_if(listeners.begin(), listeners.end(),
                      [&callback](const auto& listener) {
                          // Production implementation: Compare function pointers for exact match
                          // For std::function, we check if the target is the same by comparing
                          // the function object's target address (best-effort comparison)
                          if (listener.target_type() != callback.target_type())
                              return false;

                          const auto* listenerTarget = listener.template target<void(*)()>();
                          const auto* callbackTarget = callback.template target<void(*)()>();
                          if (listenerTarget != nullptr && callbackTarget != nullptr)
                              return *listenerTarget == *callbackTarget;

                          return false;
                      }),
        listeners.end());
}

void UndoManager::notifyListeners()
{
    for (const auto& listener : listeners)
    {
        if (listener)
            listener();
    }
}

void UndoManager::trimHistory()
{
    // Trim undo stack if it exceeds max size
    while (undoStack.size() > maxHistorySize)
    {
        undoStack.erase(undoStack.begin());
    }
}

// ========== AddTrackCommand ==========

AddTrackCommand::AddTrackCommand(const std::string& name, juce::Colour color)
    : UndoableCommand("Add Track: " + name)
    , trackName(name)
    , trackColor(color)
{
}

bool AddTrackCommand::execute(ProjectModel& model)
{
    auto* track = model.addTrack(trackName, trackColor);
    if (track != nullptr)
    {
        createdTrackId = track->getId();
        return true;
    }
    return false;
}

bool AddTrackCommand::undo(ProjectModel& model)
{
    if (createdTrackId != 0)
    {
        model.removeTrack(createdTrackId);
        return true;
    }
    return false;
}

// ========== RemoveTrackCommand ==========

RemoveTrackCommand::RemoveTrackCommand(uint32_t trackId)
    : UndoableCommand("Remove Track")
    , trackId(trackId)
{
}

bool RemoveTrackCommand::execute(ProjectModel& model)
{
    const auto* track = model.getTrack(trackId);
    if (track != nullptr)
    {
        trackName = track->getName();
        trackColor = track->getColor();
        trackGain = track->getGainDb();
        trackPan = track->getPan();
        trackMuted = track->isMuted();
        trackSoloed = track->isSoloed();
        model.removeTrack(trackId);
        return true;
    }
    return false;
}

bool RemoveTrackCommand::undo(ProjectModel& model)
{
    auto* track = model.addTrack(trackName, trackColor);
    if (track != nullptr)
    {
        track->setGainDb(trackGain);
        track->setPan(trackPan);
        track->setMuted(trackMuted);
        track->setSoloed(trackSoloed);
        return true;
    }
    return false;
}

// ========== AddClipCommand ==========

AddClipCommand::AddClipCommand(uint32_t trackId, double startBeats, double lengthBeats, const std::string& label)
    : UndoableCommand("Add Clip")
    , trackId(trackId)
    , startBeats(startBeats)
    , lengthBeats(lengthBeats)
    , label(label)
{
}

bool AddClipCommand::execute(ProjectModel& model)
{
    auto* clip = model.addClip(trackId, startBeats, lengthBeats, label);
    if (clip != nullptr)
    {
        createdClipId = clip->getId();
        return true;
    }
    return false;
}

bool AddClipCommand::undo(ProjectModel& model)
{
    if (createdClipId != 0)
    {
        model.removeClip(createdClipId);
        return true;
    }
    return false;
}

// ========== RemoveClipCommand ==========

RemoveClipCommand::RemoveClipCommand(uint32_t clipId)
    : UndoableCommand("Remove Clip")
    , clipId(clipId)
{
}

bool RemoveClipCommand::execute(ProjectModel& model)
{
    const auto* clip = model.getClip(clipId);
    if (clip != nullptr)
    {
        trackId = clip->getTrackId();
        startBeats = clip->getStartBeats();
        lengthBeats = clip->getLengthBeats();
        label = clip->getLabel();
        patternId = clip->hasPattern() ? clip->getPatternId() : 0;
        model.removeClip(clipId);
        return true;
    }
    return false;
}

bool RemoveClipCommand::undo(ProjectModel& model)
{
    auto* clip = model.addClip(trackId, startBeats, lengthBeats, label);
    if (clip != nullptr && patternId != 0)
    {
        model.linkClipToPattern(clip->getId(), patternId);
        return true;
    }
    return clip != nullptr;
}

// ========== RenameTrackCommand ==========

RenameTrackCommand::RenameTrackCommand(uint32_t trackId, const std::string& newName)
    : UndoableCommand("Rename Track")
    , trackId(trackId)
    , newName(newName)
{
}

bool RenameTrackCommand::execute(ProjectModel& model)
{
    auto* track = model.getTrack(trackId);
    if (track != nullptr)
    {
        oldName = track->getName();
        track->setName(newName);
        return true;
    }
    return false;
}

bool RenameTrackCommand::undo(ProjectModel& model)
{
    auto* track = model.getTrack(trackId);
    if (track != nullptr)
    {
        track->setName(oldName);
        return true;
    }
    return false;
}

} // namespace daw::project

