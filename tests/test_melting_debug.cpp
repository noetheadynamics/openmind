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
    mp.chemical.composition = "H2O";
    mp.chemical.flammability = 0.0f;
    mp.visual.baseColor = "#E0FFFF";
    mp.health.maxHealth = 100.0f;
    mp.health.currentHealth = 100.0f;
    return mp;
}

int main() {
    std::cout << "=== MELTING DEBUG V2 ===" << std::endl;
    std::cout << "Test: place ice at y=0 with fire adjacent, check after each sub-system" << std::endl;

    VoxelOctree world;
    PhysicsEngine engine;
    engine.setScanRange(16);
    engine.setTemperature(293.15f);

    MaterialProps iceProps = createIceBlockProps();
    MaterialProps fireProps = createFireBlockProps();

    int iceX = 5, iceY = 0, iceZ = 5;
    int fireX = 6, fireY = 0, fireZ = 5;

    world.setBlock(iceX, iceY, iceZ, BlockType::CUSTOM, iceProps);
    world.setBlockTemperature(iceX, iceY, iceZ, 273.15f);
    world.setBlock(fireX, fireY, fireZ, BlockType::CUSTOM, fireProps);

    auto check = [&](const char* label) {
        VoxelData data;
        bool found = world.getBlock(iceX, iceY, iceZ, data);
        std::cout << "  [" << label << "] found=" << found
                  << " type=" << (int)data.type
                  << " occupied=" << data.occupied
                  << " state=" << (data.state == BlockState::SOLID ? "SOLID" : "LIQUID")
                  << " temp=" << data.currentTemperature
                  << "K comp=\"" << data.props.chemical.composition
                  << "\" dens=" << data.props.general.density << std::endl;
    };

    check("setup");

    // Test: single tick
    std::cout << "\n--- Calling tick() ---" << std::endl;
    engine.tick(world, 0.02f);
    check("after tick 1");

    engine.tick(world, 0.02f);
    check("after tick 2");

    engine.tick(world, 0.02f);
    check("after tick 3");

    return 0;
}
