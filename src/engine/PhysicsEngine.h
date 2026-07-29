#pragma once

#include "MaterialProperties.h"
#include "VoxelOctree.h"
#include "PhysicsTypes.h"
#include <vector>
#include <array>
#include <random>
#include <functional>
#include <unordered_map>

namespace OpenMind {

class PhysicsEngine {
public:
    PhysicsEngine();
    ~PhysicsEngine() = default;

    void tick(VoxelOctree& world, float deltaTime);

    void setGravity(float g);
    void setTimeScale(float scale);
    void setTemperature(float ambient);
    void setWind(float wx, float wy, float wz);
    void setScanRange(int range);

    float getGravity() const;
    float getTimeScale() const;
    float getAmbientTemperature() const;
    uint64_t getCurrentTick() const;
    const WorldStats& getStats() const;

    WorldSnapshot saveSnapshot(const VoxelOctree& world) const;
    void restoreSnapshot(VoxelOctree& world, const WorldSnapshot& snapshot);
    bool rewindTime(VoxelOctree& world, int ticks);

    const std::vector<Fragment>& getPendingFragments() const;
    void clearPendingFragments();

    std::vector<Particle>& getParticles();
    std::vector<Agent>& getAgents();
    const std::vector<Agent>& getAgents() const;

    void addAgent(const Agent& agent);
    void removeAgent(int id);

    float getAtmosphereDensity(float altitude) const;
    float getOxygenLevel(float altitude) const;
    float getAtmosphereDragFactor(float altitude, float velocity) const;
    float getReentryHeat(float altitude, float velocity) const;
    void applyAtmosphericDrag(VoxelOctree& world, float dt);
    float getBuoyancyForce(float blockDensity, float fluidDensity) const;

    void addReaction(const Reaction& reaction);
    const Reaction* findReaction(const std::string& compA, const std::string& compB);

    void addDisease(const Disease& disease);

    void addCelestialBody(const GravitationalBody& body);
    void removeCelestialBody(int index);
    std::vector<GravitationalBody>& getCelestialBodies();
    void setGravitationalConstant(float g);
    float calculateEscapeVelocity(float centralMass, float distance) const;
    void applyOrbitalGravity(VoxelOctree& world, float dt);

    bool isBurning(int x, int y, int z) const;
    void extinguishBlock(int x, int y, int z);

    void triggerExplosion(VoxelOctree& world, int x, int y, int z);

    PlantState getPlantState(int x, int y, int z) const;
    const PlantData* getPlantData(int x, int y, int z) const;

    void processRocketPhysics(VoxelOctree& world, float dt);
    float calculateDeltaV(float Isp, float m0, float m1) const;
    void detachStage(VoxelOctree& world, int stageIndex);
    void addRocketStage(const RocketStage& stage);
    std::vector<RocketStage>& getRocketStages();
    void setRocketThrottle(int stageIndex, float throttle);

    void emitSound(float x, float y, float z, float level, float range, const std::string& type);
    void emitSoundWithReverb(float x, float y, float z, float level, float range, const std::string& type, const ReverbParams& reverb);
    void propagateSound(VoxelOctree& world, float dt);
    float getSoundIntensity(const VoxelOctree& world, float sx, float sy, float sz, float ax, float ay, float az, const SoundSource& source) const;
    std::vector<SoundSource>& getSoundSources();
    void calculateReverb(const VoxelOctree& world, SoundSource& source);
    void calculateEcho(const VoxelOctree& world, SoundSource& source);
    float getReflectionCoefficient(int x, int y, int z, const VoxelOctree& world) const;

    void emitLight(float x, float y, float z, float radius, float intensity, const std::string& color);
    void emitDirectionalLight(float x, float y, float z, float dx, float dy, float dz, float intensity, const std::string& color);
    void propagateLight(VoxelOctree& world, float dt);
    float getLightLevel(const VoxelOctree& world, int x, int y, int z) const;
    std::vector<LightSource>& getLightSources();

    void setTimeOfDay(float time);
    float getTimeOfDay() const;
    void setCycleDuration(float hours);
    float getCycleDuration() const;
    SunPosition getSunPosition() const;
    SunPosition getMoonPosition() const;
    float getSunlightIntensity() const;
    float getColorTemperature() const;
    float getDaylightFactor() const;

