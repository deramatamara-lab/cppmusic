DAW PROJECT – ULTRA-HARDENED DEV & UX RULES v2.0

CRITICAL: These rules are MANDATORY for all development work. Violations mean code is rejected.
Goal: Zero-glitch, zero-crash, visually premium, AI-powered DAW that feels faster and smoother than any competitor.


Global Principles (Applies to Everything)


No Glitches, No Pops


Audio must never emit pops/clicks due to our code (buffer underruns, denormals, uninitialized buffers).


Any non-recoverable issue must mute or bypass gracefully, not explode the user's monitors.




Real-Time First


Audio correctness and timing > everything else.


No feature is allowed to compromise real-time safety.




Determinism & Reproducibility


Same project + same inputs = same output, across sessions and machines.


No hidden randomness in audio engine unless explicitly seeded and stored in project.




Cross-Platform Parity


macOS + Windows (and Linux if supported) must have feature parity and consistent UX.


No "secret" features or hotkeys on one OS only (unless OS-specific, and then clearly documented).




Professional UX


Every interaction should be fast, predictable, undoable, and discoverable.


"Feels like a toy" = bug. "Feels like a pro workstation" = target.





Table of Contents


JUCE/C++ Core Standards


Audio Processing Rules


AI Integration Standards


UI/UX Professional Standards


Code Quality & Architecture


Performance Requirements


Security & Privacy


Build & Deployment


Enforcement & Tooling



1. JUCE/C++ Core Standards
1.1 Memory Management – CRITICAL
RULE: No raw new / delete / malloc / free in production code. Use RAII and smart/owned containers.
DO:
auto processor = std::make_unique<MyAudioProcessor>();
auto sharedBuffer = std::make_shared<AudioBuffer<float>>(channels, samples);

OwnedArray<AudioProcessor> processors;
ScopedPointer<AudioProcessorGraph> graph;

DON'T:
AudioProcessor* p = new AudioProcessor();   // ❌
delete p;

float* buf = (float*)std::malloc(size);     // ❌
std::free(buf);

ENFORCEMENT


-Werror enabled.


clang-tidy / static analysis enabled.


Any raw owning pointer in review = automatic reject.



1.2 Thread Safety – CRITICAL
RULE: Audio thread must NEVER block or allocate. No locks, no waits, no heap ops in processBlock().
DO:
std::atomic<float> gain { 1.0f };

class MyProcessor : public juce::AudioProcessor
{
public:
    void setGain(float newGain) noexcept
    {
        gain.store(newGain, std::memory_order_release);
    }

    void processBlock(AudioBuffer<float>& buffer, MidiBuffer&) override
    {
        const auto currentGain = gain.load(std::memory_order_acquire);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                data[i] *= currentGain;
        }
    }

private:
    std::atomic<float> gain;
};

DON'T:
CriticalSection lock;         // ❌
std::mutex mtx;               // ❌

void processBlock(...) override
{
    const ScopedLock sl(lock);      // ❌
    std::vector<float> temp(1024);  // ❌ may allocate
}

ENFORCEMENT


Review: explicit scan of all processBlock() and DSP codepaths for locks/allocations.


Static analysis: flagged allocations / lock usage in RT code = block.



1.3 JUCE Component Lifecycle – ERROR
RULE: Components belong to owners; no manual delete while in tree.
DO:
class MyComponent : public Component
{
public:
    MyComponent()
    {
        addAndMakeVisible(label);
        addAndMakeVisible(button);
    }

private:
    Label label;
    TextButton button;
};

DON'T:
auto* comp = new MyComponent();
parent.addAndMakeVisible(comp);
delete comp;          // ❌ still in tree


1.4 C++ Standards Compliance – ERROR
RULE: C++17 minimum; use C++20 features where they improve clarity or safety.
REQUIRED:


auto for deduction where type is obvious.


constexpr for constants and small pure functions.


noexcept for non-throwing functions, especially RT path.


