#pragma once

#include "AgentCognitive.h"
#include "VoxelOctree.h"
#include "PhysicsEngine.h"
#include <string>
#include <sstream>
#include <cmath>

namespace OpenMind {

class PerceptionSystem {
public:
    PerceptionSystem() = default;

    void setWorld(VoxelOctree* w) { world = w; }
    void setEngine(PhysicsEngine* e) { engine = e; }

    Observation observe(float ax, float ay, float az, float visionRange,
                        int agentId, float temperature) const {
        Observation obs;
        obs.temperature = temperature;

        if (!world) return obs;

        int ix = (int)ax, iy = (int)ay, iz = (int)az;
        int range = (int)visionRange;

        int blockCount[64] = {};
        std::string blockNames[] = {"AIR","STONE","DIRT","GRASS","WATER","SAND","GLASS",
            "WOOD","LEAVES","IRON","COPPER","GOLD","STEEL","DIAMOND","COAL","BEDROCK",
            "ASH","TNT","SNOW"};

        for (int dx = -range; dx <= range; dx++) {
            for (int dy = -range; dy <= range; dy++) {
                for (int dz = -range; dz <= range; dz++) {
                    float dist = std::sqrt((float)(dx*dx + dy*dy + dz*dz));
                    if (dist > visionRange) continue;

                    int bx = ix + dx, by = iy + dy, bz = iz + dz;
                    VoxelData vd;
                    if (world->getBlock(bx, by, bz, vd) && vd.occupied) {
                        int idx = (int)vd.type;
                        if (idx >= 0 && idx < static_cast<int>(sizeof(blockNames)/sizeof(blockNames[0]))) blockCount[idx]++;
                        obs.visibleBlocks.push_back({bx, by, bz});
                    }
                }
            }
        }

        std::ostringstream desc;
        bool first = true;
        for (int i = 1; i < static_cast<int>(sizeof(blockNames)/sizeof(blockNames[0])); i++) {
            if (blockCount[i] > 0) {
                if (!first) desc << ", ";
                desc << blockCount[i] << " " << blockNames[i] << " blocks";
                first = false;
            }
        }
        if (first) desc << "empty space";

        if (engine) {
            const auto& agents = engine->getAgents();
            for (auto& a : agents) {
                if (a.id == agentId || !a.isAlive) continue;
                float dx = a.x - ax, dy = a.y - ay, dz = a.z - az;
                float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
                if (dist <= visionRange) {
                    obs.visibleAgents.push_back(a.id);
                    desc << ", agent " << a.id << " at distance " << (int)dist;
                }
            }
        }

        obs.description = desc.str();
        return obs;
    }

    std::string formatObservationForLLM(const Observation& obs, float ax, float ay, float az,
                                         const std::string& agentName, const std::string& role) const {
        std::ostringstream ss;
        ss << "You are " << agentName << ", a " << role << ".\n";
        ss << "Position: (" << (int)ax << ", " << (int)ay << ", " << (int)az << ")\n";
        ss << "Temperature: " << (int)(obs.temperature - 273.15) << "C\n";
        ss << "Nearby: " << obs.description << "\n";
        if (!obs.visibleAgents.empty()) {
            ss << "Visible agents: " << obs.visibleAgents.size() << "\n";
        }
        return ss.str();
    }

private:
    VoxelOctree* world = nullptr;
    PhysicsEngine* engine = nullptr;
};

} // namespace OpenMind
