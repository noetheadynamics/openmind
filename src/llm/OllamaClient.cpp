#include "OllamaClient.h"
#include <sstream>
#include <chrono>
#include <thread>

namespace OpenMind {

void OllamaClient::initialize(const LLMConfig& cfg) {
    config = cfg;
    if (config.baseUrl.empty()) config.baseUrl = "http://localhost:11434";
    if (config.model.empty()) config.model = "llama3";
    http.setTimeout(config.timeoutMs);
    http.setMaxRetries(config.maxRetries);
}

bool OllamaClient::isAvailable() const {
    return !config.baseUrl.empty();
}

std::string OllamaClient::buildRequestBody(const std::string& prompt, const std::string& systemInstruction) {
    std::ostringstream json;
    json << "{\"model\":\"" << config.model << "\",\"stream\":false,\"options\":{"
         << "\"temperature\":" << config.temperature
         << ",\"num_predict\":" << config.maxTokens
         << ",\"top_p\":" << config.topP
         << "},\"messages\":[";

    if (!systemInstruction.empty()) {
        json << "{\"role\":\"system\",\"content\":\"";
        for (char c : systemInstruction) {
            if (c == '"') json << "\\\"";
            else if (c == '\\') json << "\\\\";
            else if (c == '\n') json << "\\n";
            else if (c < 0x20) { json << "\\u00" << std::hex << (int)c; }
            else json << c;
        }
        json << "\"},";
    }

    json << "{\"role\":\"user\",\"content\":\"";
    for (char c : prompt) {
        if (c == '"') json << "\\\"";
        else if (c == '\\') json << "\\\\";
        else if (c == '\n') json << "\\n";
        else if (c < 0x20) { json << "\\u00" << std::hex << (int)c; }
        else json << c;
    }
    json << "\"}]}";
    return json.str();
}

std::string OllamaClient::extractContent(const std::string& body) {
    size_t pos = body.find("\"content\":");
    if (pos == std::string::npos) return "";
    pos = body.find('\"', pos + 11);
    if (pos == std::string::npos) return "";
    pos++;
    size_t end = pos;
    while (end < body.size()) {
        if (body[end] == '\\') { end += 2; continue; }
        if (body[end] == '"') break;
        end++;
    }
    std::string content = body.substr(pos, end - pos);
    std::string result;
    for (size_t i = 0; i < content.size(); i++) {
        if (content[i] == '\\' && i + 1 < content.size()) {
            switch (content[i + 1]) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                default: result += content[i]; result += content[i + 1]; break;
            }
            i++;
        } else {
            result += content[i];
        }
    }
    return result;
}

int OllamaClient::extractTokens(const std::string& body, const std::string& field) {
    size_t pos = body.find("\"" + field + "\"");
    if (pos == std::string::npos) return 0;
    pos = body.find(':', pos + field.size() + 2);
    if (pos == std::string::npos) return 0;
    pos++;
    size_t end = pos;
    while (end < body.size() && std::isdigit(body[end])) end++;
    try { return std::stoi(body.substr(pos, end - pos)); }
    catch (...) { return 0; }
}

LLMResponse OllamaClient::sendPrompt(const std::string& prompt, const std::string& systemInstruction) {
    LLMResponse resp;
    if (!isAvailable()) { resp.error = "Base URL not set"; return resp; }

    auto start = std::chrono::steady_clock::now();

    std::string url = config.baseUrl + "/api/chat";
    std::string body = buildRequestBody(prompt, systemInstruction);

    HttpHeaderMap headers;
    headers["Content-Type"] = "application/json";

    HttpResponse httpResp;
    for (int attempt = 0; attempt <= config.maxRetries; attempt++) {
        httpResp = http.post(url, body, "application/json", headers);
        if (httpResp.success) break;
        if (httpResp.statusCode >= 500) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500 * (attempt + 1)));
            continue;
        }
        break;
    }

    auto elapsed = std::chrono::steady_clock::now() - start;
    float latencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    resp.statusCode = httpResp.statusCode;

    if (!httpResp.success && httpResp.error.empty()) {
        resp.error = "HTTP error " + std::to_string(httpResp.statusCode);
    } else if (httpResp.error.empty()) {
        resp.content = extractContent(httpResp.body);
        resp.success = !resp.content.empty();
        resp.promptTokens = extractTokens(httpResp.body, "prompt_eval_count");
        resp.completionTokens = extractTokens(httpResp.body, "eval_count");
        resp.finishReason = "stop";
    } else {
        resp.error = httpResp.error;
    }

    reportMetrics("Ollama", resp.promptTokens + resp.completionTokens, latencyMs);
    return resp;
}

} // namespace OpenMind
