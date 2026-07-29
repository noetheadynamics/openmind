#include "openmind_engine.h"
#include <cstdio>

using namespace OpenMind;

int main() {
    OpenMindEngine engine;
    engine.init();
    engine.setScanRange(16);

    engine.setBlockSimple(10, 10, 10, BlockType::DIRT);
    BlockType b;
    VoxelData d;
    engine.getBlock(10, 10, 10, d);
    if (d.type != BlockType::DIRT) { printf("FAIL: block type %d\n", (int)d.type); return 1; }

    engine.tick(1.0f);
    WorldStats s = engine.getStats();
    printf("Blocks: %d\n", s.totalBlocks);

    engine.setTimeOfDay(12.0f);
    float sunlight = engine.getSunlightIntensity();
    printf("Sunlight at noon: %f\n", sunlight);
    if (sunlight < 0.3f) { printf("FAIL: sunlight too low\n"); return 1; }

    engine.setWeather(WeatherType::RAIN);
    WeatherType w = engine.getWeatherType();
    if (w != WeatherType::RAIN) { printf("FAIL: weather\n"); return 1; }

    Agent a;
    a.x = 5.0f; a.y = 5.0f; a.z = 5.0f;
    engine.addAgent(a);
    int ac = engine.getAgentCount();
    if (ac != 1) { printf("FAIL: agent count %d\n", ac); return 1; }

    printf("ALL PASS: engine wrapper verified\n");
    return 0;
}
