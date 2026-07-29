#include "PhysicsEngine.h"
#include "VoxelOctree.h"
#include <iostream>
#include <cassert>

using namespace OpenMind;

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
    props.biological.isOrganic = true;
    props.biological.decayThreshold = 500.0f;
    props.visual.baseColor = "#8B4513";
    props.health.maxHealth = 100.0f;
    props.health.currentHealth = 100.0f;
    return props;
}

MaterialProps createWaterProps() {
    MaterialProps props;
    props.general.mass = 1.0f;
    props.general.density = 1000.0f;
    props.general.hardness = 0.0f;
    props.mechanical.tensileStrength = 0.0f;
    props.mechanical.compressiveStrength = 0.0f;
    props.mechanical.shearStrength = 0.0f;
    props.mechanical.fractureToughness = 0.0f;
    props.thermal.thermalConductivity = 0.6f;
    props.thermal.specificHeat = 4186.0f;
    props.thermal.meltingPoint = 273.15f;
    props.thermal.boilingPoint = 373.15f;
    props.chemical.composition = "H2O";
    props.chemical.flammability = 0.0f;
    props.chemical.corrosionRate = 0.0f;
    props.chemical.chemicalResistance = 0.0f;
    props.visual.baseColor = "#006994";
    props.health.maxHealth = 100.0f;
    props.health.currentHealth = 100.0f;
    return props;
}

MaterialProps createStoneProps() {
    MaterialProps props;
    props.general.mass = 2.5f;
    props.general.density = 2500.0f;
    props.general.hardness = 7.0f;
    props.mechanical.tensileStrength = 10.0f;
    props.mechanical.compressiveStrength = 100.0f;
    props.mechanical.shearStrength = 8.0f;
    props.mechanical.fractureToughness = 3.0f;
    props.thermal.thermalConductivity = 2.0f;
    props.thermal.specificHeat = 800.0f;
    props.thermal.meltingPoint = 1500.0f;
    props.thermal.boilingPoint = 2500.0f;
    props.chemical.composition = "SiO2";
    props.chemical.chemicalResistance = 0.9f;
    props.visual.baseColor = "#808080";
    props.health.maxHealth = 500.0f;
    props.health.currentHealth = 500.0f;
    return props;
}

void sealChamber(VoxelOctree& world, int cx, int cy, int cz, const MaterialProps& stoneProps) {
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            world.setBlock(cx + dx, cy - 1, cz + dz, BlockType::STONE, stoneProps);
            if (dx != 0 || dz != 0) {
                world.setBlock(cx + dx, cy, cz + dz, BlockType::STONE, stoneProps);
            }
            world.setBlock(cx + dx, cy + 1, cz + dz, BlockType::STONE, stoneProps);
        }
    }
}

