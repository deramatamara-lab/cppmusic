#include "MockImplementations.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <thread>
#include <filesystem>
#include <fstream>

namespace daw::core::testing {

MockAudioBufferGenerator::MockAudioBufferGenerator(const Config& config)
    : config_(config)
    , phase_(0.0)
    , noiseState_(config.seed)
{}

void MockAudioBufferGenerator::generate(std::vector<float>& buffer)
{
    buffer.resize(config_.numSamples);

    switch (config_.type)
    {
        case Silence:       generateSilence(buffer); break;
        case SineWave:      generateSineWave(buffer); break;
        case SquareWave:    generateSquareWave(buffer); break;
        case SawtoothWave:  generateSawtoothWave(buffer); break;
        case TriangleWave:  generateTriangleWave(buffer); break;
        case WhiteNoise:    generateWhiteNoise(buffer); break;
        case PinkNoise:     generatePinkNoise(buffer); break;
        case Impulse:       generateImpulse(buffer); break;
    }
}

void MockAudioBufferGenerator::generate(std::vector<std::vector<float>>& buffers)
{
    buffers.resize(config_.numChannels);
    for (auto& buffer : buffers)
    {
        buffer.resize(config_.numSamples);
    }

    // Generate mono signal first
    std::vector<float> monoBuffer;
    generate(monoBuffer);

    // Copy to all channels (or implement proper stereo if needed)
    for (int ch = 0; ch < config_.numChannels; ++ch)
    {
        std::copy(monoBuffer.begin(), monoBuffer.end(), buffers[ch].begin());
    }
}

void MockAudioBufferGenerator::setConfig(const Config& config)
{
    config_ = config;
    phase_ = 0.0; // Reset phase on config change
    noiseState_ = config.seed;
}

// Private implementation methods
void MockAudioBufferGenerator::generateSilence(std::vector<float>& buffer)
{
    std::fill(buffer.begin(), buffer.end(), 0.0f);
}

void MockAudioBufferGenerator::generateSineWave(std::vector<float>& buffer)
{
    const double phaseIncrement = 2.0 * M_PI * config_.frequency / config_.sampleRate;

    for (size_t i = 0; i < buffer.size(); ++i)
    {
        buffer[i] = static_cast<float>(std::sin(phase_) * config_.amplitude);
        phase_ += phaseIncrement;
        if (phase_ > 2.0 * M_PI)
        {
            phase_ -= 2.0 * M_PI;
        }
    }
}

void MockAudioBufferGenerator::generateSquareWave(std::vector<float>& buffer)
{
    const double periodSamples = config_.sampleRate / config_.frequency;

    for (size_t i = 0; i < buffer.size(); ++i)
    {
        const double sampleInPeriod = std::fmod(i, periodSamples);
        buffer[i] = static_cast<float>((sampleInPeriod < periodSamples / 2.0) ? config_.amplitude : -config_.amplitude);
    }
}

void MockAudioBufferGenerator::generateSawtoothWave(std::vector<float>& buffer)
{
    const double periodSamples = config_.sampleRate / config_.frequency;

    for (size_t i = 0; i < buffer.size(); ++i)
    {
        const double sampleInPeriod = std::fmod(i, periodSamples);
        const double normalizedPos = sampleInPeriod / periodSamples;
        buffer[i] = static_cast<float>((2.0 * normalizedPos - 1.0) * config_.amplitude);
    }
}

void MockAudioBufferGenerator::generateTriangleWave(std::vector<float>& buffer)
{
    const double periodSamples = config_.sampleRate / config_.frequency;

    for (size_t i = 0; i < buffer.size(); ++i)
    {
        const double sampleInPeriod = std::fmod(i, periodSamples);
        const double normalizedPos = sampleInPeriod / periodSamples;
        double value;

        if (normalizedPos < 0.25)
        {
            value = 4.0 * normalizedPos;
        }
        else if (normalizedPos < 0.75)
        {
            value = 2.0 - 4.0 * normalizedPos;
        }
        else
        {
            value = 4.0 * (normalizedPos - 1.0);
        }

        buffer[i] = static_cast<float>(value * config_.amplitude);
    }
}

void MockAudioBufferGenerator::generateWhiteNoise(std::vector<float>& buffer)
{
    for (size_t i = 0; i < buffer.size(); ++i)
    {
        // Simple LCG-based white noise
        noiseState_ = noiseState_ * 1103515245 + 12345;
        const float normalized = static_cast<float>(noiseState_) / static_cast<float>(UINT32_MAX);
        buffer[i] = (2.0f * normalized - 1.0f) * config_.amplitude;
    }
}

void MockAudioBufferGenerator::generatePinkNoise(std::vector<float>& buffer)
{
    // Simple approximation of pink noise using filtered white noise
    static float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, b3 = 0.0f, b4 = 0.0f, b5 = 0.0f, b6 = 0.0f;

    for (size_t i = 0; i < buffer.size(); ++i)
    {
        // Generate white noise sample
        noiseState_ = noiseState_ * 1103515245 + 12345;
        const float white = static_cast<float>(noiseState_) / static_cast<float>(UINT32_MAX) * 2.0f - 1.0f;

        // Paul Kellet's refined pink noise algorithm
        b0 = 0.99886f * b0 + white * 0.0555179f;
        b1 = 0.99332f * b1 + white * 0.0750759f;
        b2 = 0.96900f * b2 + white * 0.1538520f;
        b3 = 0.86650f * b3 + white * 0.3104856f;
        b4 = 0.55000f * b4 + white * 0.5329522f;
        b5 = -0.7616f * b5 - white * 0.0168980f;
        const float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
        b6 = white * 0.115926f;

        buffer[i] = pink * config_.amplitude * 0.1f; // Scale down as pink noise has more energy
    }
}

void MockAudioBufferGenerator::generateImpulse(std::vector<float>& buffer)
{
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    if (!buffer.empty())
    {
        buffer[0] = config_.amplitude; // Single impulse at start
    }
}

//==============================================================================

MockInferenceClient::MockInferenceClient(const Config& config)
    : config_(config)
    , activeRequests_(0)
    , stats_{}
{}

bool MockInferenceClient::submitRequest(const InferenceRequest& request, ResponseCallback callback)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!isReady())
    {
        return false;
    }

    ++activeRequests_;

    // Simulate async processing
    std::thread([this, request, callback]() {
        processRequest(request, callback);
    }).detach();

    return true;
}

