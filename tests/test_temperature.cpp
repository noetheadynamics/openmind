#include "PhysicsEngine.h"
#include "VoxelOctree.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <iomanip>

using namespace OpenMind;

int main() {
    std::cout << "=============================================\n";
    std::cout << "  TEMPERATURE PERCEPTION TEST (Feature #54)\n";
    std::cout << "=============================================\n\n";

    VoxelOctree world;
    PhysicsEngine engine;
    engine.setGravity(0.0f);
    engine.setTimeScale(1.0f);
    engine.setScanRange(16);

    MaterialProps stoneProps;
    stoneProps.general.hardness = 7.0f;
    stoneProps.mechanical.tensileStrength = 100.0f;
    stoneProps.chemical.lightAbsorption = 0.8f;
    stoneProps.thermal.heatOutput = 50.0f;

    MaterialProps leafProps;
    leafProps.general.hardness = 1.0f;
    leafProps.mechanical.tensileStrength = 2.0f;
    leafProps.chemical.lightAbsorption = 0.1f;

    for (int x = 0; x < 8; x++) {
        for (int z = 0; z < 8; z++) {
            world.setBlock(x, 0, z, BlockType::STONE, stoneProps);
        }
    }

    world.setBlock(7, 3, 7, BlockType::WOOD, leafProps);
    world.setBlock(7, 4, 7, BlockType::LEAVES, leafProps);

    Agent agent;
    agent.id = 1;
    agent.x = 4.0f;
    agent.y = 2.0f;
    agent.z = 4.0f;
    agent.vx = 0.0f;
    agent.vy = 0.0f;
    agent.vz = 0.0f;
    agent.energy = 100.0f;
    agent.maxEnergy = 100.0f;
    agent.health = 100.0f;
    agent.maxHealth = 100.0f;
    agent.isAlive = true;
    agent.speed = 5.0f;
    agent.visionRange = 10.0f;
    agent.hearingRange = 20.0f;

    std::cout << "  [Phase 1] Felt Temperature Calculation...\n";
    engine.setTemperature(20.0f);
    engine.addAgent(agent);

    float felt20 = engine.calculateFeltTemperature(engine.getAgents()[0]);
    std::cout << "  [Check] Felt temp at 20C ambient: " << felt20 << "\n";
    assert(felt20 > 18.0f && felt20 < 22.0f && "Felt temp should be near ambient at low wind");

    engine.setWind(5.0f, 0.0f, 0.0f);
    float felt20wind = engine.calculateFeltTemperature(engine.getAgents()[0]);
    std::cout << "  [Check] Felt temp at 20C with wind: " << felt20wind << "\n";
    assert(felt20wind < felt20 && "Wind chill should lower felt temperature");
    std::cout << "  [Phase 1] PASSED\n\n";

    std::cout << "  [Phase 2] Comfort Level Calculation...\n";
    float comfort20 = engine.getComfortLevel(20.0f);
    float comfort10 = engine.getComfortLevel(10.0f);
    float comfort30 = engine.getComfortLevel(30.0f);
    float comfortMinus15 = engine.getComfortLevel(-15.0f);
    float comfort45 = engine.getComfortLevel(45.0f);

    std::cout << "  [Check] Comfort at 20C: " << comfort20 << "\n";
    std::cout << "  [Check] Comfort at 10C: " << comfort10 << "\n";
    std::cout << "  [Check] Comfort at 30C: " << comfort30 << "\n";
    std::cout << "  [Check] Comfort at -15C: " << comfortMinus15 << "\n";
    std::cout << "  [Check] Comfort at 45C: " << comfort45 << "\n";

    assert(comfort20 == 1.0f && "20C should be fully comfortable");
    assert(comfort10 < 1.0f && comfort10 > 0.0f && "10C should be somewhat uncomfortable");
    assert(comfort30 < 1.0f && comfort30 > 0.0f && "30C should be somewhat uncomfortable");
    assert(comfortMinus15 == 0.0f && "-15C should be zero comfort (extreme cold)");
    assert(comfort45 == 0.0f && "45C should be zero comfort (extreme heat)");
    std::cout << "  [Phase 2] PASSED\n\n";

    std::cout << "  [Phase 3] Cold Agent Seeks Warmth...\n";
    engine.setTemperature(-5.0f);
    engine.setWind(0.0f, 0.0f, 0.0f);
    engine.getAgents()[0].x = 4.0f;
    engine.getAgents()[0].y = 2.0f;
    engine.getAgents()[0].z = 4.0f;
    engine.getAgents()[0].vx = 0.0f;
    engine.getAgents()[0].vz = 0.0f;

    for (int i = 0; i < 10; i++) {
        engine.tick(world, 0.5f);
    }

    const Agent& coldAgent = engine.getAgents()[0];
    std::cout << "  [Check] Felt temperature: " << coldAgent.feltTemperature << "\n";
    std::cout << "  [Check] Comfort level: " << coldAgent.comfortLevel << "\n";
    std::cout << "  [Check] Seeking warmth: " << (coldAgent.isSeekingWarmth ? "YES" : "NO") << "\n";
    std::cout << "  [Check] Seeking shade: " << (coldAgent.isSeekingShade ? "YES" : "NO") << "\n";

    assert(coldAgent.isSeekingWarmth && "Cold agent should seek warmth");
    assert(!coldAgent.isSeekingShade && "Cold agent should not seek shade");
    assert(coldAgent.feltTemperature < 15.0f && "Felt temperature should be cold");
    std::cout << "  [Phase 3] PASSED\n\n";

    std::cout << "  [Phase 4] Hot Agent Seeks Shade...\n";
    engine.setTemperature(35.0f);
    engine.setWind(0.0f, 0.0f, 0.0f);
    engine.getAgents()[0].x = 4.0f;
    engine.getAgents()[0].y = 2.0f;
    engine.getAgents()[0].z = 4.0f;
    engine.getAgents()[0].vx = 0.0f;
    engine.getAgents()[0].vz = 0.0f;
    engine.getAgents()[0].health = 100.0f;
    engine.getAgents()[0].isAlive = true;

    for (int i = 0; i < 10; i++) {
        engine.tick(world, 0.5f);
    }

    const Agent& hotAgent = engine.getAgents()[0];
    std::cout << "  [Check] Felt temperature: " << hotAgent.feltTemperature << "\n";
    std::cout << "  [Check] Comfort level: " << hotAgent.comfortLevel << "\n";
    std::cout << "  [Check] Seeking warmth: " << (hotAgent.isSeekingWarmth ? "YES" : "NO") << "\n";
    std::cout << "  [Check] Seeking shade: " << (hotAgent.isSeekingShade ? "YES" : "NO") << "\n";

    assert(hotAgent.isSeekingShade && "Hot agent should seek shade");
    assert(!hotAgent.isSeekingWarmth && "Hot agent should not seek warmth");
    assert(hotAgent.feltTemperature > 25.0f && "Felt temperature should be hot");
    std::cout << "  [Phase 4] PASSED\n\n";

    std::cout << "  [Phase 5] Extreme Cold Damages Health...\n";
    engine.setTemperature(-20.0f);
    engine.setWind(0.0f, 0.0f, 0.0f);
    engine.getAgents()[0].x = 4.0f;
    engine.getAgents()[0].y = 2.0f;
    engine.getAgents()[0].z = 4.0f;
    engine.getAgents()[0].vx = 0.0f;
    engine.getAgents()[0].vz = 0.0f;
    engine.getAgents()[0].health = 100.0f;
    engine.getAgents()[0].isAlive = true;

    float initialHealth = engine.getAgents()[0].health;
    for (int i = 0; i < 10; i++) {
        engine.tick(world, 0.5f);
    }
    float finalHealth = engine.getAgents()[0].health;

    std::cout << "  [Check] Initial health: " << initialHealth << "\n";
    std::cout << "  [Check] Final health: " << finalHealth << "\n";
    assert(finalHealth < initialHealth && "Extreme cold should reduce health");
    std::cout << "  [Phase 5] PASSED\n\n";

    std::cout << "  [Phase 6] Extreme Heat Damages Health...\n";
    engine.setTemperature(50.0f);
    engine.setWind(0.0f, 0.0f, 0.0f);
    engine.getAgents()[0].x = 4.0f;
    engine.getAgents()[0].y = 2.0f;
    engine.getAgents()[0].z = 4.0f;
    engine.getAgents()[0].vx = 0.0f;
    engine.getAgents()[0].vz = 0.0f;
    engine.getAgents()[0].health = 100.0f;
    engine.getAgents()[0].isAlive = true;

    initialHealth = engine.getAgents()[0].health;
    for (int i = 0; i < 10; i++) {
        engine.tick(world, 0.5f);
    }
    finalHealth = engine.getAgents()[0].health;

    std::cout << "  [Check] Initial health: " << initialHealth << "\n";
    std::cout << "  [Check] Final health: " << finalHealth << "\n";
    assert(finalHealth < initialHealth && "Extreme heat should reduce health");
    std::cout << "  [Phase 6] PASSED\n\n";

    std::cout << "  [Phase 7] Comfortable Agent Normal Speed...\n";
    engine.setTemperature(20.0f);
    engine.setWind(0.0f, 0.0f, 0.0f);
    engine.getAgents()[0].x = 4.0f;
    engine.getAgents()[0].y = 2.0f;
    engine.getAgents()[0].z = 4.0f;
    engine.getAgents()[0].health = 100.0f;
    engine.getAgents()[0].isAlive = true;

    for (int i = 0; i < 5; i++) {
        engine.tick(world, 0.5f);
    }

    const Agent& comfyAgent = engine.getAgents()[0];
    std::cout << "  [Check] Speed at 20C: " << comfyAgent.speed << "\n";
    assert(comfyAgent.speed >= 0.9f && "Comfortable agent should have near-full speed");
    assert(!comfyAgent.isSeekingWarmth && "Comfortable agent should not seek warmth");
    assert(!comfyAgent.isSeekingShade && "Comfortable agent should not seek shade");
    std::cout << "  [Phase 7] PASSED\n\n";

    std::cout << "  [Phase 8] Cold Agent Reduced Speed...\n";
    engine.setTemperature(-5.0f);
    engine.setWind(0.0f, 0.0f, 0.0f);
    engine.getAgents()[0].x = 4.0f;
    engine.getAgents()[0].y = 2.0f;
    engine.getAgents()[0].z = 4.0f;
    engine.getAgents()[0].health = 100.0f;
    engine.getAgents()[0].isAlive = true;

    for (int i = 0; i < 5; i++) {
        engine.tick(world, 0.5f);
    }

    const Agent& coldAgent2 = engine.getAgents()[0];
    std::cout << "  [Check] Speed at -5C: " << coldAgent2.speed << "\n";
    assert(coldAgent2.speed < 0.9f && "Cold agent should have reduced speed");
    std::cout << "  [Phase 8] PASSED\n\n";

    std::cout << "=============================================\n";
    std::cout << "  TEMPERATURE PERCEPTION TEST PASSED\n";
    std::cout << "=============================================\n";

    return 0;
}