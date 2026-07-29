#pragma once

#include "MaterialProperties.h"
#include <vector>
#include <unordered_map>
#include <array>
#include <memory>
#include <functional>
#include <random>
#include <cmath>
#include <utility>

namespace OpenMind {

enum class BiologicalStage : uint8_t {
    SEED = 0,
    SPROUT,
    PLANT,
    FRUIT,
    DEAD
};

enum class PlantState : uint8_t {
    NONE = 0,
    SEED,
    SPROUT,
    PLANT,
    FRUIT
};

enum class DiseaseState : uint8_t {
    HEALTHY = 0,
    INCUBATING,
    SYMPTOMATIC,
    RECOVERED,
    IMMUNE
};

enum class TransmissionMode : uint8_t {
    PROXIMITY = 0,
    CONTACT,
    AIRBORNE
};

struct Disease {
    int diseaseID = 0;
    std::string name = "Unknown";
    TransmissionMode transmissionMode = TransmissionMode::PROXIMITY;
    float transmissionRange = 3.0f;
    float infectivity = 0.5f;
    float severity = 0.3f;
    float incubationPeriod = 100.0f;
    float symptomDuration = 300.0f;
    float mortalityRate = 0.1f;
    float immunityGain = 0.7f;
    float healthDrainPerTick = 0.05f;
    float hungerIncreasePerTick = 0.02f;
    float speedPenalty = 0.3f;
};

struct PlantData {
    PlantState state = PlantState::NONE;
    float growthTimer = 0.0f;
    float stageThreshold = 500.0f;
    float fruitTimer = 0.0f;
    float fruitInterval = 200.0f;
    bool conditionsMet = false;
};

struct PhysicsData {
    float temperature = 293.15f;
    float prevTemperature = 293.15f;
    BlockState state = BlockState::SOLID;
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    float velocityZ = 0.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float offsetZ = 0.0f;
    bool isOnFire = false;
    float fireTimer = 0.0f;
    bool isSubmerged = false;
    float submergedDepth = 0.0f;
    BiologicalStage bioStage = BiologicalStage::SEED;
    float growthTimer = 0.0f;
    float growthThreshold = 100.0f;
    bool hasSunlight = false;
    bool hasWater = false;
    bool hasSoil = false;
    bool isDiseased = false;
    float diseaseTimer = 0.0f;
    float health = 100.0f;
    float acidAmount = 100.0f;
    float structuralIntegrity = 1.0f;
    bool isRigid = true;
};

struct SoundReflection {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float intensity = 0.0f;
    float delay = 0.0f;
    float damping = 0.0f;
    int bounceCount = 0;
};

struct ReverbParams {
    float reverbTime = 0.5f;
    float roomSize = 10.0f;
    float damping = 0.5f;
    float wetDryMix = 0.3f;
    float diffusion = 1.0f;
    float density = 1.0f;
};

struct SoundSource {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float soundLevel = 0.0f;
    float soundRange = 50.0f;
    std::string soundType = "generic";
    float life = 1.0f;
    bool isActive = true;
    ReverbParams reverb;
    std::vector<SoundReflection> reflections;
    float directIntensity = 0.0f;
    float reflectedIntensity = 0.0f;
};

struct HeardSound {
    float intensity = 0.0f;
    std::string soundType = "";
    float sourceX = 0.0f;
    float sourceY = 0.0f;
    float sourceZ = 0.0f;
    float timer = 0.0f;
};

struct LightSource {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float lightRadius = 50.0f;
    float lightIntensity = 1.0f;
    std::string lightColor = "white";
    bool isDirectional = false;
    float dirX = 0.0f;
    float dirY = -1.0f;
    float dirZ = 0.0f;
    bool isActive = true;
    float flickerRate = 0.0f;
    float flickerAmount = 0.0f;
};

struct SunPosition {
    float angle = 0.0f;
    float x = 0.0f;
    float y = 1.0f;
    float z = 0.0f;
    float intensity = 0.0f;
    float colorTemperature = 6500.0f;
    bool isAboveHorizon = false;
    bool isMoon = false;
};

enum class WeatherType : uint8_t {
    CLEAR = 0,
    RAIN,
    SNOW,
    STORM,
    FOG
};

struct Weather {
    WeatherType type = WeatherType::CLEAR;
    float precipitationRate = 0.0f;
    float windSpeedX = 0.0f;
    float windSpeedY = 0.0f;
    float windSpeedZ = 0.0f;
    float visibility = 1.0f;
    float temperatureOffset = 0.0f;
    float lightningChance = 0.0f;
    float humidity = 0.5f;
    float pressure = 101325.0f;
    float transitionTimer = 0.0f;
    float transitionDuration = 30.0f;
    WeatherType previousType = WeatherType::CLEAR;
    float prevPrecipitationRate = 0.0f;
    float prevWindSpeedX = 0.0f;
    float prevWindSpeedY = 0.0f;
    float prevWindSpeedZ = 0.0f;
    float prevVisibility = 1.0f;
    float prevTemperatureOffset = 0.0f;
    float snowAccumulation = 0.0f;
    float rainAccumulation = 0.0f;
    float windForceX = 0.0f;
    float windForceY = 0.0f;
    float windForceZ = 0.0f;
};

struct Agent {
    int id = 0;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float vz = 0.0f;
    float energy = 100.0f;
    float maxEnergy = 100.0f;
    bool isAlive = true;
    bool isPredator = false;
    bool isPrey = false;
    bool isDiseased = false;
    float diseaseImmunity = 0.5f;
    DiseaseState diseaseState = DiseaseState::HEALTHY;
    int infectedDiseaseID = -1;
    float infectionTimer = 0.0f;
    float symptomTimer = 0.0f;
    float immunityLevel = 0.0f;
    float speed = 1.0f;
    float health = 100.0f;
    float maxHealth = 100.0f;
    float hunger = 0.0f;
    float maxHunger = 100.0f;
    float hungerRate = 0.1f;
    float visionRange = 10.0f;
    float dangerRange = 8.0f;
    float attackPower = 10.0f;
    float defensePower = 5.0f;
    float reproductionTimer = 0.0f;
    float reproductionInterval = 500.0f;
    float attackCooldown = 0.0f;
    float attackCooldownMax = 10.0f;
    float energyDrainRate = 0.1f;
    float foodValue = 30.0f;
    float hearingRange = 30.0f;
    std::vector<HeardSound> heardSounds;

