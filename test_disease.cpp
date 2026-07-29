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

Agent createHealthyAgent(float x, float y, float z) {
    Agent a;
    a.isPrey = true;
    a.x = x; a.y = y; a.z = z;
    a.health = 100.0f;
    a.maxHealth = 100.0f;
    a.energy = 100.0f;
    a.maxEnergy = 100.0f;
    a.speed = 1.0f;
    a.diseaseState = DiseaseState::HEALTHY;
    a.immunityLevel = 0.0f;
    a.isDiseased = false;
    return a;
}

Agent createInfectedAgent(float x, float y, float z, int diseaseID) {
    Agent a = createHealthyAgent(x, y, z);
    a.diseaseState = DiseaseState::SYMPTOMATIC;
    a.infectedDiseaseID = diseaseID;
    a.isDiseased = true;
    a.infectionTimer = 0.0f;
    a.symptomTimer = 300.0f;
    a.health = 90.0f;
    return a;
}

std::string diseaseStateStr(DiseaseState s) {
    switch (s) {
        case DiseaseState::HEALTHY:    return "HEALTHY";
        case DiseaseState::INCUBATING: return "INCUBATING";
        case DiseaseState::SYMPTOMATIC:return "SYMPTOMATIC";
        case DiseaseState::RECOVERED:  return "RECOVERED";
        case DiseaseState::IMMUNE:     return "IMMUNE";
    }
    return "UNKNOWN";
}

int countByState(const std::vector<Agent>& agents, DiseaseState state) {
    int count = 0;
    for (const auto& a : agents) {
        if (!a.isAlive) continue;
        if (a.diseaseState == state) count++;
    }
    return count;
}

int countAlive(const std::vector<Agent>& agents) {
    int count = 0;
    for (const auto& a : agents) {
        if (a.isAlive) count++;
    }
    return count;
}

