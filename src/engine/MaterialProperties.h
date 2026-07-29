#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>
#include <cctype>

namespace OpenMind {

enum class BlockState : uint8_t {
    SOLID = 0,
    LIQUID,
    GAS,
    PLASMA
};

struct GeneralProps {
    float mass = 0.0f;
    float density = 0.0f;
    float hardness = 0.0f;
    float elasticity = 0.0f;
};

struct MechanicalProps {
    float tensileStrength = 0.0f;
    float compressiveStrength = 0.0f;
    float shearStrength = 0.0f;
    float fractureToughness = 0.0f;
};

struct ThermalProps {
    float thermalConductivity = 0.0f;
    float specificHeat = 0.0f;
    float meltingPoint = 0.0f;
    float boilingPoint = 0.0f;
    float thermalSofteningPoint = 0.0f;
    float heatOutput = 0.0f;
    float emissivity = 0.0f;
    float radiationAbsorption = 0.0f;
    float latentHeatOfFusion = 0.0f;
    float latentHeatOfVaporization = 0.0f;
    float liquidDensityFactor = 0.95f;
    float freezingPoint = -1.0f;
    float gasDensityFactor = 0.001f;
    float condensationPoint = 0.0f;
};

struct ChemicalProps {
    std::string composition;
    float flammability = 0.0f;
    float combustionPoint = 0.0f;
    float corrosionRate = 0.0f;
    float chemicalResistance = 0.0f;
    float explosivePower = 0.0f;
    float detonationTemperature = 0.0f;
    float explosionRadius = 0.0f;
    float absorptionCoefficient = 0.1f;
    float lightAbsorption = 0.1f;
};

struct BiologicalProps {
    bool isOrganic = false;
    bool isBiological = false;
    float growthRate = 0.0f;
    float sunlightRequirement = 0.0f;
    float waterRequirement = 0.0f;
    std::string soilType;
    float decayThreshold = 1000.0f;
};

struct ElectricalProps {
    float conductivity = 0.0f;
    float resistivity = 0.0f;
};

struct RocketProps {
    float Isp = 0.0f;
    float thrust = 0.0f;
    float throttle = 0.0f;
    float fuelMass = 0.0f;
    float fuelBurnRate = 0.0f;
    float dryMass = 0.0f;
    bool isEngine = false;
    bool isFuelTank = false;
    bool isPayload = false;
    std::string fuelType = "";
};

struct VisualProps {
    std::string baseColor;
    float roughness = 0.0f;
    float metallicness = 0.0f;
    float opacity = 1.0f;
    std::string textureStyle;
};

struct EnvironmentalProps {
    float buoyancy = 0.0f;
    float permeability = 0.0f;
    float friction = 0.0f;
};

struct LayeringProps {
    std::string layerAbove;
    std::string layerBelow;
};

struct HealthProps {
    float maxHealth = 100.0f;
    float currentHealth = 100.0f;
};

struct MaterialProps {
    GeneralProps general;
    MechanicalProps mechanical;
    ThermalProps thermal;
    ChemicalProps chemical;
    BiologicalProps biological;
    ElectricalProps electrical;
    RocketProps rocket;
    VisualProps visual;
    EnvironmentalProps environmental;
    LayeringProps layering;
    HealthProps health;

    std::unordered_map<std::string, float> extraFloatProps;
    std::unordered_map<std::string, int> extraIntProps;
    std::unordered_map<std::string, std::string> extraStringProps;

    bool operator==(const MaterialProps& other) const {
        return general.mass == other.general.mass &&
               general.density == other.general.density &&
               general.hardness == other.general.hardness &&
               general.elasticity == other.general.elasticity &&
               mechanical.tensileStrength == other.mechanical.tensileStrength &&
               thermal.meltingPoint == other.thermal.meltingPoint &&
               chemical.flammability == other.chemical.flammability &&
               visual.baseColor == other.visual.baseColor;
    }

