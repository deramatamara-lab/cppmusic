#pragma once

#include "Theme.hpp"
#include "Shortcuts.hpp"
#include "panels/TransportBar.hpp"
#include "panels/BrowserPanel.hpp"
#include "panels/ChannelRackPanel.hpp"
#include "panels/PianoRollPanel.hpp"
#include "panels/PlaylistPanel.hpp"
#include "panels/MixerPanel.hpp"
#include "panels/InspectorPanel.hpp"
#include "audio/AudioEngine.hpp"

#include <SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include <memory>
#include <string>
#include <filesystem>
#include <chrono>

namespace daw::ui::imgui
{

/**
 * @brief Performance metrics for frame instrumentation
 */
struct PerformanceMetrics
{
    float frameTimeMs{0.0f};
    float avgFrameTimeMs{0.0f};
    float fps{0.0f};
    int drawCalls{0};
    int vertexCount{0};
    int indexCount{0};
    int clippedItems{0};
    bool showOverlay{false};

    void update(float dt);
    void draw();
};

/**
 * @brief Application configuration
 */
struct AppConfig
{
    std::string windowTitle{"cppmusic DAW"};
    int windowWidth{1920};
    int windowHeight{1080};
    bool fullscreen{false};
    bool vsync{true};
    float targetFps{144.0f};
    std::filesystem::path themePath{"assets/themes/default.json"};
    std::filesystem::path shortcutsPath{"config/shortcuts.json"};
    std::filesystem::path layoutPath{"config/layout.ini"};
    float dpiScale{0.0f};  // 0 = auto-detect
};

/**
 * @brief Main application class for the ImGui-based DAW UI
 *
 * Manages the SDL2 window, OpenGL context, ImGui initialization,
 * dockspace layout, theme system, and all UI panels.
 */
class App
{
public:
    App();
    ~App();

    // Non-copyable, non-movable
    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) = delete;
    App& operator=(App&&) = delete;

    /**
     * @brief Initialize the application
     * @param config Application configuration
     * @return true if initialization successful
     */
    bool initialize(const AppConfig& config = AppConfig{});

    /**
     * @brief Run the main application loop
     * @return Exit code
     */
    int run();

    /**
     * @brief Request application shutdown
     */
    void quit() { running_ = false; }

    /**
     * @brief Check if application is running
     */
    [[nodiscard]] bool isRunning() const { return running_; }

    /**
     * @brief Get the theme manager
     */
    [[nodiscard]] Theme& getTheme() { return theme_; }
    [[nodiscard]] const Theme& getTheme() const { return theme_; }

    /**
     * @brief Get the shortcut manager
     */
    [[nodiscard]] Shortcuts& getShortcuts() { return shortcuts_; }
    [[nodiscard]] const Shortcuts& getShortcuts() const { return shortcuts_; }

    /**
     * @brief Get performance metrics
     */
    [[nodiscard]] PerformanceMetrics& getMetrics() { return metrics_; }
    [[nodiscard]] const PerformanceMetrics& getMetrics() const { return metrics_; }

    /**
     * @brief Get the audio engine
     */
    [[nodiscard]] AudioEngine* getAudioEngine() { return audioEngine_.get(); }
    [[nodiscard]] const AudioEngine* getAudioEngine() const { return audioEngine_.get(); }

    /**
     * @brief Reload theme from file
     */
    void reloadTheme();

    /**
     * @brief Toggle performance overlay
     */
    void togglePerformanceOverlay() { metrics_.showOverlay = !metrics_.showOverlay; }

    /**
     * @brief Initialize with offscreen context for headless testing
     * @return true if initialization successful
     */
    bool initializeHeadless();

    /**
     * @brief Render a single frame (for testing)
     * @return true if frame rendered successfully
     */
    bool renderFrame();

    /**
     * @brief Cleanup resources
     */
    void shutdown();

private:
    // SDL/OpenGL resources
    SDL_Window* window_{nullptr};
    SDL_GLContext glContext_{nullptr};

    // Configuration
    AppConfig config_;
    bool running_{false};
    bool initialized_{false};
    bool headless_{false};

    // Core systems
    Theme theme_;
    Shortcuts shortcuts_;
    PerformanceMetrics metrics_;

    // Panels
    std::unique_ptr<TransportBar> transportBar_;
    std::unique_ptr<BrowserPanel> browserPanel_;
    std::unique_ptr<ChannelRackPanel> channelRackPanel_;
    std::unique_ptr<PianoRollPanel> pianoRollPanel_;
    std::unique_ptr<PlaylistPanel> playlistPanel_;
    std::unique_ptr<MixerPanel> mixerPanel_;
    std::unique_ptr<InspectorPanel> inspectorPanel_;

    // Audio Engine (real sound!)
    std::unique_ptr<AudioEngine> audioEngine_;

    // State tracking
    TransportMode lastTransportMode_{TransportMode::Pattern};

    // Panel visibility
    bool showBrowser_{true};
    bool showChannelRack_{true};
    bool showPianoRoll_{true};
    bool showPlaylist_{true};
    bool showMixer_{true};
    bool showInspector_{true};

    // Timing
    std::chrono::steady_clock::time_point lastFrameTime_;
    float deltaTime_{0.0f};

    // Private methods
    bool initializeSDL();
    bool initializeOpenGL();
    bool initializeImGui();
    void setupFonts();
    void loadLayout();
    void saveLayout();
    void processEvents();
    void beginFrame();
    void renderDockspace();
    void renderMenuBar();
    void renderPanels();
    void endFrame();
    void handleResize(int width, int height);
    float calculateDpiScale();
    void registerViewCommands();
};

} // namespace daw::ui::imgui
