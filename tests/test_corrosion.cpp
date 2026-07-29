#include "PhysicsEngine.h"
#include "VoxelOctree.h"
#include "MaterialProperties.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <string>

using namespace OpenMind;

MaterialProps createIronProps() {
    MaterialProps mp;
    mp.general.mass = 7.87f;
    mp.general.density = 7874.0f;
    mp.general.hardness = 4.0f;
    mp.general.elasticity = 0.29f;
    mp.mechanical.tensileStrength = 350.0f;
    mp.mechanical.compressiveStrength = 350.0f;
    mp.mechanical.shearStrength = 200.0f;
    mp.mechanical.fractureToughness = 50.0f;
    mp.thermal.thermalConductivity = 80.0f;
    mp.thermal.specificHeat = 449.0f;
    mp.thermal.meltingPoint = 1811.0f;
    mp.thermal.boilingPoint = 3134.0f;
    mp.chemical.composition = "Fe";
    mp.chemical.corrosionRate = 0.1f;
    mp.chemical.chemicalResistance = 0.1f;
    mp.health.maxHealth = 200.0f;
    mp.health.currentHealth = 200.0f;
    mp.visual.baseColor = "#708090";
    mp.visual.metallicness = 0.9f;
    return mp;
}

MaterialProps createWaterProps() {
    MaterialProps mp;
    mp.general.mass = 1.0f;
    mp.general.density = 1000.0f;
    mp.thermal.specificHeat = 4186.0f;
    mp.thermal.thermalConductivity = 0.6f;
    mp.thermal.meltingPoint = 273.15f;
    mp.thermal.boilingPoint = 373.15f;
    mp.chemical.composition = "H2O";
    mp.health.maxHealth = 100.0f;
    mp.health.currentHealth = 100.0f;
    mp.visual.baseColor = "#4682B4";
    return mp;
}

