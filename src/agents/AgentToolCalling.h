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

    ToolResult placeBlocks(const std::vector<std::tuple<int,int,int,std::string>>& blocks) {
        ToolResult r;
        if (!world) { r.output = "No world"; return r; }
        int count = 0;
        for (const auto& b : blocks) {
            int x = std::get<0>(b), y = std::get<1>(b), z = std::get<2>(b);
            if (x < 0 || x > 255 || y < 0 || y > 255 || z < 0 || z > 255) continue;
            BlockType t = stringToBlockType(std::get<3>(b));
            if (t != BlockType::AIR) { world->setBlock(x, y, z, t); count++; }
        }
        r.success = count > 0;
        r.output = "Placed " + std::to_string(count) + " blocks";
        return r;
    }

    ToolResult fillRect(int x1, int y1, int z1, int x2, int y2, int z2, const std::string& typeName) {
        ToolResult r;
        if (!world) { r.output = "No world"; return r; }
        BlockType t = stringToBlockType(typeName);
        if (t == BlockType::AIR) { r.output = "Unknown type: " + typeName; return r; }
        int xa = std::min(x1, x2), xb = std::max(x1, x2);
        int ya = std::min(y1, y2), yb = std::max(y1, y2);
        int za = std::min(z1, z2), zb = std::max(z1, z2);
        int count = 0;
        for (int x = xa; x <= xb; x++)
            for (int y = ya; y <= yb; y++)
                for (int z = za; z <= zb; z++)
                    if (x >= 0 && x < 256 && y >= 0 && y < 256 && z >= 0 && z < 256)
                        { world->setBlock(x, y, z, t); count++; }
        r.success = true;
        r.output = "Filled rect: " + std::to_string(count) + " blocks";
        return r;
    }

    ToolResult hollowRect(int x1, int y1, int z1, int x2, int y2, int z2, const std::string& typeName) {
        ToolResult r;
        if (!world) { r.output = "No world"; return r; }
        BlockType t = stringToBlockType(typeName);
        if (t == BlockType::AIR) { r.output = "Unknown type: " + typeName; return r; }
        int xa = std::min(x1, x2), xb = std::max(x1, x2);
        int ya = std::min(y1, y2), yb = std::max(y1, y2);
        int za = std::min(z1, z2), zb = std::max(z1, z2);
        int count = 0;
        for (int x = xa; x <= xb; x++)
            for (int y = ya; y <= yb; y++)
                for (int z = za; z <= zb; z++)
                    if ((x == xa || x == xb || y == ya || y == yb || z == za || z == zb)
                        && x >= 0 && x < 256 && y >= 0 && y < 256 && z >= 0 && z < 256)
                        { world->setBlock(x, y, z, t); count++; }
        r.success = true;
        r.output = "Hollow rect: " + std::to_string(count) + " wall blocks";
        return r;
    }

    ToolResult surveyArea(int x1, int y1, int z1, int x2, int y2, int z2) {
        ToolResult r;
        if (!world) { r.output = "No world"; return r; }
        int xa = std::min(x1, x2), xb = std::max(x1, x2);
        int ya = std::min(y1, y2), yb = std::max(y1, y2);
        int za = std::min(z1, z2), zb = std::max(z1, z2);
        int counts[256] = {0};
        int total = 0;
        for (int x = xa; x <= xb; x++)
            for (int y = ya; y <= yb; y++)
                for (int z = za; z <= zb; z++) {
                    VoxelData vd;
                    if (world->getBlock(x, y, z, vd) && vd.occupied && vd.type < 256)
                        counts[(int)vd.type]++;
                    total++;
                }
        std::ostringstream ss;
        ss << "Surveyed " << total << " positions.";
        r.success = true;
        r.output = ss.str();
        return r;
    }

    ToolResult clearArea(int x1, int y1, int z1, int x2, int y2, int z2) {
        ToolResult r;
        if (!world) { r.output = "No world"; return r; }
        int xa = std::min(x1, x2), xb = std::max(x1, x2);
        int ya = std::min(y1, y2), yb = std::max(y1, y2);
        int za = std::min(z1, z2), zb = std::max(z1, z2);
        int count = 0;
        for (int x = xa; x <= xb; x++)
            for (int y = ya; y <= yb; y++)
                for (int z = za; z <= zb; z++)
                    if (x >= 0 && x < 256 && y >= 0 && y < 256 && z >= 0 && z < 256)
                        { world->setBlock(x, y, z, BlockType::AIR); count++; }
        r.success = true;
        r.output = "Cleared " + std::to_string(count) + " blocks";
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
        if (toolName == "fill_rect" && args.size() >= 7) {
            return fillRect(safeStoi(args[0]), safeStoi(args[1]), safeStoi(args[2]),
                           safeStoi(args[3]), safeStoi(args[4]), safeStoi(args[5]), args[6]);
        }
        if (toolName == "hollow_rect" && args.size() >= 7) {
            return hollowRect(safeStoi(args[0]), safeStoi(args[1]), safeStoi(args[2]),
                             safeStoi(args[3]), safeStoi(args[4]), safeStoi(args[5]), args[6]);
        }
        if (toolName == "survey_area" && args.size() >= 6) {
            return surveyArea(safeStoi(args[0]), safeStoi(args[1]), safeStoi(args[2]),
                             safeStoi(args[3]), safeStoi(args[4]), safeStoi(args[5]));
        }
        if (toolName == "clear_area" && args.size() >= 6) {
            return clearArea(safeStoi(args[0]), safeStoi(args[1]), safeStoi(args[2]),
                            safeStoi(args[3]), safeStoi(args[4]), safeStoi(args[5]));
        }

        ToolResult r;
        r.output = "Unknown tool: " + toolName;
        return r;
    }

    static std::string getToolDescriptions() {
        return
            "Available tools:\n"
            "- move_to(x, y, z) - Walk to a location\n"
            "- place_block(x, y, z, blockType) - Place a single block\n"
            "- place_blocks(blocks) - Place multiple blocks at once\n"
            "- fill_rect(x1, y1, z1, x2, y2, z2, blockType) - Fill a rectangular volume\n"
            "- hollow_rect(x1, y1, z1, x2, y2, z2, blockType) - Build hollow rectangular walls\n"
            "- break_block(x, y, z) - Break a block\n"
            "- survey_area(x1, y1, z1, x2, y2, z2) - Count block types in a region\n"
            "- clear_area(x1, y1, z1, x2, y2, z2) - Remove all blocks in a region\n"
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
        return BlockType::CUSTOM;
    }
};

} // namespace OpenMind
