#include "PhysicsEngine.h"
#include "VoxelOctree.h"
#include <iostream>
#include <iomanip>
#include <cassert>
#include <cmath>
#include <array>

using namespace OpenMind;

int main() {
    std::cout << "=============================================\n";
    std::cout << "  ROCKET EQUATION TEST (Feature #48)\n";
    std::cout << "=============================================\n\n";

    const float G0 = 9.81f;
    const float EPS = 0.1f;

    {
        std::cout << "  [Phase 1] Tsiolkovsky Rocket Equation...\n";

        float Isp = 300.0f;
        float fuelMass = 100.0f;
        float dryMass = 50.0f;
        float m0 = fuelMass + dryMass;
        float m1 = dryMass;

        float expectedDV = Isp * G0 * std::log(m0 / m1);

        PhysicsEngine engine;
        float computedDV = engine.calculateDeltaV(Isp, m0, m1);

        std::cout << "  [Setup] Isp=" << Isp << ", fuelMass=" << fuelMass
                  << ", dryMass=" << dryMass << "\n";
        std::cout << "  [Setup] m0=" << m0 << ", m1=" << m1 << "\n";
        std::cout << "  [Check] Expected dV: " << std::fixed << std::setprecision(2) << expectedDV << "\n";
        std::cout << "  [Check] Computed dV: " << computedDV << "\n";

        assert(std::abs(computedDV - expectedDV) < EPS && "DeltaV should match Tsiolkovsky equation");
        std::cout << "  [Phase 1] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 2] Single Stage Rocket Simulation...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        float Isp = 300.0f;
        float fuelMass = 100.0f;
        float dryMass = 50.0f;
        float thrust = 500.0f;
        float burnRate = 1.0f;

        RocketStage stage;
        stage.stageIndex = 0;
        stage.fuelMass = fuelMass;
        stage.currentFuel = fuelMass;
        stage.dryMass = dryMass;
        stage.totalMass = fuelMass + dryMass;
        stage.Isp = Isp;
        stage.thrust = thrust;
        stage.throttle = 1.0f;
        stage.fuelBurnRate = burnRate;
        stage.isActive = true;
        engine.addRocketStage(stage);

        Agent rocket;
        rocket.x = 10.0f; rocket.y = 10.0f; rocket.z = 10.0f;
        rocket.vx = 0.0f; rocket.vy = 0.0f; rocket.vz = 0.0f;
        rocket.energy = 1000.0f; rocket.maxEnergy = 1000.0f;
        rocket.health = 1000.0f; rocket.maxHealth = 1000.0f;
        rocket.isPrey = false; rocket.isPredator = false;
        engine.addAgent(rocket);

        float expectedDV = Isp * G0 * std::log((fuelMass + dryMass) / dryMass);

        std::cout << "  [Setup] Isp=" << Isp << ", thrust=" << thrust << "\n";
        std::cout << "  [Setup] fuelMass=" << fuelMass << ", dryMass=" << dryMass << "\n";
        std::cout << "  [Setup] Expected total dV: " << std::fixed << std::setprecision(2) << expectedDV << "\n\n";

        std::cout << "  [Tick]  Fuel    Speed   dV_achieved\n";
        std::cout << "  [----]  ------  ------  -----------\n";

        float totalDV = 0.0f;
        for (int tick = 0; tick <= 150; tick++) {
            auto& stages = engine.getRocketStages();
            if (stages.size() > 0 && stages[0].currentFuel > 0.0f) {
                float prevSpeed = 0.0f;
                auto& agents = engine.getAgents();
                if (agents.size() > 0) {
                    prevSpeed = std::sqrt(agents[0].vx * agents[0].vx +
                                          agents[0].vy * agents[0].vy +
                                          agents[0].vz * agents[0].vz);
                }

                engine.processRocketPhysics(world, 1.0f);

                if (agents.size() > 0) {
                    float newSpeed = std::sqrt(agents[0].vx * agents[0].vx +
                                              agents[0].vy * agents[0].vy +
                                              agents[0].vz * agents[0].vz);
                    totalDV += (newSpeed - prevSpeed);
                }

                if (tick % 25 == 0) {
                    std::cout << "  " << std::setw(6) << tick
                              << "  " << std::setw(6) << std::fixed << std::setprecision(1) << stages[0].currentFuel
                              << "  " << std::setw(6) << std::setprecision(1)
                              << (agents.size() > 0 ? agents[0].vy : 0.0f)
                              << "  " << std::setw(10) << std::setprecision(2) << totalDV << "\n";
                }
            }
        }

        auto& agents = engine.getAgents();
        auto& stages = engine.getRocketStages();
        float finalSpeed = agents.size() > 0 ?
            std::sqrt(agents[0].vx * agents[0].vx + agents[0].vy * agents[0].vy + agents[0].vz * agents[0].vz) : 0.0f;
        float fuelRemaining = stages.size() > 0 ? stages[0].currentFuel : 0.0f;
        bool fuelConsumed = fuelRemaining < fuelMass * 0.5f;
        bool velocityGained = finalSpeed > 10.0f;

        std::cout << "\n  [Result] Final speed: " << std::fixed << std::setprecision(2) << finalSpeed << "\n";
        std::cout << "  [Result] Fuel remaining: " << fuelRemaining << "\n";
        std::cout << "  [Result] Total dV achieved: " << totalDV << "\n";
        std::cout << "  [Result] Expected dV: " << expectedDV << "\n";
        std::cout << "  [Result] Fuel consumed: " << (fuelConsumed ? "YES" : "NO") << "\n";
        std::cout << "  [Result] Velocity gained: " << (velocityGained ? "YES" : "NO") << "\n";

        assert(fuelConsumed && "Fuel should be consumed over time");
        assert(velocityGained && "Rocket should gain velocity from thrust");

        std::cout << "  [Phase 2] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 3] Throttle Control...\n";
        PhysicsEngine engine;

        RocketStage stage;
        stage.stageIndex = 0;
        stage.fuelMass = 100.0f;
        stage.currentFuel = 100.0f;
        stage.dryMass = 50.0f;
        stage.Isp = 300.0f;
        stage.thrust = 500.0f;
        stage.throttle = 1.0f;
        stage.fuelBurnRate = 1.0f;
        stage.isActive = true;
        engine.addRocketStage(stage);

        engine.setRocketThrottle(0, 0.5f);
        std::cout << "  [Check] Throttle after set to 0.5: " << engine.getRocketStages()[0].throttle << "\n";
        assert(std::abs(engine.getRocketStages()[0].throttle - 0.5f) < 0.01f);

        engine.setRocketThrottle(0, 1.5f);
        std::cout << "  [Check] Throttle clamped at 1.0: " << engine.getRocketStages()[0].throttle << "\n";
        assert(std::abs(engine.getRocketStages()[0].throttle - 1.0f) < 0.01f);

        engine.setRocketThrottle(0, -0.5f);
        std::cout << "  [Check] Throttle clamped at 0.0: " << engine.getRocketStages()[0].throttle << "\n";
        assert(std::abs(engine.getRocketStages()[0].throttle - 0.0f) < 0.01f);

        std::cout << "  [Phase 3] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 4] Stage Detachment...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        for (int x = 10; x <= 12; x++) {
            for (int z = 10; z <= 12; z++) {
                world.setBlock(x, 10, z, BlockType::STEEL, MaterialProps());
            }
        }

        RocketStage stage1;
        stage1.stageIndex = 0;
        stage1.fuelMass = 100.0f;
        stage1.currentFuel = 100.0f;
        stage1.dryMass = 50.0f;
        stage1.Isp = 300.0f;
        stage1.thrust = 500.0f;
        stage1.throttle = 1.0f;
        stage1.fuelBurnRate = 10.0f;
        stage1.isActive = true;
        stage1.blockPositions = {std::array<int,3>{10,10,10},std::array<int,3>{11,10,10},std::array<int,3>{12,10,10},std::array<int,3>{10,10,11},std::array<int,3>{11,10,11},std::array<int,3>{12,10,11},std::array<int,3>{10,10,12},std::array<int,3>{11,10,12},std::array<int,3>{12,10,12}};
        engine.addRocketStage(stage1);

        RocketStage stage2;
        stage2.stageIndex = 1;
        stage2.fuelMass = 50.0f;
        stage2.currentFuel = 50.0f;
        stage2.dryMass = 25.0f;
        stage2.Isp = 350.0f;
        stage2.thrust = 300.0f;
        stage2.throttle = 0.0f;
        stage2.fuelBurnRate = 0.5f;
        stage2.isActive = false;
        engine.addRocketStage(stage2);

        Agent rocket;
        rocket.x = 10.0f; rocket.y = 10.0f; rocket.z = 10.0f;
        rocket.vx = 0.0f; rocket.vy = 0.0f; rocket.vz = 0.0f;
        rocket.energy = 1000.0f; rocket.maxEnergy = 1000.0f;
        rocket.health = 1000.0f; rocket.maxHealth = 1000.0f;
        rocket.isPrey = false; rocket.isPredator = false;
        engine.addAgent(rocket);

        for (int tick = 0; tick <= 15; tick++) {
            engine.processRocketPhysics(world, 1.0f);
        }

        auto& stages = engine.getRocketStages();
        bool stage1Empty = stages[0].currentFuel <= 0.0f;
        std::cout << "  [Check] Stage 1 fuel: " << stages[0].currentFuel << "\n";
        std::cout << "  [Check] Stage 1 empty: " << (stage1Empty ? "YES" : "NO") << "\n";

        engine.detachStage(world, 0);
        bool stage1Detached = stages[0].isDetached;
        std::cout << "  [Check] Stage 1 detached: " << (stage1Detached ? "YES" : "NO") << "\n";

        VoxelData vd;
        bool blockRemoved = !world.getBlock(10, 10, 10, vd) || vd.type == BlockType::AIR;
        std::cout << "  [Check] Blocks removed: " << (blockRemoved ? "YES" : "NO") << "\n";

        assert(stage1Empty && "Stage 1 should run out of fuel");
        assert(stage1Detached && "Stage 1 should be detachable");
        assert(blockRemoved && "Detached blocks should be removed from world");

        std::cout << "  [Phase 4] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 5] Multi-Stage Efficiency...\n";
        float Isp = 300.0f;
        float fuelMass = 100.0f;
        float dryMass = 50.0f;
        float m0 = fuelMass + dryMass;
        float m1 = dryMass;
        float singleStageDV = Isp * G0 * std::log(m0 / m1);

        float stage1Fuel = 60.0f;
        float stage1Dry = 30.0f;
        float stage2Fuel = 40.0f;
        float stage2Dry = 20.0f;
        float m0_1 = stage1Fuel + stage1Dry + stage2Fuel + stage2Dry;
        float m1_1 = stage1Dry + stage2Fuel + stage2Dry;
        float dv1 = Isp * G0 * std::log(m0_1 / m1_1);

        float m0_2 = stage2Fuel + stage2Dry;
        float m1_2 = stage2Dry;
        float dv2 = Isp * G0 * std::log(m0_2 / m1_2);

        float twoStageDV = dv1 + dv2;

        std::cout << "  [Check] Single stage dV: " << std::fixed << std::setprecision(2) << singleStageDV << "\n";
        std::cout << "  [Check] Two stage dV:    " << twoStageDV << "\n";
        std::cout << "  [Check] Stage 1 dV:      " << dv1 << "\n";
        std::cout << "  [Check] Stage 2 dV:      " << dv2 << "\n";

        assert(twoStageDV > singleStageDV && "Multi-stage should be more efficient");
        std::cout << "  [Result] Multi-stage provides " << std::setprecision(1)
                  << ((twoStageDV / singleStageDV - 1.0f) * 100.0f) << "% more dV\n";

        std::cout << "  [Phase 5] PASSED\n\n";
    }

    std::cout << "=============================================\n";
    std::cout << "  ROCKET EQUATION TEST PASSED\n";
    std::cout << "=============================================\n";

    return 0;
}
