/**
 * @file GpuFFTNode.cpp
 * @brief GPU-accelerated FFT node implementation (stub)
 */

#include "GpuFFTNode.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace cppmusic::dsp::nodes {

struct GpuFFTNode::Impl {
    GpuFFTConfig config;
    std::vector<float> window;
    std::vector<float> overlapBuffer;
    bool gpuAvailable{false};
    
    void buildWindow() {
        uint32_t size = static_cast<uint32_t>(config.fftSize);
        window.resize(size);
        
        switch (config.windowType) {
            case WindowType::Rectangular:
                std::fill(window.begin(), window.end(), 1.0f);
                break;
                
            case WindowType::Hann:
                for (uint32_t i = 0; i < size; ++i) {
                    window[i] = 0.5f * (1.0f - std::cos(2.0f * std::numbers::pi_v<float> * i / (size - 1)));
                }
                break;
                
            case WindowType::Hamming:
                for (uint32_t i = 0; i < size; ++i) {
                    window[i] = 0.54f - 0.46f * std::cos(2.0f * std::numbers::pi_v<float> * i / (size - 1));
                }
                break;
                
            case WindowType::Blackman:
                for (uint32_t i = 0; i < size; ++i) {
                    float t = 2.0f * std::numbers::pi_v<float> * i / (size - 1);
                    window[i] = 0.42f - 0.5f * std::cos(t) + 0.08f * std::cos(2.0f * t);
                }
                break;
                
            case WindowType::BlackmanHarris:
                for (uint32_t i = 0; i < size; ++i) {
                    float t = 2.0f * std::numbers::pi_v<float> * i / (size - 1);
                    window[i] = 0.35875f - 0.48829f * std::cos(t) + 
                                0.14128f * std::cos(2.0f * t) - 0.01168f * std::cos(3.0f * t);
                }
                break;
                
            case WindowType::Kaiser:
                buildKaiserWindow(size, config.kaiserBeta);
                break;
        }
    }
    
    void buildKaiserWindow(uint32_t size, float beta) {
        // Simplified Kaiser window using approximation
        // In production, use proper Bessel function
        for (uint32_t i = 0; i < size; ++i) {
            float x = 2.0f * i / (size - 1) - 1.0f;
            float arg = beta * std::sqrt(1.0f - x * x);
            // Approximate I0(x) using series expansion
            float i0 = 1.0f;
            float term = 1.0f;
            for (int k = 1; k <= 10; ++k) {
                term *= (arg / (2.0f * k)) * (arg / (2.0f * k));
                i0 += term;
            }
            float i0_beta = 1.0f;
            term = 1.0f;
            for (int k = 1; k <= 10; ++k) {
                term *= (beta / (2.0f * k)) * (beta / (2.0f * k));
                i0_beta += term;
            }
            window[i] = i0 / i0_beta;
        }
    }
    
    // Stub FFT implementation (DFT for correctness, not performance)
    void dft(std::span<const float> input, std::span<Complex> output) {
        size_t N = input.size();
        size_t numBins = N / 2 + 1;
        
        for (size_t k = 0; k < numBins && k < output.size(); ++k) {
            Complex sum{0.0f, 0.0f};
            for (size_t n = 0; n < N; ++n) {
                float angle = -2.0f * std::numbers::pi_v<float> * k * n / N;
                sum += input[n] * Complex(std::cos(angle), std::sin(angle));
            }
            output[k] = sum;
        }
    }
    
    void idft(std::span<const Complex> input, std::span<float> output) {
        size_t N = output.size();
        size_t numBins = input.size();
        
        for (size_t n = 0; n < N; ++n) {
            float sum = 0.0f;
            for (size_t k = 0; k < numBins; ++k) {
                float angle = 2.0f * std::numbers::pi_v<float> * k * n / N;
                Complex w(std::cos(angle), std::sin(angle));
                Complex val = input[k] * w;
                
                if (k == 0 || k == N / 2) {
                    sum += val.real();
                } else {
                    sum += 2.0f * val.real();
                }
            }
            output[n] = sum / N;
        }
    }
};

GpuFFTNode::GpuFFTNode(const GpuFFTConfig& config)
    : impl_(std::make_unique<Impl>()) {
    configure(config);
}

GpuFFTNode::~GpuFFTNode() = default;

GpuFFTNode::GpuFFTNode(GpuFFTNode&&) noexcept = default;
GpuFFTNode& GpuFFTNode::operator=(GpuFFTNode&&) noexcept = default;

void GpuFFTNode::configure(const GpuFFTConfig& config) {
    impl_->config = config;
    impl_->buildWindow();
    
    uint32_t size = static_cast<uint32_t>(config.fftSize);
    impl_->overlapBuffer.resize(size, 0.0f);
    
#ifdef ENABLE_GPU
    // TODO: Initialize GPU FFT plan (cuFFT or Vulkan compute)
    impl_->gpuAvailable = config.useGpuIfAvailable;
#endif
}

