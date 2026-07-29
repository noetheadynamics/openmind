#include "JSONValidator.h"
#include <algorithm>
#include <sstream>
#include <cctype>
#include <cmath>

namespace OpenMind {

std::string JSONValidator::trimWhitespace(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

bool JSONValidator::hasField(const std::string& json, const std::string& field) {
    std::string search = "\"" + field + "\"";
    return json.find(search) != std::string::npos;
}

std::string JSONValidator::getFieldRaw(const std::string& json, const std::string& field) {
    std::string search = "\"" + field + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return "";
    pos++;
    size_t end = pos;
    int depth = 0;
    bool inString = false;
    while (end < json.size()) {
        char c = json[end];
        if (inString) {
            if (c == '\\') { end += 2; continue; }
            if (c == '"') inString = false;
        } else {
            if (c == '"') inString = true;
            else if (c == '{' || c == '[') depth++;
            else if (c == '}' || c == ']') { if (depth == 0) break; depth--; }
            else if (c == ',' && depth == 0) break;
        }
        end++;
    }
    return trimWhitespace(json.substr(pos, end - pos));
}

bool JSONValidator::isArray(const std::string& json) {
    std::string t = trimWhitespace(json);
    return !t.empty() && t.front() == '[';
}

int JSONValidator::arraySize(const std::string& json) {
    std::string t = trimWhitespace(json);
    if (t.empty() || t.front() != '[') return 0;
    if (t.size() <= 2) return 0;
    int count = 1;
    bool inString = false;
    int depth = 0;
    for (size_t i = 1; i < t.size() - 1; i++) {
        char c = t[i];
        if (inString) { if (c == '\\') { i++; continue; } if (c == '"') inString = false; }
        else {
            if (c == '"') inString = true;
            else if (c == '{' || c == '[') depth++;
            else if (c == '}' || c == ']') depth--;
            else if (c == ',' && depth == 0) count++;
        }
    }
    return count;
}

std::string JSONValidator::arrayGet(const std::string& json, int index) {
    std::string t = trimWhitespace(json);
    if (t.empty() || t.front() != '[') return "";
    int count = 0;
    size_t start = 1;
    bool inString = false;
    int depth = 0;
    for (size_t i = 1; i < t.size(); i++) {
        char c = t[i];
        if (inString) {
            if (c == '\\') { i++; continue; }
            if (c == '"') inString = false;
        } else {
            if (c == '"') inString = true;
            else if (c == '{' || c == '[') depth++;
            else if (c == '}' || c == ']') depth--;
            else if (c == ',' && depth == 0) {
                if (count == index) return trimWhitespace(t.substr(start, i - start));
                count++;
                start = i + 1;
            }
        }
    }
    if (count == index) return trimWhitespace(t.substr(start, t.size() - 1 - start));
    return "";
}

bool JSONValidator::containsSuspiciousContent(const std::string& json) {
    std::string lower = json;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    auto hasPattern = [&](const std::string& pat) -> bool {
        size_t pos = 0;
        while ((pos = lower.find(pat, pos)) != std::string::npos) {
            if (pos > 0) {
                char before = lower[pos - 1];
                if (std::isalnum(before) || before == '_') {
                    pos += pat.size();
                    continue;
                }
            }
            return true;
        }
        return false;
    };

    if (hasPattern("<script")) return true;
    if (hasPattern("javascript:")) return true;
    if (hasPattern("onerror=")) return true;
    if (hasPattern("onload=")) return true;
    if (hasPattern("eval(")) return true;
    if (hasPattern("exec(")) return true;
    if (hasPattern("system(")) return true;
    if (hasPattern("popen(")) return true;
    if (lower.find("import os") != std::string::npos) return true;
    if (lower.find("import subprocess") != std::string::npos) return true;
    if (lower.find("__import__") != std::string::npos) return true;
    if (lower.find("rm -rf") != std::string::npos) return true;
    if (lower.find("format c:") != std::string::npos) return true;
    if (lower.find("del /f") != std::string::npos) return true;
    if (lower.find("drop table") != std::string::npos) return true;
    if (lower.find("delete from") != std::string::npos) return true;
    if (lower.find("insert into") != std::string::npos) return true;
    if (hasPattern("${")) return true;
    if (lower.find("\\x00") != std::string::npos) return true;
    return false;
}

std::string JSONValidator::extractJsonFromResponse(const std::string& response) {
    std::string trimmed = trimWhitespace(response);
    if (trimmed.empty()) return trimmed;

    // Case 1: proper JSON
    if (trimmed.front() == '{' || trimmed.front() == '[') {
        return trimmed;
    }

    // Case 2: missing opening brace/bracket
    if (trimmed.back() == '}') {
        std::string result = "{" + trimmed;
        // Fix missing key quotes: {key":value -> {"key":value
        if (result.size() > 2 && result[1] != '"') {
            size_t keyEnd = result.find("\":");
            if (keyEnd != std::string::npos && keyEnd > 1) {
                size_t keyStart = 1;
                result = result.substr(0, keyStart) + "\"" + result.substr(keyStart);
                keyEnd++;
                result = result.substr(0, keyEnd + 1) + "\"" + result.substr(keyEnd + 1);
            }
        }
        return result;
    }
    if (trimmed.back() == ']') {
        if (trimmed.front() == '[') return trimmed;
        return "[" + trimmed + "]";
    }

    return trimmed;
}

ValidationResult JSONValidator::sanitize(const std::string& json) {
    ValidationResult result;
    result.sanitizedOutput = extractJsonFromResponse(json);

    if (containsSuspiciousContent(json)) {
        result.valid = false;
        result.errors.push_back("Suspicious/malicious content detected in response");
    }
    return result;
}

ValidationResult JSONValidator::validate(const std::string& json, const JsonSchema& schema) {
    ValidationResult result = sanitize(json);
    if (!result.valid) return result;

    ValidationResult vr = validateValue(result.sanitizedOutput, schema, "$");
    result.errors = vr.errors;
    result.valid = vr.valid;
    return result;
}

ValidationResult JSONValidator::validateValue(const std::string& json, const JsonSchema& schema,
                                              const std::string& path) {
    ValidationResult result;
    std::string t = trimWhitespace(json);

    switch (schema.type) {
    case SchemaType::OBJECT: {
        if (t.empty() || t.front() != '{') {
            result.valid = false;
            result.errors.push_back(path + ": Expected object");
            return result;
        }
        for (auto& req : schema.required) {
            if (!hasField(t, req)) {
                result.valid = false;
                result.errors.push_back(path + ": Missing required field '" + req + "'");
            }
        }
        for (auto& [fieldName, fieldSchema] : schema.properties) {
            if (hasField(t, fieldName)) {
                std::string fieldVal = getFieldRaw(t, fieldName);
                ValidationResult fr = validateValue(fieldVal, fieldSchema, path + "." + fieldName);
                result.errors.insert(result.errors.end(), fr.errors.begin(), fr.errors.end());
                if (!fr.valid) result.valid = false;
            }
        }
        break;
    }
    case SchemaType::ARRAY: {
        if (!isArray(t)) {
            result.valid = false;
            result.errors.push_back(path + ": Expected array");
            return result;
        }
        int size = arraySize(t);
        if (size < schema.minLength) {
            result.valid = false;
            result.errors.push_back(path + ": Array too short");
        }
        if (schema.items && schema.items->type != SchemaType::NULL_TYPE) {
            for (int i = 0; i < size; i++) {
                std::string elem = arrayGet(t, i);
                ValidationResult er = validateValue(elem, *schema.items, path + "[" + std::to_string(i) + "]");
                result.errors.insert(result.errors.end(), er.errors.begin(), er.errors.end());
                if (!er.valid) result.valid = false;
            }
        }
        break;
    }
    case SchemaType::STRING: {
        if (t.size() >= 2 && t.front() == '"' && t.back() == '"') {
            std::string val = t.substr(1, t.size() - 2);
            if ((int)val.size() < schema.minLength) {
                result.valid = false;
                result.errors.push_back(path + ": String too short");
            }
            if ((int)val.size() > schema.maxLength) {
                result.valid = false;
                result.errors.push_back(path + ": String too long");
            }
        } else {
            result.valid = false;
            result.errors.push_back(path + ": Expected string");
        }
        break;
    }
    case SchemaType::NUMBER:
    case SchemaType::INTEGER: {
        if (t.empty()) {
            result.valid = false;
            result.errors.push_back(path + ": Expected number");
            return result;
        }
        char first = t.front();
        if (!std::isdigit(first) && first != '-' && first != '+') {
            result.valid = false;
            result.errors.push_back(path + ": Expected number");
            return result;
        }
        try {
            double val = std::stod(t);
            if (val < schema.minimum) {
                result.valid = false;
                result.errors.push_back(path + ": Value below minimum");
            }
            if (val > schema.maximum) {
                result.valid = false;
                result.errors.push_back(path + ": Value above maximum");
            }
        } catch (...) {
            result.valid = false;
            result.errors.push_back(path + ": Invalid number");
        }
        break;
    }
    case SchemaType::BOOLEAN: {
        if (t != "true" && t != "false") {
            result.valid = false;
            result.errors.push_back(path + ": Expected boolean");
        }
        break;
    }
    default:
        break;
    }
    return result;
}

JsonSchema JSONValidator::createWorldGenerationSchema() {
    JsonSchema schema(SchemaType::OBJECT);
    schema.required = {"worldName", "blocks"};
    schema.additionalProperties = false;

    schema.properties["worldName"] = JsonSchema(SchemaType::STRING);
    schema.properties["worldName"].minLength = 1;
    schema.properties["worldName"].maxLength = 256;

    schema.properties["description"] = JsonSchema(SchemaType::STRING);
    schema.properties["description"].maxLength = 1024;

    JsonSchema blockSchema(SchemaType::OBJECT);
    blockSchema.required = {"x", "y", "z", "type"};
    blockSchema.additionalProperties = false;
    blockSchema.properties["x"] = JsonSchema(SchemaType::INTEGER);
    blockSchema.properties["x"].minimum = 0;
    blockSchema.properties["x"].maximum = 255;
    blockSchema.properties["y"] = JsonSchema(SchemaType::INTEGER);
    blockSchema.properties["y"].minimum = 0;
    blockSchema.properties["y"].maximum = 255;
    blockSchema.properties["z"] = JsonSchema(SchemaType::INTEGER);
    blockSchema.properties["z"].minimum = 0;
    blockSchema.properties["z"].maximum = 255;
    blockSchema.properties["type"] = JsonSchema(SchemaType::STRING);
    blockSchema.properties["type"].minLength = 1;
    blockSchema.properties["type"].maxLength = 32;

    JsonSchema blockArray(SchemaType::ARRAY);
    blockArray.minLength = 1;
    blockArray.maxLength = 2000;
    blockArray.setItems(std::move(blockSchema));
    schema.properties["blocks"] = std::move(blockArray);

    return schema;
}

JsonSchema JSONValidator::createMaterialGenerationSchema() {
    JsonSchema schema(SchemaType::OBJECT);
    schema.required = {"materialName", "blockType", "properties"};
    schema.additionalProperties = false;

    schema.properties["materialName"] = JsonSchema(SchemaType::STRING);
    schema.properties["materialName"].minLength = 1;
    schema.properties["blockType"] = JsonSchema(SchemaType::STRING);
    schema.properties["category"] = JsonSchema(SchemaType::STRING);

    JsonSchema propsSchema(SchemaType::OBJECT);
    propsSchema.required = {"mass", "density", "hardness", "tensileStrength"};
    propsSchema.additionalProperties = false;
    propsSchema.properties["mass"] = JsonSchema(SchemaType::NUMBER);
    propsSchema.properties["mass"].minimum = 0.001;
    propsSchema.properties["mass"].maximum = 1e6;
    propsSchema.properties["density"] = JsonSchema(SchemaType::NUMBER);
    propsSchema.properties["density"].minimum = 0.01;
    propsSchema.properties["density"].maximum = 25000;
    propsSchema.properties["hardness"] = JsonSchema(SchemaType::NUMBER);
    propsSchema.properties["hardness"].minimum = 0;
    propsSchema.properties["hardness"].maximum = 10;
    propsSchema.properties["tensileStrength"] = JsonSchema(SchemaType::NUMBER);
    propsSchema.properties["tensileStrength"].minimum = 0;
    propsSchema.properties["tensileStrength"].maximum = 10000;
    propsSchema.properties["thermalConductivity"] = JsonSchema(SchemaType::NUMBER);
    propsSchema.properties["specificHeat"] = JsonSchema(SchemaType::NUMBER);
    propsSchema.properties["meltingPoint"] = JsonSchema(SchemaType::NUMBER);
    propsSchema.properties["boilingPoint"] = JsonSchema(SchemaType::NUMBER);
    propsSchema.properties["baseColor"] = JsonSchema(SchemaType::STRING);
    propsSchema.properties["composition"] = JsonSchema(SchemaType::STRING);

    schema.properties["properties"] = std::move(propsSchema);
    return schema;
}

JsonSchema JSONValidator::createAgentBehaviorSchema() {
    JsonSchema schema(SchemaType::OBJECT);
    schema.required = {"action", "reasoning"};
    schema.additionalProperties = false;

    schema.properties["action"] = JsonSchema(SchemaType::STRING);
    schema.properties["reasoning"] = JsonSchema(SchemaType::STRING);

    JsonSchema targetSchema(SchemaType::OBJECT);
    targetSchema.required = {"x", "y", "z"};
    targetSchema.properties["x"] = JsonSchema(SchemaType::INTEGER);
    targetSchema.properties["y"] = JsonSchema(SchemaType::INTEGER);
    targetSchema.properties["z"] = JsonSchema(SchemaType::INTEGER);
    schema.properties["target"] = std::move(targetSchema);

    schema.properties["intensity"] = JsonSchema(SchemaType::NUMBER);
    schema.properties["intensity"].minimum = 0;
    schema.properties["intensity"].maximum = 1;
    schema.properties["priority"] = JsonSchema(SchemaType::INTEGER);
    schema.properties["priority"].minimum = 1;
    schema.properties["priority"].maximum = 10;
    schema.properties["emotion"] = JsonSchema(SchemaType::STRING);

    return schema;
}

JsonSchema JSONValidator::createCommandParsingSchema() {
    JsonSchema schema(SchemaType::OBJECT);
    schema.required = {"commands"};
    schema.additionalProperties = false;

    JsonSchema cmdSchema(SchemaType::OBJECT);
    cmdSchema.required = {"action"};
    cmdSchema.properties["action"] = JsonSchema(SchemaType::STRING);

    JsonSchema cmdArray(SchemaType::ARRAY);
    cmdArray.minLength = 1;
    cmdArray.setItems(std::move(cmdSchema));
    schema.properties["commands"] = std::move(cmdArray);
    schema.properties["confirmation"] = JsonSchema(SchemaType::STRING);

    return schema;
}

JsonSchema JSONValidator::createSocialDialogueSchema() {
    JsonSchema schema(SchemaType::OBJECT);
    schema.required = {"dialogue"};
    schema.additionalProperties = false;

    JsonSchema lineSchema(SchemaType::OBJECT);
    lineSchema.required = {"speaker", "text"};
    lineSchema.properties["speaker"] = JsonSchema(SchemaType::STRING);
    lineSchema.properties["text"] = JsonSchema(SchemaType::STRING);
    lineSchema.properties["emotion"] = JsonSchema(SchemaType::STRING);

    JsonSchema dialogueArray(SchemaType::ARRAY);
    dialogueArray.minLength = 1;
    dialogueArray.maxLength = 20;
    dialogueArray.setItems(std::move(lineSchema));
    schema.properties["dialogue"] = std::move(dialogueArray);
    schema.properties["relationship_change"] = JsonSchema(SchemaType::NUMBER);
    schema.properties["relationship_change"].minimum = -10;
    schema.properties["relationship_change"].maximum = 10;

    return schema;
}

} // namespace OpenMind
