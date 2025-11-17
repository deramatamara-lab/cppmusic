# DAW PROJECT - HARDENED DEVELOPMENT RULES

> **CRITICAL**: These rules are MANDATORY for all development work. Violations will result in code rejection. This document ensures professional-grade, production-ready code for our ultra-enhanced music platform with AI integration.

---

## Table of Contents

1. [JUCE/C++ Core Standards](#1-jucec-core-standards)
2. [Audio Processing Rules](#2-audio-processing-rules)
3. [AI Integration Standards](#3-ai-integration-standards)
4. [UI/UX Professional Standards](#4-uiux-professional-standards)
5. [Code Quality & Architecture](#5-code-quality--architecture)
6. [Performance Requirements](#6-performance-requirements)
7. [Security & Privacy](#7-security--privacy)
8. [Build & Deployment](#8-build--deployment)
9. [Enforcement & Tools](#9-enforcement--tools)

---

## 1. JUCE/C++ Core Standards

### 1.1 Memory Management - CRITICAL

**RULE**: Never use raw `new`/`delete` or `malloc`/`free`. Always use RAII and smart pointers.

**DO:**
```cpp
// Use smart pointers
auto processor = std::make_unique<AudioProcessor>();
auto buffer = std::make_shared<AudioBuffer<float>>(channels, samples);

// Use JUCE's ScopedPointer or OwnedArray
OwnedArray<AudioProcessor> processors;
ScopedPointer<AudioProcessorGraph> graph;
```

**DON'T:**
```cpp
// NEVER do this
AudioProcessor* processor = new AudioProcessor();
delete processor;

// NEVER do this
float* buffer = (float*)malloc(size * sizeof(float));
free(buffer);
```

**ENFORCEMENT**: All code must compile with `-Werror` and static analysis tools must pass.

---

### 1.2 Thread Safety - CRITICAL

**RULE**: Audio thread operations must NEVER block or allocate memory. Use lock-free data structures or message passing.

**DO:**
```cpp
// Use lock-free FIFO for audio thread communication
AbstractFifo fifo;
std::atomic<float> gain{1.0f};

// Message passing from UI to audio thread
class AudioProcessor : public juce::AudioProcessor
{
    LockFreeFifo<ParameterChange> parameterQueue;
    
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi) override
    {
        // Process parameter changes lock-free
        ParameterChange change;
        while (parameterQueue.read(change))
        {
            applyParameterChange(change);
        }
        // ... audio processing
    }
};
```

**DON'T:**
```cpp
// NEVER use locks in audio thread
CriticalSection lock;  // ❌ FORBIDDEN in processBlock()
std::mutex mtx;        // ❌ FORBIDDEN in audio thread

// NEVER allocate in audio thread
void processBlock(...) override
{
    auto* newBuffer = new float[1024];  // ❌ FORBIDDEN
    std::vector<float> temp;            // ❌ FORBIDDEN (may allocate)
}
```

**ENFORCEMENT**: Code review must verify no locks or allocations in `processBlock()`.

---

### 1.3 JUCE Component Lifecycle - ERROR

**RULE**: Components must properly manage their lifecycle. Never delete Components while they're still referenced.

**DO:**
```cpp
class MyComponent : public Component
{
public:
    MyComponent()
    {
        addAndMakeVisible(label);
        addAndMakeVisible(button);
    }
    
    ~MyComponent()
    {
        // Components are automatically removed from parent
        // No manual cleanup needed for child components
    }
    
private:
    Label label;
    TextButton button;
};
```

**DON'T:**
```cpp
// NEVER delete a Component that's still in the component tree
void someFunction()
{
    auto* comp = new MyComponent();
    parent.addAndMakeVisible(comp);
    delete comp;  // ❌ FORBIDDEN - Component still in tree
}
```

---

### 1.4 C++ Standards Compliance - ERROR

**RULE**: Code must compile with C++17 minimum, prefer C++20 features where beneficial.

**REQUIRED:**
- Use `auto` for type deduction where appropriate
- Use `constexpr` for compile-time constants
- Use `noexcept` for functions that don't throw
- Use `[[nodiscard]]` for functions where return value must be checked
- Use structured bindings where appropriate

**DO:**
```cpp
[[nodiscard]] constexpr float calculateGain(float input) noexcept
{
    return input * 0.5f;
}

auto [min, max] = getRange();
```

**ENFORCEMENT**: CMake must set `CMAKE_CXX_STANDARD` to 17 or higher.

---

### 1.5 Exception Handling - ERROR

**RULE**: Audio processing code must NEVER throw exceptions. UI code must handle all exceptions gracefully.

**DO:**
```cpp
// Audio thread - no exceptions
void processBlock(...) noexcept override
{
    try {
        // Even in try-catch, don't throw
        processAudio();
    } catch (...) {
        // Silently handle or use fallback
        applySafeFallback();
    }
}

// UI thread - handle exceptions
void loadProject()
{
    try {
        loadProjectFile(path);
    } catch (const std::exception& e) {
        AlertWindow::showMessageBox(
            AlertWindow::WarningIcon,
            "Error",
            "Failed to load project: " + String(e.what())
        );
    }
}
```

---

## 2. Audio Processing Rules

### 2.1 Real-Time Audio Thread Safety - CRITICAL

**RULE**: The `processBlock()` method runs in a real-time audio thread. It must complete within the buffer period (typically <10ms for 512 samples at 48kHz).

**REQUIREMENTS:**
- Zero memory allocations
- Zero locks or blocking operations
- Deterministic execution time
- No file I/O
- No network operations
- No logging to disk

**DO:**
```cpp
void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi) override
{
    // Pre-allocated buffers only
    static thread_local AudioBuffer<float> tempBuffer;
    tempBuffer.setSize(buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);
    
    // Lock-free parameter access
    auto currentGain = gain.load(std::memory_order_acquire);
    
    // Process audio
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            channelData[sample] *= currentGain;
        }
    }
}
```

**DON'T:**
```cpp
void processBlock(...) override
{
    // ❌ FORBIDDEN - Memory allocation
    std::vector<float> temp(buffer.getNumSamples());
    
    // ❌ FORBIDDEN - Lock
    const ScopedLock sl(lock);
    
    // ❌ FORBIDDEN - File I/O
    FileLogger::writeToLog("Processing...");
    
    // ❌ FORBIDDEN - Network
    httpClient.get("http://api.example.com");
}
```

**ENFORCEMENT**: Static analysis tools must detect allocations in audio thread.

---

### 2.2 Sample Rate and Buffer Size Handling - ERROR

**RULE**: All audio processing must handle variable sample rates and buffer sizes correctly.

**DO:**
```cpp
void prepareToPlay(double sampleRate, int maximumBlockSize) override
{
    // Store sample rate
    currentSampleRate = sampleRate;
    
    // Pre-allocate buffers for maximum size
    internalBuffer.setSize(getTotalNumInputChannels(), maximumBlockSize, false, false, true);
    
    // Reset DSP state
    filter.reset();
    filter.setSampleRate(sampleRate);
}

void processBlock(...) override
{
    // Handle variable buffer sizes
    const int numSamples = buffer.getNumSamples();
    const double sampleRate = getSampleRate();
    
    // Process with current buffer size
    processAudio(buffer, numSamples, sampleRate);
}
```

**DON'T:**
```cpp
// ❌ FORBIDDEN - Hardcoded sample rate
void processBlock(...) override
{
    filter.process(buffer, 44100.0);  // Wrong! Use getSampleRate()
}

// ❌ FORBIDDEN - Assumed buffer size
void processBlock(...) override
{
    float temp[512];  // Wrong! Buffer size may vary
}
```

---

### 2.3 Latency Requirements - ERROR

**RULE**: Total plugin latency must be reported accurately. Latency must be constant and deterministic.

**DO:**
```cpp
int getLatencySamples() const override
{
    return delayLine.getDelayInSamples();
}

void setLatencySamples(int latency) override
{
    delayLine.setDelayInSamples(latency);
    setLatencySamples(latency);  // Notify host
}
```

**DON'T:**
```cpp
// ❌ FORBIDDEN - Variable or unreported latency
int getLatencySamples() const override
{
    return randomLatency;  // Wrong! Must be constant
}
```

---

### 2.4 DSP Algorithm Implementation - ERROR

**RULE**: All DSP algorithms must be numerically stable, handle edge cases, and prevent denormals.

**DO:**
```cpp
class BiquadFilter
{
public:
    void processBlock(AudioBuffer<float>& buffer)
    {
        // Prevent denormals
        const float denormalPrevention = 1e-20f;
        
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* samples = buffer.getWritePointer(channel);
            
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                // Process with stability checks
                float input = samples[i] + denormalPrevention;
                float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
                
                // Update state
                x2 = x1;
                x1 = input;
                y2 = y1;
                y1 = output;
                
                // Clamp to prevent overflow
                samples[i] = jlimit(-1.0f, 1.0f, output);
            }
        }
    }
    
private:
    float b0, b1, b2, a1, a2;
    float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
};
```

**REQUIREMENTS:**
- Handle NaN and Inf values
- Prevent denormals (use small offset)
- Clamp outputs to valid range
- Use appropriate numerical precision
- Document algorithm limitations

---

### 2.5 Plugin Format Compatibility - ERROR

**RULE**: Code must work correctly as VST3, AU, and AAX plugins. Use JUCE's abstraction layer.

**DO:**
```cpp
// Use JUCE's AudioProcessor base class
class MyPlugin : public juce::AudioProcessor
{
    // JUCE handles format-specific code
};

// Use JUCE's parameter system
AudioProcessorValueTreeState parameters;
```

**DON'T:**
```cpp
// ❌ FORBIDDEN - Format-specific code
#ifdef JucePlugin_Build_VST3
    // VST3-specific code
#elif JucePlugin_Build_AU
    // AU-specific code
#endif
```

**ENFORCEMENT**: Must build and test on all target formats.

---

## 3. AI Integration Standards

### 3.1 Async AI Processing - CRITICAL

**RULE**: AI inference must NEVER run on the audio thread. All AI operations must be asynchronous.

**DO:**
```cpp
class AIProcessor
{
public:
    AIProcessor()
    {
        // Run AI on separate thread
        aiThread = std::thread(&AIProcessor::aiProcessingThread, this);
    }
    
    void processAudio(AudioBuffer<float>& buffer)
    {
        // Audio thread - only read results
        if (resultsReady.load(std::memory_order_acquire))
        {
            applyAIResults(buffer);
            resultsReady.store(false, std::memory_order_release);
        }
    }
    
    void requestAIProcessing(const AudioBuffer<float>& input)
    {
        // UI thread - queue request
        {
            const ScopedLock sl(requestLock);
            pendingRequest = input;
            requestPending = true;
        }
        conditionVariable.notify_one();
    }
    
private:
    void aiProcessingThread()
    {
        while (!shouldStop)
        {
            std::unique_lock<std::mutex> lock(requestLock);
            conditionVariable.wait(lock, [this] { return requestPending || shouldStop; });
            
            if (requestPending)
            {
                // Perform AI inference (may take time)
                auto results = aiModel.infer(pendingRequest);
                
                // Store results atomically
                {
                    const ScopedLock sl(resultsLock);
                    aiResults = results;
                    resultsReady.store(true, std::memory_order_release);
                }
                
                requestPending = false;
            }
        }
    }
    
    std::thread aiThread;
    std::mutex requestLock, resultsLock;
    std::condition_variable conditionVariable;
    std::atomic<bool> requestPending{false};
    std::atomic<bool> resultsReady{false};
    std::atomic<bool> shouldStop{false};
    AudioBuffer<float> pendingRequest;
    AIModelResults aiResults;
};
```

**DON'T:**
```cpp
// ❌ FORBIDDEN - AI in audio thread
void processBlock(...) override
{
    auto results = aiModel.infer(buffer);  // May take 100ms+!
    applyResults(results);
}
```

**ENFORCEMENT**: Code review must verify AI operations are never in audio thread.

---

### 3.2 Model Loading and Memory Management - ERROR

**RULE**: AI models must be loaded asynchronously. Model memory must be managed efficiently.

**DO:**
```cpp
class AIModelManager
{
public:
    void loadModelAsync(const File& modelFile, std::function<void(bool)> callback)
    {
        // Load on background thread
        std::thread([this, modelFile, callback]()
        {
            try
            {
                // Load model (may take seconds)
                auto newModel = std::make_unique<AIModel>();
                if (newModel->loadFromFile(modelFile))
                {
                    // Swap models atomically
                    {
                        const ScopedLock sl(modelLock);
                        currentModel = std::move(newModel);
                    }
                    MessageManager::callAsync([callback]() { callback(true); });
                }
                else
                {
                    MessageManager::callAsync([callback]() { callback(false); });
                }
            }
            catch (const std::exception& e)
            {
                MessageManager::callAsync([callback]() { callback(false); });
            }
        }).detach();
    }
    
    std::shared_ptr<AIModel> getCurrentModel() const
    {
        const ScopedLock sl(modelLock);
        return currentModel;
    }
    
private:
    mutable CriticalSection modelLock;
    std::shared_ptr<AIModel> currentModel;
};
```

**REQUIREMENTS:**
- Models loaded on background thread
- Progress reporting for large models
- Error handling for corrupted/invalid models
- Memory cleanup when models unloaded
- Support for model versioning

---

### 3.3 AI Inference Thread Separation - ERROR

**RULE**: AI inference must run on dedicated thread(s), separate from audio and UI threads.

**DO:**
```cpp
class AIInferenceEngine
{
public:
    AIInferenceEngine(int numThreads = 1)
    {
        for (int i = 0; i < numThreads; ++i)
        {
            inferenceThreads.emplace_back(&AIInferenceEngine::inferenceWorker, this);
        }
    }
    
    ~AIInferenceEngine()
    {
        {
            const ScopedLock sl(queueLock);
            shouldStop = true;
        }
        conditionVariable.notify_all();
        
        for (auto& thread : inferenceThreads)
        {
            if (thread.joinable())
                thread.join();
        }
    }
    
    void queueInference(InferenceRequest request)
    {
        {
            const ScopedLock sl(queueLock);
            inferenceQueue.push(std::move(request));
        }
        conditionVariable.notify_one();
    }
    
private:
    void inferenceWorker()
    {
        while (!shouldStop)
        {
            std::unique_lock<std::mutex> lock(queueLock);
            conditionVariable.wait(lock, [this] { return !inferenceQueue.empty() || shouldStop; });
            
            if (!inferenceQueue.empty())
            {
                auto request = std::move(inferenceQueue.front());
                inferenceQueue.pop();
                lock.unlock();
                
                // Perform inference (may take time)
                auto result = performInference(request);
                
                // Callback on message thread
                MessageManager::callAsync([request, result]()
                {
                    if (request.callback)
                        request.callback(result);
                });
            }
        }
    }
    
    std::vector<std::thread> inferenceThreads;
    std::queue<InferenceRequest> inferenceQueue;
    std::mutex queueLock;
    std::condition_variable conditionVariable;
    std::atomic<bool> shouldStop{false};
};
```

---

### 3.4 Error Handling for AI Failures - ERROR

**RULE**: All AI operations must handle failures gracefully without crashing the application.

**DO:**
```cpp
class SafeAIProcessor
{
public:
    void processWithAI(const AudioBuffer<float>& input, AudioBuffer<float>& output)
    {
        try
        {
            if (!model || !model->isLoaded())
            {
                // Fallback to non-AI processing
                processWithoutAI(input, output);
                return;
            }
            
            auto result = model->infer(input);
            
            if (result.isValid())
            {
                applyAIResult(result, output);
            }
            else
            {
                // Invalid result - use fallback
                processWithoutAI(input, output);
                reportError("AI inference returned invalid result");
            }
        }
        catch (const std::exception& e)
        {
            // Log error and use fallback
            Logger::writeToLog("AI Error: " + String(e.what()));
            processWithoutAI(input, output);
        }
        catch (...)
        {
            // Unknown error - use fallback
            Logger::writeToLog("AI Error: Unknown exception");
            processWithoutAI(input, output);
        }
    }
    
private:
    void processWithoutAI(const AudioBuffer<float>& input, AudioBuffer<float>& output)
    {
        // Non-AI fallback processing
        output.makeCopyOf(input);
    }
};
```

**REQUIREMENTS:**
- Always have fallback processing
- Never crash on AI errors
- Log errors for debugging
- Notify user of AI failures when appropriate
- Graceful degradation

---

### 3.5 Performance Monitoring and Optimization - WARNING

**RULE**: Monitor AI inference performance and optimize for real-time use cases.

**REQUIREMENTS:**
- Profile AI inference time
- Use model quantization where possible
- Cache frequently used results
- Batch processing when appropriate
- Report performance metrics

**DO:**
```cpp
class MonitoredAIModel
{
public:
    InferenceResult infer(const AudioBuffer<float>& input)
    {
        auto startTime = Time::getMillisecondCounterHiRes();
        
        auto result = model->infer(input);
        
        auto elapsed = Time::getMillisecondCounterHiRes() - startTime;
        
        // Track performance
        inferenceTimes.add(elapsed);
        if (inferenceTimes.size() > 100)
            inferenceTimes.remove(0);
        
        // Warn if too slow
        if (elapsed > maxInferenceTime)
        {
            Logger::writeToLog("Warning: AI inference took " + String(elapsed) + "ms");
        }
        
        return result;
    }
    
    float getAverageInferenceTime() const
    {
        if (inferenceTimes.isEmpty())
            return 0.0f;
        
        float sum = 0.0f;
        for (auto time : inferenceTimes)
            sum += time;
        return sum / inferenceTimes.size();
    }
    
private:
    std::unique_ptr<AIModel> model;
    Array<float> inferenceTimes;
    const float maxInferenceTime = 50.0f;  // 50ms max
};
```

---

## 4. UI/UX Professional Standards

### 4.1 JUCE LookAndFeel Customization - ERROR

**RULE**: All UI components must use a consistent custom LookAndFeel. Never use default JUCE styling.

**DO:**
```cpp
class CustomLookAndFeel : public LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        // Define color scheme
        setColour(ResizableWindow::backgroundColourId, Colour(0xff1a1a1a));
        setColour(TextButton::buttonColourId, Colour(0xff2a2a2a));
        setColour(TextButton::textColourOffId, Colour(0xffffffff));
        setColour(Slider::thumbColourId, Colour(0xff00aaff));
        setColour(Slider::rotarySliderFillColourId, Colour(0xff00aaff));
    }
    
    void drawRotarySlider(Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          Slider& slider) override
    {
        // Custom rotary slider drawing
        auto bounds = Rectangle<int>(x, y, width, height).toFloat();
        auto centre = bounds.getCentre();
        auto radius = jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f - 4.0f;
        
        // Draw track
        Path track;
        track.addCentredArc(centre.x, centre.y, radius, radius, 0.0f,
                           rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(findColour(Slider::rotarySliderOutlineColourId));
        g.strokePath(track, PathStrokeType(2.0f));
        
        // Draw fill
        Path fill;
        fill.addCentredArc(centre.x, centre.y, radius, radius, 0.0f,
                          rotaryStartAngle, sliderPos, true);
        g.setColour(findColour(Slider::rotarySliderFillColourId));
        g.strokePath(fill, PathStrokeType(3.0f));
        
        // Draw thumb
        auto thumbAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        Point<float> thumbPoint(centre.x + radius * std::cos(thumbAngle - MathConstants<float>::halfPi),
                               centre.y + radius * std::sin(thumbAngle - MathConstants<float>::halfPi));
        g.setColour(findColour(Slider::thumbColourId));
        g.fillEllipse(Rectangle<float>(8.0f, 8.0f).withCentre(thumbPoint));
    }
};

// Use in application
class MainComponent : public Component
{
public:
    MainComponent()
    {
        lookAndFeel = std::make_unique<CustomLookAndFeel>();
        setLookAndFeel(lookAndFeel.get());
    }
    
    ~MainComponent()
    {
        setLookAndFeel(nullptr);
    }
    
private:
    std::unique_ptr<CustomLookAndFeel> lookAndFeel;
};
```

**DON'T:**
```cpp
// ❌ FORBIDDEN - Default JUCE styling
class MyComponent : public Component
{
    // No custom LookAndFeel - uses default ugly styling
};
```

---

### 4.2 Responsive Design - ERROR

**RULE**: UI must adapt to different screen sizes and DPI settings. Use relative positioning and scaling.

**DO:**
```cpp
class ResponsiveComponent : public Component
{
public:
    void resized() override
    {
        auto bounds = getLocalBounds();
        auto margin = getMargin();
        
        // Use relative positioning
        auto headerHeight = bounds.getHeight() * 0.1f;
        auto contentArea = bounds.reduced(margin);
        
        header.setBounds(contentArea.removeFromTop(headerHeight));
        
        // Grid layout for controls
        auto controlArea = contentArea;
        auto controlWidth = controlArea.getWidth() / 4.0f;
        auto controlHeight = controlArea.getHeight() / 2.0f;
        
        for (int i = 0; i < controls.size(); ++i)
        {
            int row = i / 4;
            int col = i % 4;
            controls[i]->setBounds(
                controlArea.getX() + col * controlWidth,
                controlArea.getY() + row * controlHeight,
                controlWidth - margin,
                controlHeight - margin
            );
        }
    }
    
private:
    int getMargin() const
    {
        // Scale margin with DPI
        return 10 * getApproximateScaleFactorForComponent(this);
    }
    
    Label header;
    OwnedArray<Component> controls;
};
```

**REQUIREMENTS:**
- Support 4K and high-DPI displays
- Minimum window size enforced
- Layout adapts to window resizing
- Text scales appropriately
- Icons scale with DPI

---

### 4.3 Accessibility - ERROR

**RULE**: All UI components must be keyboard accessible and support screen readers.

**DO:**
```cpp
class AccessibleComponent : public Component
{
public:
    AccessibleComponent()
    {
        // Set accessible name and description
        setTitle("Audio Control");
        setDescription("Controls the main audio output level");
        
        // Enable keyboard focus
        setWantsKeyboardFocus(true);
    }
    
    bool keyPressed(const KeyPress& key) override
    {
        if (key == KeyPress::upKey || key == KeyPress::downKey)
        {
            // Handle keyboard control
            float delta = key == KeyPress::upKey ? 0.1f : -0.1f;
            setValue(getValue() + delta);
            return true;
        }
        return Component::keyPressed(key);
    }
    
    std::unique_ptr<AccessibilityHandler> createAccessibilityHandler() override
    {
        return std::make_unique<AccessibilityHandler>(
            *this,
            AccessibilityRole::slider,
            AccessibilityActions(),
            AccessibilityState().withFocusable()
        );
    }
};
```

**REQUIREMENTS:**
- All interactive elements keyboard accessible
- Tab order is logical
- Screen reader announcements for state changes
- High contrast mode support
- Keyboard shortcuts documented

---

### 4.4 UI Performance (60fps) - ERROR

**RULE**: UI must maintain 60fps. Use efficient repainting and avoid unnecessary redraws.

**DO:**
```cpp
class EfficientComponent : public Component
{
public:
    void paint(Graphics& g) override
    {
        // Only repaint dirty regions
        auto clipBounds = g.getClipBounds();
        
        // Use cached images for static content
        if (backgroundImage.isNull() || backgroundImage.getBounds() != getLocalBounds())
        {
            backgroundImage = Image(Image::ARGB, getWidth(), getHeight(), true);
            Graphics bg(backgroundImage);
            drawBackground(bg);
        }
        g.drawImageAt(backgroundImage, 0, 0);
        
        // Draw dynamic content
        drawDynamicContent(g, clipBounds);
    }
    
    void valueChanged()
    {
        // Only repaint affected region
        repaint(affectedRegion);
    }
    
private:
    Image backgroundImage;
    Rectangle<int> affectedRegion;
    
    void drawBackground(Graphics& g)
    {
        // Draw static background once
        g.fillAll(Colour(0xff1a1a1a));
        // ... more static content
    }
    
    void drawDynamicContent(Graphics& g, const Rectangle<int>& clip)
    {
        // Only draw what's visible in clip region
        if (clip.intersects(dynamicRegion))
        {
            // Draw dynamic content
        }
    }
};
```

**DON'T:**
```cpp
// ❌ FORBIDDEN - Repaint entire component unnecessarily
void valueChanged()
{
    repaint();  // Wrong! Only repaint what changed
}

// ❌ FORBIDDEN - Complex operations in paint()
void paint(Graphics& g) override
{
    // Don't do file I/O, network, or heavy computation
    loadImageFromDisk();  // Wrong!
    performHeavyCalculation();  // Wrong!
}
```

**REQUIREMENTS:**
- Use `repaint(region)` instead of `repaint()`
- Cache static graphics
- Avoid allocations in `paint()`
- Profile UI performance regularly
- Use `ComponentAnimator` for smooth animations

---

### 4.5 Consistent Design System - ERROR

**RULE**: All UI components must follow a consistent design system with defined colors, typography, and spacing.

**DO:**
```cpp
namespace DesignSystem
{
    namespace Colors
    {
        constexpr Colour background = Colour(0xff1a1a1a);
        constexpr Colour surface = Colour(0xff2a2a2a);
        constexpr Colour primary = Colour(0xff00aaff);
        constexpr Colour secondary = Colour(0xff666666);
        constexpr Colour text = Colour(0xffffffff);
        constexpr Colour textSecondary = Colour(0xffaaaaaa);
        constexpr Colour error = Colour(0xffff4444);
        constexpr Colour success = Colour(0xff44ff44);
    }
    
    namespace Typography
    {
        constexpr float heading1 = 24.0f;
        constexpr float heading2 = 20.0f;
        constexpr float body = 14.0f;
        constexpr float caption = 12.0f;
        
        Font getHeadingFont() { return Font(heading1, Font::bold); }
        Font getBodyFont() { return Font(body); }
    }
    
    namespace Spacing
    {
        constexpr int unit = 8;
        constexpr int small = unit;
        constexpr int medium = unit * 2;
        constexpr int large = unit * 3;
        constexpr int xlarge = unit * 4;
    }
}

// Use in components
class ThemedComponent : public Component
{
    void paint(Graphics& g) override
    {
        g.fillAll(DesignSystem::Colors::background);
        g.setColour(DesignSystem::Colors::text);
        g.setFont(DesignSystem::Typography::getBodyFont());
        g.drawText("Hello", getLocalBounds(), Justification::centred);
    }
};
```

**REQUIREMENTS:**
- Centralized design tokens
- Consistent spacing system
- Defined color palette
- Typography scale
- Component library documentation

---

## 5. Code Quality & Architecture

### 5.1 SOLID Principles Enforcement - ERROR

**RULE**: All code must follow SOLID principles. Code reviews must verify compliance.

**Single Responsibility Principle:**
```cpp
// DO: Each class has one responsibility
class AudioProcessor
{
    void processAudio(AudioBuffer<float>& buffer);
};

class ParameterManager
{
    void updateParameter(int id, float value);
};

// DON'T: One class doing multiple things
class AudioProcessorAndUIAndFileIO  // ❌ FORBIDDEN
{
    void processAudio(...);
    void drawUI(...);
    void saveFile(...);
};
```

**Open/Closed Principle:**
```cpp
// DO: Extensible through inheritance/interface
class EffectProcessor
{
public:
    virtual ~EffectProcessor() = default;
    virtual void process(AudioBuffer<float>& buffer) = 0;
};

class ReverbProcessor : public EffectProcessor
{
    void process(AudioBuffer<float>& buffer) override;
};

// DON'T: Modify base class for new features
class EffectProcessor
{
    void process(...);
    void processWithReverb(...);  // ❌ Adding new method breaks OCP
};
```

**Liskov Substitution Principle:**
```cpp
// DO: Derived classes must be substitutable
class BaseProcessor
{
public:
    virtual void process(AudioBuffer<float>& buffer) = 0;
    virtual int getLatency() const { return 0; }
};

class DelayProcessor : public BaseProcessor
{
    void process(AudioBuffer<float>& buffer) override;
    int getLatency() const override { return delaySamples; }  // Valid override
};

// DON'T: Derived class that breaks contract
class BrokenProcessor : public BaseProcessor
{
    void process(AudioBuffer<float>& buffer) override
    {
        throw std::exception();  // ❌ Breaks LSP - base doesn't throw
    }
};
```

**Interface Segregation Principle:**
```cpp
// DO: Small, focused interfaces
class IPlayable
{
    virtual void play() = 0;
    virtual void stop() = 0;
};

class IRecordable
{
    virtual void record() = 0;
};

// DON'T: Large interface forcing unused methods
class IAudioDevice  // ❌ FORBIDDEN
{
    virtual void play() = 0;
    virtual void record() = 0;
    virtual void mix() = 0;
    virtual void master() = 0;
    // Not all devices need all methods
};
```

**Dependency Inversion Principle:**
```cpp
// DO: Depend on abstractions
class AudioEngine
{
public:
    AudioEngine(std::shared_ptr<IAudioDevice> device) : device(device) {}
    
private:
    std::shared_ptr<IAudioDevice> device;  // Abstract interface
};

// DON'T: Depend on concrete classes
class AudioEngine
{
    AudioDevice device;  // ❌ Concrete dependency
};
```

---

### 5.2 Module/Namespace Organization - ERROR

**RULE**: Code must be organized into logical modules with clear namespaces.

**STRUCTURE:**
```
src/
  audio/
    processors/
    effects/
    synthesis/
  ui/
    components/
    lookandfeel/
  ai/
    models/
    inference/
  core/
    utilities/
    state/
```

**DO:**
```cpp
namespace daw::audio::processors
{
    class Compressor : public AudioProcessor
    {
        // ...
    };
}

namespace daw::ui::components
{
    class WaveformViewer : public Component
    {
        // ...
    };
}

namespace daw::ai::inference
{
    class InferenceEngine
    {
        // ...
    };
}
```

**DON'T:**
```cpp
// ❌ FORBIDDEN - Everything in global namespace
class Compressor { };
class WaveformViewer { };
class InferenceEngine { };
```

**REQUIREMENTS:**
- Clear namespace hierarchy
- One class per file (with exceptions for related classes)
- Header guards or `#pragma once`
- Forward declarations in headers
- Implementation in .cpp files

---

### 5.3 Documentation Requirements - WARNING

**RULE**: All public APIs must be documented with Doxygen-style comments.

**DO:**
```cpp
/**
 * @brief Audio compressor processor with sidechain support
 * 
 * This processor applies dynamic range compression to audio signals.
 * It supports both standard compression and sidechain compression modes.
 * 
 * @note This class is thread-safe for parameter changes but processBlock()
 *       must only be called from the audio thread.
 * 
 * @example
 * @code
 * Compressor compressor;
 * compressor.setThreshold(-12.0f);
 * compressor.setRatio(4.0f);
 * compressor.processBlock(buffer, midi);
 * @endcode
 */
class Compressor : public AudioProcessor
{
public:
    /**
     * @brief Sets the compression threshold in dB
     * 
     * Signals above this threshold will be compressed according to the ratio.
     * 
     * @param thresholdDb Threshold in decibels (typically -60 to 0)
     * @throws std::invalid_argument if threshold is out of valid range
     */
    void setThreshold(float thresholdDb);
    
    /**
     * @brief Gets the current compression threshold
     * @return Threshold in decibels
     */
    float getThreshold() const noexcept;
};
```

**REQUIREMENTS:**
- All public classes documented
- All public methods documented
- Parameter descriptions
- Return value descriptions
- Exception documentation
- Usage examples for complex APIs
- Thread safety notes

---

### 5.4 Testing Requirements - ERROR

**RULE**: All critical functionality must have unit tests. Code coverage must be >80% for audio processing code.

**DO:**
```cpp
// tests/audio/CompressorTest.cpp
class CompressorTest : public juce::UnitTest
{
public:
    CompressorTest() : juce::UnitTest("Compressor Tests") {}
    
    void runTest() override
    {
        beginTest("Threshold Test");
        {
            Compressor compressor;
            compressor.setThreshold(-12.0f);
            expectEquals(compressor.getThreshold(), -12.0f);
        }
        
        beginTest("Compression Test");
        {
            Compressor compressor;
            compressor.setThreshold(-6.0f);
            compressor.setRatio(4.0f);
            
            AudioBuffer<float> buffer(1, 512);
            buffer.fill(-3.0f);  // Above threshold
            
            compressor.processBlock(buffer, MidiBuffer());
            
            // Verify compression applied
            auto maxLevel = buffer.getMagnitude(0, 512);
            expectLessThan(maxLevel, -3.0f, "Signal should be compressed");
        }
        
        beginTest("Real-time Safety");
        {
            // Verify no allocations in processBlock
            Compressor compressor;
            AudioBuffer<float> buffer(2, 512);
            
            // Profile allocation count
            int allocationsBefore = getAllocationCount();
            compressor.processBlock(buffer, MidiBuffer());
            int allocationsAfter = getAllocationCount();
            
            expectEquals(allocationsAfter, allocationsBefore, 
                        "No allocations in audio thread");
        }
    }
};

static CompressorTest compressorTest;
```

**REQUIREMENTS:**
- Unit tests for all audio processors
- Integration tests for complex workflows
- Performance tests for real-time safety
- UI component tests
- AI model inference tests
- Continuous integration runs all tests

---

### 5.5 Code Review Checklist - ERROR

**RULE**: All code must pass code review before merging. Use this checklist:

**MANDATORY CHECKS:**
- [ ] No raw pointers or manual memory management
- [ ] No locks or allocations in audio thread
- [ ] All public APIs documented
- [ ] Unit tests written and passing
- [ ] Follows SOLID principles
- [ ] Proper error handling
- [ ] Thread safety verified
- [ ] Performance profiled (if applicable)
- [ ] UI is accessible and responsive
- [ ] No hardcoded values (use constants)
- [ ] Code formatted consistently
- [ ] No compiler warnings
- [ ] Cross-platform compatibility verified

---

## 6. Performance Requirements

### 6.1 Audio Thread Performance Budget - CRITICAL

**RULE**