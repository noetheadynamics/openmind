#include "PhysicsEngine.h"
#include "VoxelOctree.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace OpenMind;

MaterialProps createTNTProps() {
    MaterialProps props;
    props.general.mass = 1.0f;
    props.general.density = 1600.0f;
    props.general.hardness = 1.0f;
    props.mechanical.tensileStrength = 10.0f;
    props.mechanical.compressiveStrength = 10.0f;
    props.mechanical.shearStrength = 10.0f;
    props.mechanical.fractureToughness = 1.0f;
    props.thermal.thermalConductivity = 0.5f;
    props.thermal.specificHeat = 1000.0f;
    props.thermal.meltingPoint = 350.0f;
    props.thermal.boilingPoint = 500.0f;
    props.chemical.composition = "C6H2N3O6(NO2)";
    props.chemical.flammability = 1.0f;
    props.chemical.combustionPoint = 443.15f;
    props.chemical.explosivePower = 100.0f;
    props.chemical.detonationTemperature = 443.15f;
    props.chemical.explosionRadius = 8.0f;
    props.visual.baseColor = "#FF0000";
    props.health.maxHealth = 100.0f;
    props.health.currentHealth = 100.0f;
    return props;
}

MaterialProps createWoodProps() {
    MaterialProps props;
    props.general.mass = 0.6f;
    props.general.density = 600.0f;
    props.general.hardness = 3.0f;
    props.mechanical.tensileStrength = 50.0f;
    props.mechanical.compressiveStrength = 30.0f;
    props.mechanical.shearStrength = 20.0f;
    props.mechanical.fractureToughness = 5.0f;
    props.thermal.thermalConductivity = 0.2f;
    props.thermal.specificHeat = 1700.0f;
    props.thermal.meltingPoint = 573.15f;
    props.thermal.boilingPoint = 673.15f;
    props.chemical.composition = "C6H10O5";
    props.chemical.flammability = 0.8f;
    props.chemical.combustionPoint = 553.15f;
    props.visual.baseColor = "#8B4513";
    props.health.maxHealth = 100.0f;
    props.health.currentHealth = 100.0f;
    return props;
}

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "  EXPLOSIONS TEST (Feature #33)" << std::endl;
    std::cout << "=============================================" << std::endl;

    VoxelOctree world;
    PhysicsEngine engine;
    engine.setGravity(0.0f);
    engine.setTimeScale(1.0f);
    engine.setScanRange(32);

    int tntX = 15, tntY = 15, tntZ = 15;
    int steelX = 20, steelY = 15, steelZ = 15;
    int woodX = 17, woodY = 15, woodZ = 15;
    int tnt2X = 15, tnt2Y = 16, tnt2Z = 15;

    MaterialProps tntProps = createTNTProps();
    world.setBlock(tntX, tntY, tntZ, BlockType::TNT, tntProps);
    world.setBlockTemperature(tntX, tntY, tntZ, 300.0f);

    MaterialProps steelProps = createSteelProps();
    world.setBlock(steelX, steelY, steelZ, BlockType::STEEL, steelProps);

    MaterialProps woodProps = createWoodProps();
    world.setBlock(woodX, woodY, woodZ, BlockType::WOOD, woodProps);

    MaterialProps tnt2Props = createTNTProps();
    world.setBlock(tnt2X, tnt2Y, tnt2Z, BlockType::TNT, tnt2Props);

    VoxelData tntData, steelData, woodData, tnt2Data;
    world.getBlock(tntX, tntY, tntZ, tntData);
    world.getBlock(steelX, steelY, steelZ, steelData);
    world.getBlock(woodX, woodY, woodZ, woodData);
    world.getBlock(tnt2X, tnt2Y, tnt2Z, tnt2Data);

    std::cout << "\n  [Setup] TNT at (" << tntX << "," << tntY << "," << tntZ
              << ") — explosivePower=" << tntData.props.chemical.explosivePower
              << " detonationTemp=" << tntData.props.chemical.detonationTemperature << std::endl;
    std::cout << "  [Setup] Steel at (" << steelX << "," << steelY << "," << steelZ
              << ") — health=" << steelData.props.health.currentHealth << std::endl;
    std::cout << "  [Setup] Wood at (" << woodX << "," << woodY << "," << woodZ
              << ") — health=" << woodData.props.health.currentHealth << std::endl;
    std::cout << "  [Setup] TNT2 at (" << tnt2X << "," << tnt2Y << "," << tnt2Z
              << ") — chain reaction target" << std::endl;

    std::cout << "\n  [Phase 1] Triggering explosion..." << std::endl;
    world.setBlockTemperature(tntX, tntY, tntZ, 500.0f);
    engine.triggerExplosion(world, tntX, tntY, tntZ);

    std::cout << "\n  [Phase 2] Checking explosion results..." << std::endl;

    VoxelData tntAfter;
    bool tntDestroyed = !world.getBlock(tntX, tntY, tntZ, tntAfter) || tntAfter.type == BlockType::AIR;
    std::cout << "  [Result] TNT destroyed: " << (tntDestroyed ? "YES" : "NO") << std::endl;

    VoxelData steelAfter;
    bool steelDamaged = false;
    if (world.getBlock(steelX, steelY, steelZ, steelAfter)) {
        steelDamaged = steelAfter.props.health.currentHealth < steelData.props.health.currentHealth;
        std::cout << "  [Result] Steel health: " << steelAfter.props.health.currentHealth
                  << "/" << steelData.props.health.currentHealth
                  << " (damaged: " << (steelDamaged ? "YES" : "NO") << ")" << std::endl;
    } else {
        steelDamaged = true;
        std::cout << "  [Result] Steel destroyed by shockwave" << std::endl;
    }

    VoxelData woodAfter;
    bool woodIgnited = false;
    if (world.getBlock(woodX, woodY, woodZ, woodAfter)) {
        woodIgnited = woodAfter.currentTemperature > woodData.props.chemical.combustionPoint;
        std::cout << "  [Result] Wood temp: " << woodAfter.currentTemperature
                  << "K (combustion: " << woodData.props.chemical.combustionPoint
                  << "K, ignited: " << (woodIgnited ? "YES" : "NO") << ")" << std::endl;
    } else {
        std::cout << "  [Result] Wood destroyed by shockwave" << std::endl;
        woodIgnited = true;
    }

    VoxelData tnt2After;
    bool chainReacted = false;
    if (world.getBlock(tnt2X, tnt2Y, tnt2Z, tnt2After)) {
        chainReacted = tnt2After.type == BlockType::AIR || tnt2After.currentTemperature > tnt2Data.props.chemical.detonationTemperature;
        std::cout << "  [Result] TNT2 type: " << blockTypeToString(tnt2After.type)
                  << " temp: " << tnt2After.currentTemperature
                  << "K (chain reacted: " << (chainReacted ? "YES" : "NO") << ")" << std::endl;
    } else {
        chainReacted = true;
        std::cout << "  [Result] TNT2 destroyed (chain reaction: YES)" << std::endl;
    }

    auto fragments = engine.getPendingFragments();
    bool fragmentsSpawned = !fragments.empty();
    std::cout << "  [Result] Fragments spawned: " << (fragmentsSpawned ? "YES" : "NO")
              << " (count: " << fragments.size() << ")" << std::endl;

    std::cout << "\n  [Phase 3] Running physics ticks to observe aftermath..." << std::endl;
    for (int i = 0; i < 10; i++) {
        engine.tick(world, 1.0f);
    }

    std::cout << "\n  [Result] Summary:" << std::endl;
    std::cout << "    TNT destroyed: " << (tntDestroyed ? "PASS" : "FAIL") << std::endl;
    std::cout << "    Steel damaged: " << (steelDamaged ? "PASS" : "FAIL") << std::endl;
    std::cout << "    Wood ignited:  " << (woodIgnited ? "PASS" : "FAIL") << std::endl;
    std::cout << "    Chain reacted: " << (chainReacted ? "PASS" : "FAIL") << std::endl;
    std::cout << "    Fragments:     " << (fragmentsSpawned ? "PASS" : "FAIL") << std::endl;

    bool allPassed = tntDestroyed && steelDamaged && woodIgnited && chainReacted && fragmentsSpawned;

    std::cout << "\n  [Physics] Explosion explanation:" << std::endl;
    std::cout << "  [Physics]   1. TNT block reaches detonationTemperature" << std::endl;
    std::cout << "  [Physics]   2. triggerExplosion() called" << std::endl;
    std::cout << "  [Physics]   3. applyShockwave() — radial force damages blocks" << std::endl;
    std::cout << "  [Physics]   4. spawnFragments() — debris created" << std::endl;
    std::cout << "  [Physics]   5. processChainReactions() — adjacent explosives triggered" << std::endl;
    std::cout << "  [Physics]   6. Heat spike ignites flammable blocks nearby" << std::endl;

    std::cout << "\n=============================================" << std::endl;
    if (allPassed) {
        std::cout << "  EXPLOSIONS TEST PASSED" << std::endl;
    } else {
        std::cout << "  EXPLOSIONS TEST FAILED" << std::endl;
    }
    std::cout << "=============================================" << std::endl;

    assert(tntDestroyed && "TNT should be destroyed by its own explosion");
    assert(steelDamaged && "Steel should be damaged by shockwave");
    assert(woodIgnited && "Wood should be ignited by explosion heat");
    assert(chainReacted && "Chain reaction should propagate to adjacent TNT");
    assert(fragmentsSpawned && "Fragments should be spawned");

    return 0;
}
