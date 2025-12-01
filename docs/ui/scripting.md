# Lua Scripting API

## Overview

The cppmusic DAW supports Lua scripting for user-defined actions, custom panels, and workflow automation. Scripts run in a sandboxed environment with resource limits for safety.

## Getting Started

### Script Location

Place Lua scripts in the `scripts/ui/actions/` directory. Scripts are loaded automatically on startup.

### Hello World

```lua
-- scripts/ui/actions/hello.lua

register_action({
    id = "hello.world",
    name = "Hello World",
    description = "A simple greeting",
    category = "Custom",
    callback = function()
        log("info", "Hello from Lua!")
    end
})
```

## API Reference

### Actions

#### register_action(config)

Register a new action available in the command palette.

```lua
register_action({
    id = "unique.action.id",        -- Required: unique identifier
    name = "Action Name",            -- Required: display name
    description = "What it does",    -- Optional: tooltip text
    category = "Category",           -- Optional: grouping in palette
    shortcut = "Ctrl+Shift+X",       -- Optional: keyboard shortcut
    callback = function()            -- Required: function to execute
        -- Your code here
    end
})
```

### Panels

#### create_panel(config)

Create a custom UI panel.

```lua
create_panel({
    id = "my.panel",           -- Required: unique identifier
    title = "Panel Title",     -- Required: window title
    draw = function()          -- Required: called each frame
        imgui.text("Hello!")
        if imgui.button("Click Me") then
            log("info", "Button clicked")
        end
    end
})
```

### Parameter Subscription

#### subscribe_param(param_id, callback)

Subscribe to parameter changes.

```lua
subscribe_param("transport.bpm", function(value)
    log("debug", "BPM changed to " .. value)
end)

subscribe_param("mixer.channel.1.volume", function(value)
    -- Volume is 0.0 to 1.0
end)
```

Available parameters:
- `transport.bpm`
- `transport.playing`
- `transport.recording`
- `transport.position`
- `mixer.channel.N.volume`
- `mixer.channel.N.pan`
- `mixer.channel.N.mute`
- `mixer.channel.N.solo`

### Transport Control

#### get_transport_state()

Get current transport state.

```lua
local state = get_transport_state()
-- state.playing: boolean
-- state.recording: boolean
-- state.position: number (beats)
-- state.bpm: number
-- state.time_sig_num: number
-- state.time_sig_den: number
```

#### set_transport_state(state)

Modify transport state.

```lua
set_transport_state({
    playing = true,
    position = 0,
    bpm = 140
})
```

### Selection

#### get_selection()

Get current selection object.

```lua
local sel = get_selection()

-- Get selected note IDs
local note_ids = sel:get_note_ids()

-- Get selected clip IDs  
local clip_ids = sel:get_clip_ids()

-- Get count
local count = sel:count()

-- Check if empty
if sel:is_empty() then
    log("info", "Nothing selected")
end

-- Add to selection
sel:add(note_id)

-- Remove from selection
sel:remove(note_id)

-- Clear selection
sel:clear()
```

### Notes

#### get_note(note_id)

Get a note by ID.

```lua
local note = get_note(123)
if note then
    -- note.id: number
    -- note.pitch: number (0-127)
    -- note.start_beat: number
    -- note.length_beats: number
    -- note.velocity: number (0.0-1.0)
    -- note.selected: boolean
end
```

#### update_note(note_id, changes)

Update a note's properties.

```lua
update_note(note_id, {
    pitch = 64,
    velocity = 0.8
})
```

#### get_visible_notes()

Get notes visible in current piano roll view.

```lua
local notes = get_visible_notes()
for _, note in ipairs(notes) do
    log("debug", "Note: " .. note.pitch)
end
```

#### get_all_notes()

Get all notes in current pattern (use sparingly for large datasets).

```lua
local notes = get_all_notes()
```

### Snap Settings

#### get_snap_settings()

Get current snap configuration.

```lua
local snap = get_snap_settings()
-- snap.enabled: boolean
-- snap.division: number (1, 2, 4, 8, 16, etc.)
-- snap.triplet: boolean
```

### Logging

#### log(level, message)

Output to the console log.

```lua
log("debug", "Debug message")
log("info", "Information")
log("warn", "Warning message")
log("error", "Error message")
```

