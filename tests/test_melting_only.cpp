#include "PhysicsEngine.h"
#include "VoxelOctree.h"
#include "MaterialProperties.h"
#include <iostream>
#include <cassert>

using namespace OpenMind;

MaterialProps createFireBlockProps() {
    MaterialProps mp;
    mp.general.mass = 0.1f;
    mp.general.density = 0.1f;
    mp.thermal.thermalConductivity = 100.0f;
    mp.thermal.specificHeat = 1000.0f;
    mp.thermal.meltingPoint = 9999.0f;
    mp.thermal.boilingPoint = 9999.0f;
    mp.thermal.heatOutput = 773.15f;
    mp.thermal.emissivity = 1.0f;
    mp.thermal.radiationAbsorption = 0.0f;
    mp.chemical.composition = "Fire";
    mp.chemical.flammability = 1.0f;
    mp.chemical.combustionPoint = 300.0f;
    mp.visual.baseColor = "#FF4500";
    mp.health.maxHealth = 10.0f;
    mp.health.currentHealth = 10.0f;
    return mp;
}

MaterialProps createIceBlockProps() {
    MaterialProps mp;
    mp.general.mass = 1.0f;
    mp.general.density = 917.0f;
    mp.thermal.thermalConductivity = 2.22f;
    mp.thermal.specificHeat = 2093.0f;
    mp.thermal.meltingPoint = 273.15f;
    mp.thermal.boilingPoint = 373.15f;
    mp.thermal.latentHeatOfFusion = 334000.0f;
    mp.thermal.liquidDensityFactor = 1.09f;
    mp.thermal.freezingPoint = 273.15f;
    mp.chemical.composition = "H2O";
    mp.chemical.flammability = 0.0f;
    mp.visual.baseColor = "#E0FFFF";
    mp.health.maxHealth = 100.0f;
    mp.health.currentHealth = 100.0f;
    return mp;
}

MaterialProps createStoneBlockProps() {
    MaterialProps mp;
    mp.general.mass = 500.0f;
    mp.general.density = 2500.0f;
    mp.thermal.thermalConductivity = 2.0f;
    mp.thermal.specificHeat = 800.0f;
    mp.thermal.meltingPoint = 1500.0f;
    mp.thermal.boilingPoint = 3000.0f;
    mp.mechanical.tensileStrength = 1000.0f;
    mp.chemical.composition = "SiO2";
    mp.visual.baseColor = "#808080";
    mp.health.maxHealth = 1000.0f;
    mp.health.currentHealth = 1000.0f;
    return mp;
}

