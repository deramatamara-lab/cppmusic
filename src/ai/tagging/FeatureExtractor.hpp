#pragma once
/**
 * @file FeatureExtractor.hpp
 * @brief Audio feature extraction for asset tagging.
 */

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace cppmusic::ai::tagging {

/**
 * @brief Extracted audio features for classification.
 */
struct FeatureSet {
    // Spectral features
    float spectralCentroid = 0.0f;    ///< Brightness (Hz)
    float spectralRolloff = 0.0f;     ///< High-frequency cutoff
    float spectralFlux = 0.0f;        ///< Rate of change
    float spectralFlatness = 0.0f;    ///< Noise vs tonal (0-1)
    
    // Temporal features
    float transientDensity = 0.0f;    ///< Attacks per second
    float rmsEnergy = 0.0f;           ///< Overall loudness
    float zeroCrossingRate = 0.0f;    ///< Noise indicator
    float duration = 0.0f;            ///< Length in seconds
    
    // Timbral features
    std::array<float, 13> mfcc{};     ///< Mel-frequency cepstral coefficients
    
    // Rhythm features (optional)
    std::optional<float> tempo;
    std::optional<float> rhythmStrength;
    
    /**
     * @brief Check if features are valid (not default).
     */
    [[nodiscard]] bool isValid() const noexcept {
        return duration > 0.0f;
    }
};

/**
 * @brief Extracts audio features from samples for classification.
 */
class FeatureExtractor {
public:
    FeatureExtractor();
    ~FeatureExtractor();
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Set the sample rate for analysis.
     */
    void setSampleRate(float sampleRate);
    
    /**
     * @brief Get the current sample rate.
     */
    [[nodiscard]] float getSampleRate() const noexcept { return sampleRate_; }
    
    /**
     * @brief Set FFT size for spectral analysis.
     */
    void setFFTSize(std::size_t fftSize);
    
    /**
     * @brief Get the current FFT size.
     */
    [[nodiscard]] std::size_t getFFTSize() const noexcept { return fftSize_; }
    
    // =========================================================================
    // Feature Extraction
    // =========================================================================
    
    /**
     * @brief Extract all features from audio samples.
     * @param samples Mono audio samples.
     * @param numSamples Number of samples.
     */
    [[nodiscard]] FeatureSet extract(const float* samples, std::size_t numSamples) const;
    
    /**
     * @brief Extract features from audio file (placeholder).
     * @param path Path to audio file.
     */
    [[nodiscard]] FeatureSet extractFromFile(const std::filesystem::path& path) const;
    
    // =========================================================================
    // Individual Feature Extractors
    // =========================================================================
    
    /**
     * @brief Compute spectral centroid (center of mass of spectrum).
     */
    [[nodiscard]] float computeSpectralCentroid(const float* samples, std::size_t numSamples) const;
    
    /**
     * @brief Compute spectral rolloff (frequency below which N% of energy exists).
     * @param rolloffPercent Percentage threshold (default 85%).
     */
    [[nodiscard]] float computeSpectralRolloff(const float* samples, std::size_t numSamples,
                                               float rolloffPercent = 0.85f) const;
    
    /**
     * @brief Compute spectral flatness (noise vs tonal).
     */
    [[nodiscard]] float computeSpectralFlatness(const float* samples, std::size_t numSamples) const;
    
    /**
     * @brief Compute transient density (attacks per second).
     */
    [[nodiscard]] float computeTransientDensity(const float* samples, std::size_t numSamples) const;
    
    /**
     * @brief Compute RMS energy.
     */
    [[nodiscard]] float computeRMSEnergy(const float* samples, std::size_t numSamples) const;
    
    /**
     * @brief Compute zero crossing rate.
     */
    [[nodiscard]] float computeZeroCrossingRate(const float* samples, std::size_t numSamples) const;
    
    /**
     * @brief Compute Mel-frequency cepstral coefficients.
     */
    [[nodiscard]] std::array<float, 13> computeMFCC(const float* samples, std::size_t numSamples) const;
    
private:
    float sampleRate_ = 22050.0f;
    std::size_t fftSize_ = 2048;
    std::size_t hopSize_ = 512;
};

} // namespace cppmusic::ai::tagging