MaterialProps createOxygenProps() {
    MaterialProps mp;
    mp.general.mass = 0.5f;
    mp.general.density = 1.4f;
    mp.thermal.specificHeat = 920.0f;
    mp.chemical.composition = "O2";
    mp.health.maxHealth = 100.0f;
    mp.health.currentHealth = 100.0f;
    mp.visual.baseColor = "#87CEEB";
    return mp;
}

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "  CORROSION TEST (Feature #31)" << std::endl;
    std::cout << "=============================================" << std::endl;

    VoxelOctree world;
    PhysicsEngine engine;
    engine.setScanRange(8);
    engine.setTemperature(293.15f);

    MaterialProps stoneProps;
    stoneProps.general.mass = 2.5f;
    stoneProps.general.density = 2600.0f;
    stoneProps.general.hardness = 7.0f;
    stoneProps.mechanical.tensileStrength = 10.0f;
    stoneProps.thermal.specificHeat = 800.0f;
    stoneProps.health.maxHealth = 500.0f;
    stoneProps.health.currentHealth = 500.0f;

    for (int x = 0; x < 8; x++) {
        for (int z = 0; z < 8; z++) {
            world.setBlock(x, 0, z, BlockType::STONE, stoneProps);
        }
    }

    int ironX = 4, ironY = 1, ironZ = 4;
    int waterX = 5, waterY = 1, waterZ = 4;
    int o2X = 4, o2Y = 2, o2Z = 4;

    world.setBlock(ironX, ironY, ironZ, BlockType::IRON, createIronProps());
    world.setBlockState(ironX, ironY, ironZ, BlockState::SOLID);
    world.setBlockTemperature(ironX, ironY, ironZ, 293.15f);

    world.setBlock(waterX, waterY, waterZ, BlockType::WATER, createWaterProps());
    world.setBlockState(waterX, waterY, waterZ, BlockState::LIQUID);
    world.setBlockTemperature(waterX, waterY, waterZ, 293.15f);

    world.setBlock(o2X, o2Y, o2Z, BlockType::CUSTOM, createOxygenProps());
    world.setBlockTemperature(o2X, o2Y, o2Z, 293.15f);

    std::cout << "\n  [Setup] Iron block at (" << ironX << "," << ironY << "," << ironZ << ") — corrosionRate=0.1" << std::endl;
    std::cout << "  [Setup] Water block at (" << waterX << "," << waterY << "," << waterZ << ") — LIQUID H2O" << std::endl;
    std::cout << "  [Setup] Oxygen block at (" << o2X << "," << o2Y << "," << o2Z << ") — O2" << std::endl;
    std::cout << "\n  [Tick]  Composition  Mass     Density   Hardness  Health   Type" << std::endl;
    std::cout << "  [----]  -----------  ------   -------   --------  ------   ----" << std::endl;

    bool rustDetected = false;
    int rustTick = -1;
    float initialMass = 0.0f;
    float initialHardness = 0.0f;

    for (int tick = 0; tick <= 1000; tick++) {
        if (tick > 0) {
            engine.tick(world, 0.05f);
        }

        if (tick % 100 == 0) {
            VoxelData ironData;
            world.getBlock(ironX, ironY, ironZ, ironData);

            std::string comp = ironData.props.chemical.composition;
            float mass = ironData.props.general.mass;
            float density = ironData.props.general.density;
            float hardness = ironData.props.general.hardness;
            float health = ironData.props.health.currentHealth;
            const char* typeName = blockTypeToString(ironData.type);

            if (tick == 0) {
                initialMass = mass;
                initialHardness = hardness;
            }

            std::cout << "  [" << (tick < 100 ? " " : "") << (tick < 10 ? " " : "") << tick << "]    "
                      << comp << "    "
                      << mass << "     "
                      << density << "   "
                      << hardness << "     "
                      << health << "    "
                      << typeName << std::endl;

            if (comp == "Fe2O3" && !rustDetected) {
                rustDetected = true;
                rustTick = tick;
            }
        }
    }

    VoxelData finalData;
    world.getBlock(ironX, ironY, ironZ, finalData);

    std::cout << "\n  [Result] Corrosion detected: " << (rustDetected ? "YES" : "NO") << std::endl;
    if (rustDetected) std::cout << "  [Result] Rust tick: " << rustTick << std::endl;
    std::cout << "  [Result] Final composition: " << finalData.props.chemical.composition << std::endl;
    std::cout << "  [Result] Final mass: " << finalData.props.general.mass << " (initial: " << initialMass << ")" << std::endl;
    std::cout << "  [Result] Final hardness: " << finalData.props.general.hardness << " (initial: " << initialHardness << ")" << std::endl;
    std::cout << "  [Result] Mass reduction: " << ((1.0f - finalData.props.general.mass / initialMass) * 100.0f) << "%" << std::endl;
    std::cout << "  [Result] Hardness reduction: " << ((1.0f - finalData.props.general.hardness / initialHardness) * 100.0f) << "%" << std::endl;

    std::cout << "\n  [Physics] Corrosion explanation:" << std::endl;
    std::cout << "  [Physics]   1. Iron block exposed to water + oxygen" << std::endl;
    std::cout << "  [Physics]   2. Corrosion timer increments each tick when conditions met" << std::endl;
    std::cout << "  [Physics]   3. Health decreases proportional to corrosionRate" << std::endl;
    std::cout << "  [Physics]   4. When threshold reached, iron converts to rust (Fe2O3)" << std::endl;
    std::cout << "  [Physics]   5. Rust has lower mass, density, and hardness than iron" << std::endl;
    std::cout << "  [Physics]   6. Corrosion stops if water or oxygen is removed" << std::endl;

    assert(rustDetected && "Iron should corrode to rust when exposed to water + oxygen");
    assert(finalData.props.chemical.composition == "Fe2O3" && "Final composition should be Fe2O3");
    assert(finalData.props.general.mass < initialMass && "Rust should have lower mass than iron");
    assert(finalData.props.general.hardness < initialHardness && "Rust should have lower hardness than iron");

    std::cout << "\n=============================================" << std::endl;
    std::cout << "  CORROSION TEST PASSED" << std::endl;
    std::cout << "=============================================" << std::endl;

    return 0;
}
