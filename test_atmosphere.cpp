#include "PhysicsEngine.h"
#include "VoxelOctree.h"
#include <iostream>
#include <iomanip>
#include <cassert>
#include <cmath>

using namespace OpenMind;

int main() {
    std::cout << "=============================================\n";
    std::cout << "  ATMOSPHERIC LAYERS TEST (Feature #45)\n";
    std::cout << "=============================================\n\n";

    const float EARTH_SD = 1.225f;
    const float EARTH_SH = 8500.0f;
    const float EPS = 0.01f;

    {
        std::cout << "  [Phase 1] Atmosphere Struct Density...\n";
        Atmosphere earth;
        earth.surfaceDensity = EARTH_SD;
        earth.scaleHeight = EARTH_SH;
        earth.maxAltitude = 100000.0f;
        earth.oxygenFraction = 0.21f;

        float d0 = earth.densityAtAltitude(0.0f);
        float d10k = earth.densityAtAltitude(10000.0f);
        float d100k = earth.densityAtAltitude(100000.0f);
        float exp10k = EARTH_SD * std::exp(-10000.0f / EARTH_SH);

        std::cout << "  [0m]      " << std::fixed << std::setprecision(4) << d0 << " (expect " << EARTH_SD << ")\n";
        std::cout << "  [10000m]  " << d10k << " (expect " << exp10k << ")\n";
        std::cout << "  [100000m] " << d100k << " (expect ~0)\n";

        assert(std::abs(d0 - EARTH_SD) < EPS);
        assert(std::abs(d10k - exp10k) < EPS);
        assert(d100k < 0.001f);
        std::cout << "  [Phase 1] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 2] Oxygen Level...\n";
        Atmosphere earth;
        earth.surfaceDensity = EARTH_SD;
        earth.scaleHeight = EARTH_SH;
        earth.maxAltitude = 100000.0f;
        earth.oxygenFraction = 0.21f;

        float o0 = earth.oxygenAtAltitude(0.0f);
        float o10k = earth.oxygenAtAltitude(10000.0f);
        float o100k = earth.oxygenAtAltitude(100000.0f);

        std::cout << "  [0m]      O2=" << std::fixed << std::setprecision(4) << o0 << "\n";
        std::cout << "  [10000m]  O2=" << o10k << "\n";
        std::cout << "  [100000m] O2=" << o100k << "\n";

        assert(o0 > 0.2f && o0 < 0.22f);
        assert(o10k > 0.0f && o10k < o0);
        assert(o100k < 0.001f);
        std::cout << "  [Phase 2] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 3] Drag Factor...\n";
        Atmosphere earth;
        earth.surfaceDensity = EARTH_SD;
        earth.scaleHeight = EARTH_SH;
        earth.maxAltitude = 100000.0f;

        float df0 = earth.dragCoefficient(0.0f);
        float df10k = earth.dragCoefficient(10000.0f);
        float df100k = earth.dragCoefficient(100000.0f);

        std::cout << "  [0m]      drag=" << std::fixed << std::setprecision(4) << df0 << "\n";
        std::cout << "  [10000m]  drag=" << df10k << "\n";
        std::cout << "  [100000m] drag=" << df100k << "\n";

        assert(std::abs(df0 - 1.0f) < EPS);
        assert(df10k < df0 && df10k > 0.0f);
        assert(df100k < 0.001f);
        std::cout << "  [Phase 3] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 4] Engine Integration...\n";
        PhysicsEngine engine;
        engine.setGravity(9.81f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        float d0 = engine.getAtmosphereDensity(0.0f);
        float d10k = engine.getAtmosphereDensity(10000.0f);
        float d100k = engine.getAtmosphereDensity(100000.0f);
        float exp10k = EARTH_SD * std::exp(-10000.0f / EARTH_SH);

        std::cout << "  [0m]      " << std::fixed << std::setprecision(4) << d0 << "\n";
        std::cout << "  [10000m]  " << d10k << "\n";
        std::cout << "  [100000m] " << d100k << "\n";

        assert(std::abs(d0 - EARTH_SD) < EPS);
        assert(std::abs(d10k - exp10k) < EPS);
        assert(d100k < 0.001f);
        std::cout << "  [Phase 4] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 5] Agent Drag and Re-entry Heating...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        Agent rocket;
        rocket.x = 10.0f; rocket.y = 10.0f; rocket.z = 10.0f;
        rocket.vx = 0.0f; rocket.vy = 150.0f; rocket.vz = 0.0f;
        rocket.energy = 1000.0f; rocket.maxEnergy = 1000.0f;
        rocket.health = 100.0f; rocket.maxHealth = 100.0f;
        rocket.isPrey = false; rocket.isPredator = false;
        engine.addAgent(rocket);

        float initHealth = engine.getAgents()[0].health;
        float initVy = engine.getAgents()[0].vy;

        std::cout << "  [Tick]  Altitude  VelY    Health\n";
        std::cout << "  [----]  --------  ------  ------\n";

        for (int tick = 0; tick <= 500; tick++) {
            engine.applyAtmosphericDrag(world, 1.0f);
            auto& a = engine.getAgents();
            if (a.size() > 0) {
                a[0].y += a[0].vy * 1.0f;
                if (tick % 100 == 0) {
                    float alt = a[0].y - SEA_LEVEL;
                    std::cout << "  " << std::setw(6) << tick
                              << "  " << std::setw(8) << std::fixed << std::setprecision(1) << alt
                              << "  " << std::setw(6) << a[0].vy
                              << "  " << std::setw(6) << a[0].health << "\n";
                }
            }
        }

        auto& a = engine.getAgents();
        bool slowed = a[0].vy < initVy;
        bool damaged = a[0].health < initHealth;
        std::cout << "\n  [Result] Drag applied: " << (slowed ? "YES" : "NO") << "\n";
        std::cout << "  [Result] Heat damage: " << (damaged ? "YES" : "NO") << "\n";
        assert(slowed && "Agent should slow from drag");
        assert(damaged && "Agent should take heat damage");
        std::cout << "  [Phase 5] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 6] Oxygen Decrease with Altitude...\n";
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        float o2_surface = engine.getOxygenLevel(0.0f);
        float o2_100m = engine.getOxygenLevel(100.0f);
        float o2_1000m = engine.getOxygenLevel(1000.0f);
        float o2_10000m = engine.getOxygenLevel(10000.0f);

        std::cout << "  [Check] O2 at 0m:      " << std::fixed << std::setprecision(4) << o2_surface << "\n";
        std::cout << "  [Check] O2 at 100m:    " << o2_100m << "\n";
        std::cout << "  [Check] O2 at 1000m:   " << o2_1000m << "\n";
        std::cout << "  [Check] O2 at 10000m:  " << o2_10000m << "\n";

        assert(o2_surface > 0.2f && o2_surface < 0.22f && "O2 at sea level ~0.21");
        assert(o2_100m < o2_surface && "O2 decreases with altitude");
        assert(o2_1000m < o2_100m && "O2 decreases further");
        assert(o2_10000m < o2_1000m && "O2 continues decreasing");
        assert(o2_10000m > 0.0f && "O2 not zero at 10km");
        std::cout << "  [Phase 6] PASSED\n\n";
    }

    std::cout << "=============================================\n";
    std::cout << "  ATMOSPHERIC LAYERS TEST PASSED\n";
    std::cout << "=============================================\n";

    return 0;
}