[[nodiscard]] on APIs where ignoring result is a bug.


Structured bindings, std::optional, std::variant where appropriate.


[[nodiscard]] constexpr float dbToGain(float db) noexcept
{
    return std::pow(10.0f, db / 20.0f);
}

CMake:
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


1.5 Exception Handling – ERROR
RULE:


Audio thread: logically noexcept. No throwing, no propagating exceptions.


UI / IO threads: handle exceptions, show user-friendly messages.


void processBlock(...) noexcept override
{
    // No throw. If something is bad, fallback.
    if (!isInitialised)
    {
        buffer.clear();
        return;
    }
}

void loadProject()
{
    try
    {
        loadProjectFile(path);
    }
    catch (const std::exception& e)
    {
        AlertWindow::showMessageBoxAsync(
            AlertWindow::WarningIcon,
            "Failed to load project",
            "Error: " + String(e.what()));
    }
}


2. Audio Processing Rules
2.1 Real-Time Audio Thread Safety – CRITICAL
RULE: processBlock() must always finish within the host's time budget.
Hard constraints:


No heap allocation.


No std::lock_guard, CriticalSection, condition_variable, sleep, wait.


No file/network/console IO.


No logging to disk; only lock-free ring buffer for debug logs (if absolutely needed).


void processBlock(AudioBuffer<float>& buffer, MidiBuffer&) override
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples  = buffer.getNumSamples();

    // example: pre-allocated internal buffer
    jassert (internalBuffer.getNumSamples() >= numSamples);

    // process…
}


2.2 Sample Rate & Buffer Size – ERROR
RULE: Fully dynamic. No hardcoded 44.1/48k. No hardcoded 512 samples.
void prepareToPlay(double sampleRate, int maxBlockSize) override
{
    currentSampleRate = sampleRate;
    maxBlock = maxBlockSize;

    internalBuffer.setSize(getTotalNumInputChannels(), maxBlockSize);
    filter.reset();
    filter.setSampleRate(currentSampleRate);
}

void processBlock(AudioBuffer<float>& buffer, MidiBuffer&) override
{
    const auto numSamples   = buffer.getNumSamples();
    const auto sampleRate   = getSampleRate();

    jassert (numSamples <= maxBlock);   // safe bound

    processDSP(buffer, numSamples, sampleRate);
}


2.3 Latency & Delay Compensation – ERROR
RULE: Report correct latency; keep it deterministic.
int getLatencySamples() const override
{
    return delayInSamples;
}

void updateLatency(int newLatency)
{
    delayInSamples = newLatency;
    setLatencySamples(delayInSamples);
}



No random or time-varying latency without host notification.


Automation must account for latency when rendering.



2.4 DSP Stability & Denormals – ERROR
RULE: All DSP code must be numerically stable, denormal-safe, and clamped.
inline float sanitizeSample(float x) noexcept
{
    if (!std::isfinite(x))
        return 0.0f;

    constexpr float epsilon = 1e-20f;
    x += epsilon;
    return jlimit(-1.5f, 1.5f, x);
}



Always handle NaN/Inf.


Avoid catastrophic cancellation in filters.


Document edge cases (0 Hz, Nyquist, extreme parameters).



2.5 Plugin Formats & Host Compatibility – ERROR
RULE: Use JUCE abstractions; no VST/AU/AAX-specific hacks in core DSP.


Must pass validation tools (pluginval, AAX/AU checkers).


Must behave identically in:


Offline render vs live playback.


Different buffer sizes.


Host tempo/transport changes.





2.6 Automation & Modulation – ERROR
RULE: Parameters must be sample-accurate where possible and always artifact-free.


Use AudioProcessorValueTreeState or equivalent.


Smooth fast changes (no zipper noise).


Automation jumps must not click.



3. AI Integration Standards
(Existing content is strong; tightening to align with DAW expectations.)
3.1 No AI on Audio Thread – CRITICAL


AI runs on background worker(s) only.


