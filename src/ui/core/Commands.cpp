/**
 * @file Commands.cpp
 * @brief Implementation of DAW command handling.
 */

#include "Commands.hpp"

namespace cppmusic::ui::core {

Commands::Commands() = default;

void Commands::initialize(juce::ApplicationCommandManager& manager) {
    manager.registerAllCommandsForTarget(this);
    addDefaultKeyMappings(manager);
}

void Commands::getAllCommands(juce::Array<juce::CommandID>& commands) {
    const juce::CommandID ids[] = {
        // Transport
        CommandIDs::play,
        CommandIDs::stop,
        CommandIDs::record,
        CommandIDs::toggleLoop,
        CommandIDs::toggleMetronome,
        CommandIDs::tapTempo,
        CommandIDs::gotoStart,
        CommandIDs::gotoEnd,
        
        // View
        CommandIDs::showPlaylist,
        CommandIDs::showChannelRack,
        CommandIDs::showPianoRoll,
        CommandIDs::showMixer,
        CommandIDs::showDevices,
        CommandIDs::toggleBrowser,
        CommandIDs::toggleInspector,
        
        // Zoom
        CommandIDs::zoomIn,
        CommandIDs::zoomOut,
        CommandIDs::zoomToFit,
        CommandIDs::zoomToSelection,
        
        // Edit
        CommandIDs::undo,
        CommandIDs::redo,
        CommandIDs::cut,
        CommandIDs::copy,
        CommandIDs::paste,
        CommandIDs::deleteSelection,
        CommandIDs::selectAll,
        CommandIDs::deselectAll,
        CommandIDs::duplicate,
        CommandIDs::quantize,
        
        // File
        CommandIDs::newProject,
        CommandIDs::openProject,
        CommandIDs::saveProject,
        CommandIDs::saveProjectAs,
        CommandIDs::exportAudio,
        
        // Track
        CommandIDs::addTrack,
        CommandIDs::deleteTrack,
        CommandIDs::muteTrack,
        CommandIDs::soloTrack,
        CommandIDs::armTrack,
        
        // Pattern
        CommandIDs::addPattern,
        CommandIDs::deletePattern,
        CommandIDs::duplicatePattern,
        
        // Application
        CommandIDs::preferences,
        CommandIDs::toggleFullscreen
    };
    
    commands.addArray(ids, std::size(ids));
}

void Commands::getCommandInfo(juce::CommandID commandID,
                               juce::ApplicationCommandInfo& result) {
    switch (commandID) {
        // Transport commands
        case CommandIDs::play:
            result.setInfo("Play", "Start playback", "Transport", 0);
            result.addDefaultKeypress(juce::KeyPress::spaceKey, 0);
            break;
        case CommandIDs::stop:
            result.setInfo("Stop", "Stop playback", "Transport", 0);
            result.addDefaultKeypress(juce::KeyPress::spaceKey, juce::ModifierKeys::shiftModifier);
            break;
        case CommandIDs::record:
            result.setInfo("Record", "Toggle recording", "Transport", 0);
            result.addDefaultKeypress('r', juce::ModifierKeys::commandModifier);
            break;
        case CommandIDs::toggleLoop:
            result.setInfo("Loop", "Toggle loop mode", "Transport", 0);
            result.addDefaultKeypress('l', juce::ModifierKeys::commandModifier);
            break;
        case CommandIDs::toggleMetronome:
            result.setInfo("Metronome", "Toggle metronome", "Transport", 0);
            result.addDefaultKeypress('m', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            break;
        case CommandIDs::tapTempo:
            result.setInfo("Tap Tempo", "Tap to set tempo", "Transport", 0);
            result.addDefaultKeypress('t', juce::ModifierKeys::commandModifier);
            break;
        case CommandIDs::gotoStart:
            result.setInfo("Go to Start", "Go to project start", "Transport", 0);
            result.addDefaultKeypress(juce::KeyPress::homeKey, 0);
            break;
        case CommandIDs::gotoEnd:
            result.setInfo("Go to End", "Go to project end", "Transport", 0);
            result.addDefaultKeypress(juce::KeyPress::endKey, 0);
            break;

        // View commands
        case CommandIDs::showPlaylist:
            result.setInfo("Playlist", "Show playlist view", "View", 0);
            result.addDefaultKeypress(juce::KeyPress::F5Key, 0);
            break;
        case CommandIDs::showChannelRack:
            result.setInfo("Channel Rack", "Show channel rack", "View", 0);
            result.addDefaultKeypress(juce::KeyPress::F6Key, 0);
            break;
        case CommandIDs::showPianoRoll:
            result.setInfo("Piano Roll", "Show piano roll", "View", 0);
            result.addDefaultKeypress(juce::KeyPress::F7Key, 0);
            break;
        case CommandIDs::showMixer:
            result.setInfo("Mixer", "Show mixer view", "View", 0);
            result.addDefaultKeypress(juce::KeyPress::F9Key, 0);
            break;
        case CommandIDs::showDevices:
            result.setInfo("Devices", "Show devices view", "View", 0);
            result.addDefaultKeypress(juce::KeyPress::F8Key, 0);
            break;
        case CommandIDs::toggleBrowser:
            result.setInfo("Toggle Browser", "Show/hide browser panel", "View", 0);
            result.addDefaultKeypress('b', juce::ModifierKeys::commandModifier);
            break;
        case CommandIDs::toggleInspector:
            result.setInfo("Toggle Inspector", "Show/hide inspector panel", "View", 0);
            result.addDefaultKeypress('i', juce::ModifierKeys::commandModifier);
            break;

        // Zoom commands
        case CommandIDs::zoomIn:
            result.setInfo("Zoom In", "Zoom in", "Zoom", 0);
            result.addDefaultKeypress('=', juce::ModifierKeys::commandModifier);
            break;
        case CommandIDs::zoomOut:
            result.setInfo("Zoom Out", "Zoom out", "Zoom", 0);
            result.addDefaultKeypress('-', juce::ModifierKeys::commandModifier);
            break;
        case CommandIDs::zoomToFit:
            result.setInfo("Zoom to Fit", "Zoom to fit content", "Zoom", 0);
            result.addDefaultKeypress('0', juce::ModifierKeys::commandModifier);
            break;
        case CommandIDs::zoomToSelection:
            result.setInfo("Zoom to Selection", "Zoom to selection", "Zoom", 0);
            result.addDefaultKeypress('f', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            break;

        // Edit commands
        case CommandIDs::undo:
            result.setInfo("Undo", "Undo last action", "Edit", 0);
            result.addDefaultKeypress('z', juce::ModifierKeys::commandModifier);
            break;
        case CommandIDs::redo:
            result.setInfo("Redo", "Redo last undone action", "Edit", 0);
            result.addDefaultKeypress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            break;
        case CommandIDs::cut:
            result.setInfo("Cut", "Cut selection", "Edit", 0);
            result.addDefaultKeypress('x', juce::ModifierKeys::commandModifier);
            break;
        case CommandIDs::copy:
            result.setInfo("Copy", "Copy selection", "Edit", 0);
            result.addDefaultKeypress('c', juce::ModifierKeys::commandModifier);
            break;
        case CommandIDs::paste:
            result.setInfo("Paste", "Paste from clipboard", "Edit", 0);
            result.addDefaultKeypress('v', juce::ModifierKeys::commandModifier);
            break;
        case CommandIDs::deleteSelection:
            result.setInfo("Delete", "Delete selection", "Edit", 0);
            result.addDefaultKeypress(juce::KeyPress::deleteKey, 0);
            break;
        case CommandIDs::selectAll:
            result.setInfo("Select All", "Select all items", "Edit", 0);
            result.addDefaultKeypress('a', juce::ModifierKeys::commandModifier);
            break;
        case CommandIDs::deselectAll:
            result.setInfo("Deselect All", "Deselect all items", "Edit", 0);
            result.addDefaultKeypress('d', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            break;
        case CommandIDs::duplicate:
            result.setInfo("Duplicate", "Duplicate selection", "Edit", 0);
            result.addDefaultKeypress('d', juce::ModifierKeys::commandModifier);
            break;
        case CommandIDs::quantize:
            result.setInfo("Quantize", "Quantize selection", "Edit", 0);
            result.addDefaultKeypress('q', juce::ModifierKeys::commandModifier);
            break;

        // File commands
        case CommandIDs::newProject:
            result.setInfo("New Project", "Create new project", "File", 0);
            result.addDefaultKeypress('n', juce::ModifierKeys::commandModifier);
            break;
        case CommandIDs::openProject:
            result.setInfo("Open Project", "Open existing project", "File", 0);
            result.addDefaultKeypress('o', juce::ModifierKeys::commandModifier);
            break;
        case CommandIDs::saveProject:
            result.setInfo("Save Project", "Save current project", "File", 0);
            result.addDefaultKeypress('s', juce::ModifierKeys::commandModifier);
            break;
        case CommandIDs::saveProjectAs:
            result.setInfo("Save Project As", "Save project as new file", "File", 0);
            result.addDefaultKeypress('s', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            break;
        case CommandIDs::exportAudio:
            result.setInfo("Export Audio", "Export audio file", "File", 0);
            result.addDefaultKeypress('e', juce::ModifierKeys::commandModifier);
            break;

        // Track commands
        case CommandIDs::addTrack:
            result.setInfo("Add Track", "Add new track", "Track", 0);
            result.addDefaultKeypress('t', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            break;
        case CommandIDs::deleteTrack:
            result.setInfo("Delete Track", "Delete selected track", "Track", 0);
            break;
        case CommandIDs::muteTrack:
            result.setInfo("Mute Track", "Toggle track mute", "Track", 0);
            result.addDefaultKeypress('m', juce::ModifierKeys::commandModifier);
            break;
        case CommandIDs::soloTrack:
            result.setInfo("Solo Track", "Toggle track solo", "Track", 0);
            result.addDefaultKeypress('s', juce::ModifierKeys::altModifier);
            break;
        case CommandIDs::armTrack:
            result.setInfo("Arm Track", "Toggle track record arm", "Track", 0);
            result.addDefaultKeypress('r', juce::ModifierKeys::altModifier);
            break;

        // Pattern commands
        case CommandIDs::addPattern:
            result.setInfo("Add Pattern", "Create new pattern", "Pattern", 0);
            result.addDefaultKeypress('p', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            break;
        case CommandIDs::deletePattern:
            result.setInfo("Delete Pattern", "Delete selected pattern", "Pattern", 0);
            break;
        case CommandIDs::duplicatePattern:
            result.setInfo("Duplicate Pattern", "Duplicate pattern", "Pattern", 0);
            break;

        // Application commands
        case CommandIDs::preferences:
            result.setInfo("Preferences", "Open preferences", "Application", 0);
            result.addDefaultKeypress(',', juce::ModifierKeys::commandModifier);
            break;
        case CommandIDs::toggleFullscreen:
            result.setInfo("Toggle Fullscreen", "Toggle fullscreen mode", "Application", 0);
            result.addDefaultKeypress(juce::KeyPress::F11Key, 0);
            break;

        default:
            result.setInfo("Unknown", "Unknown command", "Unknown", 0);
            break;
    }
}

bool Commands::perform(const juce::ApplicationCommandTarget::InvocationInfo& info) {
    auto it = callbacks_.find(info.commandID);
    if (it != callbacks_.end() && it->second) {
        it->second();
        return true;
    }
    return false;
}

juce::ApplicationCommandTarget* Commands::getNextCommandTarget() {
    return nullptr;
}

void Commands::setCommandCallback(juce::CommandID commandID,
                                   std::function<void()> callback) {
    callbacks_[commandID] = std::move(callback);
}

void Commands::addDefaultKeyMappings(juce::ApplicationCommandManager& manager) {
    // Key mappings are added via getCommandInfo defaultKeypress
    juce::ignoreUnused(manager);
}

std::string getKeyMappingDescription(int commandID) {
    switch (commandID) {
        case CommandIDs::play:           return "Space";
        case CommandIDs::stop:           return "Shift+Space";
        case CommandIDs::record:         return "Ctrl+R";
        case CommandIDs::toggleLoop:     return "Ctrl+L";
        case CommandIDs::showPlaylist:   return "F5";
        case CommandIDs::showChannelRack:return "F6";
        case CommandIDs::showPianoRoll:  return "F7";
        case CommandIDs::showDevices:    return "F8";
        case CommandIDs::showMixer:      return "F9";
        case CommandIDs::zoomIn:         return "Ctrl++";
        case CommandIDs::zoomOut:        return "Ctrl+-";
        case CommandIDs::undo:           return "Ctrl+Z";
        case CommandIDs::redo:           return "Ctrl+Shift+Z";
        case CommandIDs::cut:            return "Ctrl+X";
        case CommandIDs::copy:           return "Ctrl+C";
        case CommandIDs::paste:          return "Ctrl+V";
        case CommandIDs::selectAll:      return "Ctrl+A";
        case CommandIDs::newProject:     return "Ctrl+N";
        case CommandIDs::openProject:    return "Ctrl+O";
        case CommandIDs::saveProject:    return "Ctrl+S";
        default:                         return "";
    }
}

} // namespace cppmusic::ui::core
