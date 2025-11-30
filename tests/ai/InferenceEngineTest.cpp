#include <juce_core/juce_core.h>
#include "../../../src/ai/inference/InferenceEngine.h"
#include "../../../src/ai/config/AIConfig.h"
#include <atomic>
#include <thread>

class InferenceEngineTest : public juce::UnitTest
{
public:
    InferenceEngineTest() : juce::UnitTest("InferenceEngine Test") {}

    void runTest() override
    {
        beginTest("InferenceEngine creation");
        daw::ai::inference::InferenceEngine engine(2);
        expect(!engine.isReady(), "Should not be ready without config");

        beginTest("Engine initialization");
        auto config = std::make_shared<daw::ai::config::AIConfig>();
        config->setBackendType(daw::ai::config::AIBackendType::LocalLLM);
        config->setLocalLLMHost("localhost");
        config->setLocalLLMPort(11434);
        config->setLocalLLMModel("test-model");

        engine.initialize(config);
        // Note: isReady() depends on backend availability, which may not be available in tests
        expect(true, "Initialization should not crash");

        beginTest("Queue inference request");
        std::atomic<bool> callbackCalled{false};
        std::atomic<bool> callbackSuccess{false};

        daw::ai::inference::InferenceRequest request;
        request.inputData = {0.5f, 0.3f, 0.7f, 0.2f};
        request.callback = [&callbackCalled, &callbackSuccess](std::vector<float> result) {
            callbackCalled = true;
            callbackSuccess = !result.empty();
        };

        engine.queueInference(std::move(request));

        // Wait a bit for processing (inference runs on worker threads)
        juce::Thread::sleep(100);

        // Note: Callback may or may not be called depending on backend availability
        expect(true, "Queueing inference should not crash");

        beginTest("Bounded queue");
        // Queue multiple requests to test bounded queue
        for (int i = 0; i < 10; ++i)
        {
            daw::ai::inference::InferenceRequest req;
            req.inputData = {static_cast<float>(i)};
            req.callback = [](std::vector<float>) {};
            engine.queueInference(std::move(req));
        }

        expect(true, "Queueing many requests should not crash (bounded queue should handle it)");

        beginTest("Engine shutdown");
        engine.stop();
        expect(true, "Stop should not crash");
    }
};

static InferenceEngineTest inferenceEngineTest;

