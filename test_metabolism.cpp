#include "PhysicsEngine.h"
#include "VoxelOctree.h"
#include <iostream>
#include <iomanip>
#include <cassert>

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

MaterialProps createFoodProps() {
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

Agent createAgent(float x, float y, float z) {
    Agent a;
    a.x = x; a.y = y; a.z = z;
    a.energy = 100.0f;
    a.maxEnergy = 100.0f;
    a.health = 100.0f;
    a.maxHealth = 100.0f;
    a.speed = 1.0f;
    a.energyDrainRate = 0.1f;
    a.foodValue = 30.0f;
    a.isPrey = true;
    return a;
}

int countAlive(const std::vector<Agent>& agents) {
    int count = 0;
    for (const auto& a : agents) {
        if (a.isAlive) count++;
    }
    return count;
}

void buildBox(VoxelOctree& world, int cx, int cy, int cz, int radius, const MaterialProps& props) {
    for (int dx = -radius; dx <= radius; dx++) {
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dz = -radius; dz <= radius; dz++) {
                if (std::abs(dx) == radius || std::abs(dy) == radius || std::abs(dz) == radius) {
                    world.setBlock(cx + dx, cy + dy, cz + dz, BlockType::STONE, props);
                }
            }
        }
    }
}

int main() {
    std::cout << "=============================================\n";
    std::cout << "  METABOLISM TEST (Feature #43)\n";
    std::cout << "=============================================\n\n";

    MaterialProps stoneProps = createStoneProps();
    MaterialProps foodProps = createFoodProps();

    {
        std::cout << "  [Phase 1] Starvation Test (no food)...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        buildBox(world, 10, 10, 10, 5, stoneProps);

        Agent agent = createAgent(10.0f, 10.0f, 10.0f);
        agent.energyDrainRate = 0.5f;
        engine.addAgent(agent);

        std::cout << "  [Tick]  Energy   Health   Alive\n";
        std::cout << "  [----]  ------   ------   -----\n";

        for (int tick = 0; tick <= 500; tick++) {
            if (tick % 50 == 0) {
                auto& agents = engine.getAgents();
                if (agents.size() > 0) {
                    std::cout << "  " << std::setw(6) << tick
                              << "  " << std::setw(6) << std::fixed << std::setprecision(1) << agents[0].energy
                              << "  " << std::setw(6) << agents[0].health
                              << "  " << (agents[0].isAlive ? "YES" : "NO") << "\n";
                }
            }
            engine.tick(world, 1.0f);
        }

        auto& agents = engine.getAgents();
        assert(agents.size() > 0);
        bool energyDecreased = agents[0].energy < 100.0f;
        bool agentDied = !agents[0].isAlive;
        std::cout << "\n  [Result] Energy decreased: " << (energyDecreased ? "YES" : "NO") << "\n";
        std::cout << "  [Result] Agent died: " << (agentDied ? "YES" : "NO") << "\n";
        assert(energyDecreased && "Energy should decrease over time");
        assert(agentDied && "Agent should die when energy reaches 0");
        std::cout << "  [Phase 1] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 2] Food Consumption Test (with food)...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        buildBox(world, 10, 10, 10, 5, stoneProps);

        world.setBlock(11, 10, 10, BlockType::CUSTOM, foodProps);
        world.setBlock(9, 10, 10, BlockType::CUSTOM, foodProps);
        world.setBlock(10, 11, 10, BlockType::CUSTOM, foodProps);
        world.setBlock(10, 9, 10, BlockType::CUSTOM, foodProps);
        world.setBlock(10, 10, 11, BlockType::CUSTOM, foodProps);
        world.setBlock(10, 10, 9, BlockType::CUSTOM, foodProps);

        Agent agent = createAgent(10.0f, 10.0f, 10.0f);
        engine.addAgent(agent);

        std::cout << "  [Tick]  Energy   Alive   FoodLeft\n";
        std::cout << "  [----]  ------   -----   --------\n";

        for (int tick = 0; tick <= 500; tick++) {
            if (tick % 50 == 0) {
                auto& agents = engine.getAgents();
                int foodCount = 0;
                for (int x = 5; x <= 15; x++) {
                    for (int y = 5; y <= 15; y++) {
                        for (int z = 5; z <= 15; z++) {
                            VoxelData vd;
                            if (world.getBlock(x, y, z, vd) && vd.type == BlockType::CUSTOM && vd.props.biological.isOrganic) {
                                foodCount++;
                            }
                        }
                    }
                }
                if (agents.size() > 0) {
                    std::cout << "  " << std::setw(6) << tick
                              << "  " << std::setw(6) << std::fixed << std::setprecision(1) << agents[0].energy
                              << "  " << std::setw(5) << (agents[0].isAlive ? "YES" : "NO")
                              << "  " << std::setw(8) << foodCount << "\n";
                }
            }
            engine.tick(world, 1.0f);
        }

        auto& agents = engine.getAgents();
        assert(agents.size() > 0);
        bool agentSurvived = agents[0].isAlive;
        std::cout << "\n  [Result] Agent survived with food: " << (agentSurvived ? "YES" : "NO") << "\n";
        assert(agentSurvived && "Agent should survive when food is available");
        std::cout << "  [Phase 2] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 3] Energy Drain Rate Test...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        buildBox(world, 10, 10, 10, 5, stoneProps);

        Agent agent = createAgent(10.0f, 10.0f, 10.0f);
        agent.energyDrainRate = 0.5f;
        engine.addAgent(agent);

        for (int tick = 0; tick <= 250; tick++) {
            engine.tick(world, 1.0f);
        }

        auto& agents = engine.getAgents();
        assert(agents.size() > 0);
        bool died = !agents[0].isAlive;
        std::cout << "  [Result] High drain rate agent died: " << (died ? "YES" : "NO") << "\n";
        assert(died && "Agent with high drain rate should die faster");
        std::cout << "  [Phase 3] PASSED\n\n";
    }

    std::cout << "=============================================\n";
    std::cout << "  METABOLISM TEST PASSED\n";
    std::cout << "=============================================\n";

    return 0;
}
