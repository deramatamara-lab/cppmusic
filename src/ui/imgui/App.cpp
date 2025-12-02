// Must define math operators before any imgui includes
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "App.hpp"
#include "imgui_internal.h"
#include <GL/gl.h>
#include <iostream>
#include <cmath>

namespace daw::ui::imgui
{

// ============================================================================
// PerformanceMetrics Implementation
// ============================================================================

void PerformanceMetrics::update(float dt)
{
    frameTimeMs = dt * 1000.0f;

    // Exponential moving average
    const float alpha = 0.1f;
    avgFrameTimeMs = alpha * frameTimeMs + (1.0f - alpha) * avgFrameTimeMs;

    fps = (avgFrameTimeMs > 0.0f) ? (1000.0f / avgFrameTimeMs) : 0.0f;

    // Get ImGui draw data
    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData)
    {
        drawCalls = drawData->CmdListsCount;
        vertexCount = drawData->TotalVtxCount;
        indexCount = drawData->TotalIdxCount;
        // Note: clippedItems would require tracking during rendering
        // and is reserved for future culling optimization metrics
    }
}

void PerformanceMetrics::draw()
{
    if (!showOverlay) return;

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

    const float padding = 10.0f;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;
    ImVec2 windowPos = ImVec2(workPos.x + workSize.x - padding, workPos.y + padding);
    ImVec2 windowPosPivot = ImVec2(1.0f, 0.0f);

    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPosPivot);
    ImGui::SetNextWindowBgAlpha(0.8f);

    if (ImGui::Begin("##PerformanceOverlay", nullptr, flags))
    {
        ImGui::Text("Performance");
        ImGui::Separator();

        // Color code FPS
        ImVec4 fpsColor = (fps >= 60.0f) ? ImVec4(0.4f, 0.9f, 0.4f, 1.0f) :
                          (fps >= 30.0f) ? ImVec4(0.9f, 0.9f, 0.4f, 1.0f) :
                                           ImVec4(0.9f, 0.4f, 0.4f, 1.0f);

        ImGui::TextColored(fpsColor, "%.1f FPS", fps);
        ImGui::Text("Frame: %.2f ms", frameTimeMs);
        ImGui::Text("Avg:   %.2f ms", avgFrameTimeMs);

        ImGui::Separator();
        ImGui::Text("Draw calls: %d", drawCalls);
        ImGui::Text("Vertices:   %d", vertexCount);
        ImGui::Text("Indices:    %d", indexCount);

        // Frame budget bar (targeting 16.67ms for 60Hz)
        float budgetUsed = frameTimeMs / 16.67f;
        ImGui::Text("Budget:");
        ImGui::SameLine();
        ImGui::ProgressBar(std::min(budgetUsed, 1.0f), ImVec2(100, 0), "");
        if (budgetUsed > 1.0f)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "!");
        }
    }
    ImGui::End();
}

// ============================================================================
// App Implementation
// ============================================================================

App::App() = default;

App::~App()
{
    shutdown();
}

bool App::initialize(const AppConfig& config)
{
    config_ = config;
    headless_ = false;

    if (!initializeSDL()) return false;
    if (!initializeOpenGL()) return false;
    if (!initializeImGui()) return false;

    // Load theme
    std::filesystem::path themePath = config_.themePath;
    if (!themePath.is_absolute())
    {
        // Try relative to executable or current directory
        if (std::filesystem::exists(themePath))
        {
            theme_.loadFromFile(themePath);
        }
    }
    else
    {
        theme_.loadFromFile(themePath);
    }
    theme_.applyToImGui();

    // Register default commands and load shortcuts
    shortcuts_.registerDefaultCommands();
    registerViewCommands();
    if (std::filesystem::exists(config_.shortcutsPath))
    {
        shortcuts_.loadFromFile(config_.shortcutsPath);
    }

    // Initialize panels
    transportBar_ = std::make_unique<TransportBar>();
    browserPanel_ = std::make_unique<BrowserPanel>();
    channelRackPanel_ = std::make_unique<ChannelRackPanel>();
    pianoRollPanel_ = std::make_unique<PianoRollPanel>();
    playlistPanel_ = std::make_unique<PlaylistPanel>();
    mixerPanel_ = std::make_unique<MixerPanel>();
    inspectorPanel_ = std::make_unique<InspectorPanel>();

    // Initialize Audio Engine
    audioEngine_ = std::make_unique<AudioEngine>();
    if (!audioEngine_->initialize())
    {
        std::cerr << "Warning: Audio engine failed to initialize" << std::endl;
    }

    // Wire up Transport callbacks to Audio Engine
    if (transportBar_ && audioEngine_)
    {
        transportBar_->setOnPlay([this](bool playing) {
            if (playing)
                audioEngine_->play();
            else
                audioEngine_->pause();
        });

        transportBar_->setOnStop([this]() {
            audioEngine_->stop();
        });

        transportBar_->setOnBpmChange([this](double bpm) {
            audioEngine_->setBPM(bpm);
        });
    }

    // Wire up Channel Rack callbacks
    if (channelRackPanel_)
    {
        channelRackPanel_->setOnChannelSelected([this](int channelIndex) {
            (void)channelIndex;
            // When a channel is selected, show the Piano Roll
            showPianoRoll_ = true;
        });

        channelRackPanel_->setOnChannelDoubleClick([this](int channelIndex) {
            (void)channelIndex;
            // Double click opens plugin/inspector
            showInspector_ = true;
        });

        // Wire step changes to audio engine
        channelRackPanel_->setOnStepChanged([this](int channel, int step, bool active) {
            if (audioEngine_)
            {
                audioEngine_->setStep(channel, step, active);
            }
        });
    }

    // Load layout
    loadLayout();

    initialized_ = true;
    running_ = true;
    lastFrameTime_ = std::chrono::steady_clock::now();

    return true;
}

