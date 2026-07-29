#pragma once

#include "MaterialProperties.h"
#include <memory>
#include <array>
#include <cstdint>
#include <string>
#include <fstream>
#include <functional>

namespace OpenMind {

static constexpr int MAX_OCTREE_DEPTH = 8;
static constexpr int ROOT_SIZE = 256;

struct VoxelData {
    BlockType type = BlockType::AIR;
    MaterialProps props;
    bool occupied = false;
    float currentTemperature = 293.15f;
    BlockState state;
    bool isCelestialBody = false;
    float gravitationalMass = 0.0f;

    VoxelData() : state(BlockState::SOLID) {}
    VoxelData(BlockType t, const MaterialProps& p) : type(t), props(p), occupied(true), state(BlockState::SOLID) {}
};

class VoxelOctreeNode {
public:
    VoxelOctreeNode(int depth, int size, int offsetX, int offsetY, int offsetZ);
    ~VoxelOctreeNode();

    VoxelOctreeNode(const VoxelOctreeNode&) = delete;
    VoxelOctreeNode& operator=(const VoxelOctreeNode&) = delete;

    void setBlock(int x, int y, int z, BlockType type, const MaterialProps& props, int targetDepth);
    bool setBlockTemperature(int x, int y, int z, float temp);
    bool setBlockState(int x, int y, int z, BlockState state);
    bool setBlockDensity(int x, int y, int z, float density);
    bool getBlock(int x, int y, int z, VoxelData& outData) const;

    bool isEmpty() const;
    int getMemoryUsage() const;

    void serialize(std::ofstream& out) const;
    static std::unique_ptr<VoxelOctreeNode> deserialize(std::ifstream& in, int depth, int size, int ox, int oy, int oz);

private:
    int depth;
    int size;
    int offsetX, offsetY, offsetZ;

    std::unique_ptr<VoxelData> voxelData;
    std::array<std::unique_ptr<VoxelOctreeNode>, 8> children;

    int getChildIndex(int x, int y, int z) const;
    void split(int childIndex);
    bool isLeaf() const;
    bool isInBounds(int x, int y, int z) const;

    using VisitCallback = std::function<bool(const VoxelData&, int, int, int, int)>;
    void traverse(VisitCallback callback) const;
    void calculateMemoryUsage(int& count) const;

    friend class VoxelOctree;
};

class VoxelOctree {
public:
    VoxelOctree();
    ~VoxelOctree();

    VoxelOctree(VoxelOctree&& other) noexcept;
    VoxelOctree& operator=(VoxelOctree&& other) noexcept;
    VoxelOctree(const VoxelOctree&) = delete;
    VoxelOctree& operator=(const VoxelOctree&) = delete;

    void setBlock(int x, int y, int z, BlockType type, const MaterialProps& props);
    void setBlock(int x, int y, int z, BlockType type);
    bool setBlockTemperature(int x, int y, int z, float temp);
    bool setBlockState(int x, int y, int z, BlockState state);
    bool setBlockDensity(int x, int y, int z, float density);

    bool getBlock(int x, int y, int z, VoxelData& outData) const;
    bool blockExists(int x, int y, int z) const;

    bool saveToFile(const std::string& path) const;
    bool loadFromFile(const std::string& path);

    void clear();
    int getMemoryUsage() const;
    int getBlockCount() const;

    static VoxelOctree createTestWorld();

private:
    std::unique_ptr<VoxelOctreeNode> root;
    int blockCount;

    void updateBlockCount();
};

}
