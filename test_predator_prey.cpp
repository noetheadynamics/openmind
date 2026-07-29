#include "PhysicsEngine.h"
#include "VoxelOctree.h"
#include <iostream>
#include <iomanip>
#include <cassert>
#include <vector>
#include <string>

using namespace OpenMind;

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

MaterialProps createGrassProps() {
    MaterialProps props;
    props.general.mass = 0.5f;
    props.general.density = 500.0f;
    props.general.hardness = 1.0f;
    props.mechanical.tensileStrength = 5.0f;
    props.mechanical.compressiveStrength = 3.0f;
    props.mechanical.shearStrength = 2.0f;
    props.mechanical.fractureToughness = 1.0f;
    props.thermal.thermalConductivity = 0.2f;
    props.thermal.specificHeat = 1500.0f;
    props.thermal.meltingPoint = 473.15f;
    props.thermal.boilingPoint = 673.15f;
    props.chemical.composition = "C6H10O5";
    props.chemical.flammability = 0.6f;
    props.chemical.combustionPoint = 553.15f;
    props.biological.isOrganic = true;
    props.biological.decayThreshold = 5000.0f;
    props.visual.baseColor = "#228B22";
    props.health.maxHealth = 50.0f;
    props.health.currentHealth = 50.0f;
    return props;
}

void buildArena(VoxelOctree& world, int size, const MaterialProps& stoneProps, const MaterialProps& grassProps) {
    for (int x = 0; x < size; x++) {
        for (int z = 0; z < size; z++) {
            world.setBlock(x, 0, z, BlockType::STONE, stoneProps);
            world.setBlock(x, 1, z, BlockType::GRASS, grassProps);
        }
    }
    for (int x = 0; x < size; x++) {
        for (int y = 0; y <= 3; y++) {
            world.setBlock(x, y, 0, BlockType::STONE, stoneProps);
            world.setBlock(x, y, size - 1, BlockType::STONE, stoneProps);
            world.setBlock(0, y, x, BlockType::STONE, stoneProps);
            world.setBlock(size - 1, y, x, BlockType::STONE, stoneProps);
        }
    }
}

Agent createPrey(float x, float y, float z) {
    Agent a;
    a.isPrey = true;
    a.isPredator = false;
    a.x = x; a.y = y; a.z = z;
    a.health = 80.0f;
    a.maxHealth = 100.0f;
    a.energy = 90.0f;
    a.maxEnergy = 100.0f;
    a.speed = 1.0f;
    a.attackPower = 2.0f;
    a.defensePower = 10.0f;
    a.visionRange = 5.0f;
    a.dangerRange = 8.0f;
    a.hungerRate = 0.0f;
    a.reproductionInterval = 300.0f;
    return a;
}

Agent createPredator(float x, float y, float z) {
    Agent a;
    a.isPredator = true;
    a.isPrey = false;
    a.x = x; a.y = y; a.z = z;
    a.health = 100.0f;
    a.maxHealth = 100.0f;
    a.energy = 100.0f;
    a.maxEnergy = 100.0f;
    a.speed = 1.3f;
    a.attackPower = 20.0f;
    a.defensePower = 5.0f;
    a.visionRange = 10.0f;
    a.dangerRange = 0.0f;
    a.hunger = 0.0f;
    a.maxHunger = 100.0f;
    a.hungerRate = 0.08f;
    a.attackCooldownMax = 8.0f;
    return a;
}

int countAlive(const std::vector<Agent>& agents, bool prey, bool predator) {
    int count = 0;
    for (const auto& a : agents) {
        if (!a.isAlive) continue;
        if (prey && a.isPrey) count++;
        if (predator && a.isPredator) count++;
    }
    return count;
}