    bool operator!=(const MaterialProps& other) const {
        return !(*this == other);
    }
};

enum class BlockType : uint8_t {
    AIR = 0,
    DIRT,
    GRASS,
    STONE,
    WOOD,
    LEAVES,
    WATER,
    SAND,
    GLASS,
    STEEL,
    IRON,
    COPPER,
    GOLD,
    DIAMOND,
    COAL,
    BEDROCK,
    ASH,
    TNT,
    SNOW,
    CUSTOM = 255
};

inline const char* blockTypeToString(BlockType type) {
    switch (type) {
        case BlockType::AIR: return "AIR";
        case BlockType::DIRT: return "DIRT";
        case BlockType::GRASS: return "GRASS";
        case BlockType::STONE: return "STONE";
        case BlockType::WOOD: return "WOOD";
        case BlockType::LEAVES: return "LEAVES";
        case BlockType::WATER: return "WATER";
        case BlockType::SAND: return "SAND";
        case BlockType::GLASS: return "GLASS";
        case BlockType::STEEL: return "STEEL";
        case BlockType::IRON: return "IRON";
        case BlockType::COPPER: return "COPPER";
        case BlockType::GOLD: return "GOLD";
        case BlockType::DIAMOND: return "DIAMOND";
        case BlockType::COAL: return "COAL";
        case BlockType::BEDROCK: return "BEDROCK";
        case BlockType::ASH: return "ASH";
        case BlockType::TNT: return "TNT";
        case BlockType::SNOW: return "SNOW";
        case BlockType::CUSTOM: return "CUSTOM";
        default: return "UNKNOWN";
    }
}

inline BlockType stringToBlockType(const std::string& str) {
    if (str == "AIR") return BlockType::AIR;
    if (str == "DIRT") return BlockType::DIRT;
    if (str == "GRASS") return BlockType::GRASS;
    if (str == "STONE") return BlockType::STONE;
    if (str == "WOOD") return BlockType::WOOD;
    if (str == "LEAVES") return BlockType::LEAVES;
    if (str == "WATER") return BlockType::WATER;
    if (str == "SAND") return BlockType::SAND;
    if (str == "GLASS") return BlockType::GLASS;
    if (str == "STEEL") return BlockType::STEEL;
    if (str == "IRON") return BlockType::IRON;
    if (str == "COPPER") return BlockType::COPPER;
    if (str == "GOLD") return BlockType::GOLD;
    if (str == "DIAMOND") return BlockType::DIAMOND;
    if (str == "COAL") return BlockType::COAL;
    if (str == "BEDROCK") return BlockType::BEDROCK;
    if (str == "ASH") return BlockType::ASH;
    if (str == "TNT") return BlockType::TNT;
    if (str == "SNOW") return BlockType::SNOW;
    return BlockType::CUSTOM;
}

inline MaterialProps createSteelProps() {
    MaterialProps props;
    props.general.mass = 7.85f;
    props.general.density = 7850.0f;
    props.general.hardness = 8.0f;
    props.general.elasticity = 0.29f;
    props.mechanical.tensileStrength = 400.0f;
    props.mechanical.compressiveStrength = 250.0f;
    props.mechanical.shearStrength = 200.0f;
    props.mechanical.fractureToughness = 50.0f;
    props.thermal.thermalConductivity = 50.0f;
    props.thermal.specificHeat = 500.0f;
    props.thermal.meltingPoint = 1510.0f;
    props.thermal.boilingPoint = 3000.0f;
    props.chemical.composition = "Fe-C alloy";
    props.chemical.flammability = 0.0f;
    props.chemical.corrosionRate = 0.01f;
    props.chemical.chemicalResistance = 0.7f;
    props.electrical.conductivity = 1.45e7f;
    props.electrical.resistivity = 6.9e-7f;
    props.visual.baseColor = "#71797E";
    props.visual.metallicness = 0.9f;
    props.visual.roughness = 0.4f;
    props.health.maxHealth = 500.0f;
    props.health.currentHealth = 500.0f;
    return props;
}

inline bool isValidComposition(const std::string& comp) {
    if (comp.empty()) return false;
    for (char c : comp) {
        if (!std::isalnum(c) && c != '(' && c != ')' && c != '-' && c != '_' && c != ' ') return false;
    }
    return true;
}

inline bool isCompositionMatch(const std::string& blockComp, const std::string& targetComp) {
    if (!isValidComposition(blockComp) || !isValidComposition(targetComp)) return false;
    return blockComp == targetComp;
}

}
