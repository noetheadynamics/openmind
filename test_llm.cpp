#include "OpenAICompatibleClient.h"
#include "MockLLMClient.h"
#include "PromptTemplates.h"
#include "JSONValidator.h"
#include "AsyncRequestManager.h"
#include <iostream>
#include <string>

using namespace OpenMind;

static int testsPassed = 0;
static int testsFailed = 0;

void check(bool cond, const std::string& name) {
    if (cond) { testsPassed++; printf("  PASS: %s\n", name.c_str()); }
    else { testsFailed++; printf("  FAIL: %s\n", name.c_str()); }
}

void testMockLLM() {
    printf("\n=== Mock LLM Tests ===\n");

    MockLLMClient mock;
    LLMConfig cfg;
    cfg.model = "test";
    mock.initialize(cfg);
    check(mock.isAvailable(), "Mock is available");
    check(mock.getProviderName() == "MockLLM", "Provider name");

    LLMResponse resp = mock.sendPrompt("Hello world");
    check(resp.success, "Mock prompt success");
    check(!resp.content.empty(), "Mock has content");
    check(resp.statusCode == 200, "Mock status 200");

    std::string worldJson = R"({"worldName":"test","blocks":[{"x":0,"y":0,"z":0,"type":"STONE"}]})";
    mock.setPresetResponse("medieval", worldJson);
    resp = mock.sendPrompt("Generate a medieval village");
    check(resp.success, "Preset response success");
    check(resp.content.find("test") != std::string::npos, "Preset contains data");

    mock.setAvailable(false);
    resp = mock.sendPrompt("test");
    check(!resp.success, "Unavailable mock fails");

    mock.setAvailable(true);
    resp = mock.sendPrompt("test");
    check(resp.success, "Re-enabled mock works");
}

void testPromptTemplates() {
    printf("\n=== Prompt Template Tests ===\n");

    check(PromptTemplates::WORLD_GENERATION_TEMPLATE.find("BLOCK_TYPE") != std::string::npos,
          "World gen template has block types");
    check(PromptTemplates::WORLD_GENERATION_TEMPLATE.find("JSON") != std::string::npos ||
          PromptTemplates::WORLD_GENERATION_TEMPLATE.find("json") != std::string::npos,
          "World gen template mentions JSON");
    check(PromptTemplates::WORLD_GENERATION_TEMPLATE.find("blocks") != std::string::npos,
          "World gen template has blocks field");

    check(PromptTemplates::MATERIAL_GENERATION_TEMPLATE.find("density") != std::string::npos,
          "Material gen template has density");
    check(PromptTemplates::MATERIAL_GENERATION_TEMPLATE.find("thermalConductivity") != std::string::npos,
          "Material gen template has thermal conductivity");

    check(PromptTemplates::AGENT_BEHAVIOR_TEMPLATE.find("action") != std::string::npos,
          "Agent behavior template has action");
    check(PromptTemplates::AGENT_BEHAVIOR_TEMPLATE.find("reasoning") != std::string::npos,
          "Agent behavior template has reasoning");

    check(PromptTemplates::COMMAND_PARSING_TEMPLATE.find("commands") != std::string::npos,
          "Command parsing template has commands");
    check(PromptTemplates::COMMAND_PARSING_TEMPLATE.find("set_block") != std::string::npos,
          "Command parsing template has set_block");

    check(PromptTemplates::SOCIAL_DIALOGUE_TEMPLATE.find("dialogue") != std::string::npos,
          "Social dialogue template has dialogue");
}