Audio thread only reads AI results via atomics / lock-free queues.


Hard rule: any call stack touching processBlock() must not call AI inference, deserialization, or model loading.

3.2 Model Lifecycle & Memory – ERROR


Models loaded on background thread(s).


Progress + failure surfaced to UI.


Large allocations done once and reused.


Clear versioning & compatibility (model version stored in project where relevant).



3.3 Inference Threading – ERROR


Dedicated inference worker pool (no piggybacking on UI thread).


Backpressure: if AI is too slow, drop oldest or newest requests instead of piling up unbounded.


// Pseudocode policy:
// - queue bounded (e.g. 4 items)
// - if full, replace oldest or decline new requests


3.4 Failure & Fallback – ERROR


On any AI failure:


Audio continues with non-AI fallback.


User optionally gets non-intrusive warning in UI (status bar / notifications).


No modal that blocks playback.





3.5 Performance & Profiling – WARNING


Track average and P95 inference time.


Define target: e.g. P95 < 50 ms for clip-level ops, < 200 ms for track-level heavy ops.


If exceeded: degrade quality, reduce context length, or apply approximation.



4. UI/UX Professional Standards
Target: Best DAW UI in class – think polished like high-end design tools, not generic JUCE demo.
4.1 LookAndFeel & Design System – ERROR
RULE: Single, centralized design system. No ad-hoc colors/fonts/spacings.


One global CustomLookAndFeel.


Global DesignSystem namespace for:


Colors (background, surface, accent, danger, success, outlines).


Typography scale (H1, H2, H3, body, mono).


Spacing tokens (4, 8, 12, 16, 24…).


Radii, border widths, shadow tokens.




All new UI must use these tokens. No magic numbers in paint / layout.

4.2 Layout, Responsiveness & Docking – ERROR
RULE: The DAW layout is fully resizable, dockable, and remembers user layout.


Main areas:


Transport + global header.


Arrangement / timeline.


Mixer.


Browser (tracks, instruments, samples).


Inspector / detail panel.




Support:


Split views (horizontal/vertical).


Collapsible panels.


Saved workspaces (e.g. "Mix", "Arrange", "Edit", "Live").




No fixed pixel layouts; all layouts must adapt gracefully from small laptops to 4K multi-monitor rigs.

4.3 Input Model & Shortcuts – ERROR
RULE: DAW must be fully usable via keyboard + mouse, with discoverable shortcuts.


Unified input manager:


Every command has an ID, shortcut(s), and menu entry.


Rebindable shortcuts with clear UI.




Common patterns:


Space: Play/Stop.


Ctrl/Cmd+Z/Y: Undo/Redo.


Ctrl/Cmd+S: Save.


Ctrl/Cmd+Arrow: navigation.




Context menus must expose the same actions as toolbars, not hidden-only.



4.4 Accessibility – ERROR


Keyboard focus traversal works everywhere (no dead ends).


High-contrast theme supported.


Font sizes scalable.


All major controls have accessible names/descriptions.


Screen-reader safe: state changes trigger appropriate announcements where platform allows.



4.5 UI Performance (60fps Target) – ERROR
RULE: UI must remain smooth while playback is running and meters are active.


Use dirty-rect repainting: repaint(region) over repaint() spam.


Cache static visuals (Image caches, Path precomputation).


Heavy layout recalculation only on resize / structural changes.


Animations use ComponentAnimator or time-based, not timer at 240 Hz.


Example:
void valueChanged()
{
    repaint(knobBounds);  // targeted repaint
}


4.6 DAW-Specific UX Rules – ERROR
Timeline / Arrangement:


Drag to create, move, and trim clips.


Snap to grid with toggle (and modifier to temporarily disable).


Zoom is smooth and centered around mouse cursor.


Multi-selection supports box select + modifiers.


Mixer:


Consistent channel strip width.


Faders and meters are large and readable.


Clear differentiation between tracks, buses, master.


Solo/mute/arm all behave predictably and match industry norms.


