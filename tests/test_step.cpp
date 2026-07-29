#include "PhysicsEngine.h"
#include "VoxelOctree.h"
#include "MaterialProperties.h"
#include <iostream>

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

void check(const VoxelOctree& world, int x, int y, int z, const char* label) {
    VoxelData data;
    bool found = world.getBlock(x, y, z, data);
    printf("  [%s] found=%d type=%d occupied=%d temp=%.2f comp=\"%s\" dens=%.1f\n",
           label, found, (int)data.type, data.occupied,
           data.currentTemperature,
           data.props.chemical.composition.c_str(),
           data.props.general.density);
}

int main() {
    printf("=== STEP DEBUG ===\n");

    VoxelOctree world;
    PhysicsEngine engine;
    engine.setScanRange(16);
    engine.setTemperature(293.15f);

    MaterialProps iceProps = createIceBlockProps();
    int x = 5, y = 2, z = 5;
    world.setBlock(x, y, z, BlockType::CUSTOM, iceProps);
    world.setBlockTemperature(x, y, z, 273.15f);
    check(world, x, y, z, "setup");

    printf("\n--- Step 1: tickPhysics only ---\n");
    // Can't call tickPhysics directly (private)
    // Use tick() but add breaks at each internal call
    // For now, use the fallthrough approach with a breakpoint
    engine.tick(world, 0.02f);
    // Check after full tick
    check(world, x, y, z, "after tick");
    
    // Try y=0 to bypass gravity
    printf("\n--- Test at y=0 (bypass gravity) ---\n");
    VoxelOctree world2;
    PhysicsEngine engine2;
    engine2.setScanRange(16);
    engine2.setTemperature(293.15f);
    world2.setBlock(5, 0, 5, BlockType::CUSTOM, iceProps);
    world2.setBlockTemperature(5, 0, 5, 273.15f);
    check(world2, 5, 0, 5, "setup y=0");
    engine2.tick(world2, 0.02f);
    check(world2, 5, 0, 5, "after tick y=0");

    // Test the gravity block specifically: make the engine call applyGravityToBlock
    // and see if the issue is in tickPhysics
    // Place at y=1 -> gravity check y<=0 ensures it's processed
    printf("\n--- Test at y=1 ---\n");
    VoxelOctree world3;
    PhysicsEngine engine3;
    engine3.setScanRange(16);
    engine3.setTemperature(293.15f);
    world3.setBlock(5, 1, 5, BlockType::CUSTOM, iceProps);
    world3.setBlockTemperature(5, 1, 5, 273.15f);
    check(world3, 5, 1, 5, "setup y=1");
    engine3.tick(world3, 0.02f);
    check(world3, 5, 1, 5, "after tick y=1");

    return 0;
}