int countBlocksOfType(const VoxelOctree& world, int cx, int cy, int cz, int radius, BlockType target) {
    int count = 0;
    for (int x = cx - radius; x <= cx + radius; x++) {
        for (int y = cy - radius; y <= cy + radius; y++) {
            for (int z = cz - radius; z <= cz + radius; z++) {
                VoxelData fd;
                if (world.getBlock(x, y, z, fd) && fd.type == target) {
                    count++;
                }
            }
        }
    }
    return count;
}

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "  DECAY TEST (Feature #40)" << std::endl;
    std::cout << "=============================================" << std::endl;

    VoxelOctree world;
    PhysicsEngine engine;
    engine.setGravity(0.0f);
    engine.setTimeScale(1.0f);
    engine.setScanRange(20);

    MaterialProps stoneProps = createStoneProps();
    MaterialProps woodProps = createWoodProps();
    MaterialProps waterProps = createWaterProps();

    for (int x = 5; x <= 15; x++) {
        for (int z = 5; z <= 15; z++) {
            world.setBlock(x, 3, z, BlockType::STONE, stoneProps);
        }
    }

    int warmMoistX = 10, warmMoistY = 6, warmMoistZ = 10;
    world.setBlock(warmMoistX, warmMoistY, warmMoistZ, BlockType::WOOD, woodProps);
    world.setBlockTemperature(warmMoistX, warmMoistY, warmMoistZ, 303.15f);
    world.setBlock(warmMoistX, 5, warmMoistZ, BlockType::WATER, waterProps);
    world.setBlockState(warmMoistX, 5, warmMoistZ, BlockState::LIQUID);
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            if (dx == 0 && dz == 0) continue;
            world.setBlock(warmMoistX + dx, 5, warmMoistZ + dz, BlockType::AIR);
        }
    }

    int coldSealedX = 8, coldSealedY = 6, coldSealedZ = 10;
    world.setBlock(coldSealedX, coldSealedY, coldSealedZ, BlockType::WOOD, woodProps);
    world.setBlockTemperature(coldSealedX, coldSealedY, coldSealedZ, 253.15f);
    sealChamber(world, coldSealedX, coldSealedY, coldSealedZ, stoneProps);

    int warmSealedX = 12, warmSealedY = 6, warmSealedZ = 10;
    world.setBlock(warmSealedX, warmSealedY, warmSealedZ, BlockType::WOOD, woodProps);
    world.setBlockTemperature(warmSealedX, warmSealedY, warmSealedZ, 303.15f);
    sealChamber(world, warmSealedX, warmSealedY, warmSealedZ, stoneProps);

    std::cout << "\n  [Setup] Wood1 at (" << warmMoistX << "," << warmMoistY << "," << warmMoistZ
              << ") — warm (303K), moist, oxygen-rich" << std::endl;
    std::cout << "  [Setup] Wood2 at (" << coldSealedX << "," << coldSealedY << "," << coldSealedZ
              << ") — cold (253K), sealed (no air)" << std::endl;
    std::cout << "  [Setup] Wood3 at (" << warmSealedX << "," << warmSealedY << "," << warmSealedZ
              << ") — warm (303K), sealed (no air)" << std::endl;

    std::cout << "\n  [Phase 1] Running 1000 ticks..." << std::endl;
    std::cout << "  [Tick]  Wood1     Wood2     Wood3     DirtCount" << std::endl;
    std::cout << "  [----]  -------   -------   -------   ---------" << std::endl;

    for (int tick = 1; tick <= 1000; tick++) {
        engine.tick(world, 1.0f);

        if (tick % 200 == 0) {
            VoxelData w1, w2, w3;
            world.getBlock(warmMoistX, warmMoistY, warmMoistZ, w1);
            world.getBlock(coldSealedX, coldSealedY, coldSealedZ, w2);
            world.getBlock(warmSealedX, warmSealedY, warmSealedZ, w3);

            std::cout << "  [" << tick << "]    "
                      << blockTypeToString(w1.type) << "    "
                      << blockTypeToString(w2.type) << "    "
                      << blockTypeToString(w3.type) << "    "
                      << countBlocksOfType(world, 10, 6, 10, 5, BlockType::DIRT) << std::endl;
        }
    }

    std::cout << "\n  [Phase 2] Checking results..." << std::endl;

    VoxelData w1Final, w2Final, w3Final;
    world.getBlock(warmMoistX, warmMoistY, warmMoistZ, w1Final);
    world.getBlock(coldSealedX, coldSealedY, coldSealedZ, w2Final);
    world.getBlock(warmSealedX, warmSealedY, warmSealedZ, w3Final);

    bool wood1Decayed = (w1Final.type != BlockType::WOOD);
    bool wood2Preserved = (w2Final.type == BlockType::WOOD);
    bool wood3Slower = (w3Final.type == BlockType::WOOD);

    std::cout << "  [Result] Wood1 (warm/moist): " << blockTypeToString(w1Final.type)
              << " (decayed: " << (wood1Decayed ? "YES" : "NO") << ")" << std::endl;
    std::cout << "  [Result] Wood2 (cold/sealed): " << blockTypeToString(w2Final.type)
              << " (preserved: " << (wood2Preserved ? "YES" : "NO") << ")" << std::endl;
    std::cout << "  [Result] Wood3 (warm/sealed): " << blockTypeToString(w3Final.type)
              << " (slower: " << (wood3Slower ? "YES" : "NO") << ")" << std::endl;

    int dirtCount = countBlocksOfType(world, 10, 6, 10, 5, BlockType::DIRT);
    bool productsSpawned = (dirtCount > 0);
    std::cout << "  [Result] Dirt blocks spawned: " << dirtCount
              << " (products: " << (productsSpawned ? "YES" : "NO") << ")" << std::endl;

    std::cout << "\n  [Result] Summary:" << std::endl;
    std::cout << "    Wood decayed:       " << (wood1Decayed ? "PASS" : "FAIL") << std::endl;
    std::cout << "    Cold preserved:     " << (wood2Preserved ? "PASS" : "FAIL") << std::endl;
    std::cout << "    Products spawned:   " << (productsSpawned ? "PASS" : "FAIL") << std::endl;
    std::cout << "    Warm sealed slower: " << (wood3Slower ? "PASS" : "FAIL") << std::endl;

    bool allPassed = wood1Decayed && wood2Preserved && productsSpawned && wood3Slower;

    std::cout << "\n  [Physics] Decay explanation:" << std::endl;
    std::cout << "  [Physics]   1. Organic blocks decay when exposed to environment" << std::endl;
    std::cout << "  [Physics]   2. Decay rate depends on temperature, moisture, oxygen" << std::endl;
    std::cout << "  [Physics]   3. Warm + moist + oxygen = fastest decay" << std::endl;
    std::cout << "  [Physics]   4. Cold or sealed = slow/no decay" << std::endl;
    std::cout << "  [Physics]   5. Decay produces dirt, compost, or gas" << std::endl;

    std::cout << "\n=============================================" << std::endl;
    if (allPassed) {
        std::cout << "  DECAY TEST PASSED" << std::endl;
    } else {
        std::cout << "  DECAY TEST FAILED" << std::endl;
    }
    std::cout << "=============================================" << std::endl;

    assert(wood1Decayed && "Wood should decay in warm/moist environment");
    assert(wood2Preserved && "Wood should be preserved in cold environment");
    assert(productsSpawned && "Decay products should be spawned");
    assert(wood3Slower && "Warm sealed should decay slower than warm moist");

    return 0;
}
