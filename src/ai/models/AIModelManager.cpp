#include "AIModelManager.h"
#include <thread>

namespace daw::ai::models
{

AIModelManager::AIModelManager()
{
}

void AIModelManager::loadModelAsync(const std::string& modelFile, 
                                   std::function<void(bool)> callback)
{
    // Load on background thread
    std::thread([this, modelFile, callback]()
    {
        try
        {
            // Load model (may take seconds)
            auto newModel = std::make_shared<AIModel>();
            if (newModel->loadFromFile(modelFile))
            {
                // Swap models atomically
                {
                    std::lock_guard<std::mutex> lock(modelLock);
                    currentModel = std::move(newModel);
                }
                // Callback on message thread (would use MessageManager::callAsync in JUCE)
                if (callback)
                    callback(true);
            }
            else
            {
                if (callback)
                    callback(false);
            }
        }
        catch (const std::exception&)
        {
            if (callback)
                callback(false);
        }
    }).detach();
}

std::shared_ptr<AIModel> AIModelManager::getCurrentModel() const
{
    std::lock_guard<std::mutex> lock(modelLock);
    return currentModel;
}

} // namespace daw::ai::models

