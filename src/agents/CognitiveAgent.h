#pragma once

#include "AgentCognitive.h"
#include "MemorySystem.h"
#include "GoalManager.h"
#include "PerceptionSystem.h"
#include "AgentToolCalling.h"
#include "AgentCommunication.h"
#include "LLMInterface.h"
#include "VoxelOctree.h"
#include "PhysicsEngine.h"
#include <string>
#include <sstream>

namespace OpenMind {

class CognitiveAgent {
public:
    CognitiveAgent() = default;

    void init(int agentId, const std::string& agentName, AgentRole agentRole,
              LLMInterface* llmPtr, VoxelOctree* worldPtr, PhysicsEngine* enginePtr,
              const CognitiveConfig& cfg = CognitiveConfig()) {
        id = agentId;
        name = agentName;
        role = agentRole;
        llm = llmPtr;
        config = cfg;

        memory.setConfig(cfg);
        tools.setWorld(worldPtr);
        tools.setEngine(enginePtr);
        perception.setWorld(worldPtr);
        perception.setEngine(enginePtr);

        posX = 10.0f + (float)(agentId % 10);
        posY = 5.0f;
        posZ = 10.0f + (float)(agentId / 10);
    }

    void addGoal(GoalType type, const std::string& desc, float priority, uint64_t tick) {
        goals.addGoal(type, desc, priority, tick);
    }

    void tick(uint64_t currentTick) {
        tickCount++;

        if (health <= 0) { isAlive = false; return; }
        hunger = std::min(100.0f, hunger + config.hungerRate * 10);
        energy = std::max(0.0f, energy - 0.01f);
        if (hunger > 80) health -= 0.1f;
        if (energy <= 0) health -= 0.05f;

        if (tickCount % config.thinkInterval != 0) return;

        Observation obs = perceive();
        memory.store("Saw: " + obs.description, MemoryType::OBSERVATION, 0.3f, currentTick,
                     posX, posY, posZ);

        AgentAction action = think(obs, currentTick);
        act(action, currentTick);

        memory.decayAll();
    }

    Observation perceive() const {
        return perception.observe(posX, posY, posZ, config.visionRange, id, temperature);
    }

    AgentAction think(const Observation& obs, uint64_t currentTick) {
        AgentAction action;
        action.type = "idle";

        if (!llm || !llm->isAvailable()) {
            action.reasoning = "No LLM available, defaulting to idle";
            return action;
        }

        Goal* activeGoal = goals.getActiveGoal();
        std::string goalDesc = activeGoal ?
            GoalManager::goalTypeToString(activeGoal->type) + ": " + activeGoal->description :
            "No active goal";

        std::string memCtx = memory.buildContextString(5);

        std::ostringstream prompt;
        prompt << perception.formatObservationForLLM(obs, posX, posY, posZ, name, roleToString()) << "\n";
        prompt << "Current goal: " << goalDesc << "\n";
        prompt << "Recent memories:\n" << memCtx << "\n";
        prompt << "Health: " << (int)health << "/100, Hunger: " << (int)hunger << "/100, Energy: " << (int)energy << "/100\n\n";
        prompt << tools.getToolDescriptions() << "\n";
        prompt << "Choose ONE tool to call. Respond with valid JSON:\n";
        prompt << R"({"tool":"tool_name","args":["arg1","arg2"],"reasoning":"why"})";

        std::string system = "You are " + name + ", a " + roleToString() + " in a voxel world. "
                             "Pick exactly one tool call. Respond ONLY with valid JSON.";

        LLMResponse resp = llm->sendPrompt(prompt.str(), system);

        if (resp.success) {
            action = parseAction(resp.content);
            action.reasoning = resp.content.substr(0, 200);
        } else {
            action.reasoning = "LLM failed: " + resp.error;
        }

        return action;
    }