void MockInferenceClient::cancelAllRequests()
{
    // In a real implementation, this would signal threads to stop
    // For mocking, just reset counter
    activeRequests_.store(0, std::memory_order_release);
}

bool MockInferenceClient::isReady() const noexcept
{
    return activeRequests_.load(std::memory_order_acquire) < 10; // Arbitrary limit
}

MockInferenceClient::Stats MockInferenceClient::getStats() const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

void MockInferenceClient::resetStats()
{
    std::lock_guard<std::mutex> lock(mutex_);
    stats_ = Stats{};
}

void MockInferenceClient::processRequest(const InferenceRequest& request, ResponseCallback callback)
{
    // Simulate processing delay
    const double delayMs = config_.baseLatencyMs +
                          (config_.deterministic ? 0.0 :
                           (static_cast<double>(rand()) / RAND_MAX - 0.5) * 2.0 * config_.latencyVarianceMs);

    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(delayMs)));

    const InferenceResponse response = generateMockResponse(request);
    updateStats(response);

    callback(response);
    --activeRequests_;
}

MockInferenceClient::InferenceResponse MockInferenceClient::generateMockResponse(const InferenceRequest& request)
{
    InferenceResponse response;
    response.processingTimeMs = static_cast<uint64_t>(config_.baseLatencyMs);

    // Simulate occasional errors
    if (config_.simulateErrors && (static_cast<float>(rand()) / RAND_MAX) < config_.errorRate)
    {
        response.success = false;
        response.errorMessage = "Mock inference error";
        response.confidence = 0.0f;
        return response;
    }

    response.success = true;

    // Generate deterministic or random response based on config
    if (config_.deterministic)
    {
        // Deterministic response based on prompt hash
        size_t hash = 0;
        for (char c : request.prompt)
        {
            hash = hash * 31 + static_cast<size_t>(c);
        }

        response.result = "Mock response for hash: " + std::to_string(hash);
        response.confidence = 0.85f;

        // Generate mock embeddings
        response.embeddings.resize(128);
        for (size_t i = 0; i < response.embeddings.size(); ++i)
        {
            response.embeddings[i] = std::sin(static_cast<float>(hash + i) * 0.1f) * 0.5f + 0.5f;
        }
    }
    else
    {
        // Random response
        response.result = "Random mock response: " + std::to_string(rand());
        response.confidence = static_cast<float>(rand()) / RAND_MAX;

        response.embeddings.resize(128);
        for (size_t i = 0; i < response.embeddings.size(); ++i)
        {
            response.embeddings[i] = static_cast<float>(rand()) / RAND_MAX;
        }
    }

    return response;
}

