/**
 * @file ConvolutionNode.cpp
 * @brief Convolution node implementation (stub with basic functionality)
 */

#include "ConvolutionNode.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace cppmusic::dsp::nodes {

struct ConvolutionNode::Impl {
    ConvolutionConfig config;
    ImpulseResponseInfo irInfo;
    std::vector<float> ir;           // Impulse response (mono)
    std::vector<float> irStereo[2];  // Stereo IR (L/R)
    std::vector<float> inputBuffer;  // Input history for convolution
    std::vector<float> inputBufferStereo[2];
    size_t inputPos{0};
    bool irLoaded{false};
    ConvolutionMethod activeMethod{ConvolutionMethod::TimeDomain};
    bool usingGpu{false};
    
    void selectMethod() {
        if (config.method != ConvolutionMethod::Auto) {
            activeMethod = config.method;
            return;
        }
        
        // Auto-select based on IR length
        size_t irLength = ir.size();
        
        if (irLength <= 64) {
            activeMethod = ConvolutionMethod::TimeDomain;
        } else if (irLength <= 4096) {
            activeMethod = ConvolutionMethod::FFT;
        } else {
            activeMethod = ConvolutionMethod::Partitioned;
        }
        
#ifdef ENABLE_GPU
        // Use GPU for very large IRs
        if (irLength > 16384 && config.method == ConvolutionMethod::Auto) {
            activeMethod = ConvolutionMethod::GpuFFT;
            usingGpu = true;
        }
#endif
    }
    
    void normalizeIR() {
        if (ir.empty()) return;
        
        float maxVal = 0.0f;
        for (float sample : ir) {
            maxVal = std::max(maxVal, std::abs(sample));
        }
        
        irInfo.peakValue = maxVal;
        
        if (config.normalize && maxVal > 0.0f) {
            float scale = 1.0f / maxVal;
            for (float& sample : ir) {
                sample *= scale;
            }
            irInfo.normalized = true;
        }
        
        // Apply the same normalization to stereo
        for (auto& ch : irStereo) {
            if (!ch.empty() && config.normalize && maxVal > 0.0f) {
                float scale = 1.0f / maxVal;
                for (float& sample : ch) {
                    sample *= scale;
                }
            }
        }
    }
    
    // Simple time-domain convolution (stub)
    void convolveTimeDomain(std::span<const float> input, std::span<float> output) {
        size_t inputLen = input.size();
        size_t irLen = ir.size();
        
        // Ensure input buffer is large enough
        if (inputBuffer.size() < irLen + inputLen) {
            inputBuffer.resize(irLen + inputLen, 0.0f);
        }
        
        // Add new input to buffer
        for (size_t i = 0; i < inputLen; ++i) {
            inputBuffer[inputPos + i] = input[i];
        }
        
        // Perform convolution
        for (size_t n = 0; n < inputLen; ++n) {
            float sum = 0.0f;
            for (size_t k = 0; k < irLen; ++k) {
                size_t idx = inputPos + n - k;
                if (idx < inputBuffer.size()) {
                    sum += inputBuffer[idx] * ir[k];
                }
            }
            
            // Apply wet/dry mix
            float dry = input[n];
            float wet = sum * config.irGain;
            output[n] = dry * (1.0f - config.wetDryMix) + wet * config.wetDryMix;
        }
        
        // Update position and shift buffer if needed
        inputPos += inputLen;
        if (inputPos >= irLen) {
            // Shift buffer
            std::copy(inputBuffer.begin() + inputLen, inputBuffer.end(),
                      inputBuffer.begin());
            inputPos -= inputLen;
        }
    }
    
    // Stub for FFT-based convolution
    void convolveFFT(std::span<const float> input, std::span<float> output) {
        // Stub: Fall back to time domain for now
        // In production: Implement overlap-save FFT convolution
        convolveTimeDomain(input, output);
    }
    
    // Stub for partitioned convolution
    void convolvePartitioned(std::span<const float> input, std::span<float> output) {
        // Stub: Fall back to time domain for now
        // In production: Implement non-uniform partitioned convolution
        convolveTimeDomain(input, output);
    }
    