    void act(const AgentAction& action, uint64_t currentTick) {
        lastAction = action;

        if (action.type == "idle") {
            memory.store("Idle - nothing to do", MemoryType::ACTION, 0.1f, currentTick, posX, posY, posZ);
            return;
        }

        if (action.type == "move_to") {
            ToolResult r = tools.moveTo(posX, posY, posZ, action.targetX, action.targetY, action.targetZ);
            memory.store("Moved toward target: " + r.output, MemoryType::ACTION, 0.4f, currentTick, posX, posY, posZ);
        }
        else if (action.type == "place_block") {
            ToolResult r = tools.placeBlock(action.targetX, action.targetY, action.targetZ, action.blockType);
            memory.store("Placed block: " + r.output, MemoryType::ACTION, 0.5f, currentTick, posX, posY, posZ);
            if (r.success) {
                Goal* g = goals.getActiveGoal();
                if (g && g->type == GoalType::BUILD_SHELTER) {
                    g->progress = std::min(1.0f, g->progress + 0.1f);
                    if (g->progress >= 1.0f) {
                        goals.completeGoal(g->id);
                        memory.store("Completed building goal!", MemoryType::GOAL_UPDATE, 0.9f, currentTick);
                    }
                }
            }
        }
        else if (action.type == "break_block") {
            ToolResult r = tools.breakBlock(action.targetX, action.targetY, action.targetZ);
            memory.store("Broke block: " + r.output, MemoryType::ACTION, 0.4f, currentTick, posX, posY, posZ);
        }
        else if (action.type == "search_for") {
            std::string resource = action.blockType.empty() ? "STONE" : action.blockType;
            ToolResult r = tools.searchFor(posX, posY, posZ, resource);
            memory.store("Search: " + r.output, MemoryType::DISCOVERY, 0.6f, currentTick, posX, posY, posZ);
        }
        else if (action.type == "talk_to") {
            memory.store("Talked to agent " + std::to_string(action.targetAgentId),
                        MemoryType::CONVERSATION, 0.5f, currentTick, posX, posY, posZ);
        }
        else if (action.type == "build_structure") {
            std::string sType = action.blockType.empty() ? "house" : action.blockType;
            ToolResult r = tools.buildStructure(sType, action.targetX, action.targetY, action.targetZ, 4);
            memory.store("Built structure: " + r.output, MemoryType::ACTION, 0.7f, currentTick, posX, posY, posZ);
            Goal* g = goals.getActiveGoal();
            if (g && g->type == GoalType::BUILD_SHELTER) {
                goals.completeGoal(g->id);
                memory.store("Completed building goal!", MemoryType::GOAL_UPDATE, 0.9f, currentTick);
            }
        }
        else if (action.type == "craft") {
            ToolResult r = tools.craft(action.blockType);
            memory.store("Crafted: " + r.output, MemoryType::ACTION, 0.5f, currentTick, posX, posY, posZ);
        }
    }

    Conversation communicate(int targetId, const std::string& targetName,
                             const std::string& topic, uint64_t tick) {
        Conversation conv = comm.startConversation(id, name, targetId, targetName, topic, llm, tick);
        comm.recordConversation(conv, memory, tick);
        return conv;
    }

    int getId() const { return id; }
    std::string getName() const { return name; }
    AgentRole getRole() const { return role; }
    bool getIsAlive() const { return isAlive; }
    float getHealth() const { return health; }
    float getHunger() const { return hunger; }
    float getEnergy() const { return energy; }
    float getX() const { return posX; }
    float getY() const { return posY; }
    float getZ() const { return posZ; }
    const AgentAction& getLastAction() const { return lastAction; }
    MemorySystem& getMemory() { return memory; }
    GoalManager& getGoals() { return goals; }
    AgentToolCalling& getTools() { return tools; }

    void setPosition(float x, float y, float z) { posX = x; posY = y; posZ = z; }
    void setHealth(float h) { health = h; }
    void setHunger(float h) { hunger = h; }
    void setEnergy(float e) { energy = e; }