bool App::initializeHeadless()
{
    headless_ = true;

    // Initialize SDL with video (required for OpenGL context)
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Set OpenGL attributes for offscreen rendering
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // Create hidden window for GL context
    window_ = SDL_CreateWindow(
        "Headless",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        800, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN
    );

    if (!window_)
    {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return false;
    }

    glContext_ = SDL_GL_CreateContext(window_);
    if (!glContext_)
    {
        std::cerr << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_MakeCurrent(window_, glContext_);

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplSDL2_InitForOpenGL(window_, glContext_);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Apply theme
    theme_.applyToImGui();

    // Register commands
    shortcuts_.registerDefaultCommands();
    registerViewCommands();

    // Initialize panels
    transportBar_ = std::make_unique<TransportBar>();
    browserPanel_ = std::make_unique<BrowserPanel>();
    channelRackPanel_ = std::make_unique<ChannelRackPanel>();
    pianoRollPanel_ = std::make_unique<PianoRollPanel>();
    playlistPanel_ = std::make_unique<PlaylistPanel>();
    mixerPanel_ = std::make_unique<MixerPanel>();
    inspectorPanel_ = std::make_unique<InspectorPanel>();

    initialized_ = true;
    running_ = true;

    return true;
}

bool App::initializeSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // Create window
    Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                         SDL_WINDOW_ALLOW_HIGHDPI;

    if (config_.fullscreen)
    {
        windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    window_ = SDL_CreateWindow(
        config_.windowTitle.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        config_.windowWidth,
        config_.windowHeight,
        windowFlags
    );

    if (!window_)
    {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

bool App::initializeOpenGL()
{
    glContext_ = SDL_GL_CreateContext(window_);
    if (!glContext_)
    {
        std::cerr << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_MakeCurrent(window_, glContext_);

    // Set VSync
    SDL_GL_SetSwapInterval(config_.vsync ? 1 : 0);

    return true;
}

bool App::initializeImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // Multi-viewport

    // Set up layout persistence
    if (!config_.layoutPath.empty())
    {
        static std::string layoutPathStr = config_.layoutPath.string();
        io.IniFilename = layoutPathStr.c_str();
    }

    // Calculate DPI scale
    float dpiScale = calculateDpiScale();
    theme_.setDpiScale(dpiScale);

    // Setup platform/renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window_, glContext_);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Setup fonts
    setupFonts();

    // Configure style
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    return true;
}

void App::setupFonts()
{
    ImGuiIO& io = ImGui::GetIO();
    float dpiScale = theme_.getDpiScale();

    // Clear existing fonts
    io.Fonts->Clear();

    // Add default font with DPI scaling
    float fontSize = theme_.getTokens().fontSizeMd * dpiScale;

    // Try to load system fonts, fallback to embedded
    ImFontConfig fontConfig;
    fontConfig.OversampleH = 2;
    fontConfig.OversampleV = 1;
    fontConfig.PixelSnapH = true;

    // Try common font paths
    const char* fontPaths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/System/Library/Fonts/SFNSText.ttf",
        "C:\\Windows\\Fonts\\segoeui.ttf"
    };

    bool fontLoaded = false;
    for (const char* path : fontPaths)
    {
        if (std::filesystem::exists(path))
        {
            io.Fonts->AddFontFromFileTTF(path, fontSize, &fontConfig);
            fontLoaded = true;
            break;
        }
    }

    if (!fontLoaded)
    {
        io.Fonts->AddFontDefault();
    }

    io.Fonts->Build();
}

float App::calculateDpiScale()
{
    if (config_.dpiScale > 0.0f)
    {
        return config_.dpiScale;
    }

    // Auto-detect DPI
    float dpi = 96.0f;
    int displayIndex = SDL_GetWindowDisplayIndex(window_);
    if (displayIndex >= 0)
    {
        float ddpi = 0, hdpi = 0, vdpi = 0;
        if (SDL_GetDisplayDPI(displayIndex, &ddpi, &hdpi, &vdpi) == 0 && ddpi > 0)
        {
            dpi = ddpi;
        }
    }

    return dpi / 96.0f;
}

void App::loadLayout()
{
    // ImGui handles layout persistence via IniFilename
}

void App::saveLayout()
{
    ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
}

int App::run()
{
    while (running_)
    {
        processEvents();

        if (!running_) break;

        renderFrame();
    }

    saveLayout();
    return 0;
}

bool App::renderFrame()
{
    auto now = std::chrono::steady_clock::now();
    deltaTime_ = std::chrono::duration<float>(now - lastFrameTime_).count();
    lastFrameTime_ = now;

    beginFrame();

    // Sync audio engine state to UI
    if (audioEngine_ && transportBar_)
    {
        auto& state = transportBar_->getState();
        state.isPlaying = audioEngine_->isPlaying();
        state.cpuUsage = audioEngine_->getCPUUsage();
        state.positionBeats = audioEngine_->getPositionBeats();

        // Sync current step to channel rack
        if (channelRackPanel_)
        {
            channelRackPanel_->setCurrentStep(audioEngine_->getCurrentStep());

            // Sync channels from audio engine to UI
            int numChannels = audioEngine_->getNumChannels();
            auto& channels = channelRackPanel_->getChannels();

            // Ensure UI has same number of channels as audio engine
            while (static_cast<int>(channels.size()) < numChannels)
            {
                channelRackPanel_->addChannel("Channel");
            }

            // Sync step states and names from audio engine to UI
            for (int ch = 0; ch < numChannels && ch < static_cast<int>(channels.size()); ++ch)
            {
                channels[ch].name = audioEngine_->getChannelName(ch);
                for (int step = 0; step < 16; ++step)
                {
                    channels[ch].steps[step] = audioEngine_->getStep(ch, step);
                }
            }
        }
    }

    // Process shortcuts
    shortcuts_.processInput();

    // Check for theme hot reload
    if (theme_.reloadIfModified())
    {
        theme_.applyToImGui();
    }

    renderDockspace();

    // FL-style workflow routing: toggle visible/active panels by mode
    if (transportBar_)
    {
        auto mode = transportBar_->getState().mode;
        // Force update on first frame or change
        static bool firstFrame = true;
        if (mode != lastTransportMode_ || firstFrame)
        {
            firstFrame = false;
            lastTransportMode_ = mode;
            if (mode == TransportMode::Pattern)
            {
                showChannelRack_ = true;
                showPianoRoll_ = true;
                showPlaylist_ = false;
                // Keep mixer available but de-emphasized
                showMixer_ = true;
            }
            else // Song mode
            {
                showChannelRack_ = true; // FL shows rack even in Song
                showPianoRoll_ = false;  // Focus on Playlist
                showPlaylist_ = true;
                showMixer_ = true;
            }
        }
    }

    renderPanels();

    // Draw command palette if open
    bool paletteOpen = shortcuts_.isCommandPaletteOpen();
    shortcuts_.drawCommandPalette(paletteOpen);
    if (!paletteOpen && shortcuts_.isCommandPaletteOpen())
    {
        shortcuts_.closeCommandPalette();
    }

    // Draw performance overlay
    metrics_.update(deltaTime_);
    metrics_.draw();

    endFrame();

    return true;
}

void App::processEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);

        switch (event.type)
        {
            case SDL_QUIT:
                running_ = false;
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE &&
                    event.window.windowID == SDL_GetWindowID(window_))
                {
                    running_ = false;
                }
                else if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    handleResize(event.window.data1, event.window.data2);
                }
                break;

            case SDL_KEYDOWN:
                // F12 toggles performance overlay
                if (event.key.keysym.sym == SDLK_F12)
                {
                    togglePerformanceOverlay();
                }
                break;
        }
    }
}

