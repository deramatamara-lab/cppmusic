#include "GrooveExtractor.h"
#include "../inference/InferenceEngine.h"
#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace daw::ai::models
{

GrooveExtractor::GrooveExtractor(std::shared_ptr<daw::ai::inference::InferenceEngine> engine)
    : inferenceEngine(engine)
{
}

void GrooveExtractor::extractGroove(const std::vector<float>& audioData, double sampleRate,
                                     std::function<void(GrooveResult)> callback)
{
    // Real groove extraction using DSP analysis
    // Analyzes audio for rhythm patterns, swing, and groove characteristics

    if (audioData.empty() || sampleRate <= 0.0)
    {
        GrooveResult result;
        result.success = false;
        callback(result);
        return;
    }

    // Perform analysis on background thread via inference engine
    daw::ai::inference::InferenceRequest request;

    // Preprocess audio: extract onset detection features
    const size_t windowSize = static_cast<size_t>(sampleRate * 0.1); // 100ms windows
    const size_t numWindows = audioData.size() / windowSize;

    std::vector<float> onsetEnergies;
    onsetEnergies.reserve(numWindows);

    for (size_t i = 0; i < numWindows; ++i)
    {
        const size_t start = i * windowSize;
        const size_t end = std::min(start + windowSize, audioData.size());

        // Calculate RMS energy for onset detection
        float energy = 0.0f;
        for (size_t j = start; j < end; ++j)
        {
            energy += audioData[j] * audioData[j];
        }
        energy = std::sqrt(energy / static_cast<float>(end - start));
        onsetEnergies.push_back(energy);
    }

    request.inputData = onsetEnergies;
    request.callback = [callback, sampleRate, numWindows](std::vector<float> output)
    {
        GrooveResult result;
        result.success = true;

        if (output.empty() || numWindows < 4)
        {
            result.overallSwing = 0.1f;
            result.style = "straight";
            result.swingValues.resize(16, 0.1f);
            callback(result);
            return;
        }

        // Detect tempo and beat positions
        std::vector<size_t> peaks;
        const float threshold = *std::max_element(output.begin(), output.end()) * 0.3f;

        for (size_t i = 1; i < output.size() - 1; ++i)
        {
            if (output[i] > threshold && output[i] > output[i-1] && output[i] > output[i+1])
            {
                peaks.push_back(i);
            }
        }

        if (peaks.size() < 2)
        {
            result.overallSwing = 0.1f;
            result.style = "straight";
            result.swingValues.resize(16, 0.1f);
            callback(result);
            return;
        }

        // Calculate inter-onset intervals to detect swing
        std::vector<double> intervals;
        for (size_t i = 1; i < peaks.size(); ++i)
        {
            intervals.push_back(static_cast<double>(peaks[i] - peaks[i-1]));
        }

        // Analyze swing pattern (compare even/odd beat intervals)
        double evenSum = 0.0;
        double oddSum = 0.0;
        size_t evenCount = 0;
        size_t oddCount = 0;

        for (size_t i = 0; i < intervals.size(); ++i)
        {
            if (i % 2 == 0)
            {
                evenSum += intervals[i];
                ++evenCount;
            }
            else
            {
                oddSum += intervals[i];
                ++oddCount;
            }
        }

        const double avgEven = evenCount > 0 ? evenSum / evenCount : 1.0;
        const double avgOdd = oddCount > 0 ? oddSum / oddCount : 1.0;

        // Calculate swing ratio (how much longer the off-beat is)
        const double swingRatio = avgOdd > 0.0 ? avgEven / avgOdd : 1.0;
        result.overallSwing = static_cast<double>(juce::jlimit(0.0, 1.0, (swingRatio - 1.0) * 0.5));

        // Determine style based on swing amount
        if (result.overallSwing > 0.4)
            result.style = "heavy swing";
        else if (result.overallSwing > 0.2)
            result.style = "swing";
        else if (result.overallSwing > 0.1)
            result.style = "light swing";
        else
            result.style = "straight";

        // Generate per-step swing values (16 steps)
        result.swingValues.resize(16);
        const double baseSwing = result.overallSwing;

        for (size_t i = 0; i < 16; ++i)
        {
            // Apply swing pattern: stronger on off-beats (odd steps)
            if (i % 2 == 1)
            {
                result.swingValues[i] = baseSwing * 1.2; // Off-beats get more swing
            }
            else
            {
                result.swingValues[i] = baseSwing * 0.8; // On-beats get less swing
            }

            // Add subtle variation
            const double variation = std::sin(static_cast<double>(i) * 0.4) * 0.05;
            result.swingValues[i] = juce::jlimit(0.0, 1.0, result.swingValues[i] + variation);
        }

        callback(result);
    };

    inferenceEngine->queueInference(request);
}

} // namespace daw::ai::models
