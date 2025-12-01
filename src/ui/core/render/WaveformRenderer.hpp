/**
 * @file WaveformRenderer.hpp
 * @brief GPU-accelerated waveform rendering with OpenGL
 */
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace daw::ui::render
{

/**
 * @brief Waveform display mode
 */
enum class WaveformMode
{
    Peak,           // Peak envelope
    RMS,            // RMS envelope
    PeakAndRMS,     // Both overlaid
    Oscilloscope    // Raw samples (for short views)
};

/**
 * @brief Color scheme for waveform
 */
struct WaveformColors
{
    float peakColor[4]{0.3f, 0.6f, 0.9f, 1.0f};
    float rmsColor[4]{0.2f, 0.4f, 0.7f, 0.8f};
    float centerLine[4]{0.5f, 0.5f, 0.5f, 0.3f};
    float backgroundColor[4]{0.1f, 0.1f, 0.12f, 1.0f};
    float clipIndicator[4]{1.0f, 0.2f, 0.2f, 1.0f};
};

/**
 * @brief Pre-computed waveform mipmap level
 */
struct WaveformMipmap
{
    std::vector<float> minPeaks;     // Min peak per pixel column
    std::vector<float> maxPeaks;     // Max peak per pixel column
    std::vector<float> rms;          // RMS per pixel column
    int samplesPerPixel{1};          // Reduction factor
    bool hasClipping{false};         // True if any samples clip
};

/**
 * @brief Waveform data with multiple mipmap levels
 */
struct WaveformData
{
    std::vector<WaveformMipmap> mipmaps;  // Multiple resolution levels
    int sampleRate{44100};
    int channels{2};
    double durationSeconds{0.0};
    std::string sourceId;                  // Source file/buffer ID
    bool ready{false};
    
    /**
     * @brief Get appropriate mipmap for given samples per pixel
     */
    [[nodiscard]] const WaveformMipmap* getMipmapForScale(int samplesPerPixel) const
    {
        if (mipmaps.empty()) return nullptr;
        
        // Find closest mipmap without excessive upsampling
        const WaveformMipmap* best = &mipmaps[0];
        for (const auto& mm : mipmaps) {
            if (mm.samplesPerPixel <= samplesPerPixel &&
                mm.samplesPerPixel > best->samplesPerPixel) {
                best = &mm;
            }
        }
        return best;
    }
};

/**
 * @brief Waveform generation task
 */
struct WaveformTask
{
    std::string id;
    std::vector<float> samples;
    int sampleRate{44100};
    int channels{2};
    std::promise<std::shared_ptr<WaveformData>> promise;
};

/**
 * @brief GPU-accelerated waveform renderer
 * 
 * Features:
 * - Async waveform generation with mipmapping
 * - OpenGL rendering (fragment shader based)
 * - Efficient batched drawing
 * - Multiple display modes
 */
class WaveformRenderer
{
public:
    WaveformRenderer();
    ~WaveformRenderer();

    // Non-copyable
    WaveformRenderer(const WaveformRenderer&) = delete;
    WaveformRenderer& operator=(const WaveformRenderer&) = delete;

    /**
     * @brief Initialize OpenGL resources
     * @return true if successful
     */
    bool initialize();

    /**
     * @brief Shutdown and release resources
     */
    void shutdown();

    /**
     * @brief Generate waveform data asynchronously
     * @param id Unique identifier for caching
     * @param samples Audio sample data (interleaved if stereo)
     * @param sampleRate Sample rate in Hz
     * @param channels Number of channels
     * @return Future with generated waveform data
     */
    std::future<std::shared_ptr<WaveformData>> generateAsync(
        const std::string& id,
        const std::vector<float>& samples,
        int sampleRate = 44100,
        int channels = 2);

    /**
     * @brief Render waveform to current framebuffer
     * @param data Waveform data to render
     * @param x X position
     * @param y Y position
     * @param width Width in pixels
     * @param height Height in pixels
     * @param startSample Start sample offset
     * @param endSample End sample offset
     */
    void render(
        const WaveformData& data,
        float x, float y,
        float width, float height,
        int64_t startSample, int64_t endSample);

    /**
     * @brief Render waveform using ImDrawList
     * 
     * Non-GPU fallback for integration with ImGui
     */
    void renderImGui(
        void* drawList,  // ImDrawList*
        const WaveformData& data,
        float x, float y,
        float width, float height,
        int64_t startSample, int64_t endSample);

    /**
     * @brief Get cached waveform by ID
     */
    [[nodiscard]] std::shared_ptr<WaveformData> getCached(const std::string& id) const;

    /**
     * @brief Remove cached waveform
     */
    void removeCached(const std::string& id);

    /**
     * @brief Clear all cached waveforms
     */
    void clearCache();

    /**
     * @brief Get/set display mode
     */
    [[nodiscard]] WaveformMode getMode() const { return mode_; }
    void setMode(WaveformMode mode) { mode_ = mode; }

    /**
     * @brief Get/set colors
     */
    [[nodiscard]] const WaveformColors& getColors() const { return colors_; }
    WaveformColors& getColorsMutable() { return colors_; }
    void setColors(const WaveformColors& colors) { colors_ = colors; }

    /**
     * @brief Check if GPU rendering is available
     */
    [[nodiscard]] bool isGPUAvailable() const { return gpuAvailable_; }

    /**
     * @brief Get render statistics
     */
    struct RenderStats
    {
        uint64_t drawCalls{0};
        uint64_t verticesRendered{0};
        double lastRenderTimeMs{0.0};
    };
    [[nodiscard]] const RenderStats& getStats() const { return stats_; }

private:
    /**
     * @brief Generate mipmap levels for waveform
     */
    void generateMipmaps(
        WaveformData& data,
        const std::vector<float>& samples,
        int channels);

    /**
     * @brief Worker thread for async generation
     */
    void workerThreadFunc();

    // OpenGL resources
    uint32_t vao_{0};
    uint32_t vbo_{0};
    uint32_t shader_{0};
    bool gpuAvailable_{false};
    bool initialized_{false};

    // Display settings
    WaveformMode mode_{WaveformMode::PeakAndRMS};
    WaveformColors colors_;

    // Cache
    mutable std::mutex cacheMutex_;
    std::unordered_map<std::string, std::shared_ptr<WaveformData>> cache_;

    // Async generation
    std::mutex taskMutex_;
    std::vector<WaveformTask> pendingTasks_;
    std::thread workerThread_;
    std::atomic<bool> running_{false};

    // Stats
    RenderStats stats_;
};

/**
 * @brief Meter renderer with batched drawing
 */
class MeterRenderer
{
public:
    /**
     * @brief Meter display style
     */
    enum class Style
    {
        Gradient,       // Smooth gradient
        LED,           // Segmented LED-style
        VU             // Classic VU-style
    };

    /**
     * @brief Per-channel meter state
     */
    struct ChannelMeter
    {
        float peakL{0.0f};
        float peakR{0.0f};
        float rmsL{0.0f};
        float rmsR{0.0f};
        float peakHoldL{0.0f};
        float peakHoldR{0.0f};
        float peakHoldTimerL{0.0f};
        float peakHoldTimerR{0.0f};
        bool clipL{false};
        bool clipR{false};
    };

    MeterRenderer() = default;

    /**
     * @brief Update meter values with smoothing
     * @param index Channel index
     * @param peakL Left peak (0-1)
     * @param peakR Right peak (0-1)
     * @param rmsL Left RMS (0-1)
     * @param rmsR Right RMS (0-1)
     * @param deltaTime Frame delta time
     */
    void updateMeter(std::size_t index, float peakL, float peakR,
                     float rmsL, float rmsR, float deltaTime);

    /**
     * @brief Render meter using ImDrawList
     */
    void renderImGui(
        void* drawList,  // ImDrawList*
        std::size_t index,
        float x, float y,
        float width, float height,
        bool stereo = true);

    /**
     * @brief Batch render multiple meters
     */
    void renderBatchImGui(
        void* drawList,
        float x, float y,
        float meterWidth, float meterHeight,
        float spacing,
        bool stereo = true);

    /**
     * @brief Get/set style
     */
    [[nodiscard]] Style getStyle() const { return style_; }
    void setStyle(Style style) { style_ = style; }

    /**
     * @brief Get meter count
     */
    [[nodiscard]] std::size_t getMeterCount() const { return meters_.size(); }

    /**
     * @brief Ensure meter exists
     */
    void ensureMeter(std::size_t index);

    /**
     * @brief Reset all meters
     */
    void resetAll();

    /**
     * @brief Smoothing settings
     */
    float peakFalloff{0.3f};      // Peak fall rate per second
    float rmsFalloff{0.5f};       // RMS fall rate per second
    float peakHoldTime{2.0f};     // Peak hold duration in seconds

private:
    void drawGradientMeter(void* drawList, const ChannelMeter& meter,
                           float x, float y, float width, float height, bool isRight);
    void drawLEDMeter(void* drawList, const ChannelMeter& meter,
                      float x, float y, float width, float height, bool isRight);
    void drawVUMeter(void* drawList, const ChannelMeter& meter,
                     float x, float y, float width, float height, bool isRight);

    std::vector<ChannelMeter> meters_;
    Style style_{Style::Gradient};
};

/**
 * @brief Icon atlas for efficient single-bind rendering
 */
class IconAtlas
{
public:
    /**
     * @brief Icon entry in atlas
     */
    struct IconEntry
    {
        std::string name;
        float u0, v0, u1, v1;  // UV coordinates
        float width, height;   // Original size
    };

    IconAtlas() = default;

    /**
     * @brief Build atlas from vector icons
     * @param iconPaths Map of icon name to SVG/vector path
     * @param size Target icon size
     * @return true if successful
     */
    bool build(const std::unordered_map<std::string, std::string>& iconPaths, int size = 32);

    /**
     * @brief Get texture ID for binding
     */
    [[nodiscard]] uint32_t getTextureId() const { return textureId_; }

    /**
     * @brief Get icon entry by name
     */
    [[nodiscard]] const IconEntry* getIcon(const std::string& name) const;

    /**
     * @brief Render icon using ImDrawList
     */
    void renderIcon(void* drawList, const std::string& name,
                    float x, float y, float size,
                    const float* tintColor = nullptr);

    /**
     * @brief Get atlas dimensions
     */
    [[nodiscard]] int getWidth() const { return width_; }
    [[nodiscard]] int getHeight() const { return height_; }

    /**
     * @brief Check if atlas is ready
     */
    [[nodiscard]] bool isReady() const { return ready_; }

private:
    uint32_t textureId_{0};
    int width_{0};
    int height_{0};
    bool ready_{false};
    std::unordered_map<std::string, IconEntry> icons_;
};

/**
 * @brief Global waveform renderer instance
 */
inline WaveformRenderer& getGlobalWaveformRenderer()
{
    static WaveformRenderer instance;
    return instance;
}

/**
 * @brief Global meter renderer instance
 */
inline MeterRenderer& getGlobalMeterRenderer()
{
    static MeterRenderer instance;
    return instance;
}

} // namespace daw::ui::render
