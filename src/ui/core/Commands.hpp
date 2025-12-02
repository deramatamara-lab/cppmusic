/**
 * @file Commands.hpp
 * @brief Command IDs and keyboard mappings for the DAW.
 *
 * Defines command IDs for:
 * - Transport (play, stop, record, loop)
 * - View switching (playlist, channel rack, piano roll, mixer)
 * - Zoom controls
 * - Panel toggles (browser, mixer, channel rack)
 *
 * Integrates with JUCE command manager and key mappings.
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <string>

namespace cppmusic::ui::core {

/**
 * @brief Command IDs for DAW operations
 *
 * All command IDs are grouped by category for easy reference.
 * These integrate with JUCE's ApplicationCommandManager.
 */
struct CommandIDs {
    // =========================================================================
    // Transport Commands (1000-1099)
    // =========================================================================
    static constexpr int play               = 1000;  ///< Start playback
    static constexpr int stop               = 1001;  ///< Stop playback
    static constexpr int record             = 1002;  ///< Toggle recording
    static constexpr int toggleLoop         = 1003;  ///< Toggle loop mode
    static constexpr int toggleMetronome    = 1004;  ///< Toggle metronome
    static constexpr int tapTempo           = 1005;  ///< Tap tempo
    static constexpr int gotoStart          = 1006;  ///< Go to project start
    static constexpr int gotoEnd            = 1007;  ///< Go to project end
    static constexpr int gotoMarker         = 1008;  ///< Go to specific marker
    static constexpr int setLoopStart       = 1009;  ///< Set loop start point
    static constexpr int setLoopEnd         = 1010;  ///< Set loop end point

    // =========================================================================
    // View Commands (1100-1199)
    // =========================================================================
    static constexpr int showPlaylist       = 1100;  ///< Show playlist view
    static constexpr int showChannelRack    = 1101;  ///< Show channel rack
    static constexpr int showPianoRoll      = 1102;  ///< Show piano roll
    static constexpr int showMixer          = 1103;  ///< Show mixer view
    static constexpr int showDevices        = 1104;  ///< Show devices view
    static constexpr int toggleBrowser      = 1105;  ///< Toggle browser panel
    static constexpr int toggleInspector    = 1106;  ///< Toggle inspector panel

    // =========================================================================
    // Zoom Commands (1200-1299)
    // =========================================================================
    static constexpr int zoomIn             = 1200;  ///< Zoom in horizontally
    static constexpr int zoomOut            = 1201;  ///< Zoom out horizontally
    static constexpr int zoomToFit          = 1202;  ///< Zoom to fit content
    static constexpr int zoomToSelection    = 1203;  ///< Zoom to selection
    static constexpr int zoomVerticalIn     = 1204;  ///< Zoom in vertically
    static constexpr int zoomVerticalOut    = 1205;  ///< Zoom out vertically

    // =========================================================================
    // Edit Commands (1300-1399)
    // =========================================================================
    static constexpr int undo               = 1300;  ///< Undo last action
    static constexpr int redo               = 1301;  ///< Redo last undone action
    static constexpr int cut                = 1302;  ///< Cut selection
    static constexpr int copy               = 1303;  ///< Copy selection
    static constexpr int paste              = 1304;  ///< Paste from clipboard
    static constexpr int deleteSelection    = 1305;  ///< Delete selection
    static constexpr int selectAll          = 1306;  ///< Select all items
    static constexpr int deselectAll        = 1307;  ///< Deselect all items
    static constexpr int duplicate          = 1308;  ///< Duplicate selection
    static constexpr int quantize           = 1309;  ///< Quantize selection

    // =========================================================================
    // File Commands (1400-1499)
    // =========================================================================
    static constexpr int newProject         = 1400;  ///< Create new project
    static constexpr int openProject        = 1401;  ///< Open existing project
    static constexpr int saveProject        = 1402;  ///< Save current project
    static constexpr int saveProjectAs      = 1403;  ///< Save project as new file
    static constexpr int exportAudio        = 1404;  ///< Export audio file
    static constexpr int exportMIDI         = 1405;  ///< Export MIDI file
    static constexpr int projectSettings    = 1406;  ///< Open project settings

    // =========================================================================
    // Track Commands (1500-1599)
    // =========================================================================
    static constexpr int addTrack           = 1500;  ///< Add new track
    static constexpr int deleteTrack        = 1501;  ///< Delete selected track
    static constexpr int muteTrack          = 1502;  ///< Toggle track mute
    static constexpr int soloTrack          = 1503;  ///< Toggle track solo
    static constexpr int armTrack           = 1504;  ///< Toggle track record arm
    static constexpr int duplicateTrack     = 1505;  ///< Duplicate selected track
    static constexpr int groupTracks        = 1506;  ///< Group selected tracks

    // =========================================================================
    // Pattern Commands (1600-1699)
    // =========================================================================
    static constexpr int addPattern         = 1600;  ///< Create new pattern
    static constexpr int deletePattern      = 1601;  ///< Delete selected pattern
    static constexpr int duplicatePattern   = 1602;  ///< Duplicate pattern
    static constexpr int renamePattern      = 1603;  ///< Rename pattern
    static constexpr int splitPattern       = 1604;  ///< Split pattern at playhead
    static constexpr int mergePatterns      = 1605;  ///< Merge selected patterns

    // =========================================================================
    // Application Commands (1700-1799)
    // =========================================================================
    static constexpr int preferences        = 1700;  ///< Open preferences
    static constexpr int showAbout          = 1701;  ///< Show about dialog
    static constexpr int showHelp           = 1702;  ///< Show help documentation
    static constexpr int toggleFullscreen   = 1703;  ///< Toggle fullscreen mode
};

/**
 * @brief Command manager for DAW operations
 *
 * Extends JUCE's ApplicationCommandManager to provide
 * FL-style keyboard shortcuts and command handling.
 */
class Commands : public juce::ApplicationCommandTarget {
public:
    Commands();
    ~Commands() override = default;

    /**
     * @brief Initialize the command manager with default key mappings
     */
    void initialize(juce::ApplicationCommandManager& manager);

    /**
     * @brief Get all command info for registration
     */
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;

    /**
     * @brief Get info for a specific command
     */
    void getCommandInfo(juce::CommandID commandID, 
                        juce::ApplicationCommandInfo& result) override;

    /**
     * @brief Perform a command
     */
    bool perform(const juce::ApplicationCommandTarget::InvocationInfo& info) override;

    /**
     * @brief Get the next command target in the chain
     */
    juce::ApplicationCommandTarget* getNextCommandTarget() override;

    /**
     * @brief Set a callback for command execution
     */
    void setCommandCallback(juce::CommandID commandID, 
                            std::function<void()> callback);

private:
    std::unordered_map<juce::CommandID, std::function<void()>> callbacks_;

    // Helper to add default key mappings
    static void addDefaultKeyMappings(juce::ApplicationCommandManager& manager);
};

/**
 * @brief Get the key mapping description for a command
 * @param commandID The command ID
 * @return Human-readable key combination string
 */
std::string getKeyMappingDescription(int commandID);

} // namespace cppmusic::ui::core
