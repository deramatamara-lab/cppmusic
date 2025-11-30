#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <functional>

namespace daw::ai::config
{

/**
 * @brief AI backend type
 */
enum class AIBackendType
{
    LocalLLM,      // Local language model (Ollama, llama.cpp, etc.)
    API,           // Remote API (OpenAI, Anthropic, etc.)
    Hybrid         // Use local for some tasks, API for others
};

/**
 * @brief AI configuration
 *
 * Configures AI backend (local LLM or API) for inference.
 * Follows DAW_DEV_RULES: configurable, no secrets in logs.
 */
class AIConfig
{
public:
    AIConfig();
    ~AIConfig() = default;

    // Backend configuration
    void setBackendType(AIBackendType type) { backendType = type; }
    [[nodiscard]] AIBackendType getBackendType() const { return backendType; }

    // Local LLM configuration
    void setLocalLLMPath(const std::string& path) { localLLMPath = path; }
    [[nodiscard]] const std::string& getLocalLLMPath() const { return localLLMPath; }

    void setLocalLLMModel(const std::string& model) { localLLMModel = model; }
    [[nodiscard]] const std::string& getLocalLLMModel() const { return localLLMModel; }

    void setLocalLLMHost(const std::string& host) { localLLMHost = host; }
    [[nodiscard]] const std::string& getLocalLLMHost() const { return localLLMHost; }

    void setLocalLLMPort(int port) { localLLMPort = port; }
    [[nodiscard]] int getLocalLLMPort() const { return localLLMPort; }

    // API configuration
    void setAPIEndpoint(const std::string& endpoint) { apiEndpoint = endpoint; }
    [[nodiscard]] const std::string& getAPIEndpoint() const { return apiEndpoint; }

    void setAPIKey(const std::string& key) { apiKey = key; }
    [[nodiscard]] const std::string& getAPIKey() const { return apiKey; }

    void setAPIModel(const std::string& model) { apiModel = model; }
    [[nodiscard]] const std::string& getAPIModel() const { return apiModel; }

    // Performance settings
    void setMaxConcurrentRequests(int max) { maxConcurrentRequests = max; }
    [[nodiscard]] int getMaxConcurrentRequests() const { return maxConcurrentRequests; }

    void setRequestTimeoutMs(int timeout) { requestTimeoutMs = timeout; }
    [[nodiscard]] int getRequestTimeoutMs() const { return requestTimeoutMs; }

    // Task-specific backend selection
    void setTaskBackend(const std::string& task, AIBackendType backend);
    [[nodiscard]] AIBackendType getTaskBackend(const std::string& task) const;

    // Persistence
    void loadFromFile(const std::string& filePath);
    void saveToFile(const std::string& filePath) const;

private:
    AIBackendType backendType{AIBackendType::LocalLLM};

    // Local LLM settings
    std::string localLLMPath;      // Path to local LLM executable
    std::string localLLMModel;      // Model name/identifier
    std::string localLLMHost{"localhost"};
    int localLLMPort{11434};       // Default Ollama port

    // API settings
    std::string apiEndpoint;
    std::string apiKey;             // Stored securely (OS keychain)
    std::string apiModel;

    // Performance
    int maxConcurrentRequests{4};
    int requestTimeoutMs{30000};   // 30 seconds

    // Task-specific backends
    std::map<std::string, AIBackendType> taskBackends;
};

/**
 * @brief AI backend interface
 */
class AIBackend
{
public:
    virtual ~AIBackend() = default;

    /**
     * @brief Initialize backend
     */
    virtual bool initialize(const AIConfig& config) = 0;

    /**
     * @brief Shutdown backend
     */
    virtual void shutdown() = 0;

    /**
     * @brief Check if backend is available
     */
    [[nodiscard]] virtual bool isAvailable() const = 0;

    /**
     * @brief Perform inference
     * @param prompt Text prompt
     * @param callback Result callback
     */
    virtual void infer(const std::string& prompt,
                      std::function<void(const std::string&, bool)> callback) = 0;

    /**
     * @brief Perform structured inference (JSON)
     * @param prompt Text prompt
     * @param schema JSON schema for response
     * @param callback Result callback
     */
    virtual void inferStructured(const std::string& prompt,
                                const std::string& schema,
                                std::function<void(const std::string&, bool)> callback) = 0;
};

/**
 * @brief Local LLM backend (Ollama, llama.cpp, etc.)
 */
class LocalLLMBackend : public AIBackend
{
public:
    LocalLLMBackend();
    ~LocalLLMBackend() override;

    bool initialize(const AIConfig& config) override;
    void shutdown() override;
    [[nodiscard]] bool isAvailable() const override;
    void infer(const std::string& prompt, std::function<void(const std::string&, bool)> callback) override;
    void inferStructured(const std::string& prompt, const std::string& schema,
                       std::function<void(const std::string&, bool)> callback) override;

private:
    std::string host;
    int port{11434};
    std::string model;
    bool available{false};

    std::string makeRequest(const std::string& endpoint, const std::string& body) const;
};

/**
 * @brief API backend (OpenAI, Anthropic, etc.)
 */
class APIBackend : public AIBackend
{
public:
    APIBackend();
    ~APIBackend() override;

    bool initialize(const AIConfig& config) override;
    void shutdown() override;
    [[nodiscard]] bool isAvailable() const override;
    void infer(const std::string& prompt, std::function<void(const std::string&, bool)> callback) override;
    void inferStructured(const std::string& prompt, const std::string& schema,
                        std::function<void(const std::string&, bool)> callback) override;

private:
    std::string endpoint;
    std::string apiKey;
    std::string model;
    bool available{false};

    std::string makeHTTPRequest(const std::string& url, const std::string& body) const;
};

/**
 * @brief AI backend factory
 */
class AIBackendFactory
{
public:
    static std::unique_ptr<AIBackend> createBackend(AIBackendType type);
    static std::unique_ptr<AIBackend> createBackend(const AIConfig& config);
};

} // namespace daw::ai::config

