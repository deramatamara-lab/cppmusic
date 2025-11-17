# DAW Upgrade & UI Modernisation Playbook

Single reference for transforming this JUCE-based DAW into a product that eclipses Ableton/Logic while keeping the playful FL Studio UX and matching the premium aesthetics from the reference screenshots.

---

## Table of Contents
1. [Guiding Principles](#guiding-principles)
2. [Part A – Product & Architecture Plan](#part-a--product--architecture-plan)
3. [Part B – Visual/UI Plan](#part-b--visualui-plan)
4. [Part C – JUCE Implementation Starter Kit](#part-c--juce-implementation-starter-kit)
5. [Part D – Roadmap & Milestones](#part-d--roadmap--milestones)

---

## Guiding Principles
- **Pattern-first, clip-centric** workflow like FL, but with bleeding-edge AI assistance and modular routing.
- **Zero friction**: advanced features surface through existing FL-style tools, never buried in menus.
- **Strict performance**: <5 ms audio latency, sample-accurate automation, deterministic scheduling.
- **Visual identity**: cinematic gradients, neon accents, macro knobs, and motion reminiscent of Native Instruments/Ozone.

---

## Part A – Product & Architecture Plan

### A1. Positioning & Repository Alignment
- **Target niche**: FL-inspired spontaneity + Bitwig-style modulation + integrated AI composition.
- **Module mapping**:
  - `src/audio/engine`, `audio/processors`: low-latency audio graph.
  - `src/project`: playlist, clips, automation, macros.
  - `src/core/state`: application state, undo/redo, parameter manager.
  - `src/ui`: layout, editors, device panels.

### A2. Audio Engine Foundations
1. **Low-latency core (<5 ms)**
	- Fix block size and lock-free queues between UI/audio threads in `audio/engine/DawEngine` & `AudioGraph`.
	- Enforce allocation-free `prepareToPlay`/`processBlock` for every processor.
	- Add CPU/xrun counters to `core/utilities/Logger` for per-node diagnostics.
2. **Sample-accurate modulation & automation**
	- Extend `AudioProcessorBase` to accept per-sample parameter buffers.
	- Pre-render automation curves from `project/Track`, `Clip` into modulation buffers.
	- Central `audio/engine/EngineContext` modulation matrix routing LFOs, envelopes, MIDI.
3. **Hybrid linear + pattern playback**
	- Treat pattern clips as first-class in `project/Clip` with cached MIDI “playback lists”.
	- Execution chain: global transport → playlist timeline → track clips → patterns → instrument queues.
4. **Parallel/distributed processing**
	- Topologically schedule independent subgraphs across worker threads via a lock-free job system.
	- Annotate processors with latency/side-effect metadata to support plugin delay compensation (PDC).
5. **Plugin sandboxing**
	- Future `src/plugins/` process hosting with shared-memory audio IPC and crash isolation.
	- Capability database stored in `core/state/ApplicationState` (channels, MIDI outs, latency, sidechain support).

### A3. Creative Tools Beyond Ableton/Logic
1. **World-class piano roll & step sequencer**
	- `ui/views/PianoRollView`: MPE lanes, scale snapping, ghost notes, advanced edit tools (strum, humanize, chord stamps).
	- `ui/components/StepSequencer`: per-lane velocity/probability/micro-timing + Elektron-style trig conditions.
	- AI hooks: buttons like “Generate Variation”, “Harmonize” invoking `ai/inference/InferenceEngine`.
2. **AI-assisted workflows**
	- Models for groove extraction, melody continuation, chord advice, preset design in `ai/models`.
	- Non-destructive application: AI writes new pattern/preset versions, user A/Bs via UI.
3. **Advanced modulation**
	- Reusable modulators in `audio/synthesis` & `audio/effects` (LFO, step, random, envelopes).
	- Drag-to-assign UI with halos showing depth.
4. **Flagship internal devices**
	- Upgrade `audio/synthesis/Synthesizer`, `Oscillator` to support wavetable, FM, multi-sample engines.
	- Effects (`Reverb`, `Delay`, `Compressor`) gain multi-band, M/S, saturation stages; share a modern device shell UI.
5. **Live clip launching**
	- `ui/views/ClipLauncher`: scenes vs tracks, per-scene quantize, performance recording back into arrangement.
	- Transport maintains Arrangement & Session timelines simultaneously for deterministic resampling.

### A4. Workflow & UX Parity with FL
- **Layout defaults**: top transport, left content browser, center playlist, bottom focused editor, right mixer/channel rack toggle.
- **Single-click focus**: selecting any clip auto-loads matching editor in bottom panel.
- **Channel-centric design**: each `project/Track` bundles instrument + insert FX; device chains open inline from channel rack.
- **Macro system**: expand `core/state/ParameterManager` for device/global macros, each with ranges & curves; automation targets macros.
- **Browser & assets**: tag-based indexing stored in `ApplicationState`; drag-and-drop into playlist, channels, devices.
- **Undo/history**: command-pattern actions recorded centrally with a dedicated history panel snapshot navigator.

### A5. Quality, Stability, Ecosystem
- **Project format**: versioned JSON/protobuf in `project/ProjectModel`, migrations per version, importers for Ableton/FL stems.
- **Testing & benchmarks**: expand `tests/audio`, `tests/core`, `tests/ui` with multithread invariants, golden audio renders, latency benchmarks.
- **Scripting/extensibility**: plan embedded Lua/JS API (read-only on RT threads), enabling custom tools/generative devices.

---

## Part B – Visual/UI Plan (Premium JUCE Look)

### B1. Design System Foundation
- **Design tokens**: central `ui/lookandfeel/DesignTokens.h` for colors, radii, spacing, elevation, typography.
- **Global LookAndFeel**: `ui/lookandfeel/MainLookAndFeel` overrides sliders, buttons, combos, text editors, tabs, scrollbars; exposes `drawPanelBackground` helper.
- **Assets & imagery**: curated gradients/abstract renders loaded as `juce::Image` resources and composited into hero panels.

### B2. Component Language
- **Panels & shells**: base `ui/components/Panel` with rounded corners, soft glow, gradient fill, optional header.
- **Dials & sliders**: neon macro knobs with halo arcs, tick marks, and modulation rings; velocity-sensitive dragging.
- **Buttons/toggles**: pill buttons with glow + active outlines; icon buttons with hover plates.
- **Meters & visualizers**: reusable LED meters, FFT/spectrum widgets (potential OpenGL) for master/analyzer modules.
- **Modal overlays**: dimming, spring-eased panels, optional blur for “glassmorphism”.

### B3. Layout & Screen Concepts
- **Workspace**: gradient background, panels not touching window edge; consistent chrome across browser/playlist/mixer.
- **Device shell**: top bar (title, preset, A/B, randomize), central visualization, lower macro row, collapsible advanced panels.
- **Piano roll**: gradient per pitch region, translucent ghost notes, curved automation lanes.
- **Clip launcher & channel rack**: rounded tiles showing play/queue state animations; channel rows with meters, icons, device previews.

### B4. Motion & Feedback
- **Micro-animations**: timer-driven fades, knob inertia, panel open/close transitions (120–200 ms sweet spot).
- **State transitions**: crossfades/slides between arrangement vs mixer vs browser focus; animated clip selection highlight.
- **Affordances**: hover indicators everywhere; destructive actions require animated confirmations.

### B5. Implementation Phases (Visual)
1. **Phase 1 – Visual base (1–2 wks)**: implement tokens + LookAndFeel, reskin top-level components.
2. **Phase 2 – Core views (2–4 wks)**: build FL-style layout, piano roll, step sequencer.
3. **Phase 3 – Devices & visualizers (3–5 wks)**: create flagship synth/effect shells with analyzers.
4. **Phase 4 – Engine & modulation (4–8+ wks)**: hook sample-accurate automation + modulators; surface macros visually.
5. **Phase 5 – AI & performance (parallel)**: integrate AI assists into editors and clip launcher.
6. **Phase 6 – Hardening**: profiling, accessibility, localization, preset UX.

---

## Part C – JUCE Implementation Starter Kit

### C1. Design Tokens Header
Create `src/ui/lookandfeel/DesignTokens.h` and expose a global `ui::tokens` struct.

```cpp
#pragma once

#include <juce_graphics/juce_graphics.h>

namespace ui
{
	 struct ColorTokens
	 {
		  juce::Colour background      { 0xff05030b };
		  juce::Colour backgroundAlt   { 0xff0c0618 };
		  juce::Colour panelBackground { 0xff120b26 };
		  juce::Colour panelHighlight  { 0xff24134a };
		  juce::Colour panelBorder     { 0x40ffffff };
		  juce::Colour accentPrimary   { 0xff8b5bff };
		  juce::Colour accentSecondary { 0xff00d0ff };
		  juce::Colour accentWarning   { 0xffffc857 };
		  juce::Colour textPrimary     { 0xfff5f5ff };
		  juce::Colour textSecondary   { 0xffa0a0c0 };
		  juce::Colour textDisabled    { 0xff55556b };
	 };

	 struct RadiusTokens { float small{4.0f}; float medium{8.0f}; float large{14.0f}; };
	 struct SpacingTokens{ int xxs{4}; int xs{8}; int sm{12}; int md{16}; int lg{24}; int xl{32}; };
	 struct ElevationTokens { float panelShadowRadius{22.0f}; float controlShadowRadius{12.0f};
									  float panelShadowAlpha{0.35f}; float controlShadowAlpha{0.25f}; };

	 struct TypographyTokens
	 {
		  float smallSize{11.0f}; float bodySize{13.0f}; float titleSize{16.0f}; float headingSize{20.0f};
		  juce::Font small()   const { return { smallSize, juce::Font::plain }; }
		  juce::Font body()    const { return { bodySize, juce::Font::plain }; }
		  juce::Font title()   const { return { titleSize, juce::Font::bold }; }
		  juce::Font heading() const { return { headingSize, juce::Font::bold }; }
	 };

	 struct DesignTokens
	 {
		  ColorTokens colours;
		  RadiusTokens radii;
		  SpacingTokens spacing;
		  ElevationTokens elevation;
		  TypographyTokens type;
	 };

	 inline DesignTokens tokens{};
}
```

### C2. Main LookAndFeel Skeleton
`src/ui/lookandfeel/MainLookAndFeel.h/.cpp` handles global chrome plus helper `drawPanelBackground` (rounded gradient + glow). Excerpt:

```cpp
class MainLookAndFeel final : public juce::LookAndFeel_V4
{
public:
	 MainLookAndFeel();
	 void drawPanelBackground (juce::Graphics&, juce::Rectangle<float> bounds);
	 void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
	 void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
private:
	 void drawOuterGlow (juce::Graphics&, juce::Rectangle<float>, float radius, float alpha);
};
```

Key behaviors:
- Button background uses gradient fill + cyan outline on hover, neon glow when toggled.
- Rotary slider draws halo, knob body gradient, arc meter, pointer needle.
- `drawPanelBackground` fills rounded rectangle with purple gradient, border, and glow to mimic the screenshot tiles.
- Instantiate once in `MainApplication::initialise()` and set via `juce::LookAndFeel::setDefaultLookAndFeel()`.

### C3. Flagship Device Panel Component
Hero component for master device / AI analyzer matching screenshot layout.

```cpp
class FlagshipDevicePanel final : public juce::Component, private juce::Timer
{
public:
	 FlagshipDevicePanel();
	 void setTitle (const juce::String&);
	 void setBackgroundImage (juce::Image);
	 juce::Slider& getMacroSlider (int index);
	 void paint (juce::Graphics&) override;
	 void resized() override;
private:
	 void timerCallback() override;
	 juce::String title{ "AI Mastering" };
	 juce::Image backgroundImage;
	 juce::OwnedArray<juce::Slider> macroSliders;
	 std::array<juce::Label, 4> macroLabels{};
	 float animationPhase = 0.0f;
};
```

Implementation details:
- `paint()` asks `MainLookAndFeel` to draw the panel, then renders background art or fallback gradient plus animated light sweeps.
- Four macro knobs (rotary sliders) sit below the hero image with labels; `timerCallback` drives subtle line animation.
- Use `getMacroSlider(i)` to attach `AudioProcessorValueTreeState` attachments or custom parameter bindings.

### C4. Integration Steps
1. Add the new LookAndFeel + tokens headers to CMake target in `src/ui/CMakeLists.txt`.
2. Instantiate `FlagshipDevicePanel` inside `MainComponent` (or a new Dashboard view) to showcase the style.
3. Gradually convert existing controls/layout containers to reuse `drawPanelBackground`, macro knobs, and panel shells.

---

## Part D – Roadmap & Milestones

| Phase | Scope | Owners/Modules | Success Metrics |
| --- | --- | --- | --- |
| 1. Visual Base (1–2 wks) | Design tokens, LookAndFeel, reskinned MainWindow/MainComponent | `ui/lookandfeel`, `ui/MainComponent` | Consistent neon aesthetic, zero legacy controls |
| 2. Core Views (2–4 wks) | Playlist, piano roll, channel rack layout | `ui/views`, `project` | FL-style workflow live, single-click focus |
| 3. Devices & Visualizers (3–5 wks) | Synth/effect shells, analyzers, meters | `audio/synthesis`, `audio/effects`, `ui/components` | New flagship devices usable end-to-end |
| 4. Engine & Modulation (4–8+ wks) | Sample-accurate automation, modulation matrix, multithread scheduler | `audio/engine`, `core/state` | <5 ms latency, per-sample mod routing |
| 5. AI & Performance (parallel) | AI pattern tools, clip launcher, performance recording | `ai/*`, `ui/views/ClipLauncher` | AI buttons create usable variations, session recording works |
| 6. Hardening & Polish | Profiling, tests, accessibility, localization | `tests/*`, `ui`, `core` | Stable release candidate |

**Next Suggested Steps**
1. Implement `DesignTokens` + `MainLookAndFeel`, verify they build via existing JUCE targets.
2. Drop in `FlagshipDevicePanel` to validate the new visual direction.
3. Start Phase 2 by scaffolding `PlaylistView`, `ChannelRackView`, and `PianoRollView`, wiring them to `project` models.

This document stays the single source for architectural direction, UI language, and initial code scaffolding.
