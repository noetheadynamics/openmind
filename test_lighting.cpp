#include "PhysicsEngine.h"
#include "VoxelOctree.h"
#include <iostream>
#include <iomanip>
#include <cassert>
#include <cmath>

using namespace OpenMind;

int main() {
    std::cout << "=============================================\n";
    std::cout << "  DYNAMIC LIGHTING TEST (Feature #51)\n";
    std::cout << "=============================================\n\n";

    MaterialProps opaqueProps;
    opaqueProps.general.mass = 100.0f;
    opaqueProps.general.density = 2500.0f;
    opaqueProps.mechanical.tensileStrength = 100.0f;
    opaqueProps.chemical.lightAbsorption = 0.95f;

    MaterialProps transparentProps;
    transparentProps.general.mass = 10.0f;
    transparentProps.general.density = 500.0f;
    transparentProps.mechanical.tensileStrength = 10.0f;
    transparentProps.chemical.lightAbsorption = 0.05f;

    {
        std::cout << "  [Phase 1] Light Intensity Decreases with Distance...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        engine.emitLight(10.0f, 10.0f, 10.0f, 50.0f, 1.0f, "white");

        float l2 = engine.getLightLevel(world, 12, 10, 10);
        float l5 = engine.getLightLevel(world, 15, 10, 10);
        float l8 = engine.getLightLevel(world, 18, 10, 10);

        std::cout << "  [Check] Light at dist 2:  " << std::fixed << std::setprecision(4) << l2 << "\n";
        std::cout << "  [Check] Light at dist 5:  " << l5 << "\n";
        std::cout << "  [Check] Light at dist 8:  " << l8 << "\n";

        assert(l2 > l5 && "Closer = brighter");
        assert(l5 > l8 && "Farther = dimmer");
        assert(l8 > 0.0f && "Light should still be present at distance 8");

        std::cout << "  [Phase 1] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 2] Shadow from Opaque Wall...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        engine.emitLight(10.0f, 10.0f, 10.0f, 50.0f, 1.0f, "white");

        for (int y = 8; y <= 12; y++) {
            for (int z = 8; z <= 12; z++) {
                world.setBlock(14, y, z, BlockType::STONE, opaqueProps);
            }
        }

        float litBehind = engine.getLightLevel(world, 12, 10, 10);
        float shadowed = engine.getLightLevel(world, 16, 10, 10);

        std::cout << "  [Check] Light before wall: " << std::fixed << std::setprecision(4) << litBehind << "\n";
        std::cout << "  [Check] Light behind wall: " << shadowed << "\n";

        assert(litBehind > shadowed && "Wall should cast shadow");
        assert(shadowed < 0.3f && "Area behind wall should be dark");

        std::cout << "  [Phase 2] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 3] No Shadow Through Transparent Block...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        engine.emitLight(10.0f, 10.0f, 10.0f, 50.0f, 1.0f, "white");

        for (int y = 8; y <= 12; y++) {
            for (int z = 8; z <= 12; z++) {
                world.setBlock(14, y, z, BlockType::GLASS, transparentProps);
            }
        }

        float litBefore = engine.getLightLevel(world, 12, 10, 10);
        float litThrough = engine.getLightLevel(world, 16, 10, 10);

        std::cout << "  [Check] Light before glass: " << std::fixed << std::setprecision(4) << litBefore << "\n";
        std::cout << "  [Check] Light through glass: " << litThrough << "\n";

        float litThroughNoGlass = 0.0f;
        {
            VoxelOctree world2;
            PhysicsEngine engine2;
            engine2.setGravity(0.0f);
            engine2.setTimeScale(1.0f);
            engine2.setScanRange(20);
            engine2.emitLight(10.0f, 10.0f, 10.0f, 50.0f, 1.0f, "white");
            litThroughNoGlass = engine2.getLightLevel(world2, 16, 10, 10);
        }

        std::cout << "  [Check] Light without glass: " << litThroughNoGlass << "\n";

        float glassTransmittance = (litThrough - 0.05f) / (litThroughNoGlass - 0.05f + 0.001f);
        std::cout << "  [Check] Glass transmittance: " << glassTransmittance << "\n";

        assert(glassTransmittance > 0.7f && "Glass should let most light through");
        assert(litThrough > litThroughNoGlass * 0.5f && "Glass should not block much light");

        std::cout << "  [Phase 3] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 4] Ambient Light in Darkness...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        float ambient = engine.getLightLevel(world, 10, 10, 10);
        std::cout << "  [Check] Ambient light level: " << std::fixed << std::setprecision(4) << ambient << "\n";

        assert(ambient > 0.0f && "Should have ambient light");
        assert(ambient < 0.2f && "Ambient should be dim");

        std::cout << "  [Phase 4] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 5] Multiple Light Sources Additive...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        engine.emitLight(10.0f, 10.0f, 10.0f, 30.0f, 0.5f, "red");
        float oneLight = engine.getLightLevel(world, 12, 10, 10);

        engine.emitLight(12.0f, 10.0f, 10.0f, 30.0f, 0.5f, "blue");
        float twoLights = engine.getLightLevel(world, 12, 10, 10);

        std::cout << "  [Check] One light:  " << std::fixed << std::setprecision(4) << oneLight << "\n";
        std::cout << "  [Check] Two lights: " << twoLights << "\n";

        assert(twoLights > oneLight && "Two lights should be brighter than one");

        std::cout << "  [Phase 5] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 6] Light Beyond Radius is Zero...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        engine.emitLight(10.0f, 10.0f, 10.0f, 5.0f, 1.0f, "white");

        float nearLight = engine.getLightLevel(world, 12, 10, 10);
        float farLight = engine.getLightLevel(world, 18, 10, 10);

        std::cout << "  [Check] Within radius:  " << std::fixed << std::setprecision(4) << nearLight << "\n";
        std::cout << "  [Check] Beyond radius:  " << farLight << "\n";

        assert(nearLight > 0.1f && "Should have light within radius");
        assert(farLight < nearLight * 0.5f && "Light should be much dimmer beyond radius");

        std::cout << "  [Phase 6] PASSED\n\n";
    }

    {
        std::cout << "  [Phase 7] Directional Light (Sun) with Shadow...\n";
        VoxelOctree world;
        PhysicsEngine engine;
        engine.setGravity(0.0f);
        engine.setTimeScale(1.0f);
        engine.setScanRange(20);

        engine.emitDirectionalLight(10.0f, 20.0f, 10.0f, 0.0f, -1.0f, 0.0f, 1.0f, "yellow");

        for (int y = 14; y <= 16; y++) {
            for (int z = 8; z <= 12; z++) {
                world.setBlock(10, y, z, BlockType::STONE, opaqueProps);
            }
        }

        float litAbove = engine.getLightLevel(world, 10, 18, 10);
        float shadowedBelow = engine.getLightLevel(world, 10, 10, 10);

        std::cout << "  [Check] Above wall (lit): " << std::fixed << std::setprecision(4) << litAbove << "\n";
        std::cout << "  [Check] Below wall (shadowed): " << shadowedBelow << "\n";

        assert(litAbove > shadowedBelow && "Wall should cast shadow from directional light");

        std::cout << "  [Phase 7] PASSED\n\n";
    }

    std::cout << "=============================================\n";
    std::cout << "  DYNAMIC LIGHTING TEST PASSED\n";
    std::cout << "=============================================\n";

    return 0;
}