    float feltTemperature = 20.0f;
    float comfortLevel = 1.0f;
    bool isSeekingWarmth = false;
    bool isSeekingShade = false;
};

struct Reaction {
    std::string reactantA;
    std::string reactantB;
    float temperatureThreshold = 0.0f;
    float activationEnergy = 0.0f;
    std::string productA;
    std::string productB;
    std::string byproduct;
    float energyReleased = 0.0f;
};

struct WorldSnapshot {
    uint64_t tick = 0;
    std::unordered_map<uint64_t, VoxelData> voxels;
    std::vector<Agent> agents;
};

struct RocketStage {
    int stageIndex = 0;
    float fuelMass = 0.0f;
    float dryMass = 0.0f;
    float totalMass = 0.0f;
    float Isp = 300.0f;
    float thrust = 0.0f;
    float throttle = 1.0f;
    float fuelBurnRate = 1.0f;
    std::string fuelType = "LH2";
    bool isActive = true;
    bool isDetached = false;
    std::vector<std::array<int,3>> blockPositions;
    float currentFuel = 0.0f;
};

struct Particle {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float vz = 0.0f;
    float life = 1.0f;
    float maxLife = 1.0f;
    float size = 1.0f;
    std::string type;
    bool active = false;
};

struct Fragment {
    int x = 0;
    int y = 0;
    int z = 0;
    float vx = 0.0f;
    float vy = 0.0f;
    float vz = 0.0f;
    float mass = 1.0f;
    BlockType type = BlockType::AIR;
};

struct Atmosphere {
    float surfaceDensity = 1.225f;
    float scaleHeight = 8500.0f;
    float maxAltitude = 100000.0f;
    float oxygenFraction = 0.21f;
    std::string composition = "N2:78%, O2:21%, Ar:1%";

    float densityAtAltitude(float altitude) const {
        if (altitude > maxAltitude) return 0.0f;
        if (altitude < 0.0f) return surfaceDensity * 1.2f;
        return surfaceDensity * std::exp(-altitude / scaleHeight);
    }

    float oxygenAtAltitude(float altitude) const {
        return densityAtAltitude(altitude) * oxygenFraction / surfaceDensity;
    }

    float dragCoefficient(float altitude) const {
        float density = densityAtAltitude(altitude);
        return density / surfaceDensity;
    }
};

struct GravitationalBody {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float vz = 0.0f;
    float mass = 0.0f;
    float radius = 1.0f;
    bool isFixed = false;
    bool isActive = true;
    int blockX = 0;
    int blockY = 0;
    int blockZ = 0;
    Atmosphere atmosphere;
    bool hasAtmosphere = false;
};

struct WorldStats {
    int totalBlocks = 0;
    int waterBlocks = 0;
    int fireBlocks = 0;
    int livingEntities = 0;
    float averageTemperature = 293.15f;
    float timeScale = 1.0f;
    uint64_t currentTick = 0;
};

inline uint64_t hashCoords(int x, int y, int z) {
    uint64_t h = 0;
    h ^= std::hash<int>{}(x) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<int>{}(y) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<int>{}(z) + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
}

static constexpr float GRAVITY_EARTH = 9.81f;
static constexpr float GRAVITY_MOON = 1.62f;
static constexpr float AIR_DENSITY = 1.225f;
static constexpr float WATER_DENSITY = 1000.0f;
static constexpr float SEA_LEVEL = 64.0f;
static constexpr float ATMOSPHERE_SCALE_HEIGHT = 8500.0f;
static constexpr float STEFAN_BOLTZMANN = 5.67e-8f;
static constexpr float KELVIN_OFFSET = 273.15f;
static constexpr int MAX_PARTICLES = 4096;
static constexpr int MAX_AGENTS = 256;
static constexpr int SNAPSHOT_BUFFER_SIZE = 120;

}