void App::beginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void App::renderDockspace()
{
    // Create full-window dockspace
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
                                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus |
                                   ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpace", nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    // Menu bar
    renderMenuBar();

    // Transport bar (above dockspace)
    if (transportBar_)
    {
        transportBar_->draw(theme_);
    }

    // Create dockspace
    ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    // Set up initial layout on first run
    static bool firstRun = true;
    if (firstRun)
    {
        firstRun = false;

        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);

        // FL Studio-like layout:
        // Left: Browser + Channel Rack (tabs)
        // Center: Piano Roll + Playlist (tabs)
        // Bottom: Mixer (full width of center)
        // Right: Inspector
        ImGuiID dockLeft, dockCenter, dockRight, dockBottom;

        // Left 22%
        ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.22f, &dockLeft, &dockCenter);
        // Right 22%
        ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Right, 0.22f, &dockRight, &dockCenter);
        // Bottom 28%
        ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Down, 0.28f, &dockBottom, &dockCenter);

        // Dock windows
        ImGui::DockBuilderDockWindow("Browser", dockLeft);
        ImGui::DockBuilderDockWindow("Channel Rack", dockLeft);
        ImGui::DockBuilderDockWindow("Piano Roll", dockCenter);
        ImGui::DockBuilderDockWindow("Playlist", dockCenter);
        ImGui::DockBuilderDockWindow("Mixer", dockBottom);
        ImGui::DockBuilderDockWindow("Inspector", dockRight);

        ImGui::DockBuilderFinish(dockspaceId);
    }

    ImGui::End();
}

