#include "AudioBenchmarks.h"
#include <random>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <thread>
#include <chrono>

namespace cppmusic::performance {

// Global instances
std::unique_ptr<AudioProcessingBenchmarks> g_audioBenchmarks;
std::unique_ptr<RealtimeAudioMonitor> g_audioMonitor;

AudioProcessingBenchmarks::AudioProcessingBenchmarks()
    : benchmarkSystem_(std::make_unique<BenchmarkSystem>()) {
    setupTestEnvironment();
}

void AudioProcessingBenchmarks::setupTestEnvironment() {
    // Register all audio-related metrics
    benchmarkSystem_->registerMetric("AnalogEQ::processBlock");
    benchmarkSystem_->registerMetric("AnalogEQ::updateParameters");
    benchmarkSystem_->registerMetric("AnalogEQ::filterProcessing");
    benchmarkSystem_->registerMetric("AnalogEQ::saturationProcessing");
    benchmarkSystem_->registerMetric("AnalogEQ::oversamplingUpsampling");
    benchmarkSystem_->registerMetric("AnalogEQ::oversamplingDownsampling");

    benchmarkSystem_->registerMetric("Synthesizer::renderVoice");
    benchmarkSystem_->registerMetric("Synthesizer::processOscillators");
    benchmarkSystem_->registerMetric("Synthesizer::processFilters");
    benchmarkSystem_->registerMetric("Synthesizer::processEnvelopes");
    benchmarkSystem_->registerMetric("Synthesizer::processEffects");

    benchmarkSystem_->registerMetric("EffectChain::processBlock");
    benchmarkSystem_->registerMetric("ParameterUpdate::applyChanges");
    benchmarkSystem_->registerMetric("AudioBuffer::copy");
    benchmarkSystem_->registerMetric("AudioBuffer::clear");
    benchmarkSystem_->registerMetric("AudioBuffer::mix");
}

void AudioProcessingBenchmarks::cleanupTestEnvironment() {
    // Clean up any test resources
}

AudioProcessingBenchmarks::EQBenchmarkResults AudioProcessingBenchmarks::benchmarkAnalogEQ(
    double sampleRate, int blockSize, int numChannels, bool enableOversampling, int numBands) {

    EQBenchmarkResults results = {};

    // Generate test signal
    auto testSignal = generateComplexSignal(blockSize * 100, sampleRate); // 100 blocks worth
    auto originalSignal = testSignal; // Keep copy for analysis

    // Measure processing time
    auto* processingMetrics = benchmarkSystem_->getMetrics("AnalogEQ::processBlock");

    const int numIterations = 1000;
    auto startTime = HighResolutionTimer::now();

    for (int i = 0; i < numIterations; ++i) {
        PROFILE_SCOPE("AnalogEQ::processBlock", *processingMetrics);

        // Simulate EQ processing (would be actual EQ in real implementation)
        // For benchmark purposes, we simulate the computational load
        for (int ch = 0; ch < numChannels; ++ch) {
            for (int band = 0; band < numBands; ++band) {
                // Simulate biquad filter processing
                for (int sample = 0; sample < blockSize; ++sample) {
                    // Biquad filter simulation (5 operations per sample per band)
                    volatile float temp = testSignal[sample] * 0.1f;
                    temp = temp * temp + temp * 0.5f - temp * 0.25f;
                    temp = std::tanh(temp); // Saturation simulation
                    testSignal[sample] = temp;
                }
            }
        }

        if (enableOversampling) {
            // Simulate oversampling overhead (2x processing)
            for (int sample = 0; sample < blockSize; ++sample) {
                volatile float temp = testSignal[sample] * 1.414f; // Upsampling gain
                temp = temp * 0.707f; // Downsampling gain
                testSignal[sample] = temp;
            }
        }
    }

    auto endTime = HighResolutionTimer::now();
    auto totalTime = HighResolutionTimer::toMilliseconds(endTime - startTime);

    results.processingTimeMs = totalTime / numIterations;

    // Calculate CPU usage (processing time vs available time)
    double blockTimeMs = (static_cast<double>(blockSize) / sampleRate) * 1000.0;
    results.cpuUsagePercent = (results.processingTimeMs / blockTimeMs) * 100.0;

    // Check real-time safety
    results.realtimeSafe = results.processingTimeMs < (blockTimeMs * 0.8); // 80% margin

    // Analyze signal quality
    results.dynamicRange = calculateDynamicRange(testSignal);

    // Estimate memory usage
    size_t filterMemory = numBands * numChannels * sizeof(float) * 6; // 6 coefficients per biquad
    size_t bufferMemory = blockSize * numChannels * sizeof(float);
    if (enableOversampling) bufferMemory *= 2;
    results.memoryUsage = filterMemory + bufferMemory;

    // Filter stability check (ensure no denormals or infinities)
    results.filterStability = 1.0;
    for (float sample : testSignal) {
        if (!std::isfinite(sample) || std::abs(sample) < 1e-30f) {
            results.filterStability = 0.0;
            break;
        }
    }

    return results;
}

AudioProcessingBenchmarks::LatencyMeasurement AudioProcessingBenchmarks::measureSystemLatency() {
    LatencyMeasurement measurement = {};

    // Simulate latency measurement
    // In real implementation, this would measure actual audio path latency

    // Simulate input-to-output latency
    auto startTime = HighResolutionTimer::now();
    std::this_thread::sleep_for(std::chrono::microseconds(500)); // Simulate processing
    auto endTime = HighResolutionTimer::now();

    measurement.inputToOutputMs = HighResolutionTimer::toMilliseconds(endTime - startTime);

    // Simulate parameter update latency
    startTime = HighResolutionTimer::now();
    std::this_thread::sleep_for(std::chrono::microseconds(100)); // Simulate parameter update
    endTime = HighResolutionTimer::now();

    measurement.parameterUpdateMs = HighResolutionTimer::toMilliseconds(endTime - startTime);

    // Measure jitter over multiple measurements
    std::vector<double> latencies;
    for (int i = 0; i < 100; ++i) {
        auto start = HighResolutionTimer::now();
        std::this_thread::sleep_for(std::chrono::microseconds(500));
        auto end = HighResolutionTimer::now();
        latencies.push_back(HighResolutionTimer::toMilliseconds(end - start));
    }

    double avgLatency = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
    double variance = 0.0;
    for (double lat : latencies) {
        variance += (lat - avgLatency) * (lat - avgLatency);
    }
    variance /= latencies.size();
    measurement.jitterMs = std::sqrt(variance);

    measurement.worstCaseMs = *std::max_element(latencies.begin(), latencies.end());

    return measurement;
}

AudioProcessingBenchmarks::LoadTestResults AudioProcessingBenchmarks::performLoadTest() {
    LoadTestResults results = {};

    // Gradually increase load until system breaks
    const int maxVoices = 128;
    const double targetCPU = 80.0; // Break at 80% CPU

    for (int voices = 1; voices <= maxVoices; ++voices) {
        // Simulate voice processing load
        auto startTime = HighResolutionTimer::now();

        for (int voice = 0; voice < voices; ++voice) {
            // Simulate synthesizer voice processing
            for (int sample = 0; sample < 512; ++sample) {
                // Oscillator
                volatile float osc = std::sin(static_cast<float>(sample) * 0.1f);
                // Filter
                osc = osc * 0.5f + osc * osc * 0.3f;
                // Envelope
                osc *= 0.8f;
                // Effect
                osc = std::tanh(osc);
            }
        }

        auto endTime = HighResolutionTimer::now();
        double processingTime = HighResolutionTimer::toMilliseconds(endTime - startTime);

        // Calculate CPU usage
        double blockTime = (512.0 / 48000.0) * 1000.0; // 512 samples at 48kHz
        double cpuUsage = (processingTime / blockTime) * 100.0;

        if (cpuUsage > targetCPU) {
            results.maxSimultaneousVoices = voices - 1;
            results.maxCPUBeforeDropouts = cpuUsage;
            results.breakingPointMs = processingTime;
            break;
        }

        // Estimate memory usage
        results.maxMemoryUsage = voices * 1024; // 1KB per voice estimate
    }

    return results;
}

void AudioProcessingBenchmarks::benchmarkEQProcessing() {
    // Test different EQ configurations
    std::vector<std::tuple<double, int, bool>> configs = {
        {44100.0, 256, false},   // Standard quality
        {48000.0, 512, false},   // High quality
        {96000.0, 512, true},    // Ultra quality with oversampling
        {192000.0, 1024, true}   // Extreme quality
    };

    for (const auto& [sampleRate, blockSize, oversampling] : configs) {
        auto results = benchmarkAnalogEQ(sampleRate, blockSize, 2, oversampling, 5);

        std::string configName = "EQ_" + std::to_string(static_cast<int>(sampleRate)) +
                                "_" + std::to_string(blockSize) +
                                (oversampling ? "_OS" : "");

        benchmarkSystem_->recordBenchmark(configName + "_ProcessingTime", results.processingTimeMs);
        benchmarkSystem_->recordBenchmark(configName + "_CPUUsage", results.cpuUsagePercent);
        benchmarkSystem_->recordBenchmark(configName + "_MemoryUsage", static_cast<double>(results.memoryUsage));
    }
}

void AudioProcessingBenchmarks::benchmarkSynthesizerVoices() {
    // Test synthesizer performance with different voice counts
    std::vector<int> voiceCounts = {1, 4, 8, 16, 32, 64};

    for (int voices : voiceCounts) {
        auto* metrics = benchmarkSystem_->getMetrics("Synthesizer::renderVoice");

        auto startTime = HighResolutionTimer::now();

        for (int voice = 0; voice < voices; ++voice) {
            PROFILE_SCOPE("Synthesizer::renderVoice", *metrics);

            // Simulate voice rendering
            for (int sample = 0; sample < 512; ++sample) {
                // Oscillators
                volatile float osc1 = std::sin(static_cast<float>(sample) * 0.1f);
                volatile float osc2 = std::sin(static_cast<float>(sample) * 0.15f);

                // Filter
                volatile float filtered = (osc1 + osc2) * 0.5f;
                filtered = filtered * 0.8f + filtered * filtered * 0.2f;

                // Envelope
                filtered *= 0.9f;

                // Effects
                filtered = std::tanh(filtered);
            }
        }

        auto endTime = HighResolutionTimer::now();
        double totalTime = HighResolutionTimer::toMilliseconds(endTime - startTime);

        benchmarkSystem_->recordBenchmark("Synth_" + std::to_string(voices) + "_voices", totalTime);
    }
}

void AudioProcessingBenchmarks::benchmarkParameterUpdates() {
    auto* metrics = benchmarkSystem_->getMetrics("ParameterUpdate::applyChanges");

    // Test parameter update performance
    const int numUpdates = 1000;
    auto startTime = HighResolutionTimer::now();

    for (int i = 0; i < numUpdates; ++i) {
        PROFILE_SCOPE("ParameterUpdate::applyChanges", *metrics);

        // Simulate parameter updates
        volatile float frequency = 1000.0f + static_cast<float>(i);
        volatile float gain = static_cast<float>(i) / numUpdates;
        volatile float q = 0.707f + static_cast<float>(i) * 0.001f;

        // Simulate coefficient calculation
        volatile float omega = 2.0f * 3.14159f * frequency / 48000.0f;
        volatile float sin_omega = std::sin(omega);
        volatile float cos_omega = std::cos(omega);
        volatile float alpha = sin_omega / (2.0f * q);
    }

    auto endTime = HighResolutionTimer::now();
    double totalTime = HighResolutionTimer::toMilliseconds(endTime - startTime);

    benchmarkSystem_->recordBenchmark("ParameterUpdates_" + std::to_string(numUpdates), totalTime);
}

void AudioProcessingBenchmarks::runFullAudioBenchmark() {
    benchmarkEQProcessing();
    benchmarkSynthesizerVoices();
    benchmarkParameterUpdates();
    benchmarkMemoryPatterns();

    // Generate comprehensive report
    auto report = benchmarkSystem_->generateReport();
    benchmarkSystem_->exportToJSON("audio_benchmark_report.json");
    benchmarkSystem_->exportToHTML("audio_benchmark_report.html");
}

std::vector<float> AudioProcessingBenchmarks::generateTestSignal(int numSamples, double frequency, double sampleRate) {
    std::vector<float> signal(numSamples);
    double phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
    double phase = 0.0;

    for (int i = 0; i < numSamples; ++i) {
        signal[i] = static_cast<float>(std::sin(phase));
        phase += phaseIncrement;
        if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
    }

    return signal;
}

std::vector<float> AudioProcessingBenchmarks::generateWhiteNoise(int numSamples, float amplitude) {
    std::vector<float> noise(numSamples);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-amplitude, amplitude);

