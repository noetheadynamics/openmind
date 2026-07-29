#pragma once

#include "AgentCognitive.h"
#include "VoxelOctree.h"
#include "PhysicsEngine.h"
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

namespace OpenMind {

inline int safeStoi(const std::string& s, int def = 0) {
    try { return std::stoi(s); } catch (...) { return def; }
}

struct ToolResult {
    bool success = false;
    std::string output;
    int errorCode = 0;
};

class AgentToolCalling {
public:
    AgentToolCalling() = default;

    void setWorld(VoxelOctree* w) { world = w; }
    void setEngine(PhysicsEngine* e) { engine = e; }

    ToolResult moveTo(float& ax, float& ay, float& az, int tx, int ty, int tz) {
        ToolResult r;
        float dx = (float)tx - ax, dy = (float)ty - ay, dz = (float)tz - az;
        float dist = std::sqrt(dx*dx + dy*dy + dz*dz);

        if (dist < 0.5f) {
            r.success = true;
            r.output = "Already at target";
            return r;
        }

        float step = std::min(1.0f, dist);
        ax += (dx / dist) * step;
        ay += (dy / dist) * step;
        az += (dz / dist) * step;

        r.success = true;
        r.output = "Moved toward (" + std::to_string(tx) + "," +
                   std::to_string(ty) + "," + std::to_string(tz) + ")";
        return r;
    }

    ToolResult placeBlock(int x, int y, int z, const std::string& typeName) {
        ToolResult r;
        if (!world) { r.output = "No world"; return r; }

        BlockType type = stringToBlockType(typeName);
        if (type == BlockType::AIR) { r.output = "Unknown block type: " + typeName; return r; }

        VoxelData existing;
        if (world->getBlock(x, y, z, existing) && existing.occupied && existing.type != BlockType::AIR) {
            r.output = "Position already occupied";
            return r;
        }

        world->setBlock(x, y, z, type);
        r.success = true;
        r.output = "Placed " + typeName + " at (" + std::to_string(x) + "," +
                   std::to_string(y) + "," + std::to_string(z) + ")";
        return r;
    }

    ToolResult breakBlock(int x, int y, int z) {
        ToolResult r;
        if (!world) { r.output = "No world"; return r; }

        VoxelData vd;
        if (!world->getBlock(x, y, z, vd) || !vd.occupied) {
            r.output = "No block at position";
            return r;
        }
        if (vd.type == BlockType::BEDROCK) {
            r.output = "Cannot break bedrock";
            return r;
        }

        world->setBlock(x, y, z, BlockType::AIR);
        r.success = true;
        r.output = "Broke block at (" + std::to_string(x) + "," +
                   std::to_string(y) + "," + std::to_string(z) + ")";
        return r;
    }

    ToolResult scanInventory(int agentId) {
        ToolResult r;
        r.output = "Inventory not yet implemented";
        r.success = false;
        return r;
    }

    ToolResult talkTo(int fromId, int toId, const std::string& message) {
        ToolResult r;
        r.success = false;
        r.output = "Message sent to agent " + std::to_string(toId) + ": " + message;
        return r;
    }

    ToolResult searchFor(float ax, float ay, float az, const std::string& resource) {
        ToolResult r;
        if (!world) { r.output = "No world"; return r; }

        BlockType targetType = stringToBlockType(resource);
        int range = 20;
        int ix = (int)ax, iy = (int)ay, iz = (int)az;

        for (int d = 1; d <= range; d++) {
            for (int dx = -d; dx <= d; dx++) {
                for (int dy = -d; dy <= d; dy++) {
                    for (int dz = -d; dz <= d; dz++) {
                        if (std::abs(dx) != d && std::abs(dy) != d && std::abs(dz) != d) continue;
                        VoxelData vd;
                        if (world->getBlock(ix+dx, iy+dy, iz+dz, vd) &&
                            vd.occupied && vd.type == targetType) {
                            r.success = true;
                            r.output = "Found " + resource + " at (" +
                                       std::to_string(ix+dx) + "," +
                                       std::to_string(iy+dy) + "," +
                                       std::to_string(iz+dz) + ")";
                            return r;
                        }
                    }
                }
            }
        }
        r.output = "No " + resource + " found within " + std::to_string(range) + " blocks";
        return r;
    }

    ToolResult craft(const std::string& item) {
        ToolResult r;
        r.success = false;
        r.output = "Crafted " + item;
        return r;
    }

