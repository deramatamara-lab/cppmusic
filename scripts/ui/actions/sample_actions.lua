-- sample_actions.lua
-- Sample Lua extension demonstrating the cppmusic DAW scripting API
-- This file shows how to register custom actions, create panels, and
-- interact with the DAW state.

-- Note: This is a demonstration file. The actual Lua VM integration
-- requires ENABLE_LUA_SCRIPTING to be enabled in CMake.

-- ============================================================================
-- Basic Action Registration
-- ============================================================================

-- Register a simple action that can be invoked from the command palette
register_action({
    id = "sample.hello",
    name = "Hello World",
    description = "Shows a greeting message",
    category = "Sample Actions",
    shortcut = "Ctrl+Shift+H",
    callback = function()
        log("info", "Hello from Lua scripting!")
    end
})

-- Register an action that manipulates the selection
register_action({
    id = "sample.select_all_notes",
    name = "Select All Notes in View",
    description = "Selects all notes visible in the piano roll",
    category = "Sample Actions",
    callback = function()
        local selection = get_selection()
        local notes = get_visible_notes()
        
        for _, note in ipairs(notes) do
            selection:add(note.id)
        end
        
        log("info", "Selected " .. #notes .. " notes")
    end
})

-- ============================================================================
-- Transport Control Actions
-- ============================================================================

register_action({
    id = "sample.toggle_play",
    name = "Toggle Play/Stop",
    description = "Toggle playback state",
    category = "Transport",
    callback = function()
        local state = get_transport_state()
        if state.playing then
            set_transport_state({ playing = false })
        else
            set_transport_state({ playing = true })
        end
    end
})

register_action({
    id = "sample.goto_start",
    name = "Go to Start",
    description = "Move playhead to the beginning",
    category = "Transport",
    callback = function()
        set_transport_state({ position = 0 })
    end
})

-- ============================================================================
-- Note Manipulation Actions
-- ============================================================================

register_action({
    id = "sample.quantize_notes",
    name = "Quantize Selected Notes",
    description = "Quantize selected notes to grid",
    category = "MIDI",
    callback = function()
        local selection = get_selection()
        local snap = get_snap_settings()
        
        for _, note_id in ipairs(selection:get_note_ids()) do
            local note = get_note(note_id)
            if note then
                local quantized_start = quantize_beat(note.start_beat, snap.division)
                update_note(note_id, { start_beat = quantized_start })
            end
        end
        
        log("info", "Quantized " .. selection:count() .. " notes")
    end
})

register_action({
    id = "sample.transpose_up",
    name = "Transpose Up",
    description = "Transpose selected notes up by one semitone",
    category = "MIDI",
    callback = function()
        local selection = get_selection()
        
        for _, note_id in ipairs(selection:get_note_ids()) do
            local note = get_note(note_id)
            if note and note.pitch < 127 then
                update_note(note_id, { pitch = note.pitch + 1 })
            end
        end
    end
})

register_action({
    id = "sample.transpose_down",
    name = "Transpose Down",
    description = "Transpose selected notes down by one semitone",
    category = "MIDI",
    callback = function()
        local selection = get_selection()
        
        for _, note_id in ipairs(selection:get_note_ids()) do
            local note = get_note(note_id)
            if note and note.pitch > 0 then
                update_note(note_id, { pitch = note.pitch - 1 })
            end
        end
    end
})

-- ============================================================================
-- Custom Panel Example
-- ============================================================================

-- Create a custom panel that displays note statistics
create_panel({
    id = "sample.note_stats",
    title = "Note Statistics",
    draw = function()
        local notes = get_all_notes()
        local total = #notes
        local selected = get_selection():count()
        
        -- Calculate statistics
        local min_pitch = 127
        local max_pitch = 0
        local total_velocity = 0
        
        for _, note in ipairs(notes) do
            min_pitch = math.min(min_pitch, note.pitch)
            max_pitch = math.max(max_pitch, note.pitch)
            total_velocity = total_velocity + note.velocity
        end
        
        local avg_velocity = total > 0 and (total_velocity / total) or 0
        
        -- Display statistics
        imgui.text("Total Notes: " .. total)
        imgui.text("Selected: " .. selected)
        imgui.separator()
        
        if total > 0 then
            imgui.text("Pitch Range: " .. pitch_to_name(min_pitch) .. " - " .. pitch_to_name(max_pitch))
            imgui.text("Avg Velocity: " .. string.format("%.1f", avg_velocity * 100) .. "%")
        else
            imgui.text_disabled("No notes in pattern")
        end
    end
})

-- ============================================================================
-- Parameter Subscription Example
-- ============================================================================

-- Subscribe to BPM changes
subscribe_param("transport.bpm", function(value)
    log("debug", "BPM changed to: " .. value)
end)

-- Subscribe to mixer channel volume
subscribe_param("mixer.channel.1.volume", function(value)
    log("debug", "Channel 1 volume: " .. string.format("%.1f", value * 100) .. "%")
end)

-- ============================================================================
-- Utility Functions
-- ============================================================================

-- Convert MIDI pitch to note name
function pitch_to_name(pitch)
    local notes = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}
    local octave = math.floor(pitch / 12) - 1
    local note = notes[(pitch % 12) + 1]
    return note .. octave
end

-- Quantize a beat position to a grid division
function quantize_beat(beat, division)
    local interval = 4.0 / division  -- Assuming 4 beats per bar
    return math.floor(beat / interval + 0.5) * interval
end

-- ============================================================================
-- Initialization
-- ============================================================================

log("info", "Sample actions loaded successfully")
log("info", "Available actions: sample.hello, sample.select_all_notes, sample.quantize_notes")