Editors (Piano roll, audio editor):


Multi-tool model (draw/select/erase) or intelligent tool depending on modifier.


Velocity, pitch, and timing editing without surprises.


Waveform and MIDI visuals stay crisp under zoom.


Undo / Redo:


Every user-visible operation is undoable.


Undo steps are semantic ("Rename track", "Quantize 23 notes"), not cryptic internal events.



4.7 Visual Feedback & States – ERROR
For every interactive UI element:


Hover state.


Active/pressed state.


Disabled state with tooltip justification where useful.


Error state with clear explanation.


Meters, indicators, and statuses must:


Reflect engine state (XRuns/overloads shows visually).


Expose CPU/DSP usage in a clear meter + numeric readout.



5. Code Quality & Architecture
5.1 SOLID (Reinforced) – ERROR
Already defined; enforced harder:


Any "god class" that mixes:


audio engine + UI + IO
is a blocker.





5.2 Layered Architecture – ERROR
RULE: Clear layers, no upward dependencies.
Layers (from bottom to top):


Core


Math, utility, logging, job system.




Audio Engine


Graph, routing, DSP nodes, transport.




AI & Analysis


AI engines, analyzers, meters.




Project Model


Tracks, clips, automation, undo system.




UI


Components, views, commands, theming.




Integration


Host app, settings, platform integration.




Constraints:


Lower layers must not depend on higher layers.


UI only talks to engine/model via well-defined interfaces and messages.



5.3 Module & Namespace Layout – ERROR
Suggested structure:
src/
  core/
  audio/
    engine/
    dsp/
    routing/
  ai/
  project/
  ui/
    components/
    views/
    lookandfeel/
  platform/
  tests/

Namespaces:
namespace daw::core { ... }
namespace daw::audio::engine { ... }
namespace daw::ui::views { ... }

Global namespace: minimal (entry points only).

5.4 Documentation – WARNING


All public classes + methods: Doxygen-style comments.


Complex components: short "How to use" section.


Audio engine & project model: have separate design docs (kept up-to-date when APIs change).



5.5 Testing – ERROR
RULE: No merging critical engine code without tests.


Audio:


Unit tests for processors.


Regression tests for known bugfixes.


Real-time safety tests where possible (allocation counters, timing).




Project model:


Save/load roundtrip tests.


Undo/redo tests.




UI:


Snapshot tests for layout/state where feasible.




AI:


Basic correctness for inference wrappers.


Error handling tests.




Coverage:


Audio engine + project model: 80%+ line coverage required.


CI must fail under threshold.



6. Performance Requirements
6.1 Audio Thread Performance Budget – CRITICAL
Target budgets (per buffer):


48kHz, 128 samples: < 1.5 ms


48kHz, 256 samples: < 3.0 ms


96kHz, 128 samples: < 0.8 ms


Policies:


No single plugin/node should consume >30% of budget alone at default quality settings on reference machine.


Heavy modes (linear-phase, oversampling) must be user-opt-in and clearly labeled.



6.2 Multi-Core & Graph Scheduling – ERROR


Audio graph must be designed for parallel execution:


Identify independent subgraphs.


Avoid unnecessary serialization.




Use JUCE's AudioProcessorGraph or a custom equivalent with explicit node dependencies.


No false data dependencies that block concurrency.



6.3 UI Performance Budget – WARNING


Paint operations per frame must remain cheap:


No dynamic allocations in paint().