void App::renderMenuBar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Project", "Ctrl+N")) { shortcuts_.executeCommand("file.new"); }
            if (ImGui::MenuItem("Open Project", "Ctrl+O")) { shortcuts_.executeCommand("file.open"); }
            if (ImGui::MenuItem("Save", "Ctrl+S")) { shortcuts_.executeCommand("file.save"); }
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) { shortcuts_.executeCommand("file.save_as"); }
            ImGui::Separator();
            if (ImGui::MenuItem("Export Audio", "Ctrl+Shift+E")) { shortcuts_.executeCommand("file.export"); }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) { quit(); }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) { shortcuts_.executeCommand("edit.undo"); }
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) { shortcuts_.executeCommand("edit.redo"); }
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "Ctrl+X")) { shortcuts_.executeCommand("edit.cut"); }
            if (ImGui::MenuItem("Copy", "Ctrl+C")) { shortcuts_.executeCommand("edit.copy"); }
            if (ImGui::MenuItem("Paste", "Ctrl+V")) { shortcuts_.executeCommand("edit.paste"); }
            if (ImGui::MenuItem("Delete", "Delete")) { shortcuts_.executeCommand("edit.delete"); }
            ImGui::Separator();
            if (ImGui::MenuItem("Select All", "Ctrl+A")) { shortcuts_.executeCommand("edit.select_all"); }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Browser", "Ctrl+B", &showBrowser_);
            ImGui::MenuItem("Channel Rack", nullptr, &showChannelRack_);
            ImGui::MenuItem("Piano Roll", "Ctrl+P", &showPianoRoll_);
            ImGui::MenuItem("Playlist", nullptr, &showPlaylist_);
            ImGui::MenuItem("Mixer", "Ctrl+M", &showMixer_);
            ImGui::MenuItem("Inspector", nullptr, &showInspector_);
            ImGui::Separator();
            ImGui::MenuItem("Performance Overlay", "F12", &metrics_.showOverlay);
            ImGui::Separator();
            if (ImGui::MenuItem("Fullscreen", "F11")) { shortcuts_.executeCommand("view.fullscreen"); }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Transport"))
        {
            if (ImGui::MenuItem("Play/Pause", "Space")) { shortcuts_.executeCommand("transport.play"); }
            if (ImGui::MenuItem("Stop", "Enter")) { shortcuts_.executeCommand("transport.stop"); }
            if (ImGui::MenuItem("Record", "Ctrl+R")) { shortcuts_.executeCommand("transport.record"); }
            ImGui::Separator();
            if (ImGui::MenuItem("Toggle Loop", "Ctrl+L")) { shortcuts_.executeCommand("transport.loop"); }
            ImGui::Separator();
            if (ImGui::MenuItem("Go to Start", "Home")) { shortcuts_.executeCommand("transport.goto_start"); }
            if (ImGui::MenuItem("Go to End", "End")) { shortcuts_.executeCommand("transport.goto_end"); }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("Command Palette", "Ctrl+K"))
            {
                shortcuts_.openCommandPalette();
            }
            ImGui::Separator();
            ImGui::MenuItem("About cppmusic");
            ImGui::EndMenu();
        }

        // Show time/position on right side of menu bar
        float rightPadding = 200.0f;
        ImGui::SameLine(ImGui::GetWindowWidth() - rightPadding);
        ImGui::TextDisabled("00:00:00.000 | 1.1.1");

        ImGui::EndMenuBar();
    }
}