    // Stub for GPU convolution
    void convolveGpu(std::span<const float> input, std::span<float> output) {
        // Stub: Fall back to CPU for now
        convolvePartitioned(input, output);
    }
};

ConvolutionNode::ConvolutionNode(const ConvolutionConfig& config)
    : impl_(std::make_unique<Impl>()) {
    configure(config);
}

ConvolutionNode::~ConvolutionNode() = default;

ConvolutionNode::ConvolutionNode(ConvolutionNode&&) noexcept = default;
ConvolutionNode& ConvolutionNode::operator=(ConvolutionNode&&) noexcept = default;

void ConvolutionNode::configure(const ConvolutionConfig& config) {
    impl_->config = config;
    if (impl_->irLoaded) {
        impl_->selectMethod();
    }
}

const ConvolutionConfig& ConvolutionNode::getConfig() const {
    return impl_->config;
}

bool ConvolutionNode::loadIR(const std::filesystem::path& path, 
                              uint32_t targetSampleRate) {
    // Stub: In production, use libsndfile or similar to load audio
    // For now, just record the path
    
    impl_->irInfo.path = path;
    impl_->irInfo.name = path.stem().string();
    
    // Create a simple stub IR (short impulse)
    std::vector<float> stubIR(1024, 0.0f);
    stubIR[0] = 1.0f;  // Delta function
    
    // Add some simple decay
    for (size_t i = 1; i < stubIR.size(); ++i) {
        stubIR[i] = stubIR[i-1] * 0.995f * (((i % 7) < 3) ? 0.1f : -0.05f);
    }
    
    return loadIR(stubIR, 1, targetSampleRate ? targetSampleRate : 44100, 
                  impl_->irInfo.name);
}

bool ConvolutionNode::loadIR(std::span<const float> samples, uint32_t channels,
                              uint32_t sampleRate, const std::string& name) {
    if (samples.empty()) {
        return false;
    }
    
    impl_->irInfo.name = name;
    impl_->irInfo.sampleRate = sampleRate;
    impl_->irInfo.channels = channels;
    impl_->irInfo.lengthSamples = samples.size() / channels;
    impl_->irInfo.durationMs = static_cast<float>(impl_->irInfo.lengthSamples) / 
                                sampleRate * 1000.0f;
    
    if (channels == 1 || impl_->config.stereoToMono) {
        // Mono or convert to mono
        impl_->ir.resize(impl_->irInfo.lengthSamples);
        
        if (channels == 1) {
            std::copy(samples.begin(), samples.end(), impl_->ir.begin());
        } else {
            // Average channels
            for (size_t i = 0; i < impl_->irInfo.lengthSamples; ++i) {
                float sum = 0.0f;
                for (uint32_t c = 0; c < channels; ++c) {
                    sum += samples[i * channels + c];
                }
                impl_->ir[i] = sum / channels;
            }
        }
    } else if (channels >= 2) {
        // Keep stereo
        impl_->irStereo[0].resize(impl_->irInfo.lengthSamples);
        impl_->irStereo[1].resize(impl_->irInfo.lengthSamples);
        impl_->ir.resize(impl_->irInfo.lengthSamples);  // Also keep mono version
        
        for (size_t i = 0; i < impl_->irInfo.lengthSamples; ++i) {
            impl_->irStereo[0][i] = samples[i * channels];
            impl_->irStereo[1][i] = samples[i * channels + 1];
            impl_->ir[i] = (impl_->irStereo[0][i] + impl_->irStereo[1][i]) * 0.5f;
        }
    }
    
    impl_->normalizeIR();
    impl_->selectMethod();
    
    // Resize input buffers
    size_t bufferSize = impl_->ir.size() + impl_->config.blockSize;
    impl_->inputBuffer.resize(bufferSize, 0.0f);
    impl_->inputBufferStereo[0].resize(bufferSize, 0.0f);
    impl_->inputBufferStereo[1].resize(bufferSize, 0.0f);
    impl_->inputPos = 0;
    
    impl_->irLoaded = true;
    return true;
}

