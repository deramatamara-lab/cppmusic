/**
 * @file FeatureExtractor.cpp
 * @brief Implementation of audio feature extraction.
 */

#include "FeatureExtractor.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace cppmusic::ai::tagging {

FeatureExtractor::FeatureExtractor() = default;
FeatureExtractor::~FeatureExtractor() = default;

void FeatureExtractor::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
}

void FeatureExtractor::setFFTSize(std::size_t fftSize) {
    fftSize_ = fftSize;
}

FeatureSet FeatureExtractor::extract(const float* samples, std::size_t numSamples) const {
    FeatureSet features;
    
    if (samples == nullptr || numSamples == 0) {
        return features;
    }
    
    // Basic temporal info
    features.duration = static_cast<float>(numSamples) / sampleRate_;
    
    // Compute all features
    features.spectralCentroid = computeSpectralCentroid(samples, numSamples);
    features.spectralRolloff = computeSpectralRolloff(samples, numSamples);
    features.spectralFlatness = computeSpectralFlatness(samples, numSamples);
    features.transientDensity = computeTransientDensity(samples, numSamples);
    features.rmsEnergy = computeRMSEnergy(samples, numSamples);
    features.zeroCrossingRate = computeZeroCrossingRate(samples, numSamples);
    features.mfcc = computeMFCC(samples, numSamples);
    
    return features;
}

FeatureSet FeatureExtractor::extractFromFile(const std::filesystem::path& /*path*/) const {
    // Placeholder: Would load audio file and extract features
    // Requires audio loading library (libsndfile, etc.)
    FeatureSet features;
    return features;
}

float FeatureExtractor::computeSpectralCentroid(const float* samples, std::size_t numSamples) const {
    if (numSamples == 0) return 0.0f;
    
    // Simplified spectral centroid using zero crossings as proxy
    // Real implementation would use FFT
    
    float weightedSum = 0.0f;
    float totalEnergy = 0.0f;
    
    // Estimate frequency content from sample variations
    for (std::size_t i = 1; i < numSamples; ++i) {
        float diff = std::abs(samples[i] - samples[i - 1]);
        float estimatedFreq = diff * sampleRate_ / 4.0f;  // Rough estimate
        float energy = samples[i] * samples[i];
        
        weightedSum += estimatedFreq * energy;
        totalEnergy += energy;
    }
    
    if (totalEnergy < 1e-10f) return 0.0f;
    return weightedSum / totalEnergy;
}

float FeatureExtractor::computeSpectralRolloff(const float* samples, std::size_t numSamples,
                                               float rolloffPercent) const {
    if (numSamples == 0) return 0.0f;
    
    // Simplified: Use sorted absolute values as proxy for spectral distribution
    std::vector<float> absValues(numSamples);
    for (std::size_t i = 0; i < numSamples; ++i) {
        absValues[i] = std::abs(samples[i]);
    }
    
    std::sort(absValues.begin(), absValues.end(), std::greater<float>());
    
    float totalEnergy = std::accumulate(absValues.begin(), absValues.end(), 0.0f);
    float threshold = totalEnergy * rolloffPercent;
    
    float cumulative = 0.0f;
    std::size_t rolloffIndex = 0;
    
    for (std::size_t i = 0; i < numSamples && cumulative < threshold; ++i) {
        cumulative += absValues[i];
        rolloffIndex = i;
    }
    
    // Convert index to frequency estimate
    return static_cast<float>(rolloffIndex) / static_cast<float>(numSamples) * sampleRate_ / 2.0f;
}

float FeatureExtractor::computeSpectralFlatness(const float* samples, std::size_t numSamples) const {
    if (numSamples == 0) return 0.0f;
    
    // Spectral flatness approximation using sample statistics
    // Real implementation would use FFT magnitude spectrum
    
    // Compute statistics of absolute values
    float sum = 0.0f;
    float logSum = 0.0f;
    int validCount = 0;
    
    for (std::size_t i = 0; i < numSamples; ++i) {
        float absVal = std::abs(samples[i]);
        if (absVal > 1e-10f) {
            sum += absVal;
            logSum += std::log(absVal);
            ++validCount;
        }
    }
    
    if (validCount == 0 || sum < 1e-10f) return 0.0f;
    
    float geometricMean = std::exp(logSum / static_cast<float>(validCount));
    float arithmeticMean = sum / static_cast<float>(validCount);
    
    return geometricMean / arithmeticMean;
}

