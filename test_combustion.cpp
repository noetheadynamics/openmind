#include "PhysicsEngine.h"
#include "VoxelOctree.h"
#include "MaterialProperties.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <string>

using namespace OpenMind;

MaterialProps createTestWoodProps() {
    MaterialProps mp;
    mp.general.mass = 0.7f;
    mp.general.density = 700.0f;
    mp.mechanical.tensileStrength = 40.0f;
    mp.thermal.thermalConductivity = 0.16f;
    mp.thermal.specificHeat = 1700.0f;
    mp.thermal.meltingPoint = 573.0f;
    mp.thermal.boilingPoint = 9999.0f;
    mp.chemical.composition = "C8H18";
    mp.chemical.flammability = 0.8f;
    mp.chemical.combustionPoint = 553.15f;
    mp.health.maxHealth = 100.0f;
    mp.health.currentHealth = 100.0f;
    mp.visual.baseColor = "#8B4513";
    return mp;
}

MaterialProps createTestFireProps() {
    MaterialProps mp;
    mp.general.mass = 0.1f;
    mp.general.density = 0.1f;
    mp.thermal.thermalConductivity = 100.0f;
    mp.thermal.specificHeat = 1000.0f;
    mp.thermal.meltingPoint = 9999.0f;
    mp.thermal.boilingPoint = 9999.0f;
    mp.thermal.heatOutput = 773.15f;
    mp.thermal.emissivity = 1.0f;
    mp.chemical.composition = "Fire";
    mp.chemical.flammability = 1.0f;
    mp.chemical.combustionPoint = 300.0f;
    mp.health.maxHealth = 10.0f;
    mp.health.currentHealth = 10.0f;
    mp.visual.baseColor = "#FF4500";
    return mp;
}

MaterialProps createTestO2Props() {
    MaterialProps mp;
    mp.general.mass = 0.5f;
    mp.general.density = 1.4f;
    mp.thermal.specificHeat = 920.0f;
    mp.chemical.composition = "O2";
    mp.health.maxHealth = 100.0f;
    mp.health.currentHealth = 100.0f;
    return mp;
}

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "  COMBUSTION TEST (Feature #30)" << std::endl;
    std::cout << "=============================================" << std::endl;

    VoxelOctree world;
    PhysicsEngine engine;
    engine.setScanRange(16);
    engine.setTemperature(293.15f);

    int woodX = 4, woodY = 4, woodZ = 4;
    int fireX = 5, fireY = 4, fireZ = 4;
    int o2X = 3, o2Y = 4, o2Z = 4;

    world.setBlock(woodX, woodY, woodZ, BlockType::WOOD, createTestWoodProps());
    world.setBlockTemperature(woodX, woodY, woodZ, 293.15f);

    world.setBlock(fireX, fireY, fireZ, BlockType::CUSTOM, createTestFireProps());
    world.setBlockTemperature(fireX, fireY, fireZ, 773.15f);

    world.setBlock(o2X, o2Y, o2Z, BlockType::CUSTOM, createTestO2Props());
    world.setBlockTemperature(o2X, o2Y, o2Z, 293.15f);

    std::cout << "\n  [Setup] Wood at (" << woodX << "," << woodY << "," << woodZ << ") — combustionPoint=553.15K, flammability=0.8, health=100" << std::endl;
    std::cout << "  [Setup] Fire at (" << fireX << "," << fireY << "," << fireZ << ") — 773.15K" << std::endl;
    std::cout << "  [Setup] O2 at (" << o2X << "," << o2Y << "," << o2Z << ")" << std::endl;
    std::cout << "\n  [Tick]  Temp(K)    Health   Burning  Type" << std::endl;
    std::cout << "  [----]  ---------  -------  -------  ----" << std::endl;

    bool ignitedDetected = false;
    int igniteTick = -1;
    bool ashDetected = false;
    int ashTick = -1;
    float maxBurnTemp = 0.0f;

    for (int tick = 0; tick <= 500; tick++) {
        if (tick > 0) {
            engine.tick(world, 0.05f);
        }

        if (tick % 10 == 0) {
            VoxelData woodData;
            world.getBlock(woodX, woodY, woodZ, woodData);

            float wtemp = woodData.currentTemperature;
            float whealth = woodData.props.health.currentHealth;
            bool burning = engine.isBurning(woodX, woodY, woodZ);
            const char* typeName = blockTypeToString(woodData.type);

            std::cout << "  [" << (tick < 10 ? " " : "") << (tick < 100 ? " " : "") << tick << "]    "
                      << wtemp << "    "
                      << whealth << "     "
                      << (burning ? "YES" : "no") << "     "
                      << typeName << std::endl;

            if (wtemp > maxBurnTemp) maxBurnTemp = wtemp;

            if (burning && !ignitedDetected) {
                ignitedDetected = true;
                igniteTick = tick;
            }

            if (woodData.type == BlockType::ASH && !ashDetected) {
                ashDetected = true;
                ashTick = tick;
            }
        }
    }

    std::cout << "\n  [Result] Wood ignited: " << (ignitedDetected ? "YES" : "NO") << std::endl;
    if (ignitedDetected) std::cout << "  [Result] Ignite tick: " << igniteTick << std::endl;
    std::cout << "  [Result] Turned to ash: " << (ashDetected ? "YES" : "NO") << std::endl;
    if (ashDetected) std::cout << "  [Result] Ash tick: " << ashTick << std::endl;
    std::cout << "  [Result] Max burn temp: " << maxBurnTemp << "K" << std::endl;

    assert(ignitedDetected && "Wood should ignite when heated above combustionPoint with O2");
    assert(ashDetected && "Wood should convert to ash after burning");
    assert(maxBurnTemp >= 553.15f && "Burn temperature should reach at least combustionPoint");

    std::cout << "\n  [Physics] Combustion explanation:" << std::endl;
    std::cout << "  [Physics]   1. Fire block heats wood via conduction" << std::endl;
    std::cout << "  [Physics]   2. Wood reaches combustionPoint (553.15K) with O2 present" << std::endl;
    std::cout << "  [Physics]   3. Wood ignites: enters self-sustaining burn state" << std::endl;
    std::cout << "  [Physics]   4. Burning wood emits heat, consumes fuel (health decreases)" << std::endl;
    std::cout << "  [Physics]   5. Fire spread: adjacent flammable blocks heated" << std::endl;
    std::cout << "  [Physics]   6. Fuel exhausted: wood converts to ash" << std::endl;

    std::cout << "\n=============================================" << std::endl;
    std::cout << "  COMBUSTION TEST PASSED" << std::endl;
    std::cout << "=============================================" << std::endl;

    return 0;
}