const GpuFFTConfig& GpuFFTNode::getConfig() const {
    return impl_->config;
}

uint32_t GpuFFTNode::getFFTSize() const {
    return static_cast<uint32_t>(impl_->config.fftSize);
}

void GpuFFTNode::forward(std::span<const float> input, std::span<Complex> output) {
    uint32_t fftSize = getFFTSize();
    if (input.size() != fftSize) {
        throw std::invalid_argument("Input size must match FFT size");
    }
    
    size_t expectedOutput = fftSize / 2 + 1;
    if (output.size() < expectedOutput) {
        throw std::invalid_argument("Output buffer too small");
    }
    
#ifdef ENABLE_GPU
    if (impl_->gpuAvailable) {
        // TODO: GPU implementation
        // For now, fall through to CPU
    }
#endif
    
    impl_->dft(input, output);
}

void GpuFFTNode::inverse(std::span<const Complex> input, std::span<float> output) {
    uint32_t fftSize = getFFTSize();
    
    size_t expectedInput = fftSize / 2 + 1;
    if (input.size() != expectedInput) {
        throw std::invalid_argument("Input size must be FFT_SIZE/2 + 1");
    }
    
    if (output.size() != fftSize) {
        throw std::invalid_argument("Output size must match FFT size");
    }
    
#ifdef ENABLE_GPU
    if (impl_->gpuAvailable) {
        // TODO: GPU implementation
    }
#endif
    
    impl_->idft(input, output);
}

void GpuFFTNode::applyWindow(std::span<float> samples) {
    if (samples.size() != impl_->window.size()) {
        throw std::invalid_argument("Sample size must match window size");
    }
    
    for (size_t i = 0; i < samples.size(); ++i) {
        samples[i] *= impl_->window[i];
    }
}

void GpuFFTNode::analyzeFrame(std::span<const float> input, std::span<Complex> output) {
    uint32_t fftSize = getFFTSize();
    
    // Apply window to a copy
    std::vector<float> windowed(input.begin(), input.end());
    windowed.resize(fftSize, 0.0f);
    applyWindow(windowed);
    
    forward(windowed, output);
}

void GpuFFTNode::synthesizeFrame(std::span<const Complex> input, std::span<float> output) {
    uint32_t fftSize = getFFTSize();
    
    std::vector<float> frame(fftSize);
    inverse(input, frame);
    
    // Overlap-add
    uint32_t hopSize = impl_->config.hopSize;
    auto& overlap = impl_->overlapBuffer;
    
    // Copy overlap to output
    size_t copySize = std::min(static_cast<size_t>(hopSize), output.size());
    for (size_t i = 0; i < copySize; ++i) {
        output[i] = overlap[i] + frame[i];
    }
    
    // Shift overlap buffer and add remaining frame
    std::copy(overlap.begin() + hopSize, overlap.end(), overlap.begin());
    std::fill(overlap.end() - hopSize, overlap.end(), 0.0f);
    
    for (size_t i = hopSize; i < fftSize; ++i) {
        overlap[i - hopSize] += frame[i];
    }
}

void GpuFFTNode::forwardBatch(const std::vector<std::span<const float>>& inputs,
                               std::vector<std::span<Complex>>& outputs) {
    // Stub: Process sequentially
    // In production, batch on GPU for efficiency
    for (size_t i = 0; i < inputs.size() && i < outputs.size(); ++i) {
        forward(inputs[i], outputs[i]);
    }
}

bool GpuFFTNode::isUsingGpu() const {
#ifdef ENABLE_GPU
    return impl_->gpuAvailable;
#else
    return false;
#endif
}

void GpuFFTNode::getMagnitudes(std::span<const Complex> spectrum,
                                std::span<float> magnitudes) {
    size_t count = std::min(spectrum.size(), magnitudes.size());
    for (size_t i = 0; i < count; ++i) {
        magnitudes[i] = std::abs(spectrum[i]);
    }
}

void GpuFFTNode::getPhases(std::span<const Complex> spectrum,
                            std::span<float> phases) {
    size_t count = std::min(spectrum.size(), phases.size());
    for (size_t i = 0; i < count; ++i) {
        phases[i] = std::arg(spectrum[i]);
    }
}

void GpuFFTNode::fromMagnitudesAndPhases(std::span<const float> magnitudes,
                                          std::span<const float> phases,
                                          std::span<Complex> spectrum) {
    size_t count = std::min({magnitudes.size(), phases.size(), spectrum.size()});
    for (size_t i = 0; i < count; ++i) {
        spectrum[i] = std::polar(magnitudes[i], phases[i]);
    }
}

}  // namespace cppmusic::dsp::nodes
