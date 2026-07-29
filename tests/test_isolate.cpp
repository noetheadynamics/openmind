#include "PhysicsEngine.h"
#include "VoxelOctree.h"
#include "MaterialProperties.h"
#include <iostream>
#include <cassert>

using namespace OpenMind;

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
    std::cout << "=== ISOLATION TEST ===" << std::endl;

    // Test 1: ice at (5,2,5) with STONE at (5,1,5), NO fire
    {
        std::cout << "\nTest 1: Ice with stone floor below, no fire" << std::endl;
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setScanRange(16);
        engine.setTemperature(293.15f);

        MaterialProps iceProps = createIceBlockProps();
        MaterialProps stoneProps = createStoneBlockProps();

        world.setBlock(5, 1, 5, BlockType::STONE, stoneProps);
        world.setBlock(5, 2, 5, BlockType::CUSTOM, iceProps);
        world.setBlockTemperature(5, 2, 5, 273.15f);

        VoxelData before;
        world.getBlock(5, 2, 5, before);
        std::cout << "  Before tick: type=" << (int)before.type << " comp=\"" << before.props.chemical.composition << "\" dens=" << before.props.general.density << std::endl;

        engine.tick(world, 0.02f);

        VoxelData after;
        world.getBlock(5, 2, 5, after);
        std::cout << "  After tick:  type=" << (int)after.type << " comp=\"" << after.props.chemical.composition << "\" dens=" << after.props.general.density << std::endl;
    }

    // Test 2: ice ONLY, no stone, no fire
    {
        std::cout << "\nTest 2: Ice at y=2 with nothing below" << std::endl;
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setScanRange(16);
        engine.setTemperature(293.15f);

        MaterialProps iceProps = createIceBlockProps();

        world.setBlock(5, 2, 5, BlockType::CUSTOM, iceProps);
        world.setBlockTemperature(5, 2, 5, 273.15f);

        VoxelData before;
        world.getBlock(5, 2, 5, before);
        std::cout << "  Before tick: type=" << (int)before.type << " comp=\"" << before.props.chemical.composition << "\" dens=" << before.props.general.density << std::endl;

        engine.tick(world, 0.02f);

        VoxelData after;
        world.getBlock(5, 2, 5, after);
        std::cout << "  After tick:  type=" << (int)after.type << " comp=\"" << after.props.chemical.composition << "\" dens=" << after.props.general.density << std::endl;
    }

    return 0;
}
