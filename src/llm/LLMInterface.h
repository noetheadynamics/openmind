#pragma once

#include <string>
#include <functional>
#include <memory>

namespace OpenMind {

enum class LLMProvider {
    OPENAI,
    ANTHROPIC,
    GOOGLE,
    OLLAMA,
    OPENAI_COMPATIBLE,
    MOCK
};

struct LLMConfig {
    std::string apiKey;
    std::string model;
    std::string baseUrl;
    float temperature = 0.7f;
    int maxTokens = 4096;
    float topP = 1.0f;
    int timeoutMs = 30000;
    int maxRetries = 3;
};

struct LLMResponse {
    std::string content;
    bool success = false;
    std::string error;
    int statusCode = 0;
    int promptTokens = 0;
    int completionTokens = 0;
    std::string finishReason;
};

using LLMMetricsCallback = std::function<void(const std::string& provider, int tokensUsed, float latencyMs)>;

class LLMInterface {
public:
    virtual ~LLMInterface() = default;

    virtual void initialize(const LLMConfig& config) = 0;
    virtual LLMResponse sendPrompt(const std::string& prompt,
                                   const std::string& systemInstruction = "") = 0;
    virtual std::string getProviderName() const = 0;
    virtual LLMProvider getProviderType() const = 0;
    virtual bool isAvailable() const = 0;

    void setMetricsCallback(LLMMetricsCallback cb) { metricsCallback = cb; }
    const LLMConfig& getConfig() const { return config; }

protected:
    LLMConfig config;
    LLMMetricsCallback metricsCallback;

    void reportMetrics(const std::string& provider, int tokens, float latencyMs) {
        if (metricsCallback) metricsCallback(provider, tokens, latencyMs);
    }
};

} // namespace OpenMind
