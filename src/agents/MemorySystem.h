#pragma once

#include "AgentCognitive.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

namespace OpenMind {

class MemorySystem {
public:
    MemorySystem() = default;
    explicit MemorySystem(const CognitiveConfig& cfg) : config(cfg) {}

    int store(const std::string& content, MemoryType type, float importance,
              uint64_t tick, float x = 0, float y = 0, float z = 0, int goalId = -1) {
        Memory mem;
        mem.id = nextMemoryId++;
        mem.type = type;
        mem.content = content;
        mem.importance = importance;
        mem.tick = tick;
        mem.x = x;
        mem.y = y;
        mem.z = z;
        mem.relatedGoalId = goalId;
        mem.accessCount = 0;
        shortTerm.push_back(mem);

        if ((int)shortTerm.size() > config.maxShortTermMemory) {
            Memory oldest = shortTerm.front();
            shortTerm.erase(shortTerm.begin());
            if (oldest.importance >= config.importanceThreshold) {
                longTerm.push_back(oldest);
            }
        }
        return mem.id;
    }

    std::vector<Memory> retrieve(int count, const std::string& context = "") const {
        std::vector<Memory> all = shortTerm;
        all.insert(all.end(), longTerm.begin(), longTerm.end());

        for (auto& m : all) {
            float age = 1.0f / (1.0f + m.accessCount);
            m.importance *= age;
        }

        std::sort(all.begin(), all.end(), [](const Memory& a, const Memory& b) {
            return a.importance > b.importance;
        });

        if ((int)all.size() > count) all.resize(count);
        return all;
    }

    std::vector<Memory> retrieveRecent(int count) const {
        std::vector<Memory> result = shortTerm;
        if ((int)result.size() > count) result.resize(count);
        return result;
    }

    std::string buildContextString(int maxMemories = 10) const {
        auto recent = retrieve(maxMemories);
        std::string ctx;
        for (auto& m : recent) {
            ctx += memoryTypeToString(m.type) + ": " + m.content + "\n";
        }
        return ctx;
    }

    void decayAll() {
        for (auto it = shortTerm.begin(); it != shortTerm.end(); ) {
            it->importance -= config.memoryDecayRate;
            if (it->importance <= 0.0f) {
                it = shortTerm.erase(it);
            } else {
                ++it;
            }
        }
    }

    void summarizeLongTerm(const std::string& summary) {
        Memory mem;
        mem.id = nextMemoryId++;
        mem.type = MemoryType::GOAL_UPDATE;
        mem.content = "[SUMMARY] " + summary;
        mem.importance = 0.8f;
        longTerm.push_back(mem);

        if ((int)longTerm.size() > config.maxLongTermMemory) {
            longTerm.erase(longTerm.begin());
        }
    }

    void clear() { shortTerm.clear(); longTerm.clear(); }
    int shortTermCount() const { return (int)shortTerm.size(); }
    int longTermCount() const { return (int)longTerm.size(); }
    const std::vector<Memory>& getShortTerm() const { return shortTerm; }
    const std::vector<Memory>& getLongTerm() const { return longTerm; }

    void setConfig(const CognitiveConfig& cfg) { config = cfg; }

    static std::string memoryTypeToString(MemoryType type) {
        switch (type) {
            case MemoryType::OBSERVATION: return "OBS";
            case MemoryType::ACTION: return "ACT";
            case MemoryType::CONVERSATION: return "TALK";
            case MemoryType::DISCOVERY: return "FIND";
            case MemoryType::EMOTION: return "FEEL";
            case MemoryType::GOAL_UPDATE: return "GOAL";
        }
        return "?";
    }

private:
    CognitiveConfig config;
    std::vector<Memory> shortTerm;
    std::vector<Memory> longTerm;
    int nextMemoryId = 0;
};

} // namespace OpenMind
