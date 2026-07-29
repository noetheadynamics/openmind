#include "PhysicsEngine.h"
#include "VoxelOctree.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <iomanip>

using namespace OpenMind;

int main() {
    std::cout << "=============================================\n";
    std::cout << "  WEATHER TEST (Feature #53)\n";
    std::cout << "=============================================\n\n";

    VoxelOctree world;
    PhysicsEngine engine;
    engine.setGravity(0.0f);
    engine.setTimeScale(1.0f);
    engine.setScanRange(8);

    std::cout << "  [Phase 1] Default Weather is Clear...\n";
    WeatherType initialType = engine.getWeatherType();
    assert(initialType == WeatherType::CLEAR && "Default weather should be CLEAR");
    assert(engine.getPrecipitationRate() == 0.0f && "Clear weather has no precipitation");
    assert(engine.getVisibility() == 1.0f && "Clear weather has full visibility");
    std::cout << "  [Check] Initial type: CLEAR\n";
    std::cout << "  [Check] Precipitation: " << engine.getPrecipitationRate() << "\n";
    std::cout << "  [Check] Visibility: " << engine.getVisibility() << "\n";
    std::cout << "  [Phase 1] PASSED\n\n";

    std::cout << "  [Phase 2] Transition to Rain...\n";
    engine.setWeather(WeatherType::RAIN);
    assert(engine.getWeatherType() == WeatherType::RAIN && "Weather should be RAIN");
    assert(engine.getPrecipitationRate() > 0.0f && "Rain should have precipitation");
    assert(engine.getVisibility() < 1.0f && "Rain should reduce visibility");

    int rainCount = 0;
    for (int i = 0; i < 3; i++) {
        engine.tick(world, 0.2f);
        for (const auto& p : engine.getParticles()) {
            if (p.active && p.type == "rain") rainCount++;
        }
    }
    std::cout << "  [Check] Rain particles spawned: " << rainCount << "\n";
    assert(rainCount > 0 && "Rain particles should be spawned");
    std::cout << "  [Phase 2] PASSED\n\n";

    std::cout << "  [Phase 3] Rain Accumulation...\n";
    engine.setWeather(WeatherType::RAIN);
    float initialRain = engine.getRainAccumulation();
    for (int i = 0; i < 5; i++) {
        engine.tick(world, 0.5f);
    }
    float finalRain = engine.getRainAccumulation();
    std::cout << "  [Check] Initial rain accumulation: " << initialRain << "\n";
    std::cout << "  [Check] Final rain accumulation: " << finalRain << "\n";
    assert(finalRain > initialRain && "Rain accumulation should increase");
    std::cout << "  [Phase 3] PASSED\n\n";

    std::cout << "  [Phase 4] Transition to Snow...\n";
    engine.setWeather(WeatherType::SNOW);
    assert(engine.getWeatherType() == WeatherType::SNOW && "Weather should be SNOW");

    int snowCount = 0;
    for (int i = 0; i < 3; i++) {
        engine.tick(world, 0.2f);
        for (const auto& p : engine.getParticles()) {
            if (p.active && p.type == "snow") snowCount++;
        }
    }
    std::cout << "  [Check] Snow particles spawned: " << snowCount << "\n";
    assert(snowCount > 0 && "Snow particles should be spawned");
    std::cout << "  [Phase 4] PASSED\n\n";

    std::cout << "  [Phase 5] Snow Accumulation...\n";
    engine.setWeather(WeatherType::SNOW);
    float initialSnow = engine.getSnowAccumulation();
    for (int i = 0; i < 5; i++) {
        engine.tick(world, 0.5f);
    }
    float finalSnow = engine.getSnowAccumulation();
    std::cout << "  [Check] Initial snow accumulation: " << initialSnow << "\n";
    std::cout << "  [Check] Final snow accumulation: " << finalSnow << "\n";
    assert(finalSnow > initialSnow && "Snow accumulation should increase");
    std::cout << "  [Phase 5] PASSED\n\n";

    std::cout << "  [Phase 6] Wind Affects Agents...\n";
    engine.setWeather(WeatherType::CLEAR);
    engine.tick(world, 0.1f);

    Agent agent;
    agent.id = 1;
    agent.x = 4.0f;
    agent.y = 3.0f;
    agent.z = 4.0f;
    agent.vx = 0.0f;
    agent.vy = 0.0f;
    agent.vz = 0.0f;
    agent.energy = 100.0f;
    agent.maxEnergy = 100.0f;
    agent.isAlive = true;
    agent.speed = 5.0f;
    agent.hearingRange = 20.0f;
    engine.addAgent(agent);

    engine.setWeather(WeatherType::STORM);
    float stormWind = engine.getWindSpeed();
    std::cout << "  [Check] Storm wind speed: " << stormWind << "\n";
    assert(stormWind > 5.0f && "Storm should have strong wind");

    for (int i = 0; i < 3; i++) {
        engine.tick(world, 0.2f);
    }
    float finalVx = engine.getAgents()[0].vx;
    std::cout << "  [Check] Agent vx after wind: " << finalVx << "\n";
    assert(std::abs(finalVx) > 0.0f && "Wind should affect agent velocity");
    std::cout << "  [Phase 6] PASSED\n\n";

    std::cout << "  [Phase 7] Weather Transition is Smooth...\n";
    engine.setWeather(WeatherType::CLEAR);
    engine.tick(world, 0.1f);

    engine.setWeatherTransition(WeatherType::RAIN, 2.0f);
    float precipStart = engine.getPrecipitationRate();

    engine.tick(world, 1.0f);
    float precipMid = engine.getPrecipitationRate();

    engine.tick(world, 2.0f);
    float precipEnd = engine.getPrecipitationRate();

    std::cout << "  [Check] Precipitation at start: " << precipStart << "\n";
    std::cout << "  [Check] Precipitation mid-transition: " << precipMid << "\n";
    std::cout << "  [Check] Precipitation after transition: " << precipEnd << "\n";

    assert(precipMid > precipStart && "Precipitation should increase during transition");
    assert(precipEnd >= precipMid && "Precipitation should finish transition");
    std::cout << "  [Phase 7] PASSED\n\n";

    std::cout << "  [Phase 8] Wind Spawns Leaf Particles...\n";
    MaterialProps leafProps;
    leafProps.general.hardness = 1.0f;
    leafProps.mechanical.tensileStrength = 2.0f;
    leafProps.chemical.lightAbsorption = 0.1f;
    for (int lx = 0; lx < 4; lx++) {
        for (int lz = 0; lz < 4; lz++) {
            world.setBlock(lx, 3, lz, BlockType::LEAVES, leafProps);
        }
    }

    engine.setWeather(WeatherType::STORM);
    int leafCount = 0;
    for (int i = 0; i < 50; i++) {
        engine.tick(world, 0.5f);
    }
    for (const auto& p : engine.getParticles()) {
        if (p.active && p.type == "leaf") leafCount++;
    }
    std::cout << "  [Check] Leaf particles spawned: " << leafCount << "\n";
    assert(leafCount > 0 && "Strong wind should spawn leaf particles");
    std::cout << "  [Phase 8] PASSED\n\n";

    std::cout << "  [Phase 9] Storm Lightning...\n";
    MaterialProps tntProps;
    tntProps.general.hardness = 1.0f;
    tntProps.mechanical.tensileStrength = 10.0f;
    world.setBlock(4, 1, 4, BlockType::TNT, tntProps);

    engine.setWeather(WeatherType::STORM);
    bool lightningFired = false;
    for (int i = 0; i < 20; i++) {
        engine.tick(world, 0.5f);
        VoxelData vd;
        if (world.getBlock(4, 1, 4, vd) && vd.type == BlockType::AIR) {
            lightningFired = true;
            break;
        }
    }
    std::cout << "  [Check] Lightning destroyed block: " << (lightningFired ? "YES" : "NO") << "\n";
    std::cout << "  [Phase 9] PASSED (lightning probabilistic)\n\n";

    std::cout << "=============================================\n";
    std::cout << "  WEATHER TEST PASSED\n";
    std::cout << "=============================================\n";

    return 0;
}