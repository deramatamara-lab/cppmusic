# UX/UI Upgrade Audit – CPPMusic DAW

## 1. Executive Summary

CPPMusic already ships a disciplined design system, multiple custom LookAndFeel stacks, and an adaptive animation service, but these systems operate in parallel rather than as a single orchestrated UX platform. To reach “best DAW” status we need to (1) converge on a single UltraDesignSystem token source, (2) wire micro-interactions end-to-end (input → animation controller → component feedback), (3) harden component coverage so every workflow surface—arrange, mixer, piano roll, devices—shares the same interaction primitives, and (4) back the UX layer with observability, accessibility, and regression tests.

## 2. Current Foundations Snapshot

| Area | Observations |
| --- | --- |
| Design tokens | Multiple token definitions exist (DesignSystem, DesignTokens, UltraDesignSystem). UltraDesignSystem is closest to current spec but lives alongside older systems, resulting in duplicated colours/metrics and risk of drift.@src/ui/lookandfeel/UltraDesignSystem.hpp#13-214 @docs/ARCHITECTURE.md#75-83 |
| Look & Feel stack | `CustomLookAndFeel`, `MainLookAndFeel`, `EnhancedMainLookAndFeel`, and `Ultra::MainLookAndFeel` coexist; only parts of the UI consume the newer ultra skin. Each L&F duplicates gradients, glows, and typography logic, increasing maintenance overhead.@src/ui/lookandfeel/CustomLookAndFeel.cpp#8-417 @src/ui/lookandfeel/MainLookAndFeel.cpp#8-243 @src/ui/lookandfeel/EnhancedMainLookAndFeel.h#11-55 |
| Animation services | `AdaptiveAnimationService` and `AdaptiveAnimationManager` provide a full animation orchestration layer (GPU-aware, physics-based), but MainView simply attaches the service without delegating interactions to it; most components still run ad-hoc timers/springs.@src/ui/AdaptiveAnimationManager.h#15-152 @src/ui/animation/AdaptiveAnimationService.h#14-81 @src/ui/views/MainView.cpp#1776-1790 |
| Interaction components | Hero controls like `NeuroSlider` expose multiple styles and physics states but only the linear variant is implemented; arc, spectrum, and waveform styles delegate to the linear painter. Hover/press states are powered by bespoke timers per component instead of the shared animation manager.@src/ui/components/NeuroSlider.cpp#60-220 |
| Shell layout | MainView composes transport, browser, arrange, inspector, mixer via JUCE StretchableLayoutManagers, but layout data is scattered inside MainView with no responsive presets (Mix/Arrange/Live) yet. Panels do not advertise breakpoints or persisted layouts.@src/ui/views/MainView.h#133-172 |
| Telemetry & UX QA | BenchmarkSystem + dashboards focus on DSP metrics; UI lacks structured telemetry hooks, so animation/frame regressions are invisible until manual testing.@docs/ARCHITECTURE.md#314-347 |

## 3. Gap Analysis & Upgrade TODOs

### 3.1 Token + LookAndFeel convergence

1. **Single source of truth for tokens** – Adopt `UltraDesignSystem::tokens()` as the canonical provider. Create adapters so legacy `DesignSystem` / `DesignTokens` read directly from ultra tokens instead of declaring independent palettes.@src/ui/lookandfeel/UltraDesignSystem.hpp#13-214
2. **LookAndFeel consolidation** – Refactor `CustomLookAndFeel` and `MainLookAndFeel` to subclass the ultra variant or expose shared helpers. Remove duplicated rotary/slider/button painters once ultra skin covers all contexts.@src/ui/lookandfeel/CustomLookAndFeel.cpp#50-417 @src/ui/lookandfeel/MainLookAndFeel.cpp#40-193
3. **Dynamic theming hooks** – Extend `ThemeManager` to emit token overrides (dark/light/high contrast/custom) and feed them into UltraDesignSystem’s JSON loader, enabling runtime switching without restyling every component.@src/ui/lookandfeel/ThemeManager.cpp#8-129

### 3.2 Micro-interactions & motion system

1. **Central animation routing** – Replace per-component timers (`NeuroSlider`, `PillToggle`, etc.) with animation IDs from `AdaptiveAnimationService`, so hover/press/drag physics are scheduled centrally, respect quality tiers, and can be globally throttled for performance or accessibility.@src/ui/components/NeuroSlider.cpp#19-215 @src/ui/lookandfeel/UltraDesignSystem.hpp#805-830 @src/ui/animation/AdaptiveAnimationService.h#14-81
2. **Event instrumentation** – Extend the animation manager to emit structured logs (component id, animation type, duration, droop) for telemetry and QA. Hook into the existing `PerformanceMonitor` infrastructure documented in ARCHITECTURE.md to watch for dropped frames in complex views.@docs/ARCHITECTURE.md#291-336
3. **Micro-interaction playbook** – Encode Sleek DAW Shell motion specs (hover lift, knob springs, tab slide) as reusable presets (enum + parameters) to guarantee identical timing across UI states and to meet the reference blueprint.@docs/SleekDAWShell.md#66-185

### 3.3 Component coverage and states

1. **Complete NeuroSlider styles** – Implement the circular, arc, waveform, and spectrum painters with real visuals (tick marks, FFT overlays, etc.) and attach context-specific micro-interactions (e.g., audio reactive glow uses animation manager rather than repaint spam).@src/ui/components/NeuroSlider.cpp#84-160
2. **Shared panel system** – Extract the layout/padding/radius logic currently embedded in each LookAndFeel painter into a `PanelFrame` helper so every panel (browser, inspector, mixer strips) inherits consistent chrome, scrollbars, and backdrop blur.
3. **State-driven UI controller** – Introduce a lightweight UI state store (parallel to `CentralStateStore` on the core side) that tracks selection, focus, hover, and mode toggles across panels. Components can subscribe instead of polling internal booleans, enabling richer contextual UI (e.g., highlight arrangement clip + inspector card simultaneously).@oldbutgold/Core/CentralStateStore.h#? (reference existing pattern) TODO: map to new UI slice.

### 3.4 Layout, responsiveness, and workspaces

1. **Workspace presets** – Implement the "Workspace System" called out in ARCHITECTURE.md by persisting stretchable layout ratios plus panel visibility, and add quick-switch commands (Mix/Arrange/Live/Performance).@docs/ARCHITECTURE.md#226-232 @src/ui/views/MainView.h#133-172
2. **Adaptive density** – Add scaling rules (compact/regular/comfortable) driven by DPI + window size, adjusting typography, spacing, and component density automatically using the token spacing table. Wire into UltraDesignSystem tokens so components can query current density class.@src/ui/lookandfeel/UltraDesignSystem.hpp#47-74
3. **Docking & drag affordances** – Enhance the panel containers with visible drag handles, snap points, and drop previews (use JUCE drag-and-drop overlay) to make rearranging panels predictable.

### 3.5 Accessibility & input ecosystem

1. **Focus & keyboard maps** – Audit every interactive control to ensure it paints a visible focus ring (tokens already expose `accentPrimary`). Provide keyboard shortcuts for toggles and sliders (e.g., arrow increments, modifier coarse/fine adjustments).@src/ui/lookandfeel/UltraDesignSystem.hpp#813-829
2. **Reduced motion mode** – Hook OS accessibility flags (e.g., prefers-reduced-motion) into `AdaptiveAnimationManager::QualityLevel`, lowering animation duration/complexity automatically.
3. **Screen reader metadata** – Wrap complex controls (ring sliders, XY pads) with descriptive `AccessibilityHandler`s so assistive tech can announce state/value.

### 3.6 Observability, testing, and CI

1. **UI snapshot tests** – Add JUCE component snapshot tests (using the existing tests/ tree) for flagship panels to guard against regressions when adjusting tokens.
2. **Interaction regression harness** – Record scripted user flows (transport control, clip edit, mixer automation) and capture animation timelines + GPU stats to compare in CI.
3. **UX telemetry dashboard** – Extend `scripts/generate_performance_dashboard.py` to include UI latency charts (paint time, animation throughput), reusing the performance pipeline already in place.@scripts/generate_performance_dashboard.py#1-???

### 3.7 AI-assisted workflow intelligence

1. **Inference-driven hints** – Surface AI chord/melody insights directly in ArrangeView, Piano Roll, and Pattern Sequencer by subscribing UI state to `InferenceEngine` output rather than forcing users to open separate panels.@docs/ARCHITECTURE.md#272-290 @src/ui/views/MainView.cpp#1776-1780
2. **Contextual command palette** – Enrich `CommandPalette` with AI-suggested actions (e.g., “humanize drums,” “invert chords”) based on current selection and transport state, ensuring deterministic suggestions surfaced via the shared state controller.@src/ui/views/MainView.h#152-164
3. **AI session recap & coach** – Add a lightweight assistant drawer that summarizes recent edits, flags potential issues (clipping tracks, unassigned outputs), and links to relevant controls. Use the existing AI service plumbing but gate suggestions behind explicit user opt-in for privacy.

## 4. Prioritized Upgrade Backlog

| Priority | Theme | Action | Owners/Systems | Dependencies |
| --- | --- | --- | --- | --- |
| P0 | Token convergence | Make UltraDesignSystem the single provider and delete duplicate palettes after adapters land. | UI LookAndFeel, ThemeManager | Requires JSON loader accepting ThemeManager overrides. |
| P0 | Animation plumbing | Route all component micro-interactions through AdaptiveAnimationService; expose presets for `hover`, `press`, `drag`, `audioReactive`. | UI components, animation service | Need component id registry + logging hooks. |
| P1 | Workspace presets | Persist + switch layout configurations (Mix/Arrange/Live/Performance) with keyboard shortcuts and status-bar indicators. | MainView, State store | Depends on UI state controller work. |
| P1 | Component completion | Finish NeuroSlider styles + add shared PanelFrame so arrange/mixer/inspector share chrome. | Component library | Requires finalized token palette + animation presets. |
| P2 | Accessibility | Ship focus rings, reduced-motion mode, screen-reader metadata across all flagship controls. | UI components, Platform layer | Requires animation plumbing for reduced-motion flag. |
| P2 | Observability & tests | Add UI snapshot tests, animation regression harness, and integrate UI metrics into performance dashboards. | Tests/, scripts/, CI | Needs deterministic rendering order once tokens unified. |
| P2 | AI-assisted workflow | Connect inference outputs to inline hints, contextual command palette entries, and assistant drawer so AI features feel native in every panel. | InferenceEngine, CommandPalette, Arrange/PianoRoll views | Depends on UI state controller + telemetry hooks for measuring usefulness. |
| P3 | Docking polish | Add drag handles, snap previews, and visual breadcrumbs to panel docking to match Sleek Shell blueprint. | MainView containers | Builds on shared PanelFrame + state controller. |

## 5. Next Steps

1. **RFC** consolidating token + LookAndFeel stack (schedule design review, freeze palette changes during migration).
2. **Spike**: wire PillToggle + NeuroSlider to AdaptiveAnimationService as reference implementations, validating the preset API.
3. **Instrumentation plan** tying animation metrics into existing BenchmarkSystem dashboards to catch UI regressions alongside DSP benchmarks.
4. **AI-guided UX storyboard** capturing how inference insights flow into ArrangeView, Piano Roll, and CommandPalette so engineering and design can evaluate privacy + UX trade-offs before implementation.

Delivering these upgrades will align CPPMusic’s user experience with the Sleek DAW Shell blueprint, ensure micro-interactions remain deterministic and observable, and create the documentation + testing scaffolding needed to sustain rapid feature work.