void MockInferenceClient::updateStats(const InferenceResponse& response)
{
    std::lock_guard<std::mutex> lock(mutex_);

    ++stats_.totalRequests;

    if (response.success)
    {
        ++stats_.successfulRequests;
    }
    else
    {
        ++stats_.failedRequests;
    }

    // Update latency stats (simplified)
    stats_.averageLatencyMs = (stats_.averageLatencyMs * (stats_.totalRequests - 1) +
                              response.processingTimeMs) / stats_.totalRequests;

    stats_.minLatencyMs = std::min(stats_.minLatencyMs, static_cast<double>(response.processingTimeMs));
    stats_.maxLatencyMs = std::max(stats_.maxLatencyMs, static_cast<double>(response.processingTimeMs));

    // Simple P95 approximation
    stats_.p95LatencyMs = stats_.maxLatencyMs * 0.95;
}

//==============================================================================

MockDeviceManager::MockDeviceManager(const Config& config)
    : config_(config)
    , isActive_(false)
    , randomState_(config.seed)
{}

bool MockDeviceManager::initialize(int numInputChannels, int numOutputChannels)
{
    if (numInputChannels > config_.deviceInfo.inputChannels ||
        numOutputChannels > config_.deviceInfo.outputChannels)
    {
        return false;
    }

    // Allocate buffers
    inputBuffer_.resize(numInputChannels * config_.deviceInfo.defaultBufferSize);
    outputBuffer_.resize(numOutputChannels * config_.deviceInfo.defaultBufferSize);

    return true;
}

bool MockDeviceManager::start()
{
    if (isActive_.load(std::memory_order_acquire))
    {
        return false; // Already started
    }

    isActive_.store(true, std::memory_order_release);

    // Start mock audio thread
    std::thread([this]() {
        while (isActive_.load(std::memory_order_acquire))
        {
            generateTestAudio();

            // Simulate buffer period
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // ~100Hz
        }
    }).detach();

    return true;
}

void MockDeviceManager::stop()
{
    isActive_.store(false, std::memory_order_release);
}

void MockDeviceManager::setAudioCallback(std::function<void(const float**, int, float**, int, int)> callback)
{
    audioCallback_ = std::move(callback);
}

