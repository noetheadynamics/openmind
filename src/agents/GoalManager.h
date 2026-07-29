#pragma once

#include "AgentCognitive.h"
#include <vector>
#include <algorithm>

namespace OpenMind {

class GoalManager {
public:
    GoalManager() = default;

    int addGoal(GoalType type, const std::string& description, float priority,
                uint64_t tick, int targetX = 0, int targetY = 0, int targetZ = 0,
                int parentGoalId = -1) {
        Goal g;
        g.id = nextGoalId++;
        g.type = type;
        g.description = description;
        g.priority = priority;
        g.createdTick = tick;
        g.targetX = targetX;
        g.targetY = targetY;
        g.targetZ = targetZ;
        g.parentGoalId = parentGoalId;
        g.status = GoalStatus::ACTIVE;
        goals.push_back(g);

        if (parentGoalId >= 0) {
            for (auto& pg : goals) {
                if (pg.id == parentGoalId) {
                    pg.subGoalIds.push_back(g.id);
                    break;
                }
            }
        }
        return g.id;
    }

    Goal* getActiveGoal() {
        Goal* best = nullptr;
        for (auto& g : goals) {
            if (g.status != GoalStatus::ACTIVE) continue;
            if (!best || g.priority > best->priority) {
                best = &g;
            }
        }
        return best;
    }

    void completeGoal(int goalId) {
        for (auto& g : goals) {
            if (g.id == goalId) {
                g.status = GoalStatus::COMPLETED;
                g.progress = 1.0f;
                break;
            }
        }
        cleanupCompleted();
    }

    void failGoal(int goalId) {
        for (auto& g : goals) {
            if (g.id == goalId) {
                g.status = GoalStatus::FAILED;
                break;
            }
        }
    }

    void updateProgress(int goalId, float progress) {
        for (auto& g : goals) {
            if (g.id == goalId) {
                g.progress = std::min(1.0f, std::max(0.0f, progress));
                break;
            }
        }
    }

    std::string describeGoals() const {
        std::string desc;
        for (auto& g : goals) {
            if (g.status != GoalStatus::ACTIVE) continue;
            desc += "- [" + goalTypeToString(g.type) + "] " + g.description
                    + " (priority=" + std::to_string(g.priority)
                    + ", progress=" + std::to_string((int)(g.progress * 100)) + "%)\n";
        }
        return desc;
    }

    void clear() { goals.clear(); }
    int activeCount() const {
        int c = 0;
        for (auto& g : goals) if (g.status == GoalStatus::ACTIVE) c++;
        return c;
    }
    const std::vector<Goal>& getAllGoals() const { return goals; }

    static std::string goalTypeToString(GoalType type) {
        switch (type) {
            case GoalType::SURVIVE: return "SURVIVE";
            case GoalType::BUILD_SHELTER: return "BUILD";
            case GoalType::FIND_FOOD: return "FOOD";
            case GoalType::EXPLORE: return "EXPLORE";
            case GoalType::TRADE: return "TRADE";
            case GoalType::CRAFT: return "CRAFT";
            case GoalType::SOCIALIZE: return "SOCIAL";
            case GoalType::CUSTOM: return "CUSTOM";
        }
        return "?";
    }

private:
    void cleanupCompleted() {
        for (auto& g : goals) {
            if (g.status == GoalStatus::COMPLETED) {
                for (auto& sub : g.subGoalIds) {
                    for (auto& sg : goals) {
                        if (sg.id == sub && sg.status == GoalStatus::ACTIVE) {
                            sg.parentGoalId = -1;
                            sg.priority = std::min(1.0f, sg.priority + 0.1f);
                        }
                    }
                }
            }
        }
    }

    std::vector<Goal> goals;
    int nextGoalId = 0;
};

} // namespace OpenMind
