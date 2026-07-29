#include "PhysicsEngine.h"
#include "VoxelOctree.h"
#include "MaterialProperties.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <string>

using namespace OpenMind;

MaterialProps createHydrogenBlockProps() {
    MaterialProps mp;
    mp.general.mass = 0.001f;
    mp.general.density = 0.089f;
    mp.thermal.specificHeat = 14300.0f;
    mp.thermal.meltingPoint = 14.0f;
    mp.thermal.boilingPoint = 20.3f;
    mp.chemical.composition = "H2";
    mp.chemical.flammability = 1.0f;
    mp.chemical.combustionPoint = 573.0f;
    mp.visual.baseColor = "#FFFFFF";
    mp.health.maxHealth = 10.0f;
    mp.health.currentHealth = 10.0f;
    return mp;
}

MaterialProps createOxygenBlockProps() {
    MaterialProps mp;
    mp.general.mass = 0.001f;
    mp.general.density = 1.429f;
    mp.thermal.specificHeat = 918.0f;
    mp.chemical.composition = "O2";
    mp.visual.baseColor = "#87CEEB";
    mp.health.maxHealth = 10.0f;
    mp.health.currentHealth = 10.0f;
    return mp;
}

int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "  REACTION MATRIX TEST" << std::endl;
    std::cout << "=============================================" << std::endl;

    VoxelOctree world;
    PhysicsEngine engine;
    engine.setScanRange(16);
    engine.setTemperature(293.15f);

    MaterialProps hydrogenProps = createHydrogenBlockProps();
    MaterialProps oxygenProps = createOxygenBlockProps();

    int h2X = 5, h2Y = 5, h2Z = 5;
    int o2X = 6, o2Y = 5, o2Z = 5;

    world.setBlock(h2X, h2Y, h2Z, BlockType::CUSTOM, hydrogenProps);
    world.setBlockTemperature(h2X, h2Y, h2Z, 600.0f);
    world.setBlock(o2X, o2Y, o2Z, BlockType::CUSTOM, oxygenProps);
    world.setBlockTemperature(o2X, o2Y, o2Z, 600.0f);

    std::cout << "[Setup] H2 at (" << h2X << "," << h2Y << "," << h2Z << ") at 600K" << std::endl;
    std::cout << "[Setup] O2 at (" << o2X << "," << o2Y << "," << o2Z << ") at 600K" << std::endl;

    VoxelData h2Start, o2Start;
    world.getBlock(h2X, h2Y, h2Z, h2Start);
    world.getBlock(o2X, o2Y, o2Z, o2Start);
    std::cout << "[Initial] H2 comp: " << h2Start.props.chemical.composition << " temp: " << h2Start.currentTemperature << "K" << std::endl;
    std::cout << "[Initial] O2 comp: " << o2Start.props.chemical.composition << " temp: " << o2Start.currentTemperature << "K" << std::endl;

    const Reaction* foundRxn = engine.findReaction("H2", "O2");
    if (foundRxn) {
        std::cout << "[Find] Reaction: " << foundRxn->reactantA << " + " << foundRxn->reactantB
                  << " -> " << foundRxn->productA << " (threshold: " << foundRxn->temperatureThreshold
                  << "K, energy: " << foundRxn->energyReleased << " J)" << std::endl;
    } else {
        std::cout << "[Find] ERROR: Reaction not found!" << std::endl;
        return 1;
    }

    std::cout << "\n[Simulate] Running 10 ticks..." << std::endl;

    for (int tick = 0; tick <= 10; tick++) {
        if (tick > 0) {
            engine.tick(world, 0.05f);
        }

        if (tick % 2 == 0) {
            VoxelData h2Data, o2Data;
            bool h2Exists = world.getBlock(h2X, h2Y, h2Z, h2Data) && h2Data.type != BlockType::AIR;
            bool o2Exists = world.getBlock(o2X, o2Y, o2Z, o2Data) && o2Data.type != BlockType::AIR;

            bool h2oFound = false;
            float maxTemp = 0.0f;
            for (int x = h2X - 1; x <= o2X + 1; x++) {
                for (int y = h2Y - 1; y <= h2Y + 1; y++) {
                    for (int z = h2Z - 1; z <= h2Z + 1; z++) {
                        VoxelData checkData;
                        if (world.getBlock(x, y, z, checkData) && checkData.type != BlockType::AIR) {
                            if (checkData.props.chemical.composition == "H2O") {
                                h2oFound = true;
                            }
                            if (checkData.currentTemperature > maxTemp) {
                                maxTemp = checkData.currentTemperature;
                            }
                        }
                    }
                }
            }

            std::cout << "  [Tick " << tick << "] H2=" << (h2Exists ? "YES" : "NO")
                      << " O2=" << (o2Exists ? "YES" : "NO")
                      << " H2O=" << (h2oFound ? "YES" : "NO")
                      << " maxTemp=" << maxTemp << "K" << std::endl;
        }
    }

    VoxelData h2Final, o2Final;
    bool h2ExistsFinal = world.getBlock(h2X, h2Y, h2Z, h2Final) && h2Final.type != BlockType::AIR && h2Final.props.chemical.composition == "H2";
    bool o2ExistsFinal = world.getBlock(o2X, o2Y, o2Z, o2Final) && o2Final.type != BlockType::AIR && o2Final.props.chemical.composition == "O2";

    bool h2oProduced = false;
    float finalMaxTemp = 0.0f;
    for (int x = h2X - 2; x <= o2X + 2; x++) {
        for (int y = h2Y - 2; y <= h2Y + 2; y++) {
            for (int z = h2Z - 2; z <= h2Z + 2; z++) {
                VoxelData checkData;
                if (world.getBlock(x, y, z, checkData) && checkData.type != BlockType::AIR) {
                    if (checkData.props.chemical.composition == "H2O") {
                        h2oProduced = true;
                    }
                    if (checkData.currentTemperature > finalMaxTemp) {
                        finalMaxTemp = checkData.currentTemperature;
                    }
                }
            }
        }
    }

    std::cout << "\n[Result] H2 consumed: " << (!h2ExistsFinal ? "YES" : "NO") << std::endl;
    std::cout << "[Result] O2 consumed: " << (!o2ExistsFinal ? "YES" : "NO") << std::endl;
    std::cout << "[Result] H2O produced: " << (h2oProduced ? "YES" : "NO") << std::endl;
    std::cout << "[Result] Max temp: " << finalMaxTemp << "K (increase: " << (finalMaxTemp - 293.15f) << "K)" << std::endl;

    assert(!h2ExistsFinal && "H2 should be consumed");
    assert(!o2ExistsFinal && "O2 should be consumed");
    assert(h2oProduced && "H2O should be produced");
    assert(finalMaxTemp > 293.15f && "Temperature should increase");

    std::cout << "\n[Test] findReaction() nullptr check..." << std::endl;
    const Reaction* noRxn = engine.findReaction("H2O", "NaCl");
    assert(noRxn == nullptr && "Non-existent reaction should return nullptr");
    std::cout << "[PASS] nullptr check passed" << std::endl;

    std::cout << "\n[Test] addReaction() custom..." << std::endl;
    Reaction customRxn;
    customRxn.reactantA = "Na";
    customRxn.reactantB = "Cl";
    customRxn.temperatureThreshold = 300.0f;
    customRxn.activationEnergy = 0.0f;
    customRxn.productA = "NaCl";
    customRxn.productB = "";
    customRxn.byproduct = "";
    customRxn.energyReleased = 411000.0f;
    engine.addReaction(customRxn);

    const Reaction* customFound = engine.findReaction("Na", "Cl");
    assert(customFound != nullptr && "Custom reaction should be found");
    assert(customFound->productA == "NaCl" && "Product should match");
    std::cout << "[PASS] addReaction() passed" << std::endl;

    std::cout << "\n=============================================" << std::endl;
    std::cout << "  ALL REACTION MATRIX TESTS PASSED" << std::endl;
    std::cout << "=============================================" << std::endl;

    return 0;
}