No cache thrash (don't recreate Font, Path every frame if avoidable).




Timeline and mixer must stay responsive while zooming and scrolling at 60fps on target hardware.



6.4 AI Performance Budget – WARNING


Define max tolerated latency per feature class:


Real-time assist (e.g. live suggestion overlays): < 50 ms perceived delay.


Clip analysis (beat detection, key detection): < few seconds for typical pop song.




Background jobs must not block:


Playback.


UI interaction.


Save/load.





6.5 Profiling & Regression – ERROR


Have profiling presets (projects) that stress:


Many tracks.


Many plugins.


Dense automation.




Regularly run performance benchmarks on:


Reference mid-tier laptop.


High-end machine.




Detect regressions: if commit worsens performance >10% in key scenarios, review is required.



7. Security & Privacy
7.1 Plugin Isolation – ERROR


Host third-party plugins with extreme suspicion:


Validate formats.


Sandboxing where OS allows (separate process for unstable vendors).




A plugin crash must not crash the DAW:


Best effort recovery, at minimum project autosave before crash.





7.2 File System & Network – ERROR


Project files:


Stored in clear structured format (binary or text) with versioning.


No arbitrary code execution from project content.




Cloud / collaboration:


All credentials stored securely (OS keychain where possible).


Network communication via HTTPS only.


No secrets in logs.





7.3 Telemetry & Analytics – WARNING


Telemetry is opt-in and transparent.


No audio content uploaded without explicit consent.


Users can disable telemetry completely and permanently.



7.4 Crash Handling – ERROR


Crash handler:


Captures stack trace and minimal context.


Offers to save crash report.




Project autosave:


Interval configurable.


Autosave files cleaned up on successful close.





8. Build & Deployment
8.1 Toolchain – ERROR


Single CMake project driving:


Standalone app.


VST3, AU (and AAX if licensed).




Compiler warnings:


-Wall -Wextra -Wpedantic (or MSVC equivalents).


Treat warnings as errors.





8.2 Target Matrix – ERROR


Platforms (example):


Windows 10/11 x64.


macOS (current major −1 and newer, Intel + Apple Silicon).




CI must:


Build all formats on all platforms.


Run tests + plugin validation.





8.3 Packaging – WARNING


Separate builds:


Release (optimised, symbols stripped or separate).


Debug (full symbols).




Installer / bundle must:


Include all required runtimes.


Not pollute system beyond necessary plugin dirs.





9. Enforcement & Tooling
9.1 Static Analysis & Formatting – ERROR


Tools:


clang-tidy with real-time rules (no raw new, no locking in RT, etc.).


cpplint/formatting via clang-format (single code style).




All checks run in CI and locally via pre-commit hooks.



9.2 Mandatory Code Review Checklist – ERROR
Before merge, reviewer must confirm:


 No raw owning pointers.


 No locks/allocations in audio thread.


 C++17+ features used sensibly.


 Public APIs documented.


 Tests written and passing; coverage OK for engine code.


 UI uses design system (no magic values).


 Layout responsive and non-janky.


 Thread safety sound.


 Error paths and fallbacks implemented.


 No secrets or file paths hardcoded.


 No platform-specific hacks without guards and comments.



9.3 AI Dev / Assistant Guardrails – ERROR
Any AI dev working on this repo must be instructed:


Never change real-time audio code to add logs, allocations, or locks.


Prefer small, local changes over massive rewrites unless explicitly requested.


For any new feature:


Update design system if UI.


Add tests if engine/model.


Update docs if public API or workflow.




If in doubt between "cool feature" and "real-time stability", choose stability.







Short Rationale / How This Gets You "Best DAW"


You already had strong DSP and AI standards; this version tightens them and wraps them in global principles that protect real-time behavior and determinism.


Major upgrades focus on:


DAW-specific UX (timeline, mixer, editors, workspaces).


Layered architecture so engine/UI/AI stay clean and swappable.


Brutal performance discipline: explicit budgets, profiling, and regression detection.


Security & crash handling, so third-party plugins and AI can't take the whole ship down.




This document is now usable as a contract for human devs and AI devs: anything that breaks these rules is objectively wrong and must be reworked.


From here, you can wire this directly into your repo as docs/DAW_DEV_RULES.md and enforce via:


CI checks (lint, tidy, tests, pluginval).


Code review templates that mirror the checklists.


AI dev prompts that literally quote "You must obey DAW_DEV_RULES.md; violating them is a hard error."