void MockDeviceManager::generateTestAudio()
{
    if (!audioCallback_)
    {
        return;
    }

    // Generate simple test audio
    const int numInputChannels = config_.deviceInfo.inputChannels;
    const int numOutputChannels = config_.deviceInfo.outputChannels;
    const int bufferSize = config_.deviceInfo.defaultBufferSize;

    // Fill input buffer with silence (or test signal)
    std::fill(inputBuffer_.begin(), inputBuffer_.end(), 0.0f);

    // Prepare output buffer pointers
    std::vector<const float*> inputPointers(numInputChannels);
    std::vector<float*> outputPointers(numOutputChannels);

    for (int ch = 0; ch < numInputChannels; ++ch)
    {
        inputPointers[ch] = inputBuffer_.data() + ch * bufferSize;
    }

    for (int ch = 0; ch < numOutputChannels; ++ch)
    {
        outputPointers[ch] = outputBuffer_.data() + ch * bufferSize;
        std::fill(outputPointers[ch], outputPointers[ch] + bufferSize, 0.0f);
    }

    // Call audio callback
    audioCallback_(inputPointers.data(), numInputChannels,
                   outputPointers.data(), numOutputChannels,
                   bufferSize);

    // Simulate occasional dropouts
    if (config_.simulateDropouts &&
        (static_cast<float>(lcgRandom()) / UINT32_MAX) < config_.dropoutRate)
    {
        // Simulate dropout by zeroing output
        for (auto& sample : outputBuffer_)
        {
            sample = 0.0f;
        }
    }
}

uint32_t MockDeviceManager::lcgRandom()
{
    randomState_ = randomState_ * 1103515245 + 12345;
    return randomState_;
}

//==============================================================================

MockPerformanceMonitor::MockPerformanceMonitor()
    : metrics_{}
{}

void MockPerformanceMonitor::recordProcessTime(std::chrono::nanoseconds processTime,
                                             int numSamples,
                                             double sampleRate)
{
    std::lock_guard<std::mutex> lock(mutex_);

    metrics_.samplesProcessed += static_cast<size_t>(numSamples);

    // Simple statistics (in real implementation, use more sophisticated tracking)
    processTimes_.push_back(processTime);

    // Keep only recent samples
    if (processTimes_.size() > 1000)
    {
        processTimes_.erase(processTimes_.begin(), processTimes_.begin() + 100);
    }

    // Update percentiles
    if (!processTimes_.empty())
    {
        std::sort(processTimes_.begin(), processTimes_.end());
        const size_t p50_idx = processTimes_.size() * 50 / 100;
        const size_t p95_idx = processTimes_.size() * 95 / 100;
        const size_t p99_idx = processTimes_.size() * 99 / 100;

        metrics_.p50ProcessTime = processTimes_[p50_idx];
        metrics_.p95ProcessTime = processTimes_[p95_idx];
        metrics_.p99ProcessTime = processTimes_[p99_idx];
    }
}

void MockPerformanceMonitor::updateCpuLoad(double load)
{
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_.cpuLoad = load;
}

void MockPerformanceMonitor::simulateXrun()
{
    std::lock_guard<std::mutex> lock(mutex_);
    ++metrics_.xrunCount;
}

MockPerformanceMonitor::Metrics MockPerformanceMonitor::getMetrics() const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    return metrics_;
}

void MockPerformanceMonitor::reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_ = Metrics{};
    processTimes_.clear();
}

//==============================================================================

bool TestUtils::approximatelyEqual(float a, float b, float tolerance)
{
    return std::abs(a - b) <= tolerance;
}

bool TestUtils::approximatelyEqual(double a, double b, double tolerance)
{
    return std::abs(a - b) <= tolerance;
}

std::vector<float> TestUtils::generateSineWave(size_t length, double frequency,
                                             double sampleRate, float amplitude)
{
    std::vector<float> buffer(length);
    const double phaseIncrement = 2.0 * M_PI * frequency / sampleRate;

    for (size_t i = 0; i < length; ++i)
    {
        buffer[i] = static_cast<float>(std::sin(i * phaseIncrement) * amplitude);
    }

    return buffer;
}

std::string TestUtils::createTempDirectory()
{
    namespace fs = std::filesystem;

    // Create temporary directory path
    std::string tempPath = (fs::temp_directory_path() / "daw_test_XXXXXX").string();

    // Create directory
    fs::create_directories(tempPath);

    return tempPath;
}

void TestUtils::cleanupTempDirectory(const std::string& path)
{
    namespace fs = std::filesystem;

    try
    {
        fs::remove_all(path);
    }
    catch (const std::exception&)
    {
        // Ignore cleanup errors in tests
    }
}

} // namespace daw::core::testing