void App::renderPanels()
{
    if (showBrowser_ && browserPanel_)
    {
        browserPanel_->draw(showBrowser_, theme_);
    }

    if (showChannelRack_ && channelRackPanel_)
    {
        channelRackPanel_->draw(showChannelRack_, theme_);
    }

    if (showPianoRoll_ && pianoRollPanel_)
    {
        pianoRollPanel_->draw(showPianoRoll_, theme_);
    }

    if (showPlaylist_ && playlistPanel_)
    {
        playlistPanel_->draw(showPlaylist_, theme_);
    }

    if (showMixer_ && mixerPanel_)
    {
        mixerPanel_->draw(showMixer_, theme_);
    }

    if (showInspector_ && inspectorPanel_)
    {
        inspectorPanel_->draw(showInspector_, theme_);
    }
}

void App::endFrame()
{
    ImGui::Render();

    int displayW, displayH;
    SDL_GL_GetDrawableSize(window_, &displayW, &displayH);
    glViewport(0, 0, displayW, displayH);

    const auto& bg = theme_.getTokens().windowBg;
    glClearColor(bg.x, bg.y, bg.z, bg.w);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Handle multi-viewport
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        SDL_Window* backupWindow = SDL_GL_GetCurrentWindow();
        SDL_GLContext backupContext = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backupWindow, backupContext);
    }

    SDL_GL_SwapWindow(window_);
}

void App::handleResize(int width, int height)
{
    config_.windowWidth = width;
    config_.windowHeight = height;

    // Recalculate DPI scale if needed
    float newDpiScale = calculateDpiScale();
    if (std::abs(newDpiScale - theme_.getDpiScale()) > 0.01f)
    {
        theme_.setDpiScale(newDpiScale);
        theme_.applyToImGui();
        setupFonts();
    }
}

void App::reloadTheme()
{
    if (theme_.loadFromFile(theme_.getCurrentPath()))
    {
        theme_.applyToImGui();
    }
}

void App::registerViewCommands()
{
    shortcuts_.registerCommand("view.toggle_browser", "Toggle Browser", "View",
        {ImGuiKey_B, KeyMod::Ctrl},
        [this]() { showBrowser_ = !showBrowser_; },
        "Show/hide browser panel");

    shortcuts_.registerCommand("view.toggle_mixer", "Toggle Mixer", "View",
        {ImGuiKey_M, KeyMod::Ctrl},
        [this]() { showMixer_ = !showMixer_; },
        "Show/hide mixer panel");

    shortcuts_.registerCommand("view.toggle_piano_roll", "Toggle Piano Roll", "View",
        {ImGuiKey_P, KeyMod::Ctrl},
        [this]() { showPianoRoll_ = !showPianoRoll_; },
        "Show/hide piano roll panel");

    shortcuts_.registerCommand("view.toggle_perf_overlay", "Toggle Performance Overlay", "View",
        {ImGuiKey_F12, KeyMod::None},
        [this]() { togglePerformanceOverlay(); },
        "Show/hide performance overlay");
}

void App::shutdown()
{
    if (!initialized_) return;

    // Shutdown audio engine first
    if (audioEngine_)
    {
        audioEngine_->shutdown();
        audioEngine_.reset();
    }

    // Clear panels
    transportBar_.reset();
    browserPanel_.reset();
    channelRackPanel_.reset();
    pianoRollPanel_.reset();
    playlistPanel_.reset();
    mixerPanel_.reset();
    inspectorPanel_.reset();

    // Shutdown ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    // Cleanup SDL
    if (glContext_)
    {
        SDL_GL_DeleteContext(glContext_);
        glContext_ = nullptr;
    }

    if (window_)
    {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    SDL_Quit();

    initialized_ = false;
}

} // namespace daw::ui::imgui