    void setWeather(WeatherType type);
    WeatherType getWeatherType() const;
    const Weather& getWeather() const;
    void setWeatherTransition(WeatherType newType, float duration);
    float getPrecipitationRate() const;
    float getWindSpeed() const;
    float getVisibility() const;
    float getSnowAccumulation() const;
    float getRainAccumulation() const;

    float calculateFeltTemperature(const Agent& agent) const;
    float getComfortLevel(float temperature) const;
    void processTemperaturePerception(VoxelOctree& world, float dt);

private:
    float gravity;
    float timeScale;
    float ambientTemperature;
    float windX, windY, windZ;
    uint64_t currentTick;
    int scanRange;
    WorldStats stats;

    float currentTime = 6.0f;
    float cycleDuration = 24.0f;
    float sunOrbitRadius = 100.0f;

    Weather currentWeather;
    float weatherTimer = 0.0f;
    float weatherChangeInterval = 300.0f;

    std::mt19937 rng;
    std::vector<Fragment> pendingFragments;
    std::vector<Particle> particles;
    std::vector<Agent> agents;
    std::vector<Reaction> reactionMatrix;
    std::vector<Disease> diseases;
    std::vector<GravitationalBody> celestialBodies;
    bool celestialScanDone = false;
    std::vector<RocketStage> rocketStages;
    std::vector<SoundSource> soundSources;
    std::vector<LightSource> lightSources;
    float gravitationalConstant = 0.5f;
    std::array<WorldSnapshot, SNAPSHOT_BUFFER_SIZE> snapshotBuffer;
    int snapshotIndex;
    int snapshotCount;

    std::unordered_map<uint64_t, float> radiationMap;
    std::unordered_map<uint64_t, float> burnTimers;
    std::unordered_map<uint64_t, float> corrosionTimers;
    std::unordered_map<uint64_t, float> dissolutionTimers;
    std::unordered_map<uint64_t, bool> explosionProcessed;
    std::unordered_map<uint64_t, PlantData> plantRegistry;
    std::unordered_map<uint64_t, float> decayTimers;
    std::unordered_map<uint64_t, struct PhysicsData> physicsDataMap;

    float hysteresisMargin = 5.0f;

    void initReactionMatrix();

    void tickPhysics(VoxelOctree& world, float dt);
    void tickThermodynamics(VoxelOctree& world, float dt);
    void tickFluids(VoxelOctree& world, float dt);
    void tickChemistry(VoxelOctree& world, float dt);
    void tickBiology(VoxelOctree& world, float dt);
    void tickAgents(VoxelOctree& world, float dt);
    void tickParticles(float dt);
    void tickSnapshots(VoxelOctree& world);
    void updateStats(const VoxelOctree& world);

    void updateWeather(VoxelOctree& world, float dt);
    void spawnRainParticles(int count, float windX, float windZ);
    void spawnSnowParticles(int count, float windX, float windZ);
    void applyWindForce(VoxelOctree& world, float dt);
    void processWeatherAccumulation(VoxelOctree& world, float dt);
    void transitionWeather(float dt);

    void applyGravityToBlock(VoxelOctree& world, int x, int y, int z, PhysicsData& pd, const MaterialProps& mp, float dt);
    float calculateDrag(int exposedFaces, float velocity, float dragCoeff) const;
    float calculateStress(float force, float area) const;
    void handleDeformation(PhysicsData& pd, float stress, float elasticLimit);
    void breakBlock(VoxelOctree& world, int x, int y, int z, const MaterialProps& mp);
    int countExposedFaces(const VoxelOctree& world, int x, int y, int z) const;

    void transferHeat(VoxelOctree& world, float dt);
    void applyConvection(VoxelOctree& world);
    void swapFluidBlocks(VoxelOctree& world, int x1, int y1, int z1, int x2, int y2, int z2);
    void handlePhaseChange(VoxelOctree& world, int x, int y, int z, PhysicsData& pd, const MaterialProps& mp);
    void handleMelting(VoxelOctree& world, int x, int y, int z);
    void handleFreezing(VoxelOctree& world, int x, int y, int z);
    void handleBoiling(VoxelOctree& world, int x, int y, int z);
    void expandGas(VoxelOctree& world, int x, int y, int z, const MaterialProps& gasProps);
    void handleCondensation(VoxelOctree& world, int x, int y, int z);
    void removeAdjacentGas(VoxelOctree& world, int x, int y, int z);
    void emitRadiation(VoxelOctree& world, int x, int y, int z, const PhysicsData& pd, float dt);
    void propagateRadiation(VoxelOctree& world, float dt);
    void absorbRadiation(VoxelOctree& world, int x, int y, int z, float intensity, float dt);