    for (int i = 0; i < numSamples; ++i) {
        noise[i] = dist(gen);
    }

    return noise;
}

std::vector<float> AudioProcessingBenchmarks::generateComplexSignal(int numSamples, double sampleRate) {
    std::vector<float> signal(numSamples);

    // Multiple frequency components
    std::vector<double> frequencies = {440.0, 880.0, 1320.0, 2200.0}; // A4 and harmonics
    std::vector<double> phases(frequencies.size(), 0.0);
    std::vector<double> phaseIncrements;

    for (double freq : frequencies) {
        phaseIncrements.push_back(2.0 * M_PI * freq / sampleRate);
    }

    for (int i = 0; i < numSamples; ++i) {
        float sample = 0.0f;

        for (size_t j = 0; j < frequencies.size(); ++j) {
            float amplitude = 1.0f / (j + 1); // Decreasing amplitude for harmonics
            sample += amplitude * std::sin(phases[j]);
            phases[j] += phaseIncrements[j];
            if (phases[j] > 2.0 * M_PI) phases[j] -= 2.0 * M_PI;
        }

        signal[i] = sample * 0.25f; // Scale down to prevent clipping
    }

    return signal;
}

double AudioProcessingBenchmarks::calculateDynamicRange(const std::vector<float>& signal) {
    if (signal.empty()) return 0.0;

    float maxValue = *std::max_element(signal.begin(), signal.end());
    float minValue = *std::min_element(signal.begin(), signal.end());

    float peakToPeak = maxValue - minValue;
    if (peakToPeak <= 0.0f) return 0.0;

    // Calculate noise floor (rough estimate)
    std::vector<float> sortedSignal = signal;
    std::sort(sortedSignal.begin(), sortedSignal.end());

    // Use bottom 10% as noise estimate
    size_t noiseCount = sortedSignal.size() / 10;
    float noiseSum = 0.0f;
    for (size_t i = 0; i < noiseCount; ++i) {
        noiseSum += std::abs(sortedSignal[i]);
    }
    float noiseFloor = noiseSum / noiseCount;

    if (noiseFloor <= 0.0f) noiseFloor = 1e-6f; // Minimum noise floor

    return 20.0 * std::log10(std::abs(maxValue) / noiseFloor);
}

