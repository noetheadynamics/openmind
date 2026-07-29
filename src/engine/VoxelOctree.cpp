#include "VoxelOctree.h"
#include <cstring>
#include <algorithm>

namespace OpenMind {

static const uint32_t FILE_MAGIC = 0x4F504D4E;
static const uint32_t FILE_VERSION = 1;

template<typename T>
void writeBinary(std::ofstream& out, const T& value) {
    T val = value;
    uint8_t bytes[sizeof(T)];
    std::memcpy(bytes, &val, sizeof(T));

    for (size_t i = 0; i < sizeof(T) / 2; ++i) {
        std::swap(bytes[i], bytes[sizeof(T) - 1 - i]);
    }

    out.write(reinterpret_cast<const char*>(bytes), sizeof(T));
}

template<typename T>
T readBinary(std::ifstream& in) {
    uint8_t bytes[sizeof(T)];
    in.read(reinterpret_cast<char*>(bytes), sizeof(T));

    for (size_t i = 0; i < sizeof(T) / 2; ++i) {
        std::swap(bytes[i], bytes[sizeof(T) - 1 - i]);
    }

    T value;
    std::memcpy(&value, bytes, sizeof(T));
    return value;
}

void writeString(std::ofstream& out, const std::string& str) {
    uint32_t len = static_cast<uint32_t>(str.size());
    writeBinary(out, len);
    out.write(str.data(), len);
}

std::string readString(std::ifstream& in) {
    uint32_t len = readBinary<uint32_t>(in);
    std::string str(len, '\0');
    in.read(&str[0], len);
    return str;
}

VoxelOctreeNode::VoxelOctreeNode(int depth, int size, int ox, int oy, int oz)
    : depth(depth), size(size), offsetX(ox), offsetY(oy), offsetZ(oz) {
}

VoxelOctreeNode::~VoxelOctreeNode() {
    for (auto& child : children) {
        child.reset();
    }
}

int VoxelOctreeNode::getChildIndex(int x, int y, int z) const {
    int childSize = size / 2;
    int idx = 0;
    if (x >= offsetX + childSize) idx |= 1;
    if (y >= offsetY + childSize) idx |= 2;
    if (z >= offsetZ + childSize) idx |= 4;
    return idx;
}

bool VoxelOctreeNode::isInBounds(int x, int y, int z) const {
    return x >= offsetX && x < offsetX + size &&
           y >= offsetY && y < offsetY + size &&
           z >= offsetZ && z < offsetZ + size;
}

bool VoxelOctreeNode::isLeaf() const {
    for (const auto& child : children) {
        if (child) return false;
    }
    return true;
}

bool VoxelOctreeNode::isEmpty() const {
    if (voxelData && voxelData->occupied) return false;
    for (const auto& child : children) {
        if (child && !child->isEmpty()) return false;
    }
    return true;
}

void VoxelOctreeNode::split(int childIndex) {
    if (children[childIndex]) return;

    int childSize = size / 2;
    int ox = offsetX + ((childIndex & 1) ? childSize : 0);
    int oy = offsetY + ((childIndex & 2) ? childSize : 0);
    int oz = offsetZ + ((childIndex & 4) ? childSize : 0);

    children[childIndex] = std::make_unique<VoxelOctreeNode>(depth + 1, childSize, ox, oy, oz);
}

void VoxelOctreeNode::setBlock(int x, int y, int z, BlockType type, const MaterialProps& props, int targetDepth) {
    if (!isInBounds(x, y, z)) return;

    if (depth == targetDepth) {
        voxelData = std::make_unique<VoxelData>(type, props);
        return;
    }

    if (isLeaf() && voxelData && voxelData->occupied) {
        BlockType existingType = voxelData->type;
        MaterialProps existingProps = voxelData->props;
        voxelData.reset();

        int childSize = size / 2;
        for (int i = 0; i < 8; ++i) {
            int ox = offsetX + ((i & 1) ? childSize : 0);
            int oy = offsetY + ((i & 2) ? childSize : 0);
            int oz = offsetZ + ((i & 4) ? childSize : 0);
            children[i] = std::make_unique<VoxelOctreeNode>(depth + 1, childSize, ox, oy, oz);
        }
    }

    int childIndex = getChildIndex(x, y, z);
    split(childIndex);
    children[childIndex]->setBlock(x, y, z, type, props, targetDepth);
}

bool VoxelOctreeNode::getBlock(int x, int y, int z, VoxelData& outData) const {
    if (!isInBounds(x, y, z)) return false;

    if (depth == MAX_OCTREE_DEPTH && voxelData && voxelData->occupied) {
        outData = *voxelData;
        return true;
    }

    if (isLeaf()) {
        if (voxelData && voxelData->occupied) {
            outData = *voxelData;
            return true;
        }
        return false;
    }

    int childIndex = getChildIndex(x, y, z);
    if (children[childIndex]) {
        return children[childIndex]->getBlock(x, y, z, outData);
    }

    return false;
}

bool VoxelOctreeNode::setBlockTemperature(int x, int y, int z, float temp) {
    if (!isInBounds(x, y, z)) return false;

    if (depth == MAX_OCTREE_DEPTH) {
        if (voxelData && voxelData->occupied) {
            voxelData->currentTemperature = temp;
            return true;
        }
        return false;
    }

    if (isLeaf()) {
        if (voxelData && voxelData->occupied) {
            voxelData->currentTemperature = temp;
            return true;
        }
        return false;
    }

    int childIndex = getChildIndex(x, y, z);
    if (children[childIndex]) {
        return children[childIndex]->setBlockTemperature(x, y, z, temp);
    }

    return false;
}

bool VoxelOctreeNode::setBlockState(int x, int y, int z, BlockState state) {
    if (!isInBounds(x, y, z)) return false;

    if (depth == MAX_OCTREE_DEPTH) {
        if (voxelData && voxelData->occupied) {
            voxelData->state = state;
            return true;
        }
        return false;
    }

    if (isLeaf()) {
        if (voxelData && voxelData->occupied) {
            voxelData->state = state;
            return true;
        }
        return false;
    }

    int childIndex = getChildIndex(x, y, z);
    if (children[childIndex]) {
        return children[childIndex]->setBlockState(x, y, z, state);
    }

    return false;
}

bool VoxelOctreeNode::setBlockDensity(int x, int y, int z, float density) {
    if (!isInBounds(x, y, z)) return false;

    if (depth == MAX_OCTREE_DEPTH) {
        if (voxelData && voxelData->occupied) {
            voxelData->props.general.density = density;
            return true;
        }
        return false;
    }

    if (isLeaf()) {
        if (voxelData && voxelData->occupied) {
            voxelData->props.general.density = density;
            return true;
        }
        return false;
    }

    int childIndex = getChildIndex(x, y, z);
    if (children[childIndex]) {
        return children[childIndex]->setBlockDensity(x, y, z, density);
    }

    return false;
}

void VoxelOctreeNode::traverse(VisitCallback callback) const {
    if (voxelData && voxelData->occupied) {
        if (!callback(*voxelData, offsetX, offsetY, offsetZ, depth)) return;
    }
    for (const auto& child : children) {
        if (child) child->traverse(callback);
    }
}

void VoxelOctreeNode::calculateMemoryUsage(int& count) const {
    if (voxelData && voxelData->occupied) count++;
    for (const auto& child : children) {
        if (child) child->calculateMemoryUsage(count);
    }
}

void VoxelOctreeNode::serialize(std::ofstream& out) const {
    writeBinary(out, static_cast<uint8_t>(voxelData && voxelData->occupied ? 1 : 0));

    if (voxelData && voxelData->occupied) {
        writeBinary(out, static_cast<uint8_t>(voxelData->type));

        writeBinary(out, voxelData->currentTemperature);

        writeBinary(out, voxelData->props.general.mass);
        writeBinary(out, voxelData->props.general.density);
        writeBinary(out, voxelData->props.general.hardness);
        writeBinary(out, voxelData->props.general.elasticity);

        writeBinary(out, voxelData->props.mechanical.tensileStrength);
        writeBinary(out, voxelData->props.mechanical.compressiveStrength);
        writeBinary(out, voxelData->props.mechanical.shearStrength);
        writeBinary(out, voxelData->props.mechanical.fractureToughness);

        writeBinary(out, voxelData->props.thermal.thermalConductivity);
        writeBinary(out, voxelData->props.thermal.specificHeat);
        writeBinary(out, voxelData->props.thermal.meltingPoint);
        writeBinary(out, voxelData->props.thermal.boilingPoint);
        writeBinary(out, voxelData->props.thermal.thermalSofteningPoint);

        writeString(out, voxelData->props.chemical.composition);
        writeBinary(out, voxelData->props.chemical.flammability);
        writeBinary(out, voxelData->props.chemical.combustionPoint);
        writeBinary(out, voxelData->props.chemical.corrosionRate);
        writeBinary(out, voxelData->props.chemical.chemicalResistance);

        writeBinary(out, voxelData->props.biological.isOrganic ? 1 : 0);
        writeBinary(out, voxelData->props.biological.isBiological ? 1 : 0);
        writeBinary(out, voxelData->props.biological.growthRate);
        writeBinary(out, voxelData->props.biological.sunlightRequirement);
        writeBinary(out, voxelData->props.biological.waterRequirement);

        writeBinary(out, voxelData->props.electrical.conductivity);
        writeBinary(out, voxelData->props.electrical.resistivity);

        writeString(out, voxelData->props.visual.baseColor);
        writeBinary(out, voxelData->props.visual.roughness);
        writeBinary(out, voxelData->props.visual.metallicness);
        writeBinary(out, voxelData->props.visual.opacity);
        writeString(out, voxelData->props.visual.textureStyle);

        writeBinary(out, voxelData->props.environmental.buoyancy);
        writeBinary(out, voxelData->props.environmental.permeability);
        writeBinary(out, voxelData->props.environmental.friction);

        writeString(out, voxelData->props.layering.layerAbove);
        writeString(out, voxelData->props.layering.layerBelow);

        writeBinary(out, voxelData->props.health.maxHealth);
        writeBinary(out, voxelData->props.health.currentHealth);

        writeBinary(out, static_cast<uint32_t>(voxelData->props.extraFloatProps.size()));
        for (const auto& [key, val] : voxelData->props.extraFloatProps) {
            writeString(out, key);
            writeBinary(out, val);
        }

        writeBinary(out, static_cast<uint32_t>(voxelData->props.extraIntProps.size()));
        for (const auto& [key, val] : voxelData->props.extraIntProps) {
            writeString(out, key);
            writeBinary(out, val);
        }

        writeBinary(out, static_cast<uint32_t>(voxelData->props.extraStringProps.size()));
        for (const auto& [key, val] : voxelData->props.extraStringProps) {
            writeString(out, key);
            writeString(out, val);
        }
    }

    uint8_t childMask = 0;
    for (int i = 0; i < 8; ++i) {
        if (children[i]) childMask |= (1 << i);
    }
    writeBinary(out, childMask);

    for (int i = 0; i < 8; ++i) {
        if (children[i]) {
            children[i]->serialize(out);
        }
    }
}

std::unique_ptr<VoxelOctreeNode> VoxelOctreeNode::deserialize(
    std::ifstream& in, int depth, int size, int ox, int oy, int oz) {

    auto node = std::make_unique<VoxelOctreeNode>(depth, size, ox, oy, oz);

    uint8_t hasVoxel = readBinary<uint8_t>(in);
    if (hasVoxel) {
        node->voxelData = std::make_unique<VoxelData>();
        node->voxelData->type = static_cast<BlockType>(readBinary<uint8_t>(in));
        node->voxelData->occupied = true;

        node->voxelData->currentTemperature = readBinary<float>(in);

        node->voxelData->props.general.mass = readBinary<float>(in);
        node->voxelData->props.general.density = readBinary<float>(in);
        node->voxelData->props.general.hardness = readBinary<float>(in);
        node->voxelData->props.general.elasticity = readBinary<float>(in);

        node->voxelData->props.mechanical.tensileStrength = readBinary<float>(in);
        node->voxelData->props.mechanical.compressiveStrength = readBinary<float>(in);
        node->voxelData->props.mechanical.shearStrength = readBinary<float>(in);
        node->voxelData->props.mechanical.fractureToughness = readBinary<float>(in);

        node->voxelData->props.thermal.thermalConductivity = readBinary<float>(in);
        node->voxelData->props.thermal.specificHeat = readBinary<float>(in);
        node->voxelData->props.thermal.meltingPoint = readBinary<float>(in);
        node->voxelData->props.thermal.boilingPoint = readBinary<float>(in);
        node->voxelData->props.thermal.thermalSofteningPoint = readBinary<float>(in);

        node->voxelData->props.chemical.composition = readString(in);
        node->voxelData->props.chemical.flammability = readBinary<float>(in);
        node->voxelData->props.chemical.combustionPoint = readBinary<float>(in);
        node->voxelData->props.chemical.corrosionRate = readBinary<float>(in);
        node->voxelData->props.chemical.chemicalResistance = readBinary<float>(in);

        node->voxelData->props.biological.isOrganic = readBinary<uint8_t>(in) != 0;
        node->voxelData->props.biological.isBiological = readBinary<uint8_t>(in) != 0;
        node->voxelData->props.biological.growthRate = readBinary<float>(in);
        node->voxelData->props.biological.sunlightRequirement = readBinary<float>(in);
        node->voxelData->props.biological.waterRequirement = readBinary<float>(in);

        node->voxelData->props.electrical.conductivity = readBinary<float>(in);
        node->voxelData->props.electrical.resistivity = readBinary<float>(in);

        node->voxelData->props.visual.baseColor = readString(in);
        node->voxelData->props.visual.roughness = readBinary<float>(in);
        node->voxelData->props.visual.metallicness = readBinary<float>(in);
        node->voxelData->props.visual.opacity = readBinary<float>(in);
        node->voxelData->props.visual.textureStyle = readString(in);

        node->voxelData->props.environmental.buoyancy = readBinary<float>(in);
        node->voxelData->props.environmental.permeability = readBinary<float>(in);
        node->voxelData->props.environmental.friction = readBinary<float>(in);

        node->voxelData->props.layering.layerAbove = readString(in);
        node->voxelData->props.layering.layerBelow = readString(in);

        node->voxelData->props.health.maxHealth = readBinary<float>(in);
        node->voxelData->props.health.currentHealth = readBinary<float>(in);

        uint32_t extraFloatCount = readBinary<uint32_t>(in);
        if (extraFloatCount > 1000) extraFloatCount = 0;
        for (uint32_t i = 0; i < extraFloatCount; ++i) {
            std::string key = readString(in);
            float val = readBinary<float>(in);
            node->voxelData->props.extraFloatProps[key] = val;
        }

        uint32_t extraIntCount = readBinary<uint32_t>(in);
        if (extraIntCount > 1000) extraIntCount = 0;
        for (uint32_t i = 0; i < extraIntCount; ++i) {
            std::string key = readString(in);
            int val = readBinary<int>(in);
            node->voxelData->props.extraIntProps[key] = val;
        }

        uint32_t extraStringCount = readBinary<uint32_t>(in);
        if (extraStringCount > 1000) extraStringCount = 0;
        for (uint32_t i = 0; i < extraStringCount; ++i) {
            std::string key = readString(in);
            std::string val = readString(in);
            node->voxelData->props.extraStringProps[key] = val;
        }
    }

    uint8_t childMask = readBinary<uint8_t>(in);
    int childSize = size / 2;

    for (int i = 0; i < 8; ++i) {
        if (childMask & (1 << i)) {
            int ox2 = ox + ((i & 1) ? childSize : 0);
            int oy2 = oy + ((i & 2) ? childSize : 0);
            int oz2 = oz + ((i & 4) ? childSize : 0);
            node->children[i] = deserialize(in, depth + 1, childSize, ox2, oy2, oz2);
        }
    }

    return node;
}

VoxelOctree::VoxelOctree() : blockCount(0) {
}

VoxelOctree::~VoxelOctree() {
    root.reset();
}

VoxelOctree::VoxelOctree(VoxelOctree&& other) noexcept
    : root(std::move(other.root)), blockCount(other.blockCount) {
    other.blockCount = 0;
}

VoxelOctree& VoxelOctree::operator=(VoxelOctree&& other) noexcept {
    if (this != &other) {
        root = std::move(other.root);
        blockCount = other.blockCount;
        other.blockCount = 0;
    }
    return *this;
}

void VoxelOctree::setBlock(int x, int y, int z, BlockType type, const MaterialProps& props) {
    if (x < 0 || x >= ROOT_SIZE || y < 0 || y >= ROOT_SIZE || z < 0 || z >= ROOT_SIZE) {
        return;
    }

    if (!root) {
        root = std::make_unique<VoxelOctreeNode>(0, ROOT_SIZE, 0, 0, 0);
    }

    VoxelData oldData;
    bool hadBlock = root->getBlock(x, y, z, oldData);

    root->setBlock(x, y, z, type, props, MAX_OCTREE_DEPTH);

    if (!hadBlock && type != BlockType::AIR) {
        blockCount++;
    } else if (hadBlock && type == BlockType::AIR && oldData.type != BlockType::AIR) {
        blockCount--;
    }
}

void VoxelOctree::setBlock(int x, int y, int z, BlockType type) {
    MaterialProps defaultProps;
    setBlock(x, y, z, type, defaultProps);
}

bool VoxelOctree::getBlock(int x, int y, int z, VoxelData& outData) const {
    if (!root) return false;
    if (x < 0 || x >= ROOT_SIZE || y < 0 || y >= ROOT_SIZE || z < 0 || z >= ROOT_SIZE) {
        return false;
    }
    return root->getBlock(x, y, z, outData);
}

bool VoxelOctree::setBlockTemperature(int x, int y, int z, float temp) {
    if (!root) return false;
    if (x < 0 || x >= ROOT_SIZE || y < 0 || y >= ROOT_SIZE || z < 0 || z >= ROOT_SIZE) {
        return false;
    }
    return root->setBlockTemperature(x, y, z, temp);
}

bool VoxelOctree::setBlockState(int x, int y, int z, BlockState state) {
    if (!root) return false;
    if (x < 0 || x >= ROOT_SIZE || y < 0 || y >= ROOT_SIZE || z < 0 || z >= ROOT_SIZE) {
        return false;
    }
    return root->setBlockState(x, y, z, state);
}

bool VoxelOctree::setBlockDensity(int x, int y, int z, float density) {
    if (!root) return false;
    if (x < 0 || x >= ROOT_SIZE || y < 0 || y >= ROOT_SIZE || z < 0 || z >= ROOT_SIZE) {
        return false;
    }
    return root->setBlockDensity(x, y, z, density);
}

bool VoxelOctree::blockExists(int x, int y, int z) const {
    VoxelData data;
    return getBlock(x, y, z, data) && data.occupied && data.type != BlockType::AIR;
}

bool VoxelOctree::saveToFile(const std::string& path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) return false;