int main() {
    std::cout << "=== MELTING TEST ===" << std::endl;

    VoxelOctree world;
    PhysicsEngine engine;
    engine.setScanRange(16);
    engine.setTemperature(293.15f);

    MaterialProps iceProps = createIceBlockProps();
    MaterialProps fireProps = createFireBlockProps();
    MaterialProps stoneProps = createStoneBlockProps();

    int iceX = 5, iceY = 2, iceZ = 5;
    int fireX = 6, fireY = 2, fireZ = 5;

    for (int x = 4; x <= 7; x++) {
        for (int z = 4; z <= 6; z++) {
            world.setBlock(x, 1, z, BlockType::STONE, stoneProps);
        }
    }

    world.setBlock(4, 2, 5, BlockType::STONE, stoneProps);
    world.setBlock(5, 2, 4, BlockType::STONE, stoneProps);
    world.setBlock(5, 2, 6, BlockType::STONE, stoneProps);

    world.setBlock(iceX, iceY, iceZ, BlockType::CUSTOM, iceProps);
    world.setBlockTemperature(iceX, iceY, iceZ, 273.15f);

    world.setBlock(fireX, fireY, fireZ, BlockType::CUSTOM, fireProps);

    std::cout << "  [Setup] Ice block at (" << iceX << "," << iceY << "," << iceZ << ") — 273.15K (0C)" << std::endl;
    std::cout << "  [Setup] Fire block at (" << fireX << "," << fireY << "," << fireZ << ") — 773.15K (500C)" << std::endl;
    std::cout << "  [Setup] Stone floor at y=1" << std::endl;
    std::cout << "  [Setup] Ice melting point: " << iceProps.thermal.meltingPoint << "K" << std::endl;
    std::cout << "  [Setup] Ice latent heat of fusion: " << iceProps.thermal.latentHeatOfFusion << " J/kg" << std::endl;
    std::cout << "  [Setup] Hysteresis margin: 5C" << std::endl;

    VoxelData iceStart;
    world.getBlock(iceX, iceY, iceZ, iceStart);
    std::cout << "  [Setup] Initial ice state: " << (iceStart.state == BlockState::SOLID ? "SOLID" : "LIQUID") << std::endl;
    std::cout << "  [Setup] Initial ice composition: " << iceStart.props.chemical.composition << std::endl;
    std::cout << "  [Setup] Initial ice density: " << iceStart.props.general.density << " kg/m3" << std::endl;

    std::cout << "\n  [Tick]  State    Temp(K)   Composition  Density" << std::endl;
    std::cout << "  [----]  -----    -------   -----------  -------" << std::endl;

    bool meltDetected = false;
    int meltTick = -1;

    for (int tick = 0; tick <= 100; tick++) {
        if (tick > 0) {
            engine.tick(world, 0.02f);
        }

        if (tick % 5 == 0) {
            VoxelData iceData;
            world.getBlock(iceX, iceY, iceZ, iceData);

            const char* stateStr = "SOLID";
            if (iceData.state == BlockState::LIQUID) stateStr = "LIQUID";
            else if (iceData.state == BlockState::GAS) stateStr = "GAS";

            std::cout << "  [" << (tick < 10 ? " " : "") << (tick < 100 ? " " : "") << tick << "]    "
                      << stateStr << "   "
                      << iceData.currentTemperature << "K   "
                      << iceData.props.chemical.composition << "          "
                      << iceData.props.general.density << std::endl;

            if (!meltDetected && iceData.state == BlockState::LIQUID) {
                meltDetected = true;
                meltTick = tick;
            }
        }
    }

    VoxelData iceFinal;
    world.getBlock(iceX, iceY, iceZ, iceFinal);

    std::cout << "\n  [Result] Melt detected: " << (meltDetected ? "YES" : "NO") << std::endl;
    if (meltDetected) {
        std::cout << "  [Result] Melt tick: " << meltTick << std::endl;
    }
    std::cout << "  [Result] Final state: " << (iceFinal.state == BlockState::LIQUID ? "LIQUID" : "SOLID") << std::endl;
    std::cout << "  [Result] Final temperature: " << iceFinal.currentTemperature << "K" << std::endl;
    std::cout << "  [Result] Final composition: " << iceFinal.props.chemical.composition << std::endl;
    std::cout << "  [Result] Final density: " << iceFinal.props.general.density << " kg/m3" << std::endl;

    std::cout << "\n  [Physics] Melting explanation:" << std::endl;
    std::cout << "  [Physics]   1. Fire radiates heat to ice via conduction and radiation" << std::endl;
    std::cout << "  [Physics]   2. Ice temperature rises toward melting point (273.15K)" << std::endl;
    std::cout << "  [Physics]   3. At meltingPoint + hysteresisMargin (278.15K), melting triggers" << std::endl;
    std::cout << "  [Physics]   4. Latent heat of fusion is absorbed: Q = mass * L_f" << std::endl;
    std::cout << "  [Physics]   5. Temperature is capped at melting point during phase change" << std::endl;
    std::cout << "  [Physics]   6. Block transitions SOLID -> LIQUID, composition retained" << std::endl;

    assert(meltDetected && "Ice should melt when heated by fire");
    assert(iceFinal.state == BlockState::LIQUID && "Ice should be in LIQUID state after melting");
    assert(iceFinal.props.chemical.composition == "H2O" && "Composition should be retained after melting");

    std::cout << "\n  [PASS] Melting test completed." << std::endl;
    return 0;
}
