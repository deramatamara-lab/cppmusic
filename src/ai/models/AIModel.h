#pragma once

#include <memory>
#include <string>

namespace daw::ai::models
{

/**
 * @brief AI model wrapper
 * 
 * Manages AI model loading and basic operations.
 * Models must be loaded asynchronously (never on audio thread).
 */
class AIModel
{
public:
    AIModel() = default;
    virtual ~AIModel() = default;

    /**
     * @brief Load model from file (async operation)
     * @param modelPath Path to model file
     * @return true if loading started successfully
     */
    virtual bool loadFromFile(const std::string& modelPath);

    /**
     * @brief Check if model is loaded and ready
     * @return true if model is ready for inference
     */
    [[nodiscard]] virtual bool isLoaded() const noexcept;

    /**
     * @brief Get model name/identifier
     * @return Model name
     */
    [[nodiscard]] virtual std::string getName() const;

protected:
    bool loaded = false;
    std::string modelPath;
};

} // namespace daw::ai::models

