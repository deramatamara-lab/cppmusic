# DAW UI Evolution Plan (FL-Style Unified Shell)

Goal: transform the current “dashboard” shell into a **single, FL Studio–style DAW window** with a usable workflow:
- persistent transport + status bar
- playlist/arrange view
- pattern/channel rack + step sequencer
- per-pattern piano roll editor
- docked mixer with inserts/sends
- left-side browser
- integrated flagship/AI panels

The engine and project model already exist; this plan focuses on **layout, navigation, and UX**.

---

## Phase 1 – Layout Skeleton Like FL Studio

### 1.1 Lock the Global Frame

**Target files**  
- `src/ui/App.h/.cpp`  
- `src/ui/MainWindow.h/.cpp`  
- `src/ui/views/MainView.{h,cpp}`  

**Tasks**  
- Keep **one entry point**: `START_JUCE_APPLICATION(daw::ui::App)` (already configured).  
- Treat `MainWindow` as a thin shell; all layout lives in `MainView`.  
- In `MainView`, formalize the main regions:

  - **Top strip**: `TransportBar` (already exists) + project info + CPU/memory meters.
  - **Left dock**: `BrowserPanel` (files, presets, channels).
  - **Center**: `ArrangeView` (playlist).
  - **Right dock**: `InspectorPanel` (clip/track properties).
  - **Bottom dock**: `MixerView` (mixer) + collapsible piano roll/step sequencer strip.

**Acceptance criteria**  
- Main window opens with a **stable, resizable layout** that roughly mirrors the FL screenshot:
  - top transport,
  - left browser column,
  - central playlist,
  - right inspector,
  - bottom mixer strip.

### 1.2 Bring New Panels Into This Frame

**Target files**  
- `src/ui/views/MainView.{h,cpp}`  
- `src/ui/components/FlagshipDevicePanel.{h,cpp}`  
- `src/ui/components/PatternSequencerPanel.{h,cpp}`  
- `src/ui/components/SessionLauncherView.{h,cpp}`  

**Tasks**  
- Mount:
  - `FlagshipDevicePanel` as a **bottom-left “AI Mastering” dock** or right-side collapsible panel (like FL’s plugin window).
  - `PatternSequencerPanel` in a **“Channel rack / Step sequencer” slot** under the transport or to the right of the playlist.
  - `SessionLauncherView` as a **Session/Performance lane** that can be toggled above the mixer (hidden by default).

- Replace demo tempo/play wiring with real `EngineContext` hooks:
  - subscribe to engine tempo / play state,
  - update `PatternSequencerPanel` and `SessionLauncherView` on transport changes,
  - send simple callbacks back into engine for play/stop (even if dummy at first).

**Acceptance criteria**  
- New panels live **inside `MainView`**, not in a separate window.  
- Hitting play in the transport visually updates sequencer / launcher.  
- Layout is responsive to window resize (no overlaps, minimum sizes respected).

---

## Phase 2 – Playlist & Pattern Workflow

### 2.1 FL-Style Playlist Lanes

**Target files**  
- `src/ui/views/ArrangeView.{h,cpp}`  
- `src/project/Track.{h,cpp}`  
- `src/project/Clip.{h,cpp}`  

**Tasks**  
- Upgrade `ArrangeView` to show:
  - track headers on the left,
  - timeline ruler on top,
  - clips as colored blocks (MIDI/audio) like FL’s playlist.
- Add basic interactions:
  - click to select clip,
  - drag horizontally to move,
  - (later) drag-copy with modifier key.

**Acceptance criteria**  
- User can see “Track 1” lane with at least one clip, visually aligned to the timeline.
- Selecting a clip updates `InspectorPanel` and a “current pattern” pointer.

### 2.2 Channel Rack / Pattern Linking

**Target files**  
- `src/ui/components/PatternSequencerPanel.{h,cpp}`  
- `src/project/Pattern.{h,cpp}`  
- `src/project/ProjectModel.{h,cpp}`  

**Tasks**  
- Treat `PatternSequencerPanel` as the **channel rack** for the currently selected pattern:
  - bind its `setPattern(...)` to the pattern associated with the selected clip or “current pattern”,
  - ensure toggling steps updates the underlying `Pattern` model.
- Add pattern list (simple `ComboBox` or `ListBox`) near the Pattern Sequencer:
  - create new pattern,
  - rename pattern,
  - select which pattern is edited.

**Acceptance criteria**  
- Selecting a pattern changes the step sequencer state.
- Toggling steps updates the project model; reopening the project reproduces the pattern.

---

## Phase 3 – Piano Roll Integration

### 3.1 Docked Piano Roll Panel

**Target files**  
- `src/ui/views` (new `PianoRollView` or reuse existing if present)  
- `src/ui/views/MainView.{h,cpp}`  
- `src/project/Pattern.{h,cpp}`  

**Tasks**  
- Create or wire a `PianoRollView` that:
  - shows notes for the currently selected clip/pattern,
  - supports basic editing: add/move/remove notes with mouse, velocity editing in a lane.
- Layout:
  - Dock piano roll in the **bottom center** above the mixer (like FL’s bottom editor),
  - Provide toggle buttons in the transport or near the playlist: “Playlist / Piano Roll”.