    void simulateWaterFlow(VoxelOctree& world, float dt);
    void simulateGasDiffusion(VoxelOctree& world, float dt);

    void checkReactions(VoxelOctree& world, int x, int y, int z);
    void igniteBlock(VoxelOctree& world, int x, int y, int z, PhysicsData& pd, const MaterialProps& mp);
    void corrodeBlock(VoxelOctree& world, int x, int y, int z, PhysicsData& pd, const MaterialProps& mp, float dt);

    void processCombustion(VoxelOctree& world, int x, int y, int z, float dt);
    void spreadFire(VoxelOctree& world, int x, int y, int z, float dt);
    bool hasAdjacentOxidizer(const VoxelOctree& world, int x, int y, int z) const;
    uint64_t posHash(int x, int y, int z) const;

    void processCorrosion(VoxelOctree& world, int x, int y, int z, float dt);
    bool hasAdjacentWater(const VoxelOctree& world, int x, int y, int z) const;
    bool hasAdjacentOxygen(const VoxelOctree& world, int x, int y, int z) const;

    void processAcidReactions(VoxelOctree& world, int x, int y, int z, float dt);
    bool isAcid(const MaterialProps& mp) const;
    bool isBase(const MaterialProps& mp) const;
    void neutralizeAcid(VoxelOctree& world, int x, int y, int z, int nx, int ny, int nz);

    void applyShockwave(VoxelOctree& world, int x, int y, int z, float power, float radius);
    void spawnFragments(VoxelOctree& world, int x, int y, int z, int count, float power);
    void processChainReactions(VoxelOctree& world, int x, int y, int z, float power, int depth);
    bool checkDetonationConditions(VoxelOctree& world, int x, int y, int z);

    void tickPlantGrowth(VoxelOctree& world, int x, int y, int z, PhysicsData& pd, const MaterialProps& mp, float dt);
    void processEcosystem(VoxelOctree& world, float dt);
    void processDiseaseSpread(VoxelOctree& world, float dt);
    void transmitDisease(Agent& source, Agent& target, const Disease& disease, float dt);
    void progressDisease(Agent& agent, const Disease& disease, float dt);
    void processDecay(VoxelOctree& world, int x, int y, int z, PhysicsData& pd, float dt);
    float calculateDecayRate(const VoxelOctree& world, int x, int y, int z, const MaterialProps& mp) const;
    void spawnDecayProducts(VoxelOctree& world, int x, int y, int z, BlockType originalType);

    void processPlantGrowth(VoxelOctree& world, int x, int y, int z, float dt);
    bool hasSunlight(const VoxelOctree& world, int x, int y, int z) const;
    bool hasWaterNearby(const VoxelOctree& world, int x, int y, int z) const;
    bool hasSoilBelow(const VoxelOctree& world, int x, int y, int z) const;
    void produceFruit(VoxelOctree& world, int x, int y, int z);

    void processPredatorPrey(VoxelOctree& world, float dt);
    void huntPredator(VoxelOctree& world, Agent& predator, float dt);
    void fleePrey(VoxelOctree& world, Agent& prey, float dt);
    void reproducePrey(VoxelOctree& world, Agent& prey, float dt, std::vector<Agent>& newAgents);

    void processMetabolism(VoxelOctree& world, float dt);
    void consumeFood(VoxelOctree& world, Agent& agent, float dt);
    void checkStarvation(Agent& agent, float dt);

    void scanCelestialBodies(const VoxelOctree& world);

    int getWaterHeight(const VoxelOctree& world, int x, int z) const;
    bool isWaterBlock(const VoxelOctree& world, int x, int y, int z) const;
    bool isGasBlock(const VoxelOctree& world, int x, int y, int z) const;
    bool isSolidBlock(const VoxelOctree& world, int x, int y, int z) const;
    bool isFluidBlock(const VoxelOctree& world, int x, int y, int z) const;
    bool isFlammable(const MaterialProps& mp) const;
    bool isCorrodable(const MaterialProps& mp) const;
};

}