    ToolResult buildStructure(const std::string& type, int bx, int by, int bz, int size) {
        ToolResult r;
        if (!world) { r.output = "No world"; return r; }
        if (size < 1) size = 1;
        if (size > 32) size = 32;
        bx = std::max(0, std::min(bx, 255 - size));
        by = std::max(0, std::min(by, 255 - size));
        bz = std::max(0, std::min(bz, 255 - size));

        if (type == "house" || type == "shelter") {
            for (int x = 0; x < size; x++) {
                for (int z = 0; z < size; z++) {
                    placeBlock(bx+x, by, bz+z, "WOOD");
                    placeBlock(bx+x, by+size, bz+z, "WOOD");
                }
                placeBlock(bx+x, by, bz, "WOOD");
                placeBlock(bx+x, by, bz+size-1, "WOOD");
            }
            for (int x = 0; x < size; x++) {
                placeBlock(bx+x, by+size, bz, "WOOD");
                placeBlock(bx+x, by+size, bz+size-1, "WOOD");
            }
            for (int x = 0; x < size; x++) {
                for (int z = 0; z < size; z++) {
                    placeBlock(bx+x, by+size, bz+z, "WOOD");
                }
            }
            r.success = true;
            r.output = "Built " + type + " at (" + std::to_string(bx) + "," +
                       std::to_string(by) + "," + std::to_string(bz) + ")";
        } else {
            r.success = false;
            r.output = "Unknown structure type: " + type;
        }
        return r;
    }

    ToolResult executeTool(const std::string& toolName, const std::vector<std::string>& args,
                           float& ax, float& ay, float& az, int agentId) {
        if (toolName == "move_to" && args.size() >= 3) {
            return moveTo(ax, ay, az, safeStoi(args[0]), safeStoi(args[1]), safeStoi(args[2]));
        }
        if (toolName == "place_block" && args.size() >= 4) {
            return placeBlock(safeStoi(args[0]), safeStoi(args[1]), safeStoi(args[2]), args[3]);
        }
        if (toolName == "break_block" && args.size() >= 3) {
            return breakBlock(safeStoi(args[0]), safeStoi(args[1]), safeStoi(args[2]));
        }
        if (toolName == "scan_inventory") {
            return scanInventory(agentId);
        }
        if (toolName == "talk_to" && args.size() >= 2) {
            return talkTo(agentId, safeStoi(args[0]), args[1]);
        }
        if (toolName == "search_for" && args.size() >= 1) {
            return searchFor(ax, ay, az, args[0]);
        }
        if (toolName == "craft" && args.size() >= 1) {
            return craft(args[0]);
        }
        if (toolName == "build_structure" && args.size() >= 5) {
            return buildStructure(args[0], safeStoi(args[1]), safeStoi(args[2]),
                                  safeStoi(args[3]), safeStoi(args[4]));
        }

        ToolResult r;
        r.output = "Unknown tool: " + toolName;
        return r;
    }

    static std::string getToolDescriptions() {
        return
            "Available tools:\n"
            "- move_to(x, y, z) - Walk to a location\n"
            "- place_block(x, y, z, blockType) - Place a block\n"
            "- break_block(x, y, z) - Break a block\n"
            "- scan_inventory() - Check inventory\n"
            "- talk_to(agentId, message) - Talk to another agent\n"
            "- search_for(resource) - Find a resource\n"
            "- craft(item) - Craft an item\n"
            "- build_structure(type, x, y, z, size) - Build a structure\n";
    }

private:
    VoxelOctree* world = nullptr;
    PhysicsEngine* engine = nullptr;

    static BlockType stringToBlockType(const std::string& s) {
        std::string lower = s;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "stone") return BlockType::STONE;
        if (lower == "dirt") return BlockType::DIRT;
        if (lower == "grass") return BlockType::GRASS;
        if (lower == "water") return BlockType::WATER;
        if (lower == "sand") return BlockType::SAND;
        if (lower == "glass") return BlockType::GLASS;
        if (lower == "wood") return BlockType::WOOD;
        if (lower == "leaves") return BlockType::LEAVES;
        if (lower == "iron") return BlockType::IRON;
        if (lower == "copper") return BlockType::COPPER;
        if (lower == "gold") return BlockType::GOLD;
        if (lower == "steel") return BlockType::STEEL;
        if (lower == "diamond") return BlockType::DIAMOND;
        if (lower == "coal") return BlockType::COAL;
        if (lower == "bedrock") return BlockType::BEDROCK;
        if (lower == "ash") return BlockType::ASH;
        if (lower == "tnt") return BlockType::TNT;
        if (lower == "snow") return BlockType::SNOW;
        return BlockType::AIR;
    }
};

} // namespace OpenMind
