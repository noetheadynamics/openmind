#include "VoxelOctree.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace OpenMind;

int main() {
    std::cout << "=== OpenMind Voxel Engine Test ===" << std::endl;

    std::cout << "\n[Test 1] Creating Steel block at (5,10,15)..." << std::endl;
    VoxelOctree world;
    MaterialProps steelProps = createSteelProps();
    world.setBlock(5, 10, 15, BlockType::STEEL, steelProps);

    VoxelData data;
    bool found = world.getBlock(5, 10, 15, data);
    assert(found && "Block should exist after setBlock");
    assert(data.type == BlockType::STEEL && "Block type should be STEEL");
    assert(data.occupied && "Block should be occupied");
    std::cout << "  Block set successfully. Type: " << blockTypeToString(data.type) << std::endl;
    std::cout << "  Mass: " << data.props.general.mass << std::endl;
    std::cout << "  Density: " << data.props.general.density << std::endl;

    std::cout << "\n[Test 2] Saving to disk..." << std::endl;
    const std::string savePath = "test_world.bin";
    bool saved = world.saveToFile(savePath);
    assert(saved && "Save should succeed");
    std::cout << "  Saved to " << savePath << std::endl;

    std::cout << "\n[Test 3] Clearing memory..." << std::endl;
    world.clear();
    assert(world.getBlockCount() == 0 && "Block count should be 0 after clear");
    assert(!world.blockExists(5, 10, 15) && "Block should not exist after clear");
    std::cout << "  Memory cleared. Block count: " << world.getBlockCount() << std::endl;

    std::cout << "\n[Test 4] Loading from disk..." << std::endl;
    VoxelOctree loadedWorld;
    bool loaded = loadedWorld.loadFromFile(savePath);
    assert(loaded && "Load should succeed");
    std::cout << "  Loaded from " << savePath << std::endl;

    std::cout << "\n[Test 5] Verifying block and properties..." << std::endl;
    found = loadedWorld.getBlock(5, 10, 15, data);
    assert(found && "Block should exist after load");
    assert(data.type == BlockType::STEEL && "Block type should be STEEL");
    assert(data.occupied && "Block should be occupied");
    assert(std::abs(data.props.general.mass - 7.85f) < 0.001f && "Mass should be intact");
    assert(std::abs(data.props.general.density - 7850.0f) < 0.01f && "Density should be intact");
    assert(std::abs(data.props.mechanical.tensileStrength - 400.0f) < 0.01f && "Tensile strength should be intact");
    assert(std::abs(data.props.thermal.meltingPoint - 1510.0f) < 0.01f && "Melting point should be intact");
    assert(data.props.chemical.composition == "Fe-C alloy" && "Composition should be intact");
    assert(std::abs(data.props.electrical.conductivity - 1.45e7f) < 1000.0f && "Conductivity should be intact");
    assert(data.props.visual.baseColor == "#71797E" && "Color should be intact");
    std::cout << "  Block verified at (5,10,15):" << std::endl;
    std::cout << "    Type: " << blockTypeToString(data.type) << std::endl;
    std::cout << "    Mass: " << data.props.general.mass << std::endl;
    std::cout << "    Density: " << data.props.general.density << std::endl;
    std::cout << "    TensileStrength: " << data.props.mechanical.tensileStrength << std::endl;
    std::cout << "    MeltingPoint: " << data.props.thermal.meltingPoint << std::endl;
    std::cout << "    Composition: " << data.props.chemical.composition << std::endl;
    std::cout << "    Conductivity: " << data.props.electrical.conductivity << std::endl;
    std::cout << "    BaseColor: " << data.props.visual.baseColor << std::endl;

    std::cout << "\n[Test 6] Overwrite test - setting new properties at same coordinate..." << std::endl;
    MaterialProps goldProps;
    goldProps.general.mass = 19.3f;
    goldProps.general.density = 19300.0f;
    goldProps.visual.baseColor = "#FFD700";
    loadedWorld.setBlock(5, 10, 15, BlockType::GOLD, goldProps);

    found = loadedWorld.getBlock(5, 10, 15, data);
    assert(found && "Block should exist after overwrite");
    assert(data.type == BlockType::GOLD && "Block type should be GOLD after overwrite");
    assert(std::abs(data.props.general.mass - 19.3f) < 0.001f && "Mass should be updated");
    assert(data.props.visual.baseColor == "#FFD700" && "Color should be updated");
    std::cout << "  Overwrite successful. Type: " << blockTypeToString(data.type) << std::endl;
    std::cout << "  New Mass: " << data.props.general.mass << std::endl;
    std::cout << "  New Color: " << data.props.visual.baseColor << std::endl;

    std::cout << "\n[Test 7] Empty block test..." << std::endl;
    found = loadedWorld.getBlock(100, 100, 100, data);
    assert(!found && "Empty block should not be found");
    std::cout << "  Empty block correctly returns false." << std::endl;

    std::cout << "\n[Test 8] Memory usage..." << std::endl;
    std::cout << "  Total blocks in world: " << loadedWorld.getBlockCount() << std::endl;
    std::cout << "  Memory allocated: " << loadedWorld.getMemoryUsage() << " voxels" << std::endl;

    std::cout << "\n=== ALL TESTS PASSED ===" << std::endl;

    return 0;
}