    writeBinary(out, FILE_MAGIC);
    writeBinary(out, FILE_VERSION);
    writeBinary(out, static_cast<uint32_t>(blockCount));

    if (root) {
        writeBinary(out, static_cast<uint8_t>(1));
        root->serialize(out);
    } else {
        writeBinary(out, static_cast<uint8_t>(0));
    }

    return out.good();
}

bool VoxelOctree::loadFromFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return false;

    uint32_t magic = readBinary<uint32_t>(in);
    if (magic != FILE_MAGIC) return false;

    uint32_t version = readBinary<uint32_t>(in);
    if (version != FILE_VERSION) return false;

    blockCount = static_cast<int>(readBinary<uint32_t>(in));

    uint8_t hasRoot = readBinary<uint8_t>(in);
    if (hasRoot) {
        root = VoxelOctreeNode::deserialize(in, 0, ROOT_SIZE, 0, 0, 0);
    } else {
        root.reset();
    }

    return in.good();
}

void VoxelOctree::clear() {
    root.reset();
    blockCount = 0;
}

int VoxelOctree::getMemoryUsage() const {
    if (!root) return 0;
    int count = 0;
    root->calculateMemoryUsage(count);
    return count;
}

int VoxelOctree::getBlockCount() const {
    return blockCount;
}

void VoxelOctree::traverse(VisitCallback callback) const {
    if (root) {
        root->traverse(callback);
    }
}

VoxelOctree VoxelOctree::createTestWorld() {
    VoxelOctree world;

    MaterialProps steelProps = createSteelProps();
    world.setBlock(5, 10, 15, BlockType::STEEL, steelProps);

    return world;
}

}