float FeatureExtractor::computeTransientDensity(const float* samples, std::size_t numSamples) const {
    if (numSamples < 2) return 0.0f;
    
    // Count sudden increases in amplitude (transients)
    float threshold = computeRMSEnergy(samples, numSamples) * 2.0f;
    int transientCount = 0;
    
    // Use short-term energy change
    constexpr std::size_t windowSize = 128;
    float prevEnergy = 0.0f;
    
    for (std::size_t i = 0; i < numSamples; i += windowSize / 2) {
        std::size_t end = std::min(i + windowSize, numSamples);
        
        float energy = 0.0f;
        for (std::size_t j = i; j < end; ++j) {
            energy += samples[j] * samples[j];
        }
        energy /= static_cast<float>(end - i);
        
        if (energy > prevEnergy + threshold && prevEnergy > 1e-10f) {
            ++transientCount;
        }
        
        prevEnergy = energy;
    }
    
    float durationSeconds = static_cast<float>(numSamples) / sampleRate_;
    return static_cast<float>(transientCount) / std::max(durationSeconds, 0.001f);
}

float FeatureExtractor::computeRMSEnergy(const float* samples, std::size_t numSamples) const {
    if (numSamples == 0) return 0.0f;
    
    float sumSquared = 0.0f;
    for (std::size_t i = 0; i < numSamples; ++i) {
        sumSquared += samples[i] * samples[i];
    }
    
    return std::sqrt(sumSquared / static_cast<float>(numSamples));
}

float FeatureExtractor::computeZeroCrossingRate(const float* samples, std::size_t numSamples) const {
    if (numSamples < 2) return 0.0f;
    
    int crossings = 0;
    for (std::size_t i = 1; i < numSamples; ++i) {
        if ((samples[i] >= 0.0f && samples[i - 1] < 0.0f) ||
            (samples[i] < 0.0f && samples[i - 1] >= 0.0f)) {
            ++crossings;
        }
    }
    
    float durationSeconds = static_cast<float>(numSamples) / sampleRate_;
    return static_cast<float>(crossings) / durationSeconds;
}

std::array<float, 13> FeatureExtractor::computeMFCC(const float* samples, std::size_t numSamples) const {
    std::array<float, 13> mfcc{};
    
    if (numSamples == 0) return mfcc;
    
    // Placeholder: Simple statistical features as MFCC proxy
    // Real implementation would use FFT -> Mel filterbank -> DCT
    
    // Compute moments of the signal
    float sum = 0.0f;
    float sumSq = 0.0f;
    float sumCube = 0.0f;
    float sumQuad = 0.0f;
    
    for (std::size_t i = 0; i < numSamples; ++i) {
        float val = samples[i];
        sum += val;
        sumSq += val * val;
        sumCube += val * val * val;
        sumQuad += val * val * val * val;
    }
    
    float n = static_cast<float>(numSamples);
    float mean = sum / n;
    float variance = (sumSq / n) - (mean * mean);
    float stdDev = std::sqrt(std::max(0.0f, variance));
    
    // Fill MFCC array with pseudo-features
    mfcc[0] = mean;
    mfcc[1] = stdDev;
    mfcc[2] = variance > 1e-10f ? (sumCube / n - 3 * mean * variance - mean * mean * mean) / 
                                  (variance * stdDev) : 0.0f;  // Skewness
    mfcc[3] = variance > 1e-10f ? (sumQuad / n) / (variance * variance) - 3.0f : 0.0f;  // Kurtosis
    mfcc[4] = computeZeroCrossingRate(samples, numSamples) / 10000.0f;
    mfcc[5] = computeRMSEnergy(samples, numSamples);
    mfcc[6] = computeSpectralCentroid(samples, numSamples) / 10000.0f;
    
    // Fill remaining with variations
    for (std::size_t i = 7; i < 13; ++i) {
        mfcc[i] = mfcc[i - 7] * 0.5f;  // Placeholder
    }
    
    return mfcc;
}

} // namespace cppmusic::ai::tagging
