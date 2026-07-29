#include "PhysicsEngine.h"
#include "VoxelOctree.h"
#include <iostream>
#include <iomanip>
#include <cassert>
#include <cmath>

using namespace OpenMind;

void buildRoom(VoxelOctree& world, int cx, int cy, int cz, int size, const MaterialProps& wallProps) {
    for (int x = cx - size; x <= cx + size; x++) {
        for (int y = cy - size; y <= cy + size; y++) {
            for (int z = cz - size; z <= cz + size; z++) {
                if (x == cx - size || x == cx + size ||
                    y == cy - size || y == cy + size ||
                    z == cz - size || z == cz + size) {
                    world.setBlock(x, y, z, BlockType::STONE, wallProps);
                }
            }
        }
    }
}

void buildCanyon(VoxelOctree& world, int cx, int cy, int cz, int width, int height, int length, const MaterialProps& wallProps) {
    for (int x = cx - length; x <= cx + length; x++) {
        for (int y = cy - height; y <= cy + height; y++) {
            for (int z = cz - width; z <= cz + width; z++) {
                if (z == cz - width || z == cz + width) {
                    world.setBlock(x, y, z, BlockType::STONE, wallProps);
                }
            }
        }
    }
}

int main() {
    std::cout << "=============================================\n";
    std::cout << "  ECHO / REVERB TEST (Feature #50)\n";
    std::cout << "=============================================\n\n";

    MaterialProps stoneProps;
    stoneProps.general.mass = 100.0f;
    stoneProps.general.density = 2500.0f;
    stoneProps.mechanical.tensileStrength = 100.0f;
    stoneProps.chemical.absorptionCoefficient = 0.05f;

    MaterialProps softProps;
    softProps.general.mass = 10.0f;
    softProps.general.density = 500.0f;
    softProps.mechanical.tensileStrength = 10.0f;
    softProps.chemical.absorptionCoefficient = 0.7f;

    {
        std::cout << "  [Phase 1] Reflection Coefficient...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setScanRange(20);

        world.setBlock(10, 10, 10, BlockType::STONE, stoneProps);
        world.setBlock(12, 10, 10, BlockType::WOOD, MaterialProps());
        world.setBlock(14, 10, 10, BlockType::AIR, MaterialProps());

        float rcStone = engine.getReflectionCoefficient(10, 10, 10, world);
        float rcWood = engine.getReflectionCoefficient(12, 10, 10, world);
        float rcAir = engine.getReflectionCoefficient(14, 10, 10, world);

        std::cout << "  [Check] Stone reflection: " << std::fixed << std::setprecision(3) << rcStone << "\n";
        std::cout << "  [Check] Wood reflection:  " << rcWood << "\n";
        std::cout << "  [Check] Air reflection:   " << rcAir << "\n";

        assert(rcStone > rcWood && "Stone reflects more than wood");
        assert(rcAir == 0.0f && "Air has no reflection");
        assert(rcStone > 0.5f && "Stone should be highly reflective");

        std::cout << "  [Phase 1] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 2] Small Room Reverb (short RT60)...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        buildRoom(world, 10, 10, 10, 4, stoneProps);

        ReverbParams roomReverb;
        roomReverb.reverbTime = 0.5f;
        roomReverb.roomSize = 4.0f;
        roomReverb.damping = 0.6f;
        roomReverb.wetDryMix = 0.3f;

        engine.emitSoundWithReverb(10.0f, 10.0f, 10.0f, 100.0f, 30.0f, "speech", roomReverb);

        auto& sources = engine.getSoundSources();
        engine.calculateReverb(world, sources[0]);
        engine.calculateEcho(world, sources[0]);

        int refCount = static_cast<int>(sources[0].reflections.size());
        float totalReflected = sources[0].reflectedIntensity;

        std::cout << "  [Check] Reflections: " << refCount << "\n";
        std::cout << "  [Check] Total reflected intensity: " << std::fixed << std::setprecision(2) << totalReflected << "\n";

        assert(refCount > 0 && "Small room should have reflections");
        assert(totalReflected > 0.0f && "Should have reflected sound energy");

        std::cout << "  [Phase 2] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 3] Large Cathedral Reverb (long RT60)...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        buildRoom(world, 10, 10, 10, 8, stoneProps);

        ReverbParams cathedralReverb;
        cathedralReverb.reverbTime = 4.0f;
        cathedralReverb.roomSize = 8.0f;
        cathedralReverb.damping = 0.2f;
        cathedralReverb.wetDryMix = 0.5f;

        engine.emitSoundWithReverb(10.0f, 10.0f, 10.0f, 100.0f, 40.0f, "organ", cathedralReverb);

        auto& sources = engine.getSoundSources();
        engine.calculateReverb(world, sources[0]);
        engine.calculateEcho(world, sources[0]);

        int refCount = static_cast<int>(sources[0].reflections.size());
        float totalReflected = sources[0].reflectedIntensity;

        std::cout << "  [Check] Reflections: " << refCount << "\n";
        std::cout << "  [Check] Total reflected intensity: " << std::fixed << std::setprecision(2) << totalReflected << "\n";

        assert(refCount > 0 && "Cathedral should have reflections");
        assert(totalReflected > 0.0f && "Should have reflected sound energy");

        std::cout << "  [Phase 3] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 4] Canyon Echo (distinct reflection)...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        buildCanyon(world, 10, 10, 10, 5, 5, 10, stoneProps);

        ReverbParams canyonReverb;
        canyonReverb.reverbTime = 2.0f;
        canyonReverb.roomSize = 15.0f;
        canyonReverb.damping = 0.3f;
        canyonReverb.wetDryMix = 0.4f;

        engine.emitSoundWithReverb(10.0f, 10.0f, 10.0f, 100.0f, 40.0f, "shout", canyonReverb);

        VoxelData vd;
        bool wallAt8 = world.getBlock(10, 10, 8, vd);
        bool wallAt12 = world.getBlock(10, 10, 12, vd);
        float rc8 = engine.getReflectionCoefficient(10, 10, 8, world);
        float rc12 = engine.getReflectionCoefficient(10, 10, 12, world);
        std::cout << "  [Debug] Wall at z=8: " << (wallAt8 ? "YES" : "NO") << " rc=" << rc8 << "\n";
        std::cout << "  [Debug] Wall at z=12: " << (wallAt12 ? "YES" : "NO") << " rc=" << rc12 << "\n";

        auto& sources = engine.getSoundSources();
        engine.calculateReverb(world, sources[0]);
        engine.calculateEcho(world, sources[0]);

        int refCount = static_cast<int>(sources[0].reflections.size());
        float totalReflected = sources[0].reflectedIntensity;

        bool hasDistinctEcho = false;
        for (auto& sr : sources[0].reflections) {
            if (sr.bounceCount == 1 && sr.delay > 0.02f) {
                hasDistinctEcho = true;
                break;
            }
        }

        std::cout << "  [Check] Reflections: " << refCount << "\n";
        std::cout << "  [Check] Total reflected intensity: " << std::fixed << std::setprecision(2) << totalReflected << "\n";
        std::cout << "  [Check] Has distinct echo: " << (hasDistinctEcho ? "YES" : "NO") << "\n";

        assert(refCount > 0 && "Canyon should have reflections");
        assert(hasDistinctEcho && "Canyon should produce distinct echoes");

        std::cout << "  [Phase 4] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 5] Reflection Delay Matches Distance...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        for (int z = 5; z <= 15; z++) {
            world.setBlock(10, 10, z, BlockType::STONE, stoneProps);
            world.setBlock(11, 10, z, BlockType::STONE, stoneProps);
        }

        ReverbParams rp;
        rp.reverbTime = 1.0f;
        rp.roomSize = 5.0f;
        rp.damping = 0.5f;
        rp.wetDryMix = 0.4f;

        engine.emitSoundWithReverb(10.0f, 10.0f, 10.0f, 100.0f, 20.0f, "clap", rp);

        auto& sources = engine.getSoundSources();
        engine.calculateReverb(world, sources[0]);

        bool foundDelay = false;
        for (auto& sr : sources[0].reflections) {
            float dist = std::sqrt((sr.x - sources[0].x) * (sr.x - sources[0].x) +
                                   (sr.y - sources[0].y) * (sr.y - sources[0].y) +
                                   (sr.z - sources[0].z) * (sr.z - sources[0].z));
            float expectedDelay = (2.0f * dist) / 343.0f;
            float delayError = std::abs(sr.delay - expectedDelay);

            std::cout << "  [Check] Refl at dist " << std::fixed << std::setprecision(1) << dist
                      << ": delay=" << sr.delay << "s (expected " << expectedDelay << "s)\n";

            if (delayError < 0.01f) foundDelay = true;
        }

        assert(foundDelay && "At least one reflection should match expected delay");
        std::cout << "  [Phase 5] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 6] Wet/Dry Mix Affects Reflected Intensity...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        buildRoom(world, 10, 10, 10, 5, stoneProps);

        ReverbParams dryReverb;
        dryReverb.reverbTime = 1.0f;
        dryReverb.roomSize = 5.0f;
        dryReverb.damping = 0.5f;
        dryReverb.wetDryMix = 0.1f;

        ReverbParams wetReverb;
        wetReverb.reverbTime = 1.0f;
        wetReverb.roomSize = 5.0f;
        wetReverb.damping = 0.5f;
        wetReverb.wetDryMix = 0.8f;

        engine.emitSoundWithReverb(10.0f, 10.0f, 10.0f, 100.0f, 30.0f, "test", dryReverb);
        engine.emitSoundWithReverb(10.0f, 10.0f, 10.0f, 100.0f, 30.0f, "test", wetReverb);

        auto& sources = engine.getSoundSources();
        engine.calculateReverb(world, sources[0]);
        engine.calculateReverb(world, sources[1]);

        float dryReflected = sources[0].reflectedIntensity;
        float wetReflected = sources[1].reflectedIntensity;

        std::cout << "  [Check] Dry mix (0.1) reflected: " << std::fixed << std::setprecision(2) << dryReflected << "\n";
        std::cout << "  [Check] Wet mix (0.8) reflected: " << wetReflected << "\n";

        assert(wetReflected > dryReflected && "Higher wet/dry mix should produce more reflected intensity");

        std::cout << "  [Phase 6] PASSED\n\n";
    }

    std::cout << "=============================================\n";
    std::cout << "  ECHO / REVERB TEST PASSED\n";
    std::cout << "=============================================\n";

    return 0;
}
