/**
 * @file ConvolutionNode.hpp
 * @brief Convolution DSP node for impulse response processing
 *
 * Supports loading impulse responses and performing convolution
 * for reverb and other IR-based effects. Can optionally use GPU
 * acceleration for large impulse responses.
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace cppmusic::dsp::nodes {

/**
 * @brief Convolution algorithm selection
 */
enum class ConvolutionMethod {
    Auto,           // Choose based on IR length
    TimeDomain,     // Direct convolution (small IRs)
    FFT,            // Overlap-save FFT convolution
    Partitioned,    // Non-uniform partitioned convolution (real-time friendly)
    GpuFFT          // GPU-accelerated FFT convolution
};

/**
 * @brief Configuration for the convolution node
 */
struct ConvolutionConfig {
    ConvolutionMethod method{ConvolutionMethod::Auto};
    uint32_t blockSize{512};         // Processing block size
    uint32_t partitionSize{4096};    // Partition size for partitioned convolution
    float wetDryMix{1.0f};           // 0.0 = dry, 1.0 = wet
    float irGain{1.0f};              // IR amplitude scaling
    bool normalize{true};            // Normalize IR on load
    bool stereoToMono{false};        // Convert stereo IR to mono
};

/**
 * @brief Information about a loaded impulse response
 */
struct ImpulseResponseInfo {
    std::string name;
    std::filesystem::path path;
    uint32_t sampleRate{44100};
    uint32_t channels{1};
    uint64_t lengthSamples{0};
    float durationMs{0.0f};
    float peakValue{0.0f};
    bool normalized{false};
};

/**
 * @brief Convolution DSP node
 *
 * This node implements convolution processing for impulse response
 * based effects like reverb, cabinet simulation, etc.
 *
 * Features:
 * - Multiple convolution algorithms
 * - Automatic algorithm selection based on IR length
 * - IR loading from file or memory
 * - Wet/dry mixing
 * - Optional GPU acceleration
 * - Zero-latency mode (partitioned convolution)
 */
class ConvolutionNode {
public:
    explicit ConvolutionNode(const ConvolutionConfig& config = {});
    ~ConvolutionNode();

    // Non-copyable, moveable
    ConvolutionNode(const ConvolutionNode&) = delete;
    ConvolutionNode& operator=(const ConvolutionNode&) = delete;
    ConvolutionNode(ConvolutionNode&&) noexcept;
    ConvolutionNode& operator=(ConvolutionNode&&) noexcept;

    /**
     * @brief Configure the convolution node
     */
    void configure(const ConvolutionConfig& config);

    /**
     * @brief Get current configuration
     */
    [[nodiscard]] const ConvolutionConfig& getConfig() const;

    /**
     * @brief Load impulse response from file
     * @param path Path to audio file (WAV, AIFF, FLAC)
     * @param targetSampleRate Target sample rate (will resample if different)
     * @return true if loaded successfully
     */
    bool loadIR(const std::filesystem::path& path, uint32_t targetSampleRate = 0);

    /**
     * @brief Load impulse response from memory
     * @param samples IR samples (interleaved if stereo)
     * @param channels Number of channels
     * @param sampleRate Sample rate of the IR
     * @param name Optional name for the IR
     * @return true if loaded successfully
     */
    bool loadIR(std::span<const float> samples, uint32_t channels,
                uint32_t sampleRate, const std::string& name = "");

    /**
     * @brief Unload the current impulse response
     */
    void unloadIR();

    /**
     * @brief Check if an IR is loaded
     */
    [[nodiscard]] bool isIRLoaded() const;

    /**
     * @brief Get information about the loaded IR
     */
    [[nodiscard]] const ImpulseResponseInfo& getIRInfo() const;

    /**
     * @brief Process audio (mono)
     * @param input Input samples
     * @param output Output samples (must be same size as input)
     */
    void process(std::span<const float> input, std::span<float> output);

    /**
     * @brief Process audio (stereo interleaved)
     * @param input Input samples (interleaved L/R)
     * @param output Output samples (interleaved L/R)
     */
    void processStereo(std::span<const float> input, std::span<float> output);

    /**
     * @brief Reset internal state (clear delay lines)
     */
    void reset();

    /**
     * @brief Get the latency introduced by the convolution
     * @return Latency in samples
     */
    [[nodiscard]] uint32_t getLatency() const;

    /**
     * @brief Get the currently selected convolution method
     */
    [[nodiscard]] ConvolutionMethod getActiveMethod() const;

    /**
     * @brief Check if GPU acceleration is being used
     */
    [[nodiscard]] bool isUsingGpu() const;

    /**
     * @brief Set wet/dry mix
     * @param mix Mix value (0.0 = dry, 1.0 = wet)
     */
    void setWetDryMix(float mix);

    /**
     * @brief Set IR gain
     * @param gain Gain value (linear)
     */
    void setIRGain(float gain);

    /**
     * @brief Get tail length (time for IR to decay to -60dB)
     * @return Tail length in samples
     */
    [[nodiscard]] uint64_t getTailLength() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace cppmusic::dsp::nodes
