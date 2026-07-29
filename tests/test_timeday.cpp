#include "PhysicsEngine.h"
#include "VoxelOctree.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <iomanip>

using namespace OpenMind;

void printTime(float hour) {
    int h = static_cast<int>(hour) % 24;
    int m = static_cast<int>((hour - static_cast<int>(hour)) * 60);
    std::cout << std::setw(2) << std::setfill('0') << h << ":" << std::setw(2) << std::setfill('0') << m;
}

int main() {
    std::cout << "=============================================\n";
    std::cout << "  TIME OF DAY TEST (Feature #52)\n";
    std::cout << "=============================================\n\n";

    VoxelOctree world;
    PhysicsEngine engine;
    engine.setGravity(0.0f);
    engine.setTimeScale(1.0f);
    engine.setScanRange(16);
    engine.setCycleDuration(24.0f);

    MaterialProps plantProps;
    plantProps.chemical.composition = "C6H10O5";
    plantProps.general.hardness = 0.3f;
    plantProps.mechanical.tensileStrength = 1.0f;
    plantProps.biological.growthRate = 0.5f;
    plantProps.biological.sunlightRequirement = 0.3f;
    plantProps.biological.decayThreshold = 1000.0f;
    plantProps.chemical.lightAbsorption = 0.1f;

    for (int x = 0; x < 8; x++) {
        for (int z = 0; z < 8; z++) {
            world.setBlock(x, 0, z, BlockType::DIRT);
            world.setBlock(x, 1, z, BlockType::GRASS, plantProps);
        }
    }

    std::cout << "  [Phase 1] Sun Position Cycles Correctly...\n";
    engine.setTimeOfDay(0.0f);
    SunPosition midnight = engine.getSunPosition();
    assert(!midnight.isAboveHorizon && "Sun should be below horizon at midnight");
    assert(midnight.intensity < 0.01f && "Sun intensity near zero at midnight");

    engine.setTimeOfDay(6.5f);
    SunPosition sunrise = engine.getSunPosition();
    assert(sunrise.isAboveHorizon && "Sun should be above horizon after sunrise");
    assert(sunrise.intensity > 0.01f && "Sun intensity > 0 at sunrise");

    engine.setTimeOfDay(12.0f);
    SunPosition noon = engine.getSunPosition();
    assert(noon.isAboveHorizon && "Sun should be above horizon at noon");
    assert(noon.intensity > 0.9f && "Sun intensity near 1.0 at noon");

    engine.setTimeOfDay(17.5f);
    SunPosition sunset = engine.getSunPosition();
    assert(sunset.isAboveHorizon && "Sun should be above horizon before sunset");

    std::cout << "  [Check] Midnight - above horizon: " << (midnight.isAboveHorizon ? "YES" : "NO") << "\n";
    std::cout << "  [Check] Midnight - intensity: " << midnight.intensity << "\n";
    std::cout << "  [Check] Sunrise - above horizon: " << (sunrise.isAboveHorizon ? "YES" : "NO") << "\n";
    std::cout << "  [Check] Noon - intensity: " << noon.intensity << "\n";
    std::cout << "  [Check] Sunset - above horizon: " << (sunset.isAboveHorizon ? "YES" : "NO") << "\n";
    std::cout << "  [Phase 1] PASSED\n\n";

    std::cout << "  [Phase 2] Moon Opposite to Sun...\n";
    engine.setTimeOfDay(0.0f);
    SunPosition moonMidnight = engine.getMoonPosition();
    SunPosition sunMidnight = engine.getSunPosition();
    assert(moonMidnight.isAboveHorizon && "Moon should be above horizon at midnight");
    assert(!sunMidnight.isAboveHorizon && "Sun should be below horizon at midnight");

    engine.setTimeOfDay(12.0f);
    SunPosition moonNoon = engine.getMoonPosition();
    assert(!moonNoon.isAboveHorizon && "Moon should be below horizon at noon");

    std::cout << "  [Check] Midnight moon: " << (moonMidnight.isAboveHorizon ? "UP" : "DOWN") << "\n";
    std::cout << "  [Check] Noon moon: " << (moonNoon.isAboveHorizon ? "UP" : "DOWN") << "\n";
    std::cout << "  [Phase 2] PASSED\n\n";

    std::cout << "  [Phase 3] Sunlight Intensity Curves...\n";
    float prevIntensity = 0.0f;
    bool peaked = false;
    bool declined = false;
    std::cout << "  [Check] Hour  | Intensity\n";
    for (float h = 0.0f; h <= 24.0f; h += 3.0f) {
        engine.setTimeOfDay(h);
        float intensity = engine.getSunlightIntensity();
        std::cout << "  [Check]  " << std::setw(5) << std::fixed << std::setprecision(1) << h << " | " << intensity << "\n";

        if (h > 0.0f && h <= 12.0f && intensity > prevIntensity) peaked = true;
        if (h > 12.0f && intensity < prevIntensity) declined = true;
        prevIntensity = intensity;
    }
    assert(peaked && "Intensity should peak at noon");
    assert(declined && "Intensity should decline after noon");
    std::cout << "  [Phase 3] PASSED\n\n";

    std::cout << "  [Phase 4] Color Temperature at Sunrise/Sunset...\n";
    engine.setTimeOfDay(6.5f);
    float sunriseTemp = engine.getColorTemperature();
    engine.setTimeOfDay(12.0f);
    float noonTemp = engine.getColorTemperature();
    engine.setTimeOfDay(17.5f);
    float sunsetTemp = engine.getColorTemperature();

    std::cout << "  [Check] Sunrise temp: " << sunriseTemp << " K\n";
    std::cout << "  [Check] Noon temp: " << noonTemp << " K\n";
    std::cout << "  [Check] Sunset temp: " << sunsetTemp << " K\n";

    assert(noonTemp > sunriseTemp && "Noon should be cooler than sunrise");
    std::cout << "  [Phase 4] PASSED\n\n";

    std::cout << "  [Phase 5] Daylight Factor...\n";
    engine.setTimeOfDay(12.0f);
    float noonDF = engine.getDaylightFactor();
    engine.setTimeOfDay(0.0f);
    float midnightDF = engine.getDaylightFactor();

    std::cout << "  [Check] Noon daylight factor: " << noonDF << "\n";
    std::cout << "  [Check] Midnight daylight factor: " << midnightDF << "\n";

    assert(noonDF > midnightDF && "Daylight factor should be higher at noon");
    assert(midnightDF >= 0.05f && "Daylight factor should have minimum ambient");
    std::cout << "  [Phase 5] PASSED\n\n";

    std::cout << "  [Phase 6] Light Level Changes with Time...\n";
    engine.setTimeOfDay(12.0f);
    float noonLight = engine.getLightLevel(world, 4, 2, 4);
    engine.setTimeOfDay(0.0f);
    float midnightLight = engine.getLightLevel(world, 4, 2, 4);

    std::cout << "  [Check] Noon light level: " << noonLight << "\n";
    std::cout << "  [Check] Midnight light level: " << midnightLight << "\n";

    assert(noonLight > midnightLight && "Light level should be higher at noon");
    std::cout << "  [Phase 6] PASSED\n\n";

    std::cout << "  [Phase 7] 48-Hour Cycle Simulation...\n";
    engine.setTimeOfDay(0.0f);
    float prevSunAngle = 0.0f;
    int fullCycles = 0;
    bool sunRisen = false;
    bool sunSet = false;
    bool sunPeaked = false;
    float peakIntensity = 0.0f;
    float minIntensity = 1.0f;

    std::cout << "  [Check] Hour  | Angle  | Intensity | Light  | DF\n";
    for (float h = 0.0f; h <= 48.0f; h += 1.0f) {
        engine.setTimeOfDay(h);
        float intensity = engine.getSunlightIntensity();
        float light = engine.getLightLevel(world, 4, 2, 4);
        float df = engine.getDaylightFactor();
        SunPosition sun = engine.getSunPosition();

        if (h > 0.0f && h <= 24.0f) {
            if (sun.intensity > 0.01f && !sunRisen) sunRisen = true;
            if (sun.intensity < 0.01f && sunRisen && !sunSet) sunSet = true;
            if (sun.intensity > peakIntensity) peakIntensity = sun.intensity;
        }

        if (intensity > 0.9f) sunPeaked = true;
        if (intensity < minIntensity) minIntensity = intensity;

        if (h <= 24.0f || (static_cast<int>(h) % 12 == 0)) {
            std::cout << "  [Check]  " << std::setw(5) << std::fixed << std::setprecision(1) << h
                      << " | " << std::setw(6) << sun.angle
                      << " | " << std::setw(9) << intensity
                      << " | " << light
                      << " | " << df << "\n";
        }

        if (h > 0.0f) {
            float angleDelta = sun.angle - prevSunAngle;
            if (angleDelta < -3.0f) fullCycles++;
        }
        prevSunAngle = sun.angle;
    }

    assert(fullCycles >= 1 && "Should complete at least 1 full cycle in 48 hours");
    assert(sunRisen && "Sun should rise during the cycle");
    assert(sunSet && "Sun should set during the cycle");
    assert(sunPeaked && "Sun should reach full intensity at noon");
    assert(minIntensity < 0.1f && "Minimum intensity should be near zero at night");

    std::cout << "  [Check] Full cycles: " << fullCycles << "\n";
    std::cout << "  [Check] Peak intensity: " << peakIntensity << "\n";
    std::cout << "  [Check] Min intensity: " << minIntensity << "\n";
    std::cout << "  [Phase 7] PASSED\n\n";

    std::cout << "  [Phase 8] Time Progression via Tick...\n";
    engine.setTimeOfDay(0.0f);
    for (int i = 0; i < 100; i++) {
        engine.tick(world, 0.24f);
    }
    float timeAfterTicks = engine.getTimeOfDay();
    std::cout << "  [Check] Time after 100 ticks (dt=0.24): " << timeAfterTicks << "\n";
    assert(timeAfterTicks > 0.0f && "Time should progress via tick");
    std::cout << "  [Phase 8] PASSED\n\n";

    std::cout << "=============================================\n";
    std::cout << "  TIME OF DAY TEST PASSED\n";
    std::cout << "=============================================\n";

    return 0;
}