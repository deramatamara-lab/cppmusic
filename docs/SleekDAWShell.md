# Sleek DAW Shell Visual System

## Purpose

Create a cohesive, premium-grade DAW shell that mirrors modern boutique plugins: dark graphite canvas, neon teal highlights, disciplined spacing, deterministic motion, and accessible contrast. This blueprint defines the tokens, layouts, component behaviors, iconography, and animation specs required to ship 4K mockups (desktop @125%), component sheets, SVG/raster icon assets, and micro-interaction references.

## Deliverable Checklist

1. **Mockups (4K @125%)**: Home/Arrange, Mixer, Piano Roll, Channel Rack & Pattern, Browser & Inspector open, Plugin Host.
2. **Component Library Page**: tokens, buttons, sliders, meters, dials, tabs, accordions, tables, lists.
3. **Icon Pack**: SVG + 1x/2x PNG for critical controls.
4. **Optional Motion Clip**: ≤10 s MP4/GIF of knob turn, meter response, tab switch.

Use a vector-first tool (Figma, Penpot, Lunacy). Create a 12-column layout template (1920×1080 working frame scaled to 2400 px for 125%). Export @1x SVG for icons, @2x PNG for raster previews. Keep all assets linked to the token sheet below.

## Design Tokens (authoritative JSON)

```json
{
  "color": {
    "bg/0": "#0E1116",
    "bg/1": "#131824",
    "bg/2": "#1A2130",
    "panel/border": "#2A3140",
    "text/primary": "#EAF2FF",
    "text/secondary": "#A9B4C7",
    "accent/primary": "#00D4FF",
    "accent/secondary": "#36D1DC",
    "accent/warn": "#FFB020",
    "accent/danger": "#FF4D4D",
    "graph/grid": "#2B3446",
    "graph/gridSubtle": "#202634",
    "meter/ok": "#22D39B",
    "meter/hot": "#FFC857",
    "meter/clip": "#FF4D4D",
    "shadow/soft": "rgba(0,0,0,0.35)"
  },
  "font": {
    "family/base": "Inter, SF Pro, Segoe UI, Roboto",
    "family/mono": "JetBrains Mono, ui-monospace",
    "size/12": 12,
    "size/14": 14,
    "size/16": 16,
    "size/18": 18,
    "size/24": 24,
    "size/32": 32
  },
  "space": { "2": 2, "4": 4, "6": 6, "8": 8, "12": 12, "16": 16, "24": 24, "32": 32 },
  "radius": { "s": 8, "m": 12, "l": 16, "xl": 22 },
  "elev": {
    "0": "none",
    "1": "0 1px 0 rgba(255,255,255,0.02), 0 8px 24px rgba(0,0,0,0.35)",
    "2": "0 1px 0 rgba(255,255,255,0.03), 0 16px 38px rgba(0,0,0,0.45)"
  },
  "anim": {
    "ease/standard": "cubic-bezier(0.22, 1, 0.36, 1)",
    "ease/inOut": "cubic-bezier(0.4, 0, 0.2, 1)",
    "ms/fast": 120,
    "ms/med": 220,
    "ms/slow": 360,
    "spring/knob": { "stiffness": 360, "damping": 26, "mass": 1.0 }
  }
}
```

## Layout Grid & Spacing

- **Frame**: 2400×1350 artboards (4K @125%).
- **Grid**: 12 columns, 88 px columns, 16 px gutters, 32 px outer margin.
- **Padding**: Panel padding 16–24 px; modal padding 24–32 px.
- **Touch target**: ≥28×28 px (map to 32 px bounding boxes in mockups).
- **Corner radii**: radius/m for panels, radius/l for modal sheets, radius/xl for hero rings.
- **Elevation**: Panels use elev/1, modals elev/2; overlay scrims = rgba(8,10,16,0.55).

## Visual Language

- Base surfaces use bg/0–bg/2 gradient sweeps (linear 4°) for subtle depth.
- Neon accents appear only on active states, focus rings, and critical meters.
- Hairline dividers (1 px) in panel/border; micro-chrome strokes at 1.5 px.
- Typography: size/14 body, size/16 labels, size/18 section headers, size/24–32 hero numbers.
- All text aligns to baseline grid multiples of 4 px.

## Component Guidelines

### 1. Transport Bar

- Full-width translucent strip on bg/1 with 12 px top/bottom radius (floating look via elev/1).
- Left cluster (spacing 12 px): Play, Stop, Rec (filled icons), BPM numeric + Tap button, Time Signature, CPU pill, Disk indicator.
- Center: project name (size/18), locator readout (HH:MM:SS / bars), Loop toggle ring button.
- Right: Quantize menu, Snap mode, Global search field, Settings gear.
- Dividers: 1 px panel/border separating clusters; focus rings in accent/primary @1.5 px outer glow.

### 2. Primary Ring Control

- 270° sweep, 64 segment ticks (8–12 px ring thickness). Active arc accent/primary with 10% outer glow.
- Center numeric (size/32) + sublabel (size/14 secondary text).
- Embedded pill toggles (SNAP/WIDE/JITTER) follow radius/s, micro-jitter button height 12 px, accent border on active.
- Outer halo uses shadow/soft and accent/secondary radial gradient.

### 3. Macro Knobs & Sliders

- Rotary knobs: 12 px track width, 1.5 px pointer, tick labels (size/12) on demand. Idle state uses bg/2; hover lifts luminance by 10% over 120 ms.
- Linear faders: compact 48 px height, 12 px width, endcaps radius/s. Hover glow accent/secondary; active fill transitions ease/standard over 220 ms.
- Include micro-jitter pill buttons near each macro; use consistent iconography.

### 4. Meter Stack

- Peak (left) + RMS (right) vertical bars with 1 px graph/grid lines. Color gradient meter/ok → meter/hot → meter/clip.
- Hold indicator: 2 px accent/secondary line, decays after 600 ms.
- Reset button: 16×16 px circular ghost button at base.

### 5. Panels

- **Browser (left)**: Tabs (Project / Samples / Plugins / Presets). 28 px rows with accent underline on selection; breadcrumbs above list.
- **Arrange (center)**: Track lanes alternate bg/1/bg/2. Clips have 1 px outline, gradient preview for audio, grid overlay (graph/gridSubtle). Ruler at top with beat markers and loop brackets.
- **Inspector (right)**: Segmented cards for Track, Clip, Pattern; each card uses radius/m and 16 px padding. Primary actions left-aligned; destructive toggles require confirm overlay.
- **Mixer (bottom)**: 14–18 channel strips, slim faders, EQ thumbnail mini-chart, send knobs as concentric circles.
- **Piano Roll**: Cyan grid @25% opacity, ghost notes 35% opacity, velocity lane with rounded pillars (radius/s).

### 6. Advanced Widgets

- **XY Pad**: 3×3 grid, accent glow handle with drop shadow; axes labeled in mono font.
- **Waterfall/Spectrogram**: Layered teal-to-gray ridges, 1 px frequency grid, 30–45 fps target.
- **Radial Matrix**: Spiral arrangement of nodes (accent/primary) with thin link lines (accent/secondary, 1 px); nodes glow on hover.

### 7. Navigation & Overlays

- Mode pills (NORMAL / MIDI / CHORD) use tab component with 220 ms slide/fade indicator.
- Keyboard shortcut overlay: modal sheet with searchable list (mono font for keys). Focus ring consistent with tooltips.

## View Blueprints

### Home / Arrange

- Transport pinned top; timeline spanning columns 2–11.
- Track stack: audio, MIDI, automation lanes; clip color coding uses muted hues referencing accent/primary sparingly.
- Bottom mixer collapsed preview showing bus meters + macro ring hero.

### Mixer

- Full-height mixer grid, 18 strips visible with horizontal scroll indicator.
- Strip layout: meter, insert slots, send rings, fader, macro controls, channel name.
- Side panels: left browser collapsed, right master bus inspector.

### Piano Roll

- Above: channel rack breadcrumb + pattern tabs.
- Main grid: vertical pitch labels on bg/2 column; horizontal bars with subtle bevel (not gradient) and 1 px outline.
- Velocity lane and CC lanes share common component.

### Channel Rack / Pattern

- Card grid for devices (radius/m). Each card includes ring control, macro knobs, step sequencer row.
- Pattern matrix uses 16-step grid with accent toggles and micro-jitter buttons aligned to top right.

### Browser / Inspector Open

- Browser width 320 px, inspector 360 px. Middle column adapts to remaining width; ensure 16 px gutters.
- Breadcrumbs + tabs at top, list rows with icons.

### Plugin Host

- Device frame uses radius/l, elev/2. Title bar includes bypass, preset, oversampling toggles.
- Internal layout: hero ring, macro knobs, XY pad, meter stack, waterfall preview.

## Component Library Page

- Section order: Tokens → Typography → Buttons → Toggles/Pills → Knobs/Sliders → Meters → Panels → Tabs/Accordions → Tables/Lists → Overlays.
- Each component card shows default, hover, active, focus, disabled states. Annotate sizes, padding, motion curves, and do/don't callouts.

## Icon Set Specification

- Build 48-core icon set (transport, edit, view, device). All icons vector, 24×24 grid, 1.5 px strokes, rounded caps.
- Export: `/design/icons/svg/[name].svg`, `/design/icons/png/1x/[name].png`, `/design/icons/png/2x/[name]@2x.png`.
- Provide naming schema: `ic-transport-play`, `ic-browser-samples`, `ic-meter-reset`, etc.
- Include tooltip + shortcut reference in documentation.

## Micro-Interactions & Motion

- **Knob Turn**: Spring (anim.spring/knob), 220 ms settle, accent glow pulse (opacity 0.4→0.1 over 360 ms).
- **Hover**: Fast lift (ms/fast) to 110% luminance; reverse on exit.
- **Tabs**: Slide indicator 12 px thickness; easing ease/standard over 220 ms.
- **Meters**: 60 Hz repaint, peak decay 12 dB/s, hold drop after 600 ms, RMS smoothing window 30 ms.
- **Piano Roll Drag**: Snap indicator flash accent/primary @80 ms, note stretch easing ease/inOut.
- **Navigation overlay**: Fade in 120 ms, scale 0.98→1.0 for depth.

## Implementation Notes

- Renderer: Prefer Direct2D GPU path with cached glyph runs; precache EQ curves and spectrum paths; throttle spectrogram to 30–45 fps.
- JUCE 8 Animation: centralize timing constants in `AnimationManager`; ensure sync to display refresh and deterministic playback.
- LookAndFeel: Single `MainLookAndFeel` ingesting JSON tokens; use getters (colorFor("bg/1")) to avoid magic numbers.
- HiDPI: Render icons as SVG per scale; fallback PNG at integral scales only.
- Performance: Batch draw calls, reuse `juce::Path` objects, allocate meter buffers once, stream analysis data in lock-free queues.
- AAX Readiness: Confirm Inter/JetBrains licensing, embed fonts if licensing allows, avoid OS-restricted faces.

## Production Workflow

1. Build token file in design tool; hook components to styles.
2. Lay out master frame template with grid & shared components.
3. Duplicate template for each mockup; adjust panels per blueprint.
4. Export component library page + icon pack via batch exporter.
5. Record micro-interaction clip using prototype mode & screen capture (60 fps, ≤10 s).
6. Package deliverables: `/design/mockups`, `/design/components`, `/design/icons`, `/design/motion` with README referencing this spec.

## QA Checklist

- Contrast verified (≥4.5:1) for all text.
- Mode focus rings rendered for every interactive element.
- Motion tests run at 60 fps on mid-tier GPU; no dropped frames.
- Transport, Arrange, Mixer, Piano Roll, Browser, Inspector mockups share identical padding and token usage.
- Component sheet cross-checks every control variant used in mockups.
