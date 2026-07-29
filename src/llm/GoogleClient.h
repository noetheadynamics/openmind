#pragma once

#include "LLMInterface.h"
#include "HttpClient.h"

namespace OpenMind {

class GoogleClient : public LLMInterface {
public:
    void initialize(const LLMConfig& cfg) override;
    LLMResponse sendPrompt(const std::string& prompt, const std::string& systemInstruction = "") override;
    std::string getProviderName() const override { return "Google"; }
    LLMProvider getProviderType() const override { return LLMProvider::GOOGLE; }
    bool isAvailable() const override;

private:
    HttpClient http;
    std::string buildRequestBody(const std::string& prompt, const std::string& systemInstruction);
    std::string extractContent(const std::string& responseBody);
    int extractTokens(const std::string& responseBody, const std::string& field);
};

} // namespace OpenMind
