#include "PhysicsEngine.h"
#include "VoxelOctree.h"
#include <iostream>
#include <iomanip>
#include <cassert>
#include <cmath>

using namespace OpenMind;

int main() {
    std::cout << "=============================================\n";
    std::cout << "  SOUND PROPAGATION TEST (Feature #49)\n";
    std::cout << "=============================================\n\n";

    const float EPS = 0.01f;

    {
        std::cout << "  [Phase 1] Inverse Square Law...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        SoundSource src;
        src.x = 10.0f; src.y = 10.0f; src.z = 10.0f;
        src.soundLevel = 100.0f;
        src.soundRange = 50.0f;
        src.soundType = "explosion";
        src.isActive = true;

        float i1 = engine.getSoundIntensity(world, 10, 10, 10, 12, 10, 10, src);
        float i5 = engine.getSoundIntensity(world, 10, 10, 10, 15, 10, 10, src);
        float i10 = engine.getSoundIntensity(world, 10, 10, 10, 20, 10, 10, src);

        std::cout << "  [Check] Intensity at dist 2:  " << std::fixed << std::setprecision(2) << i1 << "\n";
        std::cout << "  [Check] Intensity at dist 5:  " << i5 << "\n";
        std::cout << "  [Check] Intensity at dist 10: " << i10 << "\n";

        assert(i1 > i5 && "Closer = louder");
        assert(i5 > i10 && "Farther = quieter");
        assert(i10 > 0.0f && "Sound should still be audible at distance 10");

        float ratio_1_5 = i1 / i5;
        float expected_ratio = (5.0f * 5.0f) / (2.0f * 2.0f);
        std::cout << "  [Check] Ratio i1/i5: " << std::setprecision(2) << ratio_1_5
                  << " (expected ~" << expected_ratio << ")\n";
        assert(std::abs(ratio_1_5 - expected_ratio) < 0.5f && "Should follow inverse square law");

        std::cout << "  [Phase 1] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 2] Sound Reaches Agent...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        engine.emitSound(10.0f, 10.0f, 10.0f, 100.0f, 50.0f, "explosion");

        Agent listener;
        listener.x = 15.0f; listener.y = 10.0f; listener.z = 10.0f;
        listener.hearingRange = 30.0f;
        listener.energy = 100.0f; listener.maxEnergy = 100.0f;
        listener.health = 100.0f; listener.maxHealth = 100.0f;
        listener.isPrey = false; listener.isPredator = false;
        engine.addAgent(listener);

        for (int tick = 0; tick <= 5; tick++) {
            engine.propagateSound(world, 1.0f);
        }

        auto& agents = engine.getAgents();
        bool heard = agents[0].heardSounds.size() > 0;
        std::cout << "  [Result] Agent heard sound: " << (heard ? "YES" : "NO") << "\n";
        if (heard) {
            std::cout << "  [Result] Sound type: " << agents[0].heardSounds[0].soundType << "\n";
            std::cout << "  [Result] Intensity: " << std::fixed << std::setprecision(4)
                      << agents[0].heardSounds[0].intensity << "\n";
        }
        assert(heard && "Agent should hear the explosion");
        std::cout << "  [Phase 2] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 3] Sound Blocked by Wall...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        MaterialProps wallProps;
        wallProps.general.mass = 100.0f;
        wallProps.general.density = 2000.0f;
        wallProps.mechanical.tensileStrength = 100.0f;
        wallProps.chemical.absorptionCoefficient = 0.9f;

        for (int y = 9; y <= 11; y++) {
            for (int z = 9; z <= 11; z++) {
                world.setBlock(12, y, z, BlockType::STONE, wallProps);
            }
        }

        SoundSource src;
        src.x = 10.0f; src.y = 10.0f; src.z = 10.0f;
        src.soundLevel = 100.0f;
        src.soundRange = 50.0f;
        src.soundType = "explosion";
        src.isActive = true;

        float iBlocked = engine.getSoundIntensity(world, 10, 10, 10, 15, 10, 10, src);
        float iUnblocked = engine.getSoundIntensity(world, 10, 10, 10, 15, 10, 18, src);

        std::cout << "  [Check] Intensity through wall: " << std::fixed << std::setprecision(4) << iBlocked << "\n";
        std::cout << "  [Check] Intensity around wall:  " << iUnblocked << "\n";

        assert(iBlocked < iUnblocked && "Wall should reduce sound intensity");
        assert(iUnblocked > 0.0f && "Unblocked path should have sound");
        std::cout << "  [Phase 3] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 4] Intensity Decreases with Distance...\n";
        PhysicsEngine engine;

        SoundSource src;
        src.x = 10.0f; src.y = 10.0f; src.z = 10.0f;
        src.soundLevel = 100.0f;
        src.soundRange = 50.0f;

        VoxelOctree world;

        float i5 = engine.getSoundIntensity(world, 10, 10, 10, 15, 10, 10, src);
        float i10 = engine.getSoundIntensity(world, 10, 10, 10, 20, 10, 10, src);
        float i15 = engine.getSoundIntensity(world, 10, 10, 10, 25, 10, 10, src);

        std::cout << "  [Check] Intensity at dist 5:  " << std::fixed << std::setprecision(4) << i5 << "\n";
        std::cout << "  [Check] Intensity at dist 10: " << i10 << "\n";
        std::cout << "  [Check] Intensity at dist 15: " << i15 << "\n";

        assert(i5 > i10 && "Closer = louder");
        assert(i10 > i15 && "Farther = quieter");
        assert(i15 > 0.0f && "Sound should still be present at distance 15");

        std::cout << "  [Phase 4] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 5] Sound Range Limit...\n";
        PhysicsEngine engine;

        SoundSource src;
        src.x = 10.0f; src.y = 10.0f; src.z = 10.0f;
        src.soundLevel = 100.0f;
        src.soundRange = 5.0f;

        VoxelOctree world;

        float iNear = engine.getSoundIntensity(world, 10, 10, 10, 13, 10, 10, src);
        float iFar = engine.getSoundIntensity(world, 10, 10, 10, 25, 10, 10, src);

        std::cout << "  [Check] Intensity within range (dist 3):  " << std::fixed << std::setprecision(4) << iNear << "\n";
        std::cout << "  [Check] Intensity outside range (dist 15): " << iFar << "\n";

        assert(iNear > 0.0f && "Sound within range should be audible");
        assert(iFar == 0.0f && "Sound beyond range should be silent");
        std::cout << "  [Phase 5] PASSED\n\n";
    }

    std::cout << "=============================================\n";
    std::cout << "  SOUND PROPAGATION TEST PASSED\n";
    std::cout << "=============================================\n";

    return 0;
}