void AudioProcessingBenchmarks::benchmarkMemoryPatterns() {
    auto& memTracker = MemoryTracker::getInstance();

    // Test different allocation patterns common in audio processing
    std::vector<std::string> patterns = {
        "SmallFrequentAllocs",    // Many small allocations
        "LargeBufferAllocs",      // Few large allocations
        "MixedPatternAllocs"      // Mixed allocation sizes
    };

    for (const std::string& pattern : patterns) {
        auto startMem = memTracker.getCurrentMemoryUsage();
        auto startTime = HighResolutionTimer::now();

        if (pattern == "SmallFrequentAllocs") {
            std::vector<void*> ptrs;
            for (int i = 0; i < 1000; ++i) {
                void* ptr = std::malloc(64); // Small allocations
                ptrs.push_back(ptr);
                memTracker.recordAllocation(ptr, 64, pattern);
            }

            for (void* ptr : ptrs) {
                memTracker.recordDeallocation(ptr);
                std::free(ptr);
            }
        }
        else if (pattern == "LargeBufferAllocs") {
            std::vector<void*> ptrs;
            for (int i = 0; i < 10; ++i) {
                void* ptr = std::malloc(65536); // Large allocations
                ptrs.push_back(ptr);
                memTracker.recordAllocation(ptr, 65536, pattern);
            }

            for (void* ptr : ptrs) {
                memTracker.recordDeallocation(ptr);
                std::free(ptr);
            }
        }

        auto endTime = HighResolutionTimer::now();
        auto endMem = memTracker.getCurrentMemoryUsage();

        double allocTime = HighResolutionTimer::toMilliseconds(endTime - startTime);
        benchmarkSystem_->recordBenchmark("Memory_" + pattern, allocTime);
    }
}

