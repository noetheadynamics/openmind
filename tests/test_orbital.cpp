#include "PhysicsEngine.h"
#include "VoxelOctree.h"
#include <iostream>
#include <iomanip>
#include <cassert>
#include <cmath>
#include <vector>

using namespace OpenMind;

float distanceTo(float ax, float ay, float az, float bx, float by, float bz) {
    float dx = bx - ax;
    float dy = by - ay;
    float dz = bz - az;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

int main() {
    std::cout << "=============================================\n";
    std::cout << "  ORBITAL MECHANICS TEST (Feature #44)\n";
    std::cout << "=============================================\n\n";

    const float G = 0.5f;
    const float PLANET_MASS = 10000.0f;
    const float ORBIT_RADIUS = 60.0f;
    const float CIRCULAR_VEL = std::sqrt(G * PLANET_MASS / ORBIT_RADIUS);
    const float PX = 10.0f, PY = 10.0f, PZ = 10.0f;
    const int TOTAL_TICKS = 2000;
    const int LOG_INTERVAL = 200;

    std::cout << "  [Setup] Planet mass: " << PLANET_MASS << "\n";
    std::cout << "  [Setup] Orbit radius: " << ORBIT_RADIUS << "\n";
    std::cout << "  [Setup] Circular velocity: " << std::fixed << std::setprecision(2) << CIRCULAR_VEL << "\n";
    std::cout << "  [Setup] G (scaled): " << G << "\n\n";

    {
        std::cout << "  [Phase 1] Circular Orbit Test...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);
        engine.setGravitationalConstant(G);

        GravitationalBody planet;
        planet.x = PX; planet.y = PY; planet.z = PZ;
        planet.mass = PLANET_MASS;
        planet.radius = 5.0f;
        planet.isFixed = true;
        engine.addCelestialBody(planet);

        Agent rocket;
        rocket.x = PX + ORBIT_RADIUS;
        rocket.y = PY;
        rocket.z = PZ;
        rocket.vx = 0.0f;
        rocket.vy = CIRCULAR_VEL;
        rocket.vz = 0.0f;
        rocket.energy = 1000.0f;
        rocket.maxEnergy = 1000.0f;
        rocket.health = 1000.0f;
        rocket.maxHealth = 1000.0f;
        rocket.isPrey = false;
        rocket.isPredator = false;
        engine.addAgent(rocket);

        float minDist = 1e10f, maxDist = 0.0f;

        std::cout << "  [Tick]  Dist     VelX     VelY     VelZ\n";
        std::cout << "  [----]  ------   ------   ------   ------\n";

        for (int tick = 0; tick <= TOTAL_TICKS; tick++) {
            if (tick % 100 == 0) {
                auto& agents = engine.getAgents();
                if (agents.size() > 0) {
                    float dist = distanceTo(PX, PY, PZ, agents[0].x, agents[0].y, agents[0].z);
                    std::cout << "  " << std::setw(6) << tick
                              << "  " << std::setw(6) << std::fixed << std::setprecision(1) << dist
                              << "  " << std::setw(6) << agents[0].vx
                              << "  " << std::setw(6) << agents[0].vy
                              << "  " << std::setw(6) << agents[0].vz << "\n";
                }
            }

            engine.applyOrbitalGravity(world, 1.0f);
            auto& agents = engine.getAgents();
            if (agents.size() > 0) {
                agents[0].x += agents[0].vx * 1.0f;
                agents[0].y += agents[0].vy * 1.0f;
                agents[0].z += agents[0].vz * 1.0f;

                float dist = distanceTo(PX, PY, PZ, agents[0].x, agents[0].y, agents[0].z);
                if (tick > 0) {
                    if (dist < minDist) minDist = dist;
                    if (dist > maxDist) maxDist = dist;
                }
            }
        }

        float eccentricity = (maxDist - minDist) / (maxDist + minDist);
        bool isStable = eccentricity < 0.5f;
        bool didNotEscape = maxDist < ORBIT_RADIUS * 3.0f;
        bool didNotSpiral = minDist > ORBIT_RADIUS * 0.2f;

        std::cout << "\n  [Result] Min distance: " << std::fixed << std::setprecision(1) << minDist << "\n";
        std::cout << "  [Result] Max distance: " << maxDist << "\n";
        std::cout << "  [Result] Eccentricity: " << std::setprecision(3) << eccentricity << "\n";
        std::cout << "  [Result] Stable orbit: " << (isStable ? "YES" : "NO") << "\n";
        std::cout << "  [Result] Did not escape: " << (didNotEscape ? "YES" : "NO") << "\n";
        std::cout << "  [Result] Did not spiral: " << (didNotSpiral ? "YES" : "NO") << "\n";

        assert(isStable && "Orbit should be stable (low eccentricity)");
        assert(didNotEscape && "Body should not escape to infinity");
        assert(didNotSpiral && "Body should not spiral into planet");
        std::cout << "  [Phase 1] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 2] Escape Velocity Test...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);
        engine.setGravitationalConstant(G);

        GravitationalBody planet;
        planet.x = PX; planet.y = PY; planet.z = PZ;
        planet.mass = PLANET_MASS;
        planet.radius = 5.0f;
        planet.isFixed = true;
        engine.addCelestialBody(planet);

        float escapeVel = engine.calculateEscapeVelocity(PLANET_MASS, ORBIT_RADIUS);
        std::cout << "  [Setup] Escape velocity: " << std::fixed << std::setprecision(2) << escapeVel << "\n";

        Agent rocket;
        rocket.x = PX + ORBIT_RADIUS;
        rocket.y = PY;
        rocket.z = PZ;
        rocket.vx = 0.0f;
        rocket.vy = escapeVel * 1.1f;
        rocket.vz = 0.0f;
        rocket.energy = 1000.0f;
        rocket.maxEnergy = 1000.0f;
        rocket.health = 1000.0f;
        rocket.maxHealth = 1000.0f;
        rocket.isPrey = false;
        rocket.isPredator = false;
        engine.addAgent(rocket);

        float initialDist = ORBIT_RADIUS;

        for (int tick = 0; tick <= 1000; tick++) {
            engine.applyOrbitalGravity(world, 1.0f);
            auto& agents = engine.getAgents();
            if (agents.size() > 0) {
                agents[0].x += agents[0].vx * 1.0f;
                agents[0].y += agents[0].vy * 1.0f;
                agents[0].z += agents[0].vz * 1.0f;
            }
        }

        auto& agents = engine.getAgents();
        float finalDist = 0.0f;
        if (agents.size() > 0) {
            finalDist = distanceTo(PX, PY, PZ, agents[0].x, agents[0].y, agents[0].z);
        }

        bool escaped = finalDist > initialDist * 2.0f;
        std::cout << "  [Result] Initial distance: " << initialDist << "\n";
        std::cout << "  [Result] Final distance: " << std::setprecision(1) << finalDist << "\n";
        std::cout << "  [Result] Escaped orbit: " << (escaped ? "YES" : "NO") << "\n";
        assert(escaped && "Body exceeding escape velocity should escape orbit");
        std::cout << "  [Phase 2] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 3] Binary Star System Test...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);
        engine.setGravitationalConstant(G);

        float starMass = 5000.0f;
        float separation = 30.0f;
        float orbitalSpeed = std::sqrt(G * starMass / separation) * 0.5f;

        GravitationalBody star1;
        star1.x = 5.0f; star1.y = 10.0f; star1.z = 10.0f;
        star1.mass = starMass;
        star1.radius = 3.0f;
        star1.isFixed = false;
        star1.vy = orbitalSpeed;
        engine.addCelestialBody(star1);

        GravitationalBody star2;
        star2.x = 35.0f; star2.y = 10.0f; star2.z = 10.0f;
        star2.mass = starMass;
        star2.radius = 3.0f;
        star2.isFixed = false;
        star2.vy = -orbitalSpeed;
        engine.addCelestialBody(star2);

        std::cout << "  [Setup] Two stars at separation " << separation << "\n";
        std::cout << "  [Setup] Orbital speed: " << std::fixed << std::setprecision(2) << orbitalSpeed << "\n";

        for (int tick = 0; tick <= 2000; tick++) {
            engine.applyOrbitalGravity(world, 1.0f);
        }

        auto& bodies = engine.getCelestialBodies();
        bool bothExist = bodies.size() >= 2;
        float finalSep = 0.0f;
        if (bothExist) {
            finalSep = distanceTo(bodies[0].x, bodies[0].y, bodies[0].z,
                                  bodies[1].x, bodies[1].y, bodies[1].z);
        }

        bool separationMaintained = bothExist && finalSep > separation * 0.3f && finalSep < separation * 3.0f;
        std::cout << "  [Result] Both stars exist: " << (bothExist ? "YES" : "NO") << "\n";
        std::cout << "  [Result] Final separation: " << std::setprecision(1) << finalSep << "\n";
        std::cout << "  [Result] System stable: " << (separationMaintained ? "YES" : "NO") << "\n";
        assert(bothExist && "Both stars should exist");
        assert(separationMaintained && "Binary system should maintain reasonable separation");
        std::cout << "  [Phase 3] PASSED\n\n";
    }

    std::cout << "=============================================\n";
    std::cout << "  ORBITAL MECHANICS TEST PASSED\n";
    std::cout << "=============================================\n";

    return 0;
}