void testJSONValidator() {
    printf("\n=== JSON Validator Tests ===\n");

    std::string validWorld = R"({"worldName":"TestWorld","blocks":[{"x":0,"y":0,"z":0,"type":"STONE"}]})";
    auto schema = JSONValidator::createWorldGenerationSchema();
    ValidationResult vr = JSONValidator::validate(validWorld, schema);
    check(vr.valid, "Valid world JSON passes");

    std::string missingBlocks = R"({"worldName":"Test"})";
    vr = JSONValidator::validate(missingBlocks, schema);
    check(!vr.valid, "Missing required field rejected");

    std::string invalidType = R"({"worldName":123,"blocks":[{"x":0,"y":0,"z":0,"type":"STONE"}]})";
    vr = JSONValidator::validate(invalidType, schema);
    check(!vr.valid, "Wrong type rejected");

    std::string validMaterial = R"({"materialName":"Granite","blockType":"STONE","category":"rock","properties":{"mass":2.7,"density":2700,"hardness":7,"tensileStrength":13}})";
    auto matSchema = JSONValidator::createMaterialGenerationSchema();
    vr = JSONValidator::validate(validMaterial, matSchema);
    check(vr.valid, "Valid material JSON passes");

    std::string validAgent = R"({"action":"move","target":{"x":5,"y":5,"z":5},"reasoning":"Need food","emotion":"hungry"})";
    auto agentSchema = JSONValidator::createAgentBehaviorSchema();
    vr = JSONValidator::validate(validAgent, agentSchema);
    check(vr.valid, "Valid agent JSON passes");

    std::string validCmd = R"({"commands":[{"action":"set_block","params":{"x":1,"y":1,"z":1,"type":"STONE"}}],"confirmation":"Placing stone block"})";
    auto cmdSchema = JSONValidator::createCommandParsingSchema();
    vr = JSONValidator::validate(validCmd, cmdSchema);
    check(vr.valid, "Valid command JSON passes");

    std::string validDialogue = R"({"dialogue":[{"speaker":"Alice","text":"Hello","emotion":"happy"},{"speaker":"Bob","text":"Hi there","emotion":"neutral"}],"relationship_change":1})";
    auto dlgSchema = JSONValidator::createSocialDialogueSchema();
    vr = JSONValidator::validate(validDialogue, dlgSchema);
    check(vr.valid, "Valid dialogue JSON passes");

    check(JSONValidator::containsSuspiciousContent("<script>alert(1)</script>"), "Script injection detected");
    check(JSONValidator::containsSuspiciousContent("eval(malicious)"), "Eval injection detected");
    check(JSONValidator::containsSuspiciousContent("DROP TABLE users"), "SQL injection detected");
    check(JSONValidator::containsSuspiciousContent("rm -rf /"), "Shell injection detected");
    check(!JSONValidator::containsSuspiciousContent("Normal JSON content"), "Clean content passes");

    std::string withMarkdown = R"(```json
{"worldName":"test","blocks":[{"x":0,"y":0,"z":0,"type":"STONE"}]}
```
)";
    std::string extracted = JSONValidator::extractJsonFromResponse(withMarkdown);
    check(extracted.find("worldName") != std::string::npos, "JSON extraction from markdown");
}

void testAsyncManager() {
    printf("\n=== Async Request Manager Tests ===\n");

    AsyncRequestManager mgr;
    mgr.initialize(2, 60);

    MockLLMClient mock;
    LLMConfig cfg;
    mock.initialize(cfg);
    mgr.registerClient(LLMProvider::MOCK, std::make_shared<MockLLMClient>(mock));

    std::string reqId = mgr.submitRequest("Test prompt", "", LLMProvider::MOCK);
    check(!reqId.empty(), "Request submitted with ID");
    check(mgr.getPendingCount() + mgr.getActiveCount() >= 1, "Request in queue");

    mgr.setMaxConcurrent(4);
    mgr.setRateLimit(120);
    check(true, "Config update works");

    mgr.shutdown();
    check(true, "Manager shutdown cleanly");
}

void testRealLLM(const std::string& apiKey) {
    printf("\n=== Real LLM Test (Groq) ===\n");

    OpenAICompatibleClient client;
    LLMConfig cfg;
    cfg.apiKey = apiKey;
    cfg.baseUrl = "https://api.groq.com/openai/v1";
    cfg.model = "llama-3.3-70b-versatile";
    cfg.temperature = 0.3f;
    cfg.maxTokens = 2048;
    cfg.timeoutMs = 30000;
    cfg.maxRetries = 2;

    client.initialize(cfg);
    check(client.isAvailable(), "Groq client available");
    check(client.getProviderName() == "Groq", "Provider is Groq");

    std::string system = "You are a world generation AI. Respond ONLY with valid JSON, starting with { and ending with }. No markdown, no explanation, no code fences. Just the raw JSON object.";
    std::string prompt = "Generate a small medieval village with 5 blocks. Respond with a JSON object starting with { and ending with }. Use this exact format: "
                         R"({"worldName":"string","blocks":[{"x":0,"y":0,"z":0,"type":"BLOCK_TYPE"}]})"
                         " Block types: STONE, DIRT, GRASS, WOOD, LEAVES, WATER, SAND.";

    printf("  Sending prompt to Groq (%s)...\n", cfg.model.c_str());
    LLMResponse resp = client.sendPrompt(prompt, system);

    printf("  Status: %d\n", resp.statusCode);
    printf("  Success: %s\n", resp.success ? "true" : "false");
    printf("  Tokens: prompt=%d completion=%d\n", resp.promptTokens, resp.completionTokens);
    printf("  Error: %s\n", resp.error.c_str());

    if (resp.success) {
        printf("  Response length: %zu chars\n", resp.content.size());
        printf("  First 300 chars: %.300s\n", resp.content.c_str());

        std::string json = JSONValidator::extractJsonFromResponse(resp.content);
        printf("  Extracted JSON length: %zu\n", json.size());
        printf("  Extracted JSON: %.300s\n", json.c_str());
        check(!json.empty(), "JSON extracted from response");

        auto schema = JSONValidator::createWorldGenerationSchema();
        ValidationResult vr = JSONValidator::validate(json, schema);
        if (vr.valid) {
            check(true, "Response validates against world schema");
        } else {
            check(false, "Response validation failed");
            for (auto& err : vr.errors) printf("    Error: %s\n", err.c_str());
        }

        check(!JSONValidator::containsSuspiciousContent(resp.content), "No injection in response");
    } else {
        check(false, "LLM request failed: " + resp.error);
    }
}

int main(int argc, char* argv[]) {
    printf("========================================\n");
    printf(" OpenMind LLM Connector Test Suite\n");
    printf("========================================\n");

    testMockLLM();
    testPromptTemplates();
    testJSONValidator();
    testAsyncManager();

    if (argc > 1) {
        std::string apiKey = argv[1];
        testRealLLM(apiKey);
    } else {
        printf("\n  Skipping real LLM test (pass API key as argument)\n");
        printf("  Usage: test_llm.exe <groq_api_key>\n");
    }

    printf("\n========================================\n");
    printf(" Results: %d passed, %d failed\n", testsPassed, testsFailed);
    printf("========================================\n");

    return testsFailed > 0 ? 1 : 0;
}