void ConvolutionNode::unloadIR() {
    impl_->ir.clear();
    impl_->irStereo[0].clear();
    impl_->irStereo[1].clear();
    impl_->inputBuffer.clear();
    impl_->inputBufferStereo[0].clear();
    impl_->inputBufferStereo[1].clear();
    impl_->irInfo = ImpulseResponseInfo{};
    impl_->irLoaded = false;
}

bool ConvolutionNode::isIRLoaded() const {
    return impl_->irLoaded;
}

const ImpulseResponseInfo& ConvolutionNode::getIRInfo() const {
    return impl_->irInfo;
}

void ConvolutionNode::process(std::span<const float> input, std::span<float> output) {
    if (!impl_->irLoaded) {
        // Pass through
        std::copy(input.begin(), 
                  input.begin() + std::min(input.size(), output.size()),
                  output.begin());
        return;
    }
    
    switch (impl_->activeMethod) {
        case ConvolutionMethod::TimeDomain:
            impl_->convolveTimeDomain(input, output);
            break;
        case ConvolutionMethod::FFT:
            impl_->convolveFFT(input, output);
            break;
        case ConvolutionMethod::Partitioned:
            impl_->convolvePartitioned(input, output);
            break;
        case ConvolutionMethod::GpuFFT:
            impl_->convolveGpu(input, output);
            break;
        case ConvolutionMethod::Auto:
            impl_->convolveTimeDomain(input, output);
            break;
    }
}

void ConvolutionNode::processStereo(std::span<const float> input, 
                                     std::span<float> output) {
    // Stub: Process as mono for now
    // In production: Implement true stereo convolution
    
    size_t numSamples = input.size() / 2;
    std::vector<float> monoIn(numSamples);
    std::vector<float> monoOut(numSamples);
    
    // Convert to mono
    for (size_t i = 0; i < numSamples; ++i) {
        monoIn[i] = (input[i * 2] + input[i * 2 + 1]) * 0.5f;
    }
    
    process(monoIn, monoOut);
    
    // Convert back to stereo
    for (size_t i = 0; i < numSamples && i * 2 + 1 < output.size(); ++i) {
        output[i * 2] = monoOut[i];
        output[i * 2 + 1] = monoOut[i];
    }
}

void ConvolutionNode::reset() {
    std::fill(impl_->inputBuffer.begin(), impl_->inputBuffer.end(), 0.0f);
    std::fill(impl_->inputBufferStereo[0].begin(), 
              impl_->inputBufferStereo[0].end(), 0.0f);
    std::fill(impl_->inputBufferStereo[1].begin(), 
              impl_->inputBufferStereo[1].end(), 0.0f);
    impl_->inputPos = 0;
}

uint32_t ConvolutionNode::getLatency() const {
    switch (impl_->activeMethod) {
        case ConvolutionMethod::TimeDomain:
            return 0;
        case ConvolutionMethod::FFT:
            return impl_->config.blockSize;
        case ConvolutionMethod::Partitioned:
            return impl_->config.blockSize;
        case ConvolutionMethod::GpuFFT:
            return impl_->config.blockSize * 2;  // Extra latency for GPU transfer
        default:
            return 0;
    }
}

ConvolutionMethod ConvolutionNode::getActiveMethod() const {
    return impl_->activeMethod;
}

bool ConvolutionNode::isUsingGpu() const {
    return impl_->usingGpu;
}

void ConvolutionNode::setWetDryMix(float mix) {
    impl_->config.wetDryMix = std::clamp(mix, 0.0f, 1.0f);
}

void ConvolutionNode::setIRGain(float gain) {
    impl_->config.irGain = std::max(0.0f, gain);
}

uint64_t ConvolutionNode::getTailLength() const {
    if (!impl_->irLoaded) {
        return 0;
    }
    
    // Find -60dB point in IR
    float threshold = impl_->irInfo.peakValue * 0.001f;  // -60dB
    
    for (size_t i = impl_->ir.size(); i > 0; --i) {
        if (std::abs(impl_->ir[i - 1]) > threshold) {
            return i;
        }
    }
    
    return impl_->ir.size();
}

}  // namespace cppmusic::dsp::nodes
