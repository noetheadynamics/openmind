#include "PhysicsEngine.h"
#include "VoxelOctree.h"
#include <iostream>
#include <cassert>

using namespace OpenMind;

MaterialProps createSeedProps() {
    MaterialProps props;
    props.general.mass = 0.1f;
    props.general.density = 800.0f;
    props.general.hardness = 1.0f;
    props.mechanical.tensileStrength = 2.0f;
    props.mechanical.compressiveStrength = 2.0f;
    props.mechanical.shearStrength = 1.0f;
    props.mechanical.fractureToughness = 0.5f;
    props.thermal.thermalConductivity = 0.1f;
    props.thermal.specificHeat = 2000.0f;
    props.thermal.meltingPoint = 350.0f;
    props.thermal.boilingPoint = 450.0f;
    props.biological.isOrganic = true;
    props.biological.growthRate = 1.0f;
    props.biological.soilType = "LOAMY_SOIL";
    props.biological.decayThreshold = 10000.0f;
    props.chemical.composition = "Organic";
    props.visual.baseColor = "#8B4513";
    props.health.maxHealth = 50.0f;
    props.health.currentHealth = 50.0f;
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

MaterialProps createLoamySoilProps() {
    MaterialProps props;
    props.general.mass = 1.5f;
    props.general.density = 1200.0f;
    props.general.hardness = 2.0f;
    props.mechanical.tensileStrength = 10.0f;
    props.mechanical.compressiveStrength = 20.0f;
    props.mechanical.shearStrength = 8.0f;
    props.mechanical.fractureToughness = 2.0f;
    props.thermal.thermalConductivity = 0.5f;
    props.thermal.specificHeat = 800.0f;
    props.thermal.meltingPoint = 1000.0f;
    props.thermal.boilingPoint = 1500.0f;
    props.biological.soilType = "LOAMY_SOIL";
    props.chemical.composition = "SiO2-Al2O3";
    props.visual.baseColor = "#8B4513";
    props.health.maxHealth = 200.0f;
    props.health.currentHealth = 200.0f;
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

const char* plantStateStr(PlantState s) {
    switch (s) {
        case PlantState::NONE: return "NONE";
        case PlantState::SEED: return "SEED";
        case PlantState::SPROUT: return "SPROUT";
        case PlantState::PLANT: return "PLANT";
        case PlantState::FRUIT: return "FRUIT";
        default: return "UNKNOWN";
    }
}

int countFruitsNear(const VoxelOctree& world, int cx, int cy, int cz, int radius) {
    int count = 0;
    for (int x = cx - radius; x <= cx + radius; x++) {
        for (int y = cy - radius; y <= cy + radius; y++) {
            for (int z = cz - radius; z <= cz + radius; z++) {
                VoxelData fd;
                if (world.getBlock(x, y, z, fd)) {
                    if (fd.type == BlockType::CUSTOM && fd.props.chemical.composition == "Organic" &&
                        fd.props.health.maxHealth < 30.0f) {
                        count++;
                    }
                }
            }
        }
    }
    return count;
}

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "  PLANT GROWTH TEST (Feature #34)" << std::endl;
    std::cout << "=============================================" << std::endl;

    VoxelOctree world;
    PhysicsEngine engine;
    engine.setGravity(0.0f);
    engine.setTimeScale(1.0f);
    engine.setScanRange(20);

    int seedX = 10, seedY = 5, seedZ = 10;
    int soilX = 10, soilY = 4, soilZ = 10;
    int waterX = 11, waterY = 4, waterZ = 11;

    MaterialProps stoneProps = createStoneProps();
    MaterialProps soilProps = createLoamySoilProps();
    MaterialProps waterProps = createWaterProps();

    for (int x = 7; x <= 13; x++) {
        for (int z = 7; z <= 13; z++) {
            world.setBlock(x, 3, z, BlockType::STONE, stoneProps);
        }
    }
    for (int x = 8; x <= 12; x++) {
        for (int z = 8; z <= 12; z++) {
            world.setBlock(x, 4, z, BlockType::DIRT, soilProps);
        }
    }
    for (int z = 8; z <= 12; z++) {
        world.setBlock(8, 5, z, BlockType::STONE, stoneProps);
        world.setBlock(12, 5, z, BlockType::STONE, stoneProps);
    }
    for (int x = 8; x <= 12; x++) {
        world.setBlock(x, 5, 8, BlockType::STONE, stoneProps);
        world.setBlock(x, 5, 12, BlockType::STONE, stoneProps);
    }

    MaterialProps seedProps = createSeedProps();
    world.setBlock(seedX, seedY, seedZ, BlockType::CUSTOM, seedProps);
    world.setBlock(waterX, waterY, waterZ, BlockType::WATER, waterProps);
    world.setBlockState(waterX, waterY, waterZ, BlockState::LIQUID);

    std::cout << "\n  [Setup] Seed at (" << seedX << "," << seedY << "," << seedZ << ")" << std::endl;
    std::cout << "  [Setup] Loamy soil at (" << soilX << "," << soilY << "," << soilZ << ")" << std::endl;
    std::cout << "  [Setup] Water at (" << waterX << "," << waterY << "," << waterZ << ")" << std::endl;

    std::cout << "\n  [Phase 1] Running 2000 ticks..." << std::endl;
    std::cout << "  [Tick]  Stage     Timer    FruitCount" << std::endl;
    std::cout << "  [----]  --------  -----    ----------" << std::endl;

    bool sproutReached = false;
    bool plantReached = false;
    bool fruitReached = false;

    for (int tick = 1; tick <= 2000; tick++) {
        engine.tick(world, 1.0f);

        const PlantData* p = engine.getPlantData(seedX, seedY, seedZ);
        PlantState ps = p ? p->state : PlantState::NONE;
        float timer = p ? p->growthTimer : -1.0f;

        if (ps == PlantState::SPROUT) sproutReached = true;
        if (ps == PlantState::PLANT) plantReached = true;
        if (ps == PlantState::FRUIT) fruitReached = true;

        if (tick == 500 || tick == 1000 || tick == 1500 || tick == 2000) {
            int fruitCount = countFruitsNear(world, seedX, seedY, seedZ, 3);
            std::cout << "  [" << tick << "]    " << plantStateStr(ps) << "    " << timer << "    " << fruitCount << std::endl;
        }
    }

    std::cout << "\n  [Phase 2] Checking stage progression..." << std::endl;
    std::cout << "  [Result] SPROUT reached: " << (sproutReached ? "YES" : "NO") << std::endl;
    std::cout << "  [Result] PLANT reached: " << (plantReached ? "YES" : "NO") << std::endl;
    std::cout << "  [Result] FRUIT reached: " << (fruitReached ? "YES" : "NO") << std::endl;

    int finalFruitCount = countFruitsNear(world, seedX, seedY, seedZ, 3);
    std::cout << "  [Result] Fruit blocks produced: " << finalFruitCount << std::endl;

    std::cout << "\n  [Phase 3] Testing growth pause when water removed..." << std::endl;
    world.setBlock(waterX, waterY, waterZ, BlockType::AIR);
    int fruitAtPause = finalFruitCount;

    for (int tick = 0; tick < 500; tick++) {
        engine.tick(world, 1.0f);
    }

    const PlantData* pdAfterPause = engine.getPlantData(seedX, seedY, seedZ);
    PlantState stateAfterPause = pdAfterPause ? pdAfterPause->state : PlantState::NONE;
    int fruitAfterPause = countFruitsNear(world, seedX, seedY, seedZ, 5);
    bool growthPaused = (fruitAfterPause == fruitAtPause);
    std::cout << "  [Result] State after pause: " << plantStateStr(stateAfterPause) << std::endl;
    std::cout << "  [Result] Fruit before/after: " << fruitAtPause << " / " << fruitAfterPause << std::endl;
    std::cout << "  [Result] Growth paused: " << (growthPaused ? "PASS" : "FAIL") << std::endl;

    std::cout << "\n  [Phase 4] Testing growth resume when water restored..." << std::endl;
    world.setBlock(waterX, waterY, waterZ, BlockType::WATER, waterProps);
    world.setBlockState(waterX, waterY, waterZ, BlockState::LIQUID);

    for (int tick = 0; tick < 500; tick++) {
        engine.tick(world, 1.0f);
    }

    int fruitAfterResume = countFruitsNear(world, seedX, seedY, seedZ, 5);
    bool growthResumed = (fruitAfterResume > fruitAfterPause);
    std::cout << "  [Result] Fruit after resume: " << fruitAfterResume << std::endl;
    std::cout << "  [Result] Growth resumed: " << (growthResumed ? "PASS" : "FAIL") << std::endl;

    bool allPassed = sproutReached && plantReached && fruitReached && finalFruitCount > 0 && growthPaused && growthResumed;

    std::cout << "\n  [Result] Summary:" << std::endl;
    std::cout << "    Stage progression:  " << (sproutReached && plantReached && fruitReached ? "PASS" : "FAIL") << std::endl;
    std::cout << "    Fruit produced:     " << (finalFruitCount > 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "    Growth paused:      " << (growthPaused ? "PASS" : "FAIL") << std::endl;
    std::cout << "    Growth resumed:     " << (growthResumed ? "PASS" : "FAIL") << std::endl;

    std::cout << "\n=============================================" << std::endl;
    if (allPassed) {
        std::cout << "  PLANT GROWTH TEST PASSED" << std::endl;
    } else {
        std::cout << "  PLANT GROWTH TEST FAILED" << std::endl;
    }
    std::cout << "=============================================" << std::endl;

    assert(sproutReached && "SEED should reach SPROUT stage");
    assert(plantReached && "SPROUT should reach PLANT stage");
    assert(fruitReached && "PLANT should reach FRUIT stage");
    assert(finalFruitCount > 0 && "Fruit blocks should be produced");
    assert(growthPaused && "Growth should pause when conditions removed");
    assert(growthResumed && "Growth should resume when conditions restored");

    return 0;
}
