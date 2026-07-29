#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace OpenMind {

enum class SchemaType {
    OBJECT,
    ARRAY,
    STRING,
    NUMBER,
    INTEGER,
    BOOLEAN,
    NULL_TYPE
};

struct JsonSchema {
    SchemaType type = SchemaType::OBJECT;
    std::vector<std::string> required;
    std::map<std::string, JsonSchema> properties;
    std::unique_ptr<JsonSchema> items;
    std::vector<std::string> enumValues;
    std::string pattern;
    double minimum = -1e18;
    double maximum = 1e18;
    int minLength = 0;
    int maxLength = 2147483647;
    bool additionalProperties = true;

    JsonSchema() = default;
    JsonSchema(SchemaType t) : type(t) {}
    JsonSchema(const JsonSchema&) = delete;
    JsonSchema& operator=(const JsonSchema&) = delete;
    JsonSchema(JsonSchema&&) = default;
    JsonSchema& operator=(JsonSchema&&) = default;

    void setItems(JsonSchema itemSchema) {
        items = std::make_unique<JsonSchema>(std::move(itemSchema));
    }
};

struct ValidationResult {
    bool valid = true;
    std::vector<std::string> errors;
    std::string sanitizedOutput;
};

class JSONValidator {
public:
    static ValidationResult validate(const std::string& json, const JsonSchema& schema);
    static ValidationResult sanitize(const std::string& json);

    static JsonSchema createWorldGenerationSchema();
    static JsonSchema createMaterialGenerationSchema();
    static JsonSchema createAgentBehaviorSchema();
    static JsonSchema createCommandParsingSchema();
    static JsonSchema createSocialDialogueSchema();

    static bool containsSuspiciousContent(const std::string& json);
    static std::string extractJsonFromResponse(const std::string& response);

private:
    static ValidationResult validateValue(const std::string& json, const JsonSchema& schema,
                                          const std::string& path);
    static bool hasField(const std::string& json, const std::string& field);
    static std::string getFieldRaw(const std::string& json, const std::string& field);
    static bool isArray(const std::string& json);
    static int arraySize(const std::string& json);
    static std::string arrayGet(const std::string& json, int index);
    static std::string trimWhitespace(const std::string& s);
};

} // namespace OpenMind
