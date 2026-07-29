#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <functional>

namespace OpenMind {

enum class MemoryType {
    OBSERVATION,
    ACTION,
    CONVERSATION,
    DISCOVERY,
    EMOTION,
    GOAL_UPDATE
};

enum class GoalType {
    SURVIVE,
    BUILD_SHELTER,
    FIND_FOOD,
    EXPLORE,
    TRADE,
    CRAFT,
    SOCIALIZE,
    CUSTOM
};

enum class GoalStatus {
    ACTIVE,
    COMPLETED,
    FAILED,
    PAUSED
};

enum class AgentRole {
    BUILDER,
    FARMER,
    MERCHANT,
    EXPLORER,
    WARRIOR,
    CRAFTSMAN,
    GENERIC
};

struct Memory {
    int id = 0;
    MemoryType type = MemoryType::OBSERVATION;
    std::string content;
    float importance = 0.5f;
    uint64_t tick = 0;
    int relatedGoalId = -1;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float decayRate = 0.01f;
    int accessCount = 0;
};

struct Goal {
    int id = 0;
    GoalType type = GoalType::CUSTOM;
    GoalStatus status = GoalStatus::ACTIVE;
    std::string description;
    float priority = 0.5f;
    float progress = 0.0f;
    int targetX = 0;
    int targetY = 0;
    int targetZ = 0;
    std::string targetResource;
    std::vector<int> subGoalIds;
    int parentGoalId = -1;
    uint64_t createdTick = 0;
    uint64_t deadlineTick = 0;
};

struct ToolCall {
    std::string toolName;
    std::vector<std::string> args;
    std::string result;
    bool success = false;
};

struct BlockPosition {
    int x = 0, y = 0, z = 0;
};

struct Observation {
    std::string description;
    std::vector<BlockPosition> visibleBlocks;
    std::vector<int> visibleAgents;
    float nearestThreatDistance = 999.0f;
    float nearestFoodDistance = 999.0f;
    float temperature = 293.15f;
    float lightLevel = 1.0f;
    bool hasShelter = false;
};

struct AgentAction {
    std::string type;
    int targetX = 0;
    int targetY = 0;
    int targetZ = 0;
    std::string blockType;
    std::string message;
    int targetAgentId = -1;
    float intensity = 1.0f;
    std::string reasoning;
};

struct CognitiveConfig {
    int maxShortTermMemory = 20;
    int maxLongTermMemory = 500;
    float visionRange = 10.0f;
    float memoryDecayRate = 0.01f;
    float importanceThreshold = 0.3f;
    int thinkInterval = 1;
    float temperature = 0.3f;
    int maxTokens = 512;
};

} // namespace OpenMind
