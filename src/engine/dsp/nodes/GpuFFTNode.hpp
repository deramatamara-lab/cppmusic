/**
 * @file GpuFFTNode.hpp
 * @brief GPU-accelerated FFT processing node (stub)
 *
 * This node provides FFT/IFFT operations that can optionally be
 * offloaded to the GPU when ENABLE_GPU is defined.
 */

#pragma once

#include <complex>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace cppmusic::dsp::nodes {

/**
 * @brief FFT size options
 */
enum class FFTSize : uint32_t {
    FFT_64 = 64,
    FFT_128 = 128,
    FFT_256 = 256,
    FFT_512 = 512,
    FFT_1024 = 1024,
    FFT_2048 = 2048,
    FFT_4096 = 4096,
    FFT_8192 = 8192
};

/**
 * @brief FFT window types
 */
enum class WindowType {
    Rectangular,
    Hann,
    Hamming,
    Blackman,
    BlackmanHarris,
    Kaiser
};

/**
 * @brief Configuration for the FFT node
 */
struct GpuFFTConfig {
    FFTSize fftSize{FFTSize::FFT_1024};
    WindowType windowType{WindowType::Hann};
    uint32_t hopSize{256};  // Overlap
    bool useGpuIfAvailable{true};
    float kaiserBeta{8.0f};  // For Kaiser window
};

/**
 * @brief GPU-accelerated FFT processing node
 *
 * This node provides:
 * - Forward FFT (time domain → frequency domain)
 * - Inverse FFT (frequency domain → time domain)
 * - Windowed STFT analysis
 * - Overlap-add synthesis
 *
 * When ENABLE_GPU is defined and GPU is available, operations
 * are offloaded to the GPU for large batch sizes.
 */
class GpuFFTNode {
public:
    using Complex = std::complex<float>;

    explicit GpuFFTNode(const GpuFFTConfig& config = {});
    ~GpuFFTNode();

    // Non-copyable, moveable
    GpuFFTNode(const GpuFFTNode&) = delete;
    GpuFFTNode& operator=(const GpuFFTNode&) = delete;
    GpuFFTNode(GpuFFTNode&&) noexcept;
    GpuFFTNode& operator=(GpuFFTNode&&) noexcept;

    /**
     * @brief Reconfigure the FFT node
     */
    void configure(const GpuFFTConfig& config);

    /**
     * @brief Get current configuration
     */
    [[nodiscard]] const GpuFFTConfig& getConfig() const;

    /**
     * @brief Get actual FFT size
     */
    [[nodiscard]] uint32_t getFFTSize() const;

    /**
     * @brief Perform forward FFT
     * @param input Time-domain samples (size must match fftSize)
     * @param output Frequency-domain complex values (size = fftSize/2 + 1)
     */
    void forward(std::span<const float> input, std::span<Complex> output);

    /**
     * @brief Perform inverse FFT
     * @param input Frequency-domain complex values (size = fftSize/2 + 1)
     * @param output Time-domain samples (size = fftSize)
     */
    void inverse(std::span<const Complex> input, std::span<float> output);

    /**
     * @brief Apply window function to samples
     * @param samples Samples to window (in-place)
     */
    void applyWindow(std::span<float> samples);

    /**
     * @brief Process STFT frame (window + forward FFT)
     * @param input Input samples
     * @param output Complex frequency bins
     */
    void analyzeFrame(std::span<const float> input, std::span<Complex> output);

    /**
     * @brief Synthesize from STFT frame (inverse FFT + overlap-add)
     * @param input Complex frequency bins
     * @param output Output samples (accumulated via overlap-add)
     */
    void synthesizeFrame(std::span<const Complex> input, std::span<float> output);

    /**
     * @brief Batch forward FFT for multiple frames
     * @param inputs Vector of input frames
     * @param outputs Vector of output frames
     */
    void forwardBatch(const std::vector<std::span<const float>>& inputs,
                      std::vector<std::span<Complex>>& outputs);

    /**
     * @brief Check if GPU acceleration is being used
     */
    [[nodiscard]] bool isUsingGpu() const;

    /**
     * @brief Get magnitudes from complex spectrum
     */
    static void getMagnitudes(std::span<const Complex> spectrum,
                              std::span<float> magnitudes);

    /**
     * @brief Get phases from complex spectrum
     */
    static void getPhases(std::span<const Complex> spectrum,
                          std::span<float> phases);

    /**
     * @brief Construct complex spectrum from magnitudes and phases
     */
    static void fromMagnitudesAndPhases(std::span<const float> magnitudes,
                                        std::span<const float> phases,
                                        std::span<Complex> spectrum);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace cppmusic::dsp::nodes