void AudioProcessingBenchmarks::benchmarkSIMDOptimizations() {
    // Benchmark SIMD vs scalar processing
    const int numSamples = 4096;
    auto signal = generateTestSignal(numSamples, 1000.0, 48000.0);

    // Scalar processing benchmark
    auto startTime = HighResolutionTimer::now();
    for (int iteration = 0; iteration < 1000; ++iteration) {
        for (int i = 0; i < numSamples; ++i) {
            signal[i] = signal[i] * 0.5f + signal[i] * signal[i] * 0.3f;
        }
    }
    auto endTime = HighResolutionTimer::now();
    double scalarTime = HighResolutionTimer::toMilliseconds(endTime - startTime);

    // Reset signal
    signal = generateTestSignal(numSamples, 1000.0, 48000.0);

    // SIMD processing benchmark (simulated - would use actual SIMD intrinsics)
    startTime = HighResolutionTimer::now();
    for (int iteration = 0; iteration < 1000; ++iteration) {
        // Process 4 samples at a time (simulated SIMD)
        for (int i = 0; i < numSamples; i += 4) {
            int remaining = std::min(4, numSamples - i);
            for (int j = 0; j < remaining; ++j) {
                signal[i + j] = signal[i + j] * 0.5f + signal[i + j] * signal[i + j] * 0.3f;
            }
        }
    }
    endTime = HighResolutionTimer::now();
    double simdTime = HighResolutionTimer::toMilliseconds(endTime - startTime);

    benchmarkSystem_->recordBenchmark("SIMD_ScalarProcessing", scalarTime);
    benchmarkSystem_->recordBenchmark("SIMD_VectorProcessing", simdTime);

    double speedup = scalarTime / simdTime;
    benchmarkSystem_->recordBenchmark("SIMD_SpeedupRatio", speedup);
}