**Acceptance criteria**  
- Selecting a clip with MIDI opens its notes in Piano Roll.
- Editing notes in the piano roll updates the pattern and is reflected in playback.

---

## Phase 4 – Mixer Upgrade

### 4.1 FL-Style Mixer Strips

**Target files**  
- `src/ui/views/MixerView.{h,cpp}`  
- `src/audio/engine/AudioGraph.{h,cpp}`  
- `src/project/Track.{h,cpp}`  

**Tasks**  
- Ensure each track in `ProjectModel` maps to a mixer strip:
  - fader, pan, mute/solo,
  - basic meter (peak/RMS).
- Layout:
  - Horizontal strip row like FL’s mixer with scroll if many tracks.
- Hook controls:
  - fader/knob movements propagate to engine parameters,
  - engine level meters feed back into UI with throttled updates.

**Acceptance criteria**  
- Each track in the playlist has a corresponding mixer strip.
- Fader moves audibly (or at minimum, value changes are visible and routed).

---

## Phase 5 – Browser & Inspector Quality

### 5.1 Browser as First-Class Navigation

**Target files**  
- `src/ui/views/BrowserPanel.{h,cpp}`  
- `src/project/ProjectModel.{h,cpp}`  

**Tasks**  
- Structure browser tabs:
  - “Current project” (patterns, tracks),
  - “Samples”,
  - “Plugins”,
  - “Presets”.
- Support drag-drop:
  - drag sample from browser into playlist → create audio clip,
  - drag instrument into track/channel rack → attach plugin (stub at first).

**Acceptance criteria**  
- Browser is the main entry point to add content, not a dead rectangle.

### 5.2 Inspector as Context Panel

**Target files**  
- `src/ui/views/InspectorPanel.{h,cpp}`  

**Tasks**  
- Display:
  - **Track mode**: name, color, routing, volume/pan snapshot.
  - **Clip mode**: source (pattern/sample), length, color.
  - **Pattern mode**: number of steps, swing, target channel.

**Acceptance criteria**  
- Clicking different objects (track, clip, pattern) updates Inspector with relevant controls.

---

## Phase 6 – FL-Grade Navigation & Polish

### 6.1 Global Navigation & Shortcuts

**Target files**  
- `src/ui/views/MainView.{h,cpp}`  
- `src/ui/App.cpp`  

**Tasks**  
- Implement keyboard shortcuts:
  - Space: play/stop,
  - Ctrl/Cmd+N: new project,
  - Ctrl/Cmd+S: save,
  - Number keys: switch patterns.
- Zoom & scroll:
  - horizontal zoom for playlist,
  - vertical zoom for mixer and piano roll.

**Acceptance criteria**  
- Usable without mouse-only clicking; navigating around feels closer to a real DAW.

### 6.2 Design System Alignment (FL-Inspired)

**Target files**  
- `src/ui/lookandfeel/DesignSystem.{h,cpp}`  
- `src/ui/lookandfeel/DesignTokens.{h,cpp}`  
- `src/ui/lookandfeel/MainLookAndFeel.{h,cpp}`  

**Tasks**  
- Tighten color and typography:
  - reduce gradients and “toy” look,
  - keep neon accents but default to flat, professional bases.
- Standardize:
  - track backgrounds,
  - piano roll grid lines,
  - mixer channel colors.

**Acceptance criteria**  
- UI looks cohesive: same font hierarchy, consistent color tokens, no mismatched gradients.
- Screenshots feel closer to FL Studio “pro” density vs current “demo dashboard”.

---

## Phase 7 – Production-Ready UX

### 7.1 Persistence & Projects

**Target files**  
- `src/project/ProjectSerializer.{h,cpp}`  
- `src/project/ProjectModel.{h,cpp}`  
- `src/ui/App.cpp`  

**Tasks**  
- Save/Load:
  - tracks, clips, patterns, mixer settings, browser state.
- “Recent projects” list in the browser or a startup screen.

**Acceptance criteria**  
- Closing and reopening the DAW restores the same playlist/mixer/pattern setup.

### 7.2 Performance & QA

**Tasks**  
- Profile UI: ensure 60fps while moving clips and resizing.
- Fix obvious UX pain points:
  - hit targets, drag handles, scroll behavior, selection clarity.

**Acceptance criteria**  
- No obvious jank when interacting with playlist/mixer/piano roll.
- Basic smoke tests for “create pattern → sequence → mix” pass without assertions.

---

## Milestone Summary

1. **Phase 1** – Single shell with FL-like frame, new panels inside `MainView`.
2. **Phase 2** – Playlist + pattern/channel rack actually drive the project model.
3. **Phase 3** – Piano roll editor wired to patterns.
4. **Phase 4** – Mixer mirrors tracks and responds to changes.
5. **Phase 5** – Browser and Inspector become functional navigation tools.
6. **Phase 6** – Shortcuts, zooming, and design cohesion.
7. **Phase 7** – Real projects, persistence, performance.

---

## Implementation Notes

- Keep **audio engine and project model as source of truth**; UI is just a view/controller.
- Avoid big-bang rewrites: after each phase, the app must build and remain usable.
- For each feature (playlist, piano roll, mixer), enforce **unit tests** in `tests/ui` or `tests/project` that at least validate model changes.
