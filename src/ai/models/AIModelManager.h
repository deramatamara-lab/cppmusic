#pragma once

#include "AIModel.h"
#include <memory>
#include <mutex>
#include <functional>

namespace daw::ai::models
{

/**
 * @brief Manages AI model lifecycle
 * 
 * Handles async loading, memory management, and model versioning.
 * Thread-safe for model access.
 */
class AIModelManager
{
public:
    AIModelManager();
    ~AIModelManager() = default;

    /**
     * @brief Load model asynchronously
     * @param modelFile Path to model file
     * @param callback Callback when loading completes (called on message thread)
     */
    void loadModelAsync(const std::string& modelFile, 
                       std::function<void(bool)> callback);

    /**
     * @brief Get current loaded model
     * @return Shared pointer to model, or nullptr if none loaded
     */
    [[nodiscard]] std::shared_ptr<AIModel> getCurrentModel() const;

private:
    mutable std::mutex modelLock;
    std::shared_ptr<AIModel> currentModel;
};

} // namespace daw::ai::models