int main() {
    std::cout << "=============================================\n";
    std::cout << "  PREDATOR-PREY DYNAMICS TEST (Feature #41)\n";
    std::cout << "=============================================\n\n";

    const int ARENA_SIZE = 20;
    const int TOTAL_TICKS = 1000;
    const int LOG_INTERVAL = 100;
    const int INITIAL_PREY = 10;
    const int INITIAL_PREDATORS = 2;

    VoxelOctree world;
    PhysicsEngine engine;
    engine.setGravity(0.0f);
    engine.setTimeScale(1.0f);
    engine.setScanRange(ARENA_SIZE);

    MaterialProps stoneProps = createStoneProps();
    MaterialProps grassProps = createGrassProps();

    std::cout << "  [Setup] Building " << ARENA_SIZE << "x" << ARENA_SIZE << " arena...\n";
    buildArena(world, ARENA_SIZE, stoneProps, grassProps);

    std::cout << "  [Setup] Spawning " << INITIAL_PREY << " prey agents...\n";
    for (int i = 0; i < INITIAL_PREY; i++) {
        float px = 3.0f + static_cast<float>((i % 5) * 3);
        float pz = 3.0f + static_cast<float>((i / 5) * 5);
        engine.addAgent(createPrey(px, 2.0f, pz));
    }

    std::cout << "  [Setup] Spawning " << INITIAL_PREDATORS << " predator agents...\n";
    for (int i = 0; i < INITIAL_PREDATORS; i++) {
        float px = 13.0f + static_cast<float>(i * 3);
        float pz = 10.0f;
        engine.addAgent(createPredator(px, 2.0f, pz));
    }

    int initialTotal = countAlive(engine.getAgents(), true, true);
    std::cout << "  [Setup] Total agents: " << initialTotal << "\n" << std::flush;
    assert(initialTotal == INITIAL_PREY + INITIAL_PREDATORS);

    std::cout << "\n  [Phase 1] Running " << TOTAL_TICKS << " ticks...\n" << std::flush;
    std::cout << "  " << std::setw(8) << "Tick"
              << std::setw(10) << "Prey"
              << std::setw(12) << "Predators"
              << std::setw(10) << "Total"
              << std::setw(12) << "PreyBorn"
              << std::setw(14) << "PreyKilled\n";
    std::cout << "  " << std::string(66, '-') << "\n" << std::flush;

    int peakPrey = 0;
    int troughPrey = INITIAL_PREY;
    int peakPredators = 0;
    int troughPredators = INITIAL_PREDATORS;
    int totalAgentsSpawned = INITIAL_PREY + INITIAL_PREDATORS;
    int totalPreyKilled = 0;
    int preyAtTick200 = 0;
    int preyAtTick500 = 0;
    int preyAtTick800 = 0;
    int preyAtTick1000 = 0;
    int predatorsAtTick1000 = 0;

    for (int tick = 0; tick <= TOTAL_TICKS; tick++) {
        if (tick % LOG_INTERVAL == 0) {
            int preyCount = countAlive(engine.getAgents(), true, false);
            int predCount = countAlive(engine.getAgents(), false, true);
            int total = preyCount + predCount;
            int totalBorn = static_cast<int>(engine.getAgents().size()) - (INITIAL_PREY + INITIAL_PREDATORS);

            if (preyCount > peakPrey) peakPrey = preyCount;
            if (preyCount < troughPrey) troughPrey = preyCount;
            if (predCount > peakPredators) peakPredators = predCount;
            if (predCount < troughPredators) troughPredators = predCount;

            if (tick == 200) preyAtTick200 = preyCount;
            if (tick == 500) preyAtTick500 = preyCount;
            if (tick == 800) preyAtTick800 = preyCount;
            if (tick == 1000) {
                preyAtTick1000 = preyCount;
                predatorsAtTick1000 = predCount;
            }

            std::cout << "  " << std::setw(8) << tick
                      << std::setw(10) << preyCount
                      << std::setw(12) << predCount
                      << std::setw(10) << total
                      << std::setw(12) << totalBorn
                      << std::setw(14) << totalPreyKilled << "\n" << std::flush;
        }

        if (tick < TOTAL_TICKS) {
            int preyBefore = countAlive(engine.getAgents(), true, false);
            engine.tick(world, 1.0f);
            int preyAfter = countAlive(engine.getAgents(), true, false);
            if (preyAfter < preyBefore) {
                totalPreyKilled += (preyBefore - preyAfter);
            }
        }
    }

    std::cout << "\n  [Phase 2] Analysis...\n" << std::flush;

    bool predatorsSurvived = predatorsAtTick1000 > 0;
    std::cout << "  [Result] Predators survived: " << (predatorsSurvived ? "YES" : "NO") << "\n";

    bool preyReproduced = static_cast<int>(engine.getAgents().size()) > (INITIAL_PREY + INITIAL_PREDATORS);
    std::cout << "  [Result] Prey reproduced: " << (preyReproduced ? "YES" : "NO") << "\n";

    bool populationFluctuated = (peakPrey - troughPrey) > 2;
    std::cout << "  [Result] Prey fluctuated (peak " << peakPrey << ", trough " << troughPrey << "): "
              << (populationFluctuated ? "YES" : "NO") << "\n";

    bool oscillationObserved = (preyAtTick200 != preyAtTick500) || (preyAtTick500 != preyAtTick1000);
    std::cout << "  [Result] Population changed over time: "
              << (oscillationObserved ? "YES" : "NO") << "\n" << std::flush;

    std::cout << "\n  [Phase 3] Assertions...\n" << std::flush;

    assert(predatorsSurvived && "Predators should survive by hunting");
    std::cout << "  [Assert] Predators survived: PASS\n";

    assert(preyReproduced && "Prey should reproduce");
    std::cout << "  [Assert] Prey reproduced: PASS\n";

    assert(populationFluctuated && "Prey population should fluctuate");
    std::cout << "  [Assert] Population fluctuated: PASS\n";

    assert(oscillationObserved && "Population should change over time");
    std::cout << "  [Assert] Oscillation observed: PASS\n" << std::flush;

    std::cout << "\n  [Result] Summary:\n";
    std::cout << "    Predators survived:     " << (predatorsSurvived ? "PASS" : "FAIL") << "\n";
    std::cout << "    Prey reproduced:        " << (preyReproduced ? "PASS" : "FAIL") << "\n";
    std::cout << "    Population fluctuated:  " << (populationFluctuated ? "PASS" : "FAIL") << "\n";
    std::cout << "    Oscillation observed:   " << (oscillationObserved ? "PASS" : "FAIL") << "\n" << std::flush;

    std::cout << "\n=============================================\n";
    std::cout << "  PREDATOR-PREY DYNAMICS TEST PASSED\n";
    std::cout << "=============================================\n" << std::flush;

    return 0;
}
