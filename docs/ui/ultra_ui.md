# Ultra UI Architecture

This document describes the architecture of the cppmusic DAW's Dear ImGui-based UI system, designed for production-grade performance and professional aesthetics.

## Overview

The Ultra UI system provides a modern, docking-enabled interface built with:
- **Dear ImGui** (docking branch) - Immediate mode GUI with advanced docking
- **SDL2** - Cross-platform window and input handling
- **OpenGL 3.3** - GPU-accelerated rendering

## Architecture

```
src/ui/imgui/
├── App.hpp/cpp           # Main application lifecycle
├── Theme.hpp/cpp         # Token-based theming system
├── Shortcuts.hpp/cpp     # Command palette & shortcuts
├── main.cpp              # Application entry point
├── panels/
│   ├── TransportBar      # Play/stop/record, BPM, meters
│   ├── BrowserPanel      # File/preset browser
│   ├── ChannelRackPanel  # Pattern step sequencer
│   ├── PianoRollPanel    # MIDI note editor
│   ├── PlaylistPanel     # Arrangement timeline
│   ├── MixerPanel        # Channel strips & faders
│   └── InspectorPanel    # Context-sensitive properties
└── tests/
    └── smoke_test.cpp    # Headless UI smoke test
```

## Theme System

### Token-Based Design

The theme system uses design tokens for consistency:

```json
{
  "spacingSm": 8,
  "spacingMd": 16,
  "radiusMd": 4,
  "fontSizeMd": 14,
  "windowBg": "#14141A",
  "buttonHovered": "#477AAD"
}
```

### Usage in Code

```cpp
const auto& tokens = theme.getTokens();
float padding = tokens.spacingSm * theme.getDpiScale();
ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
```

### Live Reload

Themes support hot-reload during development:

```cpp
if (theme.reloadIfModified()) {
    theme.applyToImGui();
}
```

### Custom Theme Files

Place themes in `assets/themes/` and load with:

```bash
./cppmusic_imgui_app --theme assets/themes/my_theme.json
```

## Panel Conventions

### Base Structure

Each panel follows this pattern:

```cpp
class MyPanel {
public:
    void draw(bool& open, const Theme& theme);
    
private:
    // State managed internally
    // No heap allocations in steady-state
};
```

### Drawing Pattern

```cpp
void MyPanel::draw(bool& open, const Theme& theme) {
    if (!open) return;
    
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    
    ImGui::PushStyleVar(...);
    
    if (ImGui::Begin("Panel Name", &open)) {
        // Draw content
    }
    ImGui::End();
    
    ImGui::PopStyleVar();
}
```

### No Heap Allocations

For 60-144Hz rendering, avoid allocations in draw loops:
- Pre-allocate buffers
- Use fixed-size arrays where possible
- Pool objects that need dynamic creation

## DPI Scaling

### Auto-Detection

DPI is auto-detected from the display:

```cpp
float dpi = SDL_GetDisplayDPI(displayIndex, &ddpi, &hdpi, &vdpi);
float scale = dpi / 96.0f;
```

### Manual Override

```cpp
AppConfig config;
config.dpiScale = 2.0f;  // Force 2x scaling
app.initialize(config);
```

### Font Scaling

Fonts are loaded at the appropriate size:

```cpp
float fontSize = tokens.fontSizeMd * dpiScale;
io.Fonts->AddFontFromFileTTF(fontPath, fontSize);
```

## Command Palette

### Opening

- **Shortcut**: `Ctrl+K` (or `Cmd+K` on macOS)
- **Menu**: Help → Command Palette

### Registering Commands

```cpp
shortcuts.registerCommand(
    "my.command",           // ID
    "My Command",           // Name
    "Category",             // Category for grouping
    {ImGuiKey_M, KeyMod::Ctrl},  // Shortcut
    []() { /* action */ },  // Callback
    "Description"           // Optional description
);
```

### Fuzzy Search

Commands are searchable by:
- Name
- Category
- ID
- Description

## Performance

### Frame Budget

Target frame times:
- 60 Hz: 16.67ms budget
- 144 Hz: 6.94ms budget

### Performance Overlay

Press `F12` to toggle the performance overlay showing:
- FPS and frame time
- Draw calls
- Vertex/index counts
- Frame budget usage

### Optimization Tips

1. **Batch Drawing**: Group similar draw calls
2. **Clip Early**: Check visibility before drawing
3. **Cache Expensive Computations**: Waveforms, peak data
4. **Use Atlas**: Combine small textures

### Memory

Zero heap allocations in steady-state panel draws:
- Pre-allocate working buffers
- Use stack allocations for temporary data
- Pool frequently created objects

## Keyboard Shortcuts

### Default Shortcuts

| Action | Shortcut |
|--------|----------|
| Command Palette | Ctrl+K |
| Performance Overlay | F12 |
| Play/Pause | Space |
| Stop | Enter |
| Record | Ctrl+R |
| Save | Ctrl+S |
| Undo | Ctrl+Z |
| Redo | Ctrl+Y |
| Toggle Browser | Ctrl+B |
| Toggle Mixer | Ctrl+M |
| Toggle Piano Roll | Ctrl+P |

### Customization

Shortcuts are saved to `config/shortcuts.json`:

```json
{
  "file.save": "Ctrl+S",
  "transport.play": "Space",
  "view.mixer": "Ctrl+M"
}
```

## Building

### Dependencies

```bash
sudo apt install libsdl2-dev libgl-dev
```

### CMake Configuration

The ImGui UI is enabled by default:

```cmake
option(ENABLE_IMGUI_UI "Enable Dear ImGui-based UI" ON)
```

### Build Commands

```bash
mkdir build && cd build
cmake -G Ninja ..
cmake --build . --target cppmusic_imgui_app
```

### Running

```bash
./bin/cppmusic_imgui_app [options]

Options:
  -f, --fullscreen    Start in fullscreen mode
  --no-vsync          Disable vertical sync
  -t, --theme <path>  Path to theme JSON file
  -w, --width <px>    Window width (default: 1920)
  -h, --height <px>   Window height (default: 1080)
```

## Testing

### Smoke Test

The smoke test validates UI construction without a display:

```bash
./imgui_smoke_test
```

Tests:
- Theme token defaults
- DPI scaling
- Shortcut registration
- Command search
- Panel construction

### CI Integration

CI runs with Xvfb for headless testing:

```yaml
- name: Run ImGui smoke test
  run: xvfb-run ./build/imgui_smoke_test
```

## Future Work

- [ ] GPU waveform renderer
- [ ] Large session virtualization
- [ ] Theme variants (light, high-contrast)
- [ ] Accessibility features
- [ ] Animation micro-interactions
- [ ] Real engine data integration