// RealtimeAudioMonitor implementation
struct RealtimeAudioMonitor::Impl {
    std::atomic<bool> running{false};
    std::thread monitorThread;

    std::atomic<double> lastProcessingTime{0.0};
    std::atomic<int> dropoutCount{0};
    std::atomic<double> lastLatency{0.0};
    std::atomic<int> voiceCount{0};

    std::vector<AudioPerformanceMetrics> history;
    std::mutex historyMutex;

    AlertCallback alertCallback;

    void monitorLoop() {
        auto lastTime = HighResolutionTimer::now();

        while (running.load()) {
            auto now = HighResolutionTimer::now();
            auto deltaTime = HighResolutionTimer::toMilliseconds(now - lastTime);
            lastTime = now;

            AudioPerformanceMetrics metrics = {};
            metrics.processingLoad = lastProcessingTime.load() / 10.67; // Assuming 512@48kHz = 10.67ms
            metrics.latency = lastLatency.load();
            metrics.dropouts = dropoutCount.load();
            metrics.activeVoices = voiceCount.load();

            // Store in history
            {
                std::lock_guard<std::mutex> lock(historyMutex);
                history.push_back(metrics);
                if (history.size() > 1000) {
                    history.erase(history.begin());
                }
            }

            // Check for alerts
            if (alertCallback) {
                if (metrics.processingLoad > 80.0) {
                    alertCallback("HIGH_CPU", "Processing load: " + std::to_string(metrics.processingLoad) + "%");
                }
                if (metrics.dropouts > 0) {
                    alertCallback("AUDIO_DROPOUTS", "Dropouts detected: " + std::to_string(metrics.dropouts));
                }
                if (metrics.latency > 20.0) {
                    alertCallback("HIGH_LATENCY", "Latency: " + std::to_string(metrics.latency) + "ms");
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};

RealtimeAudioMonitor::RealtimeAudioMonitor()
    : pImpl_(std::make_unique<Impl>()) {}

RealtimeAudioMonitor::~RealtimeAudioMonitor() {
    stop();
}

void RealtimeAudioMonitor::start() {
    if (!pImpl_->running.exchange(true)) {
        pImpl_->monitorThread = std::thread(&Impl::monitorLoop, pImpl_.get());
    }
}

void RealtimeAudioMonitor::stop() {
    if (pImpl_->running.exchange(false)) {
        if (pImpl_->monitorThread.joinable()) {
            pImpl_->monitorThread.join();
        }
    }
}

void RealtimeAudioMonitor::recordProcessingTime(double timeMs) {
    pImpl_->lastProcessingTime.store(timeMs);
}

void RealtimeAudioMonitor::recordDropout() {
    pImpl_->dropoutCount.fetch_add(1);
}

void RealtimeAudioMonitor::recordLatency(double latencyMs) {
    pImpl_->lastLatency.store(latencyMs);
}

void RealtimeAudioMonitor::updateVoiceCount(int voices) {
    pImpl_->voiceCount.store(voices);
}

RealtimeAudioMonitor::AudioPerformanceMetrics RealtimeAudioMonitor::getCurrentMetrics() const {
    AudioPerformanceMetrics metrics = {};
    metrics.processingLoad = pImpl_->lastProcessingTime.load() / 10.67;
    metrics.latency = pImpl_->lastLatency.load();
    metrics.dropouts = pImpl_->dropoutCount.load();
    metrics.activeVoices = pImpl_->voiceCount.load();
    return metrics;
}

std::vector<RealtimeAudioMonitor::AudioPerformanceMetrics> RealtimeAudioMonitor::getHistory(size_t maxSamples) const {
    std::lock_guard<std::mutex> lock(pImpl_->historyMutex);

    if (pImpl_->history.size() <= maxSamples) {
        return pImpl_->history;
    }

    return std::vector<AudioPerformanceMetrics>(
        pImpl_->history.end() - maxSamples,
        pImpl_->history.end()
    );
}

void RealtimeAudioMonitor::setAlertCallback(AlertCallback callback) {
    pImpl_->alertCallback = callback;
}

void initializeAudioBenchmarking() {
    if (!g_audioBenchmarks) {
        g_audioBenchmarks = std::make_unique<AudioProcessingBenchmarks>();
    }
    if (!g_audioMonitor) {
        g_audioMonitor = std::make_unique<RealtimeAudioMonitor>();
        g_audioMonitor->start();
    }
}

void shutdownAudioBenchmarking() {
    if (g_audioMonitor) {
        g_audioMonitor->stop();
        g_audioMonitor.reset();
    }
    g_audioBenchmarks.reset();
}

} // namespace cppmusic::performance