    std::string statusString() const {
        std::ostringstream ss;
        ss << name << " (" << roleToString() << ") "
           << "HP:" << (int)health << " HNG:" << (int)hunger << " NRG:" << (int)energy
           << " @(" << (int)posX << "," << (int)posY << "," << (int)posZ << ")"
           << " goals:" << goals.activeCount()
           << " mem:" << memory.shortTermCount() << "/" << memory.longTermCount();
        return ss.str();
    }

private:
    AgentAction parseAction(const std::string& json) {
        AgentAction action;
        action.type = "idle";

        auto extract = [&](const std::string& key) -> std::string {
            std::string search = "\"" + key + "\"";
            size_t pos = json.find(search);
            if (pos == std::string::npos) return "";
            pos = json.find(':', pos + search.size());
            if (pos == std::string::npos) return "";
            pos++;
            while (pos < json.size() && json[pos] == ' ') pos++;
            if (pos < json.size() && json[pos] == '"') {
                pos++;
                size_t end = json.find('"', pos);
                if (end != std::string::npos) return json.substr(pos, end - pos);
            }
            return "";
        };

        std::string tool = extract("tool");
        if (tool.empty()) tool = extract("action");
        if (tool.empty()) return action;

        action.type = tool;

        size_t argsPos = json.find("\"args\"");
        if (argsPos != std::string::npos) {
            size_t arrStart = json.find('[', argsPos);
            size_t arrEnd = json.find(']', arrStart);
            if (arrStart != std::string::npos && arrEnd != std::string::npos) {
                std::string arr = json.substr(arrStart + 1, arrEnd - arrStart - 1);
                std::vector<std::string> args;
                size_t p = 0;
                while ((p = arr.find('"', p)) != std::string::npos) {
                    p++;
                    size_t end = arr.find('"', p);
                    if (end != std::string::npos) {
                        args.push_back(arr.substr(p, end - p));
                        p = end + 1;
                    } else break;
                }

                if (tool == "move_to" && args.size() >= 3) {
                    action.targetX = safeStoi(args[0]);
                    action.targetY = safeStoi(args[1]);
                    action.targetZ = safeStoi(args[2]);
                }
                else if (tool == "place_block" && args.size() >= 4) {
                    action.targetX = safeStoi(args[0]);
                    action.targetY = safeStoi(args[1]);
                    action.targetZ = safeStoi(args[2]);
                    action.blockType = args[3];
                }
                else if (tool == "break_block" && args.size() >= 3) {
                    action.targetX = safeStoi(args[0]);
                    action.targetY = safeStoi(args[1]);
                    action.targetZ = safeStoi(args[2]);
                }
                else if (tool == "search_for" && args.size() >= 1) {
                    action.blockType = args[0];
                }
                else if (tool == "talk_to" && args.size() >= 2) {
                    action.targetAgentId = safeStoi(args[0]);
                    action.message = args[1];
                }
                else if (tool == "build_structure" && args.size() >= 5) {
                    action.blockType = args[0];
                    action.targetX = safeStoi(args[1]);
                    action.targetY = safeStoi(args[2]);
                    action.targetZ = safeStoi(args[3]);
                }
                else if (tool == "craft" && args.size() >= 1) {
                    action.blockType = args[0];
                }
            }
        }

        return action;
    }

    std::string roleToString() const {
        switch (role) {
            case AgentRole::BUILDER: return "builder";
            case AgentRole::FARMER: return "farmer";
            case AgentRole::MERCHANT: return "merchant";
            case AgentRole::EXPLORER: return "explorer";
            case AgentRole::WARRIOR: return "warrior";
            case AgentRole::CRAFTSMAN: return "craftsman";
            case AgentRole::GENERIC: return "villager";
        }
        return "villager";
    }

    int id = 0;
    std::string name = "Agent";
    AgentRole role = AgentRole::GENERIC;
    LLMInterface* llm = nullptr;
    CognitiveConfig config;

    float posX = 0, posY = 0, posZ = 0;
    float health = 100.0f;
    float hunger = 0.0f;
    float energy = 100.0f;
    float temperature = 293.15f;
    bool isAlive = true;

    uint64_t tickCount = 0;

    MemorySystem memory;
    GoalManager goals;
    PerceptionSystem perception;
    AgentToolCalling tools;
    AgentCommunication comm;

    AgentAction lastAction;
};

} // namespace OpenMind
