#pragma once

// Mock LLM client for unit testing and development. Not used in production builds.

#include "LLMInterface.h"
#include <map>
#include <functional>

namespace OpenMind {

class MockLLMClient : public LLMInterface {
public:
    using ResponseGenerator = std::function<std::string(const std::string& prompt, const std::string& system)>;

    MockLLMClient() = default;

    void initialize(const LLMConfig& cfg) override {
        config = cfg;
        available = true;
    }

    LLMResponse sendPrompt(const std::string& prompt, const std::string& systemInstruction = "") override {
        LLMResponse resp;
        if (!available) {
            resp.error = "Mock client not available";
            return resp;
        }
        if (responseGenerator) {
            resp.content = responseGenerator(prompt, systemInstruction);
        } else {
            resp.content = generateDefaultResponse(prompt);
        }
        resp.success = true;
        resp.statusCode = 200;
        resp.promptTokens = (int)prompt.size() / 4;
        resp.completionTokens = (int)resp.content.size() / 4;
        resp.finishReason = "stop";
        reportMetrics("Mock", resp.promptTokens + resp.completionTokens, 1.0f);
        return resp;
    }

    std::string getProviderName() const override { return "MockLLM"; }
    LLMProvider getProviderType() const override { return LLMProvider::MOCK; }
    bool isAvailable() const override { return available; }

    void setResponseGenerator(ResponseGenerator gen) { responseGenerator = gen; }
    void setAvailable(bool v) { available = v; }

    void setPresetResponse(const std::string& key, const std::string& response) {
        presets[key] = response;
    }

private:
    bool available = false;
    ResponseGenerator responseGenerator;
    std::map<std::string, std::string> presets;

    std::string generateDefaultResponse(const std::string& prompt) {
        for (auto& [key, val] : presets) {
            if (prompt.find(key) != std::string::npos) return val;
        }
        return "{\"status\":\"ok\",\"message\":\"Mock response for: " + prompt.substr(0, 64) + "\"}";
    }
};

} // namespace OpenMind