int main() {
    std::cout << "=============================================\n";
    std::cout << "  DISEASE SPREAD TEST (Feature #42)\n";
    std::cout << "=============================================\n\n";

    const int ARENA_SIZE = 20;
    const int TOTAL_TICKS = 2000;
    const int LOG_INTERVAL = 200;
    const int INITIAL_HEALTHY = 10;
    const int INITIAL_INFECTED = 1;

    VoxelOctree world;
    PhysicsEngine engine;
    engine.setGravity(0.0f);
    engine.setTimeScale(1.0f);
    engine.setScanRange(ARENA_SIZE);

    MaterialProps stoneProps = createStoneProps();
    MaterialProps grassProps = createGrassProps();

    std::cout << "  [Setup] Building " << ARENA_SIZE << "x" << ARENA_SIZE << " arena...\n";
    buildArena(world, ARENA_SIZE, stoneProps, grassProps);

    Disease flu;
    flu.diseaseID = 1;
    flu.name = "Flu";
    flu.transmissionMode = TransmissionMode::PROXIMITY;
    flu.transmissionRange = 4.0f;
    flu.infectivity = 0.8f;
    flu.severity = 0.3f;
    flu.incubationPeriod = 100.0f;
    flu.symptomDuration = 300.0f;
    flu.mortalityRate = 0.05f;
    flu.immunityGain = 0.7f;
    flu.healthDrainPerTick = 0.04f;
    flu.hungerIncreasePerTick = 0.01f;
    flu.speedPenalty = 0.2f;
    engine.addDisease(flu);

    std::cout << "  [Setup] Spawning " << INITIAL_HEALTHY << " healthy agents...\n";
    for (int i = 0; i < INITIAL_HEALTHY; i++) {
        float px = 4.0f + static_cast<float>((i % 5) * 3);
        float pz = 4.0f + static_cast<float>((i / 5) * 8);
        engine.addAgent(createHealthyAgent(px, 2.0f, pz));
    }

    std::cout << "  [Setup] Spawning " << INITIAL_INFECTED << " infected agent(s)...\n";
    engine.addAgent(createInfectedAgent(10.0f, 2.0f, 10.0f, 1));

    int initialTotal = countAlive(engine.getAgents());
    std::cout << "  [Setup] Total agents: " << initialTotal << "\n\n";
    assert(initialTotal == INITIAL_HEALTHY + INITIAL_INFECTED);

    std::cout << "  [Phase 1] Running " << TOTAL_TICKS << " ticks...\n";
    std::cout << "  " << std::setw(8) << "Tick"
              << std::setw(8) << "Alive"
              << std::setw(12) << "Healthy"
              << std::setw(12) << "Incubating"
              << std::setw(14) << "Symptomatic"
              << std::setw(12) << "Recovered"
              << std::setw(12) << "Dead"
              << "\n";
    std::cout << "  " << std::string(78, '-') << "\n" << std::flush;

    int totalSpawned = INITIAL_HEALTHY + INITIAL_INFECTED;
    int totalDead = 0;
    int maxInfected = 0;
    int maxRecovered = 0;
    int maxDead = 0;
    bool anySymptomaticObserved = false;
    bool anyRecoveredObserved = false;

    for (int tick = 0; tick <= TOTAL_TICKS; tick++) {
        if (tick % LOG_INTERVAL == 0) {
            int alive = countAlive(engine.getAgents());
            int healthy = countByState(engine.getAgents(), DiseaseState::HEALTHY);
            int incubating = countByState(engine.getAgents(), DiseaseState::INCUBATING);
            int symptomatic = countByState(engine.getAgents(), DiseaseState::SYMPTOMATIC);
            int recovered = countByState(engine.getAgents(), DiseaseState::RECOVERED);
            totalDead = totalSpawned - alive - recovered;
            if (totalDead < 0) totalDead = 0;

            int infected = incubating + symptomatic;
            if (infected > maxInfected) maxInfected = infected;
            if (recovered > maxRecovered) maxRecovered = recovered;
            if (totalDead > maxDead) maxDead = totalDead;
            if (symptomatic > 0) anySymptomaticObserved = true;
            if (recovered > 0) anyRecoveredObserved = true;

            std::cout << "  " << std::setw(8) << tick
                      << std::setw(8) << alive
                      << std::setw(12) << healthy
                      << std::setw(12) << incubating
                      << std::setw(14) << symptomatic
                      << std::setw(12) << recovered
                      << std::setw(12) << totalDead
                      << "\n" << std::flush;
        }

        if (tick < TOTAL_TICKS) {
            engine.tick(world, 1.0f);
        }
    }

    std::cout << "\n  [Phase 2] Analysis...\n" << std::flush;

    int finalAlive = countAlive(engine.getAgents());
    int finalHealthy = countByState(engine.getAgents(), DiseaseState::HEALTHY);
    int finalRecovered = countByState(engine.getAgents(), DiseaseState::RECOVERED);
    int finalDeadCount = totalSpawned - finalAlive - finalRecovered;
    if (finalDeadCount < 0) finalDeadCount = 0;

    bool diseaseSpread = maxInfected > INITIAL_INFECTED;
    std::cout << "  [Result] Disease spread beyond initial: " << (diseaseSpread ? "YES" : "NO")
              << " (peak infected: " << maxInfected << ")\n";

    bool symptomsObserved = anySymptomaticObserved;
    std::cout << "  [Result] Symptoms observed: " << (symptomsObserved ? "YES" : "NO") << "\n";

    bool recoveryOccurred = anyRecoveredObserved;
    std::cout << "  [Result] Recovery occurred: " << (recoveryOccurred ? "YES" : "NO")
              << " (final recovered: " << finalRecovered << ")\n";

    bool deathsOccurred = finalDeadCount > 0;
    std::cout << "  [Result] Deaths occurred: " << (deathsOccurred ? "YES" : "NO")
              << " (final dead: " << finalDeadCount << ")\n";

    std::cout << "\n  [Phase 3] Assertions...\n" << std::flush;

    assert(diseaseSpread && "Disease should spread to healthy agents");
    std::cout << "  [Assert] Disease spread: PASS\n";

    assert(symptomsObserved && "Symptoms should be observed");
    std::cout << "  [Assert] Symptoms observed: PASS\n";

    assert(recoveryOccurred && "Some agents should recover");
    std::cout << "  [Assert] Recovery occurred: PASS\n";

    std::cout << "\n  [Result] Summary:\n";
    std::cout << "    Disease spread:       " << (diseaseSpread ? "PASS" : "FAIL") << "\n";
    std::cout << "    Symptoms observed:    " << (symptomsObserved ? "PASS" : "FAIL") << "\n";
    std::cout << "    Recovery occurred:    " << (recoveryOccurred ? "PASS" : "FAIL") << "\n";
    std::cout << "    Deaths occurred:      " << (deathsOccurred ? "PASS" : "FAIL") << "\n";
    std::cout << std::flush;

    std::cout << "\n=============================================\n";
    std::cout << "  DISEASE SPREAD TEST PASSED\n";
    std::cout << "=============================================\n" << std::flush;

    return 0;
}