### ImGui Bindings

A subset of ImGui is available for custom panels:

```lua
-- Text
imgui.text("Hello")
imgui.text_colored(r, g, b, a, "Colored text")
imgui.text_disabled("Disabled text")

-- Buttons
if imgui.button("Click") then
    -- Handle click
end
if imgui.small_button("Small") then end

-- Input
local changed, new_value = imgui.slider_float("Slider", current_value, 0.0, 1.0)
local changed, new_value = imgui.input_text("Label", current_text)

-- Layout
imgui.separator()
imgui.same_line()
imgui.spacing()
imgui.indent()
imgui.unindent()

-- Groups
if imgui.collapsing_header("Section") then
    -- Content
end

if imgui.begin_child("scrollable", 0, 100) then
    -- Scrollable content
    imgui.end_child()
end
```

## Security Model

### Sandbox Restrictions

For security, Lua scripts have limited capabilities:

| Feature | Allowed | Notes |
|---------|---------|-------|
| File read | ✅ Limited | Only `scripts/` directory |
| File write | ❌ | Not allowed |
| Network | ❌ | Not allowed by default |
| System calls | ❌ | `os.execute` disabled |
| Debug library | ❌ | Disabled |
| `loadstring` | ❌ | Dynamic code loading disabled |

### Resource Limits

| Limit | Default | Purpose |
|-------|---------|---------|
| Instructions | 1,000,000 per call | Prevent infinite loops |
| Memory | 16 MB | Prevent memory exhaustion |
| Call depth | 100 | Prevent stack overflow |
| Execution time | 5 seconds | Prevent hangs |

### Error Handling

Scripts that exceed limits are terminated:

```lua
-- This will be stopped after 1M instructions
while true do
    -- infinite loop
end
-- Error: "Instruction limit exceeded"
```

## Best Practices

### Do

- ✅ Use unique action IDs (namespace with your name)
- ✅ Provide descriptions for discoverability
- ✅ Handle nil values from API calls
- ✅ Use `get_visible_notes()` instead of `get_all_notes()` when possible
- ✅ Log errors for debugging

### Don't

- ❌ Create infinite loops
- ❌ Allocate large tables unnecessarily
- ❌ Call expensive operations every frame
- ❌ Assume API calls will always succeed

## Examples

### Transpose Selection

```lua
register_action({
    id = "custom.transpose_octave_up",
    name = "Transpose Octave Up",
    shortcut = "Ctrl+Up",
    callback = function()
        local sel = get_selection()
        for _, note_id in ipairs(sel:get_note_ids()) do
            local note = get_note(note_id)
            if note and note.pitch <= 115 then
                update_note(note_id, { pitch = note.pitch + 12 })
            end
        end
    end
})
```

### Randomize Velocity

```lua
register_action({
    id = "custom.randomize_velocity",
    name = "Randomize Velocity",
    callback = function()
        local sel = get_selection()
        for _, note_id in ipairs(sel:get_note_ids()) do
            local vel = 0.5 + math.random() * 0.5  -- 50-100%
            update_note(note_id, { velocity = vel })
        end
    end
})
```

### BPM Display Panel

```lua
local current_bpm = 120

subscribe_param("transport.bpm", function(value)
    current_bpm = value
end)

create_panel({
    id = "custom.bpm_display",
    title = "BPM",
    draw = function()
        imgui.text("Current BPM: " .. string.format("%.1f", current_bpm))
        
        if imgui.button("-") then
            set_transport_state({ bpm = current_bpm - 1 })
        end
        imgui.same_line()
        if imgui.button("+") then
            set_transport_state({ bpm = current_bpm + 1 })
        end
    end
})
```

## Troubleshooting

### Script not loading

1. Check file is in `scripts/ui/actions/`
2. Verify `.lua` extension
3. Check for syntax errors: `luac -p your_script.lua`

### Action not appearing

1. Verify `register_action` is called (not just defined)
2. Check action ID is unique
3. Look for errors in log

### Performance issues

1. Avoid calling `get_all_notes()` in panel `draw` functions
2. Cache values instead of recalculating each frame
3. Use `get_visible_notes()` for rendering

## Version History

| Version | Changes |
|---------|---------|
| 1.0 | Initial Lua scripting support |
