#include "PhysicsEngine.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <climits>

namespace OpenMind {

PhysicsEngine::PhysicsEngine()
    : gravity(GRAVITY_EARTH)
    , timeScale(1.0f)
    , ambientTemperature(293.15f)
    , windX(0.0f), windY(0.0f), windZ(0.0f)
    , currentTick(0)
    , scanRange(64)
    , rng(std::random_device{}())
    , snapshotIndex(0)
    , snapshotCount(0)
    , currentWeather{}
    , stats{} {
    particles.resize(MAX_PARTICLES);
    initReactionMatrix();
    stats.timeScale = timeScale;

    Disease flu;
    flu.diseaseID = 1;
    flu.name = "Flu";
    flu.transmissionMode = TransmissionMode::PROXIMITY;
    flu.transmissionRange = 3.0f;
    flu.infectivity = 0.8f;
    flu.severity = 0.3f;
    flu.incubationPeriod = 100.0f;
    flu.symptomDuration = 300.0f;
    flu.mortalityRate = 0.1f;
    flu.immunityGain = 0.7f;
    flu.healthDrainPerTick = 0.05f;
    flu.hungerIncreasePerTick = 0.02f;
    flu.speedPenalty = 0.3f;
    diseases.push_back(flu);
}

void PhysicsEngine::initReactionMatrix() {
    reactionMatrix.push_back({"H2", "O2", 573.0f, 0.0f, "H2O", "", "", 286000.0f});
    reactionMatrix.push_back({"C8H18", "O2", 493.0f, 0.0f, "CO2", "H2O", "", 5470000.0f});
    reactionMatrix.push_back({"CH4", "O2", 810.0f, 0.0f, "CO2", "H2O", "", 890000.0f});
    reactionMatrix.push_back({"Fe", "O2", 1100.0f, 0.0f, "Fe2O3", "", "", -824000.0f});
    reactionMatrix.push_back({"Fe", "H2O", 293.0f, 0.0f, "Fe(OH)2", "", "", -286000.0f});
    reactionMatrix.push_back({"C", "O2", 573.0f, 0.0f, "CO2", "", "", 393500.0f});
    reactionMatrix.push_back({"Na", "H2O", 293.0f, 0.0f, "NaOH", "H2", "", -368000.0f});
}

void PhysicsEngine::setGravity(float g) { gravity = g; }
void PhysicsEngine::setTimeScale(float scale) { timeScale = std::max(0.0f, std::min(scale, 1000.0f)); stats.timeScale = timeScale; }
void PhysicsEngine::setTemperature(float ambient) { ambientTemperature = ambient; }
void PhysicsEngine::setWind(float wx, float wy, float wz) { windX = wx; windY = wy; windZ = wz; }
void PhysicsEngine::setScanRange(int range) { scanRange = std::max(1, std::min(range, 256)); }

float PhysicsEngine::getGravity() const { return gravity; }
float PhysicsEngine::getTimeScale() const { return timeScale; }
float PhysicsEngine::getAmbientTemperature() const { return ambientTemperature; }
uint64_t PhysicsEngine::getCurrentTick() const { return currentTick; }
const WorldStats& PhysicsEngine::getStats() const { return stats; }

const std::vector<Fragment>& PhysicsEngine::getPendingFragments() const { return pendingFragments; }
void PhysicsEngine::clearPendingFragments() { pendingFragments.clear(); }
std::vector<Particle>& PhysicsEngine::getParticles() { return particles; }
std::vector<Agent>& PhysicsEngine::getAgents() { return agents; }
const std::vector<Agent>& PhysicsEngine::getAgents() const { return agents; }

void PhysicsEngine::addAgent(const Agent& agent) {
    if (static_cast<int>(agents.size()) < MAX_AGENTS) {
        Agent a = agent;
        a.id = static_cast<int>(agents.size()) + 1;
        agents.push_back(a);
    }
}

void PhysicsEngine::removeAgent(int id) {
    agents.erase(std::remove_if(agents.begin(), agents.end(),
        [id](const Agent& a) { return a.id == id; }), agents.end());
}

void PhysicsEngine::addReaction(const Reaction& reaction) {
    reactionMatrix.push_back(reaction);
}

const Reaction* PhysicsEngine::findReaction(const std::string& compA, const std::string& compB) {
    for (const auto& rxn : reactionMatrix) {
        if ((compA == rxn.reactantA && compB == rxn.reactantB) ||
            (compA == rxn.reactantB && compB == rxn.reactantA)) {
            return &rxn;
        }
    }
    return nullptr;
}

void PhysicsEngine::tick(VoxelOctree& world, float deltaTime) {
    float dt = deltaTime * timeScale;

    tickPhysics(world, dt);
    tickThermodynamics(world, dt);
    tickFluids(world, dt);
    tickChemistry(world, dt);
    tickBiology(world, dt);
    tickAgents(world, dt);
    tickParticles(dt);
    tickSnapshots(world);

    currentTick++;
    updateStats(world);
}

bool PhysicsEngine::isWaterBlock(const VoxelOctree& world, int x, int y, int z) const {
    VoxelData data;
    if (!world.getBlock(x, y, z, data)) return false;
    return data.type == BlockType::WATER || (data.props.chemical.composition == "H2O" && data.state == BlockState::LIQUID);
}

bool PhysicsEngine::isGasBlock(const VoxelOctree& world, int x, int y, int z) const {
    VoxelData data;
    if (!world.getBlock(x, y, z, data)) return false;
    return data.type == BlockType::AIR && data.props.chemical.composition != "";
}

bool PhysicsEngine::isSolidBlock(const VoxelOctree& world, int x, int y, int z) const {
    VoxelData data;
    if (!world.getBlock(x, y, z, data)) return false;
    return data.type != BlockType::AIR && data.type != BlockType::WATER;
}

bool PhysicsEngine::isFluidBlock(const VoxelOctree& world, int x, int y, int z) const {
    VoxelData data;
    if (!world.getBlock(x, y, z, data)) return false;
    if (data.type == BlockType::WATER) return true;
    if (data.type == BlockType::AIR && !data.props.chemical.composition.empty()) return true;
    return false;
}

void PhysicsEngine::swapFluidBlocks(VoxelOctree& world, int x1, int y1, int z1, int x2, int y2, int z2) {
    VoxelData blockA, blockB;
    bool hasA = world.getBlock(x1, y1, z1, blockA);
    bool hasB = world.getBlock(x2, y2, z2, blockB);

    float tempA = hasA ? blockA.currentTemperature : ambientTemperature;
    float tempB = hasB ? blockB.currentTemperature : ambientTemperature;
    BlockState stateA = hasA ? blockA.state : BlockState::SOLID;
    BlockState stateB = hasB ? blockB.state : BlockState::SOLID;

    if (hasA && hasB) {
        world.setBlock(x2, y2, z2, blockA.type, blockA.props);
        world.setBlockTemperature(x2, y2, z2, tempA);
        world.setBlockState(x2, y2, z2, stateA);
        world.setBlock(x1, y1, z1, blockB.type, blockB.props);
        world.setBlockTemperature(x1, y1, z1, tempB);
        world.setBlockState(x1, y1, z1, stateB);
    } else if (hasA && !hasB) {
        world.setBlock(x2, y2, z2, blockA.type, blockA.props);
        world.setBlockTemperature(x2, y2, z2, tempA);
        world.setBlockState(x2, y2, z2, stateA);
        world.setBlock(x1, y1, z1, BlockType::AIR);
    } else if (!hasA && hasB) {
        world.setBlock(x1, y1, z1, blockB.type, blockB.props);
        world.setBlockTemperature(x1, y1, z1, tempB);
        world.setBlockState(x1, y1, z1, stateB);
        world.setBlock(x2, y2, z2, BlockType::AIR);
    }
}

void PhysicsEngine::applyConvection(VoxelOctree& world) {
    static constexpr float THERMAL_EXPANSION = 0.0002f;

    for (int x = 1; x < scanRange - 1; x++) {
        for (int z = 1; z < scanRange - 1; z++) {
            for (int y = scanRange - 2; y >= 1; y--) {
                VoxelData data;
                if (!world.getBlock(x, y, z, data)) continue;
                if (!isFluidBlock(world, x, y, z)) continue;

                float blockTemp = data.currentTemperature;
                float blockDensity = data.props.general.density;
                if (blockDensity < 0.001f) blockDensity = 1.0f;

                float effectiveDensity = blockDensity * (1.0f - THERMAL_EXPANSION * (blockTemp - ambientTemperature));

                VoxelData aboveData;
                bool hasAbove = world.getBlock(x, y + 1, z, aboveData);
                bool aboveIsFluid = hasAbove && (aboveData.type == BlockType::WATER ||
                    (aboveData.type == BlockType::AIR && !aboveData.props.chemical.composition.empty()));

                if (aboveIsFluid || !hasAbove) {
                    float aboveTemp = hasAbove ? aboveData.currentTemperature : ambientTemperature;
                    float aboveDensity = hasAbove ? aboveData.props.general.density : 0.0f;
                    if (aboveDensity < 0.001f && hasAbove) aboveDensity = 1.0f;

                    float aboveEffDensity = hasAbove ?
                        aboveDensity * (1.0f - THERMAL_EXPANSION * (aboveTemp - ambientTemperature)) : 0.0f;

                    if (blockTemp > aboveTemp || effectiveDensity < aboveEffDensity) {
                        float convectionForce = (blockTemp - aboveTemp) * 0.01f;
                        float gravityResist = gravity * 0.001f;
                        float netForce = convectionForce - gravityResist;

                        if (netForce > 0.0f) {
                            swapFluidBlocks(world, x, y, z, x, y + 1, z);
                            continue;
                        }
                    }
                }

                VoxelData belowData;
                bool hasBelow = world.getBlock(x, y - 1, z, belowData);
                bool belowIsFluid = hasBelow && (belowData.type == BlockType::WATER ||
                    (belowData.type == BlockType::AIR && !belowData.props.chemical.composition.empty()));

                if (belowIsFluid || !hasBelow) {
                    float belowTemp = hasBelow ? belowData.currentTemperature : ambientTemperature;
                    float belowDensity = hasBelow ? belowData.props.general.density : 0.0f;
                    if (belowDensity < 0.001f && hasBelow) belowDensity = 1.0f;

                    float belowEffDensity = hasBelow ?
                        belowDensity * (1.0f - THERMAL_EXPANSION * (belowTemp - ambientTemperature)) : 0.0f;

                    if (blockTemp < belowTemp || effectiveDensity > belowEffDensity) {
                        float convectionForce = (belowTemp - blockTemp) * 0.01f;
                        float gravityAssist = gravity * 0.001f;
                        float netForce = convectionForce + gravityAssist;

                        if (netForce > 0.0f) {
                            swapFluidBlocks(world, x, y, z, x, y - 1, z);
                        }
                    }
                }
            }
        }
    }
}

bool PhysicsEngine::isFlammable(const MaterialProps& mp) const {
    return mp.chemical.flammability > 0.0f;
}

bool PhysicsEngine::isCorrodable(const MaterialProps& mp) const {
    return mp.chemical.corrosionRate > 0.0f;
}

int PhysicsEngine::countExposedFaces(const VoxelOctree& world, int x, int y, int z) const {
    int count = 0;
    const int offsets[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    for (int i = 0; i < 6; i++) {
        if (!isSolidBlock(world, x + offsets[i][0], y + offsets[i][1], z + offsets[i][2])) {
            count++;
        }
    }
    return count;
}

int PhysicsEngine::getWaterHeight(const VoxelOctree& world, int x, int z) const {
    for (int y = scanRange - 1; y >= 0; y--) {
        if (isWaterBlock(world, x, y, z)) return y;
    }
    return -1;
}

float PhysicsEngine::calculateDrag(int exposedFaces, float velocity, float dragCoeff) const {
    float speed = std::abs(velocity);
    return 0.5f * AIR_DENSITY * speed * speed * dragCoeff * static_cast<float>(exposedFaces);
}

float PhysicsEngine::calculateStress(float force, float area) const {
    if (area <= 0.0f) return 0.0f;
    return force / area;
}

void PhysicsEngine::handleDeformation(PhysicsData& pd, float stress, float elasticLimit) {
    if (stress > elasticLimit && pd.isRigid) {
        pd.isRigid = false;
        pd.structuralIntegrity *= 0.95f;
        float bend = (stress - elasticLimit) * 0.001f;
        pd.offsetX += (std::uniform_real_distribution<float>(-bend, bend)(rng));
        pd.offsetY += (std::uniform_real_distribution<float>(-bend, bend)(rng));
        pd.offsetZ += (std::uniform_real_distribution<float>(-bend, bend)(rng));
    } else if (stress < elasticLimit * 0.5f && !pd.isRigid) {
        pd.offsetX *= 0.9f;
        pd.offsetY *= 0.9f;
        pd.offsetZ *= 0.9f;
        if (std::abs(pd.offsetX) < 0.001f && std::abs(pd.offsetY) < 0.001f && std::abs(pd.offsetZ) < 0.001f) {
            pd.isRigid = true;
        }
    }
}

void PhysicsEngine::breakBlock(VoxelOctree& world, int x, int y, int z, const MaterialProps& mp) {
    int fragCount = std::uniform_int_distribution<int>(5, 20)(rng);
    for (int i = 0; i < fragCount; i++) {
        Fragment frag;
        frag.x = x;
        frag.y = y;
        frag.z = z;
        frag.vx = std::uniform_real_distribution<float>(-3.0f, 3.0f)(rng);
        frag.vy = std::uniform_real_distribution<float>(1.0f, 5.0f)(rng);
        frag.vz = std::uniform_real_distribution<float>(-3.0f, 3.0f)(rng);
        frag.mass = mp.general.mass * std::uniform_real_distribution<float>(0.05f, 0.2f)(rng);
        frag.type = BlockType::DIRT;
        pendingFragments.push_back(frag);
    }
    world.setBlock(x, y, z, BlockType::AIR);
}

void PhysicsEngine::applyGravityToBlock(VoxelOctree& world, int x, int y, int z, PhysicsData& pd, const MaterialProps& mp, float dt) {
    if (mp.general.mass <= 0.0f) return;
    if (y <= 0) return;

    float weight = mp.general.mass * gravity;

    bool supportedBelow = isSolidBlock(world, x, y - 1, z);

    if (supportedBelow) {
        if (pd.velocityY > 0.5f) {
            float impactForce = pd.velocityY * mp.general.mass;
            float stress = calculateStress(impactForce, 1.0f);
            handleDeformation(pd, stress, mp.mechanical.tensileStrength * 0.3f);
            if (stress > mp.mechanical.tensileStrength) {
                breakBlock(world, x, y, z, mp);
                return;
            }
        }
        pd.velocityY = 0.0f;
        pd.offsetY = 0.0f;
        return;
    }

    int exposedFaces = countExposedFaces(world, x, y, z);
    float drag = calculateDrag(exposedFaces, pd.velocityY, 0.47f);
    float netForce = weight - drag;
    float acceleration = netForce / mp.general.mass;

    pd.velocityY += acceleration * dt;

    float newY = pd.offsetY + pd.velocityY * dt;
    int targetY = y + static_cast<int>(std::floor(newY));

    if (targetY < y && !isSolidBlock(world, x, targetY, z)) {
        pd.offsetY = newY - std::floor(newY);
        if (pd.offsetY <= -1.0f) {
            pd.offsetY = 0.0f;
            pd.velocityY = 0.0f;
            VoxelData blockData;
            if (world.getBlock(x, y, z, blockData)) {
                world.setBlock(x, targetY, z, blockData.type, blockData.props);
                world.setBlockTemperature(x, targetY, z, blockData.currentTemperature);
                world.setBlockState(x, targetY, z, blockData.state);
                world.setBlockDensity(x, targetY, z, blockData.props.general.density);
                world.setBlock(x, y, z, BlockType::AIR);
            }
        }
    } else if (isSolidBlock(world, x, targetY, z)) {
        float impactForce = std::abs(pd.velocityY) * mp.general.mass;
        float stress = calculateStress(impactForce, 1.0f);
        handleDeformation(pd, stress, mp.mechanical.tensileStrength * 0.3f);
        if (stress > mp.mechanical.tensileStrength) {
            breakBlock(world, x, y, z, mp);
            return;
        }
        pd.velocityY *= -0.2f * mp.general.elasticity;
        pd.offsetY = 0.0f;
        if (std::abs(pd.velocityY) < 0.1f) pd.velocityY = 0.0f;
    }
}

void PhysicsEngine::tickPhysics(VoxelOctree& world, float dt) {
    currentTime += dt;
    if (currentTime >= cycleDuration) {
        currentTime -= cycleDuration;
    }

    updateWeather(world, dt);

    for (int x = 0; x < scanRange; x++) {
        for (int z = 0; z < scanRange; z++) {
            for (int y = scanRange - 1; y >= 0; y--) {
                VoxelData data;
                if (!world.getBlock(x, y, z, data)) continue;
                if (data.type == BlockType::AIR) continue;

                if (data.props.thermal.heatOutput > 0.0f) {
                    world.setBlockTemperature(x, y, z, data.currentTemperature + data.props.thermal.heatOutput * dt);
                }

                PhysicsData& pd = physicsDataMap[posHash(x, y, z)];
                if (pd.temperature == 0.0f) pd.temperature = data.props.thermal.meltingPoint * 0.5f;
                applyGravityToBlock(world, x, y, z, pd, data.props, dt);
            }
        }
    }
}

void PhysicsEngine::transferHeat(VoxelOctree& world, float dt) {
    const int offsets[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};

    for (int x = 1; x < scanRange - 1; x++) {
        for (int y = 1; y < scanRange - 1; y++) {
            for (int z = 1; z < scanRange - 1; z++) {
                VoxelData data;
                if (!world.getBlock(x, y, z, data)) continue;
                if (data.type == BlockType::AIR) continue;

                float blockTemp = data.currentTemperature;

                for (int i = 0; i < 6; i++) {
                    int nx = x + offsets[i][0];
                    int ny = y + offsets[i][1];
                    int nz = z + offsets[i][2];

                    VoxelData neighbor;
                    if (!world.getBlock(nx, ny, nz, neighbor)) continue;
                    if (neighbor.type == BlockType::AIR) continue;

                    float neighborTemp = neighbor.currentTemperature;
                    float deltaT = blockTemp - neighborTemp;

                    if (std::abs(deltaT) < 0.001f) continue;

                    float k = data.props.thermal.thermalConductivity;
                    float Q = k * deltaT * dt;

                    float thermalMassHot = data.props.general.mass * data.props.thermal.specificHeat;
                    float thermalMassCold = neighbor.props.general.mass * neighbor.props.thermal.specificHeat;
                    if (thermalMassHot < 0.001f) thermalMassHot = 0.001f;
                    if (thermalMassCold < 0.001f) thermalMassCold = 0.001f;

                    float dT_hot = Q / thermalMassHot;
                    float maxDT = 0.1f * std::abs(deltaT);
                    if (std::abs(dT_hot) > maxDT) {
                        Q *= maxDT / std::abs(dT_hot);
                        dT_hot = (deltaT > 0) ? maxDT : -maxDT;
                    }

                    float dT_cold = Q / thermalMassCold;

                    blockTemp -= dT_hot;
                    world.setBlockTemperature(nx, ny, nz, neighborTemp + dT_cold);
                }

                world.setBlockTemperature(x, y, z, blockTemp);
            }
        }
    }
}

void PhysicsEngine::handlePhaseChange(VoxelOctree& world, int x, int y, int z, PhysicsData& pd, const MaterialProps& mp) {
    VoxelData data;
    if (!world.getBlock(x, y, z, data)) return;
    if (data.type == BlockType::AIR) return;

    float temp = data.currentTemperature;
    if (temp <= 0.0f && data.state == BlockState::LIQUID) {
        world.setBlockState(x, y, z, BlockState::SOLID);
    } else if (temp >= mp.thermal.boilingPoint && data.state == BlockState::LIQUID) {
        world.setBlock(x, y, z, BlockType::AIR);
    }
}

void PhysicsEngine::handleMelting(VoxelOctree& world, int x, int y, int z) {
    VoxelData data;
    if (!world.getBlock(x, y, z, data)) return;

    if (data.type == BlockType::AIR) return;
    if (isBurning(x, y, z)) return;

    float temp = data.currentTemperature;
    float meltThreshold = data.props.thermal.meltingPoint + hysteresisMargin;

    if (data.state == BlockState::SOLID && temp >= meltThreshold) {
        float latentHeat = data.props.thermal.latentHeatOfFusion;
        float thermalMass = data.props.general.mass * data.props.thermal.specificHeat;

        if (thermalMass > 0.001f && latentHeat > 0.0f) {
            float energyToMelt = latentHeat * data.props.general.mass;
            float tempReduction = energyToMelt / thermalMass;
            temp -= tempReduction;
            if (temp < data.props.thermal.meltingPoint) {
                temp = data.props.thermal.meltingPoint;
            }
        }

        world.setBlockState(x, y, z, BlockState::LIQUID);
        world.setBlockTemperature(x, y, z, temp);
        world.setBlockDensity(x, y, z, data.props.general.density * data.props.thermal.liquidDensityFactor);
    }
}

void PhysicsEngine::handleFreezing(VoxelOctree& world, int x, int y, int z) {
    VoxelData data;
    if (!world.getBlock(x, y, z, data)) return;

    if (data.type == BlockType::AIR) return;

    float fp = data.props.thermal.freezingPoint;
    if (fp <= 0.0f) fp = data.props.thermal.meltingPoint;

    float temp = data.currentTemperature;
    float freezeThreshold = fp - hysteresisMargin;

    if (data.state == BlockState::LIQUID && temp <= freezeThreshold) {
        float latentHeat = data.props.thermal.latentHeatOfFusion;
        float thermalMass = data.props.general.mass * data.props.thermal.specificHeat;

        if (thermalMass > 0.001f && latentHeat > 0.0f) {
            float energyToFreeze = latentHeat * data.props.general.mass;
            float tempIncrease = energyToFreeze / thermalMass;
            temp += tempIncrease;
            if (temp > fp) {
                temp = fp;
            }
        }

        float solidDensity = data.props.general.density;
        if (data.props.thermal.liquidDensityFactor > 0.001f) {
            solidDensity = data.props.general.density / data.props.thermal.liquidDensityFactor;
        }

        world.setBlockState(x, y, z, BlockState::SOLID);
        world.setBlockTemperature(x, y, z, temp);
        world.setBlockDensity(x, y, z, solidDensity);
    }
}

void PhysicsEngine::handleBoiling(VoxelOctree& world, int x, int y, int z) {
    VoxelData data;
    if (!world.getBlock(x, y, z, data)) return;

    if (data.type == BlockType::AIR) return;
    if (isBurning(x, y, z)) return;

    float temp = data.currentTemperature;
    float boilThreshold = data.props.thermal.boilingPoint + hysteresisMargin;

    if (data.state == BlockState::LIQUID && temp >= boilThreshold) {
        float latentHeat = data.props.thermal.latentHeatOfVaporization;
        float thermalMass = data.props.general.mass * data.props.thermal.specificHeat;

        if (thermalMass > 0.001f && latentHeat > 0.0f) {
            float energyToBoil = latentHeat * data.props.general.mass;
            float tempReduction = energyToBoil / thermalMass;
            temp -= tempReduction;
            if (temp < data.props.thermal.boilingPoint) {
                temp = data.props.thermal.boilingPoint;
            }
        }

        MaterialProps gasProps = data.props;
        float gasDensity = data.props.general.density * data.props.thermal.gasDensityFactor;
        if (gasDensity < 0.001f) gasDensity = 0.1f;
        gasProps.general.density = gasDensity;

        world.setBlockState(x, y, z, BlockState::GAS);
        world.setBlockTemperature(x, y, z, temp);
        world.setBlockDensity(x, y, z, gasDensity);

        expandGas(world, x, y, z, gasProps);
    }
}

void PhysicsEngine::expandGas(VoxelOctree& world, int x, int y, int z, const MaterialProps& gasProps) {
    const int offsets[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};

    int spawned = 0;
    int maxSpawn = 3;

    for (int i = 0; i < 6 && spawned < maxSpawn; i++) {
        int nx = x + offsets[i][0];
        int ny = y + offsets[i][1];
        int nz = z + offsets[i][2];

        if (nx < 0 || nx >= scanRange || ny < 0 || ny >= scanRange || nz < 0 || nz >= scanRange) continue;

        VoxelData neighbor;
        if (world.getBlock(nx, ny, nz, neighbor)) {
            if (neighbor.type != BlockType::AIR) continue;
        }

        world.setBlock(nx, ny, nz, BlockType::AIR, gasProps);
        world.setBlockState(nx, ny, nz, BlockState::GAS);
        world.setBlockTemperature(nx, ny, nz, ambientTemperature);
        world.setBlockDensity(nx, ny, nz, gasProps.general.density);
        spawned++;
    }
}

void PhysicsEngine::handleCondensation(VoxelOctree& world, int x, int y, int z) {
    VoxelData data;
    if (!world.getBlock(x, y, z, data)) return;

    if (data.type == BlockType::AIR) return;

    float temp = data.currentTemperature;
    float condenseThreshold = data.props.thermal.condensationPoint - hysteresisMargin;

    if (condenseThreshold <= 0.0f) {
        condenseThreshold = data.props.thermal.boilingPoint - hysteresisMargin;
    }

    if (data.state == BlockState::GAS && temp <= condenseThreshold) {
        float latentHeat = data.props.thermal.latentHeatOfVaporization;
        float thermalMass = data.props.general.mass * data.props.thermal.specificHeat;

        if (thermalMass > 0.001f && latentHeat > 0.0f) {
            float energyToCondense = latentHeat * data.props.general.mass;
            float tempIncrease = energyToCondense / thermalMass;
            temp += tempIncrease;
            if (temp > data.props.thermal.boilingPoint) {
                temp = data.props.thermal.boilingPoint;
            }
        }

        MaterialProps liquidProps = data.props;
        float liquidDensity = data.props.general.density;
        if (data.props.thermal.gasDensityFactor >= 0.001f) {
            liquidDensity = data.props.general.density / data.props.thermal.gasDensityFactor;
        }
        liquidProps.general.density = liquidDensity;

        world.setBlockState(x, y, z, BlockState::LIQUID);
        world.setBlockTemperature(x, y, z, temp);
        world.setBlockDensity(x, y, z, liquidDensity);

        removeAdjacentGas(world, x, y, z);
    }
}

void PhysicsEngine::removeAdjacentGas(VoxelOctree& world, int x, int y, int z) {
    const int offsets[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};

    int removed = 0;
    int maxRemove = 2;

    for (int i = 0; i < 6 && removed < maxRemove; i++) {
        int nx = x + offsets[i][0];
        int ny = y + offsets[i][1];
        int nz = z + offsets[i][2];

        if (nx < 0 || nx >= scanRange || ny < 0 || ny >= scanRange || nz < 0 || nz >= scanRange) continue;

        VoxelData neighbor;
        if (!world.getBlock(nx, ny, nz, neighbor)) continue;

        if (neighbor.type == BlockType::AIR && neighbor.state == BlockState::GAS) {
            VoxelData nd;
            if (world.getBlock(nx, ny, nz, nd)) {
                if (!nd.props.chemical.composition.empty()) {
                    world.setBlock(nx, ny, nz, BlockType::AIR);
                    removed++;
                }
            }
        }
    }
}

void PhysicsEngine::emitRadiation(VoxelOctree& world, int x, int y, int z, const PhysicsData& pd, float dt) {
    VoxelData data;
    if (!world.getBlock(x, y, z, data)) return;

    float temp = data.currentTemperature;
    float emissionThreshold = 600.0f;

    if (temp < emissionThreshold) return;

    float emissivity = data.props.thermal.emissivity;
    if (emissivity <= 0.0f) return;

    float radiationPower = emissivity * STEFAN_BOLTZMANN * temp * temp * temp * temp;

    float linearPower = (temp - emissionThreshold) * emissivity * 0.5f;

    float totalPower = radiationPower * 0.0001f + linearPower;
    totalPower *= dt;

    const int directions[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};

    for (int dir = 0; dir < 6; dir++) {
        for (int dist = 1; dist <= 10; dist++) {
            int nx = x + directions[dir][0] * dist;
            int ny = y + directions[dir][1] * dist;
            int nz = z + directions[dir][2] * dist;

            if (nx < 0 || nx >= scanRange || ny < 0 || ny >= scanRange || nz < 0 || nz >= scanRange) break;

            float intensity = totalPower / (static_cast<float>(dist * dist) + 1.0f);

            if (intensity < 0.001f) break;

            uint64_t key = hashCoords(nx, ny, nz);
            radiationMap[key] += intensity;

            VoxelData blockData;
            if (world.getBlock(nx, ny, nz, blockData)) {
                if (blockData.type != BlockType::AIR) {
                    absorbRadiation(world, nx, ny, nz, intensity, dt);
                    break;
                }
            }
        }
    }
}

void PhysicsEngine::propagateRadiation(VoxelOctree& world, float dt) {
    for (int x = 0; x < scanRange; x++) {
        for (int y = 0; y < scanRange; y++) {
            for (int z = 0; z < scanRange; z++) {
                VoxelData data;
                if (!world.getBlock(x, y, z, data)) continue;
                if (data.type == BlockType::AIR) continue;
                float emission = data.props.thermal.radiationAbsorption;
                if (emission <= 0.0f) continue;
                float intensity = emission * data.currentTemperature * 0.0001f;
                const int offsets[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
                for (int i = 0; i < 6; i++) {
                    int nx = x + offsets[i][0], ny = y + offsets[i][1], nz = z + offsets[i][2];
                    if (nx < 0 || nx >= scanRange || ny < 0 || ny >= scanRange || nz < 0 || nz >= scanRange) continue;
                    absorbRadiation(world, nx, ny, nz, intensity * 0.25f, dt);
                }
            }
        }
    }
}

void PhysicsEngine::absorbRadiation(VoxelOctree& world, int x, int y, int z, float intensity, float dt) {
    VoxelData data;
    if (!world.getBlock(x, y, z, data)) return;

    if (data.type == BlockType::AIR) return;

    float absorption = data.props.thermal.radiationAbsorption;
    if (absorption <= 0.0f) return;

    float absorbedEnergy = intensity * absorption;

    float thermalMass = data.props.general.mass * data.props.thermal.specificHeat;
    if (thermalMass < 0.001f) thermalMass = 0.001f;

    float tempIncrease = absorbedEnergy / thermalMass;

    float maxTempIncrease = 10.0f * dt;
    if (tempIncrease > maxTempIncrease) {
        tempIncrease = maxTempIncrease;
    }

    float currentTemp = data.currentTemperature;
    float newTemp = currentTemp + tempIncrease;

    world.setBlockTemperature(x, y, z, newTemp);
}

void PhysicsEngine::tickThermodynamics(VoxelOctree& world, float dt) {
    radiationMap.clear();

    for (int x = 0; x < scanRange; x++) {
        for (int y = 0; y < scanRange; y++) {
            for (int z = 0; z < scanRange; z++) {
                VoxelData data;
                if (!world.getBlock(x, y, z, data)) continue;
                if (data.type == BlockType::AIR) continue;

                PhysicsData& pd = physicsDataMap[posHash(x, y, z)];
                if (pd.state == BlockState{}) {
                    pd.state = BlockState::SOLID;
                    if (data.type == BlockType::WATER) pd.state = BlockState::LIQUID;
                }
                pd.isOnFire = false;

                emitRadiation(world, x, y, z, pd, dt);
            }
        }
    }

    transferHeat(world, dt);
    applyConvection(world);

    for (int x = 0; x < scanRange; x++) {
        for (int y = 0; y < scanRange; y++) {
            for (int z = 0; z < scanRange; z++) {
                VoxelData data;
                if (!world.getBlock(x, y, z, data)) continue;
                if (data.type == BlockType::AIR) continue;

                float temp = data.currentTemperature;

                if (data.props.thermal.heatOutput > 0.0f) {
                    world.setBlockTemperature(x, y, z, data.currentTemperature + data.props.thermal.heatOutput * dt);
                } else {
                    float coolingRate = 0.01f;
                    float deltaT = temp - ambientTemperature;
                    if (std::abs(deltaT) > 0.001f) {
                        temp -= deltaT * coolingRate * dt;
                        world.setBlockTemperature(x, y, z, temp);
                    }
                }

                PhysicsData& pd = physicsDataMap[posHash(x, y, z)];
                if (pd.state == BlockState{}) {
                    pd.state = BlockState::SOLID;
                    if (data.type == BlockType::WATER) pd.state = BlockState::LIQUID;
                }
                pd.isOnFire = false;

                handleMelting(world, x, y, z);
                handleFreezing(world, x, y, z);
                handleBoiling(world, x, y, z);
                handleCondensation(world, x, y, z);

                VoxelData freshData;
                if (world.getBlock(x, y, z, freshData)) {
                    PhysicsData& pd2 = physicsDataMap[posHash(x, y, z)];
                    pd2.state = freshData.state;
                    handlePhaseChange(world, x, y, z, pd2, freshData.props);
                }
            }
        }
    }
}

void PhysicsEngine::simulateWaterFlow(VoxelOctree& world, float dt) {
    for (int x = 0; x < scanRange; x++) {
        for (int z = 0; z < scanRange; z++) {
            for (int y = scanRange - 2; y >= 1; y--) {
                if (!isWaterBlock(world, x, y, z)) continue;

                if (y > 0 && !isSolidBlock(world, x, y - 1, z) && !isWaterBlock(world, x, y - 1, z)) {
                    VoxelData wData;
                    world.getBlock(x, y, z, wData);
                    world.setBlock(x, y - 1, z, BlockType::WATER, wData.props);
                    world.setBlockTemperature(x, y - 1, z, wData.currentTemperature);
                    world.setBlockState(x, y - 1, z, wData.state);
                    world.setBlockDensity(x, y - 1, z, wData.props.general.density);
                    world.setBlock(x, y, z, BlockType::AIR);
                    continue;
                }

                if (isSolidBlock(world, x, y - 1, z) || isWaterBlock(world, x, y - 1, z)) {
                    bool flowed = false;
                    int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
                    for (int d = 0; d < 4 && !flowed; d++) {
                        int nx = x + dirs[d][0];
                        int nz = z + dirs[d][1];
                        if (nx >= 0 && nx < scanRange && nz >= 0 && nz < scanRange) {
                            if (!isSolidBlock(world, nx, y, nz) && !isWaterBlock(world, nx, y, nz)) {
                                VoxelData wData;
                                world.getBlock(x, y, z, wData);
                                world.setBlock(nx, y, nz, BlockType::WATER, wData.props);
                                world.setBlockTemperature(nx, y, nz, wData.currentTemperature);
                                world.setBlockState(nx, y, nz, wData.state);
                                world.setBlockDensity(nx, y, nz, wData.props.general.density);
                                world.setBlock(x, y, z, BlockType::AIR);
                                flowed = true;
                            }
                        }
                    }
                }
            }
        }
    }
}

void PhysicsEngine::simulateGasDiffusion(VoxelOctree& world, float dt) {
    for (int x = 0; x < scanRange; x++) {
        for (int y = scanRange - 1; y >= 0; y--) {
            for (int z = 0; z < scanRange; z++) {
                if (!isGasBlock(world, x, y, z)) continue;

                int dirs[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
                for (int d = 0; d < 6; d++) {
                    int nx = x + dirs[d][0];
                    int ny = y + dirs[d][1];
                    int nz = z + dirs[d][2];
                    if (nx >= 0 && nx < scanRange && ny >= 0 && ny < scanRange && nz >= 0 && nz < scanRange) {
                        if (!isSolidBlock(world, nx, ny, nz) && !isGasBlock(world, nx, ny, nz)) {
                            if (std::uniform_real_distribution<float>(0.0f, 1.0f)(rng) < 0.3f) {
                                VoxelData gData;
                                world.getBlock(x, y, z, gData);
                                world.setBlock(nx, ny, nz, gData.type, gData.props);
                                world.setBlockTemperature(nx, ny, nz, gData.currentTemperature);
                                world.setBlockState(nx, ny, nz, gData.state);
                                world.setBlockDensity(nx, ny, nz, gData.props.general.density);
                                world.setBlock(x, y, z, BlockType::AIR);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

float PhysicsEngine::getBuoyancyForce(float blockDensity, float fluidDensity) const {
    return (fluidDensity - blockDensity) * gravity;
}

void PhysicsEngine::tickFluids(VoxelOctree& world, float dt) {
    simulateWaterFlow(world, dt);
    simulateGasDiffusion(world, dt);

    for (int x = 0; x < scanRange; x++) {
        for (int y = 0; y < scanRange; y++) {
            for (int z = 0; z < scanRange; z++) {
                VoxelData data;
                if (!world.getBlock(x, y, z, data)) continue;
                if (data.type == BlockType::AIR) continue;

                int waterY = getWaterHeight(world, x, z);
                if (waterY > y) {
                    float depth = static_cast<float>(waterY - y);
                    (void)depth;
                }
            }
        }
    }
}

void PhysicsEngine::checkReactions(VoxelOctree& world, int x, int y, int z) {
    VoxelData data;
    if (!world.getBlock(x, y, z, data)) return;

    if (data.props.chemical.composition.empty()) return;
    if (isBurning(x, y, z)) return;

    const int offsets[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};

    for (int i = 0; i < 6; i++) {
        int nx = x + offsets[i][0];
        int ny = y + offsets[i][1];
        int nz = z + offsets[i][2];

        if (isBurning(nx, ny, nz)) continue;

        VoxelData neighbor;
        if (!world.getBlock(nx, ny, nz, neighbor)) continue;

        if (neighbor.props.chemical.composition.empty()) continue;

        const Reaction* rxn = findReaction(data.props.chemical.composition, neighbor.props.chemical.composition);
        if (rxn == nullptr) continue;

        float blockTemp = data.currentTemperature;
        float neighborTemp = neighbor.currentTemperature;
        float avgTemp = (blockTemp + neighborTemp) * 0.5f;

        if (avgTemp < rxn->temperatureThreshold) continue;

        float totalEnergy = rxn->energyReleased;
        if (totalEnergy > 0) {
            float reactionTemp = avgTemp + totalEnergy * 0.00001f;

            auto makeProductProps = [&](const std::string& comp, float mass, float density) {
                MaterialProps prod;
                prod.chemical.composition = comp;
                prod.thermal.specificHeat = 4186.0f;
                prod.thermal.thermalConductivity = 0.6f;
                prod.thermal.meltingPoint = 273.15f;
                prod.thermal.boilingPoint = 373.15f;
                prod.general.mass = mass;
                prod.general.density = density;
                prod.general.hardness = 3.0f;
                prod.general.elasticity = 0.1f;
                prod.mechanical.tensileStrength = 1000.0f;
                prod.mechanical.compressiveStrength = 1000.0f;
                prod.mechanical.shearStrength = 500.0f;
                prod.mechanical.fractureToughness = 100.0f;
                prod.health.maxHealth = 100.0f;
                prod.health.currentHealth = 100.0f;
                prod.visual.baseColor = "#4682B4";
                return prod;
            };

            if (!rxn->productA.empty()) {
                MaterialProps prodA = makeProductProps(rxn->productA, 1.0f, 1000.0f);
                world.setBlock(x, y, z, BlockType::CUSTOM, prodA);
                world.setBlockTemperature(x, y, z, reactionTemp);
                world.setBlockState(x, y, z, BlockState::SOLID);
            }

            if (!rxn->productB.empty()) {
                MaterialProps prodB = makeProductProps(rxn->productB, 1.0f, 1000.0f);
                world.setBlock(nx, ny, nz, BlockType::CUSTOM, prodB);
                world.setBlockTemperature(nx, ny, nz, reactionTemp);
                world.setBlockState(nx, ny, nz, BlockState::SOLID);
            }

            if (!rxn->byproduct.empty()) {
                MaterialProps byprod = makeProductProps(rxn->byproduct, 0.5f, 500.0f);
                int bx = x + std::uniform_int_distribution<int>(-1, 1)(rng);
                int bz = z + std::uniform_int_distribution<int>(-1, 1)(rng);
                int by = y + 1;
                if (bx >= 0 && bx < scanRange && bz >= 0 && bz < scanRange && by >= 0 && by < scanRange) {
                    world.setBlock(bx, by, bz, BlockType::CUSTOM, byprod);
                    world.setBlockTemperature(bx, by, bz, reactionTemp);
                }
            }

            float tempIncrease = totalEnergy * 0.00001f;
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dz = -1; dz <= 1; dz++) {
                        int tx = x + dx;
                        int ty = y + dy;
                        int tz = z + dz;
                        VoxelData td;
                        if (world.getBlock(tx, ty, tz, td) && td.type != BlockType::AIR) {
                            world.setBlockTemperature(tx, ty, tz, td.currentTemperature + tempIncrease);
                        }
                    }
                }
            }

            break;
        }
    }
}

uint64_t PhysicsEngine::posHash(int x, int y, int z) const {
    uint64_t h = static_cast<uint64_t>(static_cast<uint32_t>(x)) * 73856093u;
    h ^= static_cast<uint64_t>(static_cast<uint32_t>(y)) * 19349663u;
    h ^= static_cast<uint64_t>(static_cast<uint32_t>(z)) * 83492791u;
    return h;
}

bool PhysicsEngine::isBurning(int x, int y, int z) const {
    auto it = burnTimers.find(posHash(x, y, z));
    return it != burnTimers.end() && it->second > 0.0f;
}

void PhysicsEngine::extinguishBlock(int x, int y, int z) {
    burnTimers.erase(posHash(x, y, z));
}

bool PhysicsEngine::hasAdjacentOxidizer(const VoxelOctree& world, int x, int y, int z) const {
    const int offsets[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    for (int i = 0; i < 6; i++) {
        int nx = x + offsets[i][0];
        int ny = y + offsets[i][1];
        int nz = z + offsets[i][2];
        VoxelData nd;
        if (world.getBlock(nx, ny, nz, nd) && nd.type != BlockType::AIR) {
            if (nd.props.chemical.composition == "O2") return true;
        }
    }
    return false;
}

void PhysicsEngine::processCombustion(VoxelOctree& world, int x, int y, int z, float dt) {
    uint64_t key = posHash(x, y, z);
    auto it = burnTimers.find(key);
    if (it == burnTimers.end() || it->second <= 0.0f) return;

    VoxelData data;
    if (!world.getBlock(x, y, z, data)) return;
    if (data.type == BlockType::AIR) { burnTimers.erase(it); return; }

    float flammability = data.props.chemical.flammability;
    float combustionPoint = data.props.chemical.combustionPoint;

    if (!hasAdjacentOxidizer(world, x, y, z)) {
        extinguishBlock(x, y, z);
        return;
    }

    float consumeRate = flammability * 1.0f;
    it->second -= dt * consumeRate;

    world.setBlockTemperature(x, y, z, std::max(data.currentTemperature, combustionPoint));

    float heatEmission = flammability * 50.0f * dt;
    const int offsets[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    for (int i = 0; i < 6; i++) {
        int nx = x + offsets[i][0];
        int ny = y + offsets[i][1];
        int nz = z + offsets[i][2];
        VoxelData nd;
        if (world.getBlock(nx, ny, nz, nd) && nd.type != BlockType::AIR) {
            world.setBlockTemperature(nx, ny, nz, nd.currentTemperature + heatEmission);
        }
    }

    float newHealth = data.props.health.currentHealth - consumeRate * dt * 10.0f;
    if (newHealth <= 0.0f || it->second <= 0.0f) {
        MaterialProps ashProps;
        ashProps.general.mass = 0.05f;
        ashProps.general.density = 200.0f;
        ashProps.thermal.specificHeat = 800.0f;
        ashProps.thermal.thermalConductivity = 0.1f;
        ashProps.chemical.composition = "Ash";
        ashProps.health.maxHealth = 10.0f;
        ashProps.health.currentHealth = 10.0f;
        ashProps.visual.baseColor = "#333333";
        float remainingTemp = data.currentTemperature;
        world.setBlock(x, y, z, BlockType::ASH, ashProps);
        world.setBlockTemperature(x, y, z, remainingTemp);
        burnTimers.erase(it);
    } else {
        MaterialProps updatedProps = data.props;
        updatedProps.health.currentHealth = newHealth;
        float currentTemp = data.currentTemperature;
        world.setBlock(x, y, z, data.type, updatedProps);
        world.setBlockTemperature(x, y, z, currentTemp);
    }
}

void PhysicsEngine::spreadFire(VoxelOctree& world, int x, int y, int z, float dt) {
    uint64_t key = posHash(x, y, z);
    auto it = burnTimers.find(key);
    if (it == burnTimers.end() || it->second <= 0.0f) return;

    VoxelData data;
    if (!world.getBlock(x, y, z, data)) return;

    float fireIntensity = data.props.chemical.flammability;
    float spreadChance = fireIntensity * 0.1f * dt;

    const int offsets[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    for (int i = 0; i < 6; i++) {
        int nx = x + offsets[i][0];
        int ny = y + offsets[i][1];
        int nz = z + offsets[i][2];

        VoxelData nd;
        if (!world.getBlock(nx, ny, nz, nd)) continue;
        if (nd.type == BlockType::AIR) continue;
        if (!isFlammable(nd.props)) continue;
        if (isBurning(nx, ny, nz)) continue;

        float currentTemp = nd.currentTemperature;
        float heatTransfer = fireIntensity * 30.0f * dt;
        float newTemp = currentTemp + heatTransfer;
        world.setBlockTemperature(nx, ny, nz, newTemp);

        if (newTemp >= nd.props.chemical.combustionPoint && hasAdjacentOxidizer(world, nx, ny, nz)) {
            float burnDuration = nd.props.chemical.flammability * 10.0f;
            burnTimers[posHash(nx, ny, nz)] = burnDuration;
        }
    }
}

void PhysicsEngine::igniteBlock(VoxelOctree& world, int x, int y, int z, PhysicsData& pd, const MaterialProps& mp) {
    if (mp.chemical.flammability <= 0.0f) return;
    float temp = ambientTemperature;
    VoxelData vd;
    if (world.getBlock(x, y, z, vd)) {
        temp = vd.currentTemperature;
    }
    if (temp >= mp.chemical.combustionPoint) {
        if (!hasAdjacentOxidizer(world, x, y, z)) return;
        pd.isOnFire = true;
        pd.fireTimer = mp.chemical.flammability * 10.0f;
        uint64_t key = posHash(x, y, z);
        if (burnTimers.find(key) == burnTimers.end()) {
            burnTimers[key] = pd.fireTimer;
        }
    }
}

void PhysicsEngine::corrodeBlock(VoxelOctree& world, int x, int y, int z, PhysicsData& pd, const MaterialProps& mp, float dt) {
}

bool PhysicsEngine::hasAdjacentWater(const VoxelOctree& world, int x, int y, int z) const {
    const int offsets[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    for (int i = 0; i < 6; i++) {
        int nx = x + offsets[i][0];
        int ny = y + offsets[i][1];
        int nz = z + offsets[i][2];
        VoxelData nd;
        if (world.getBlock(nx, ny, nz, nd) && nd.type != BlockType::AIR) {
            if (nd.props.chemical.composition == "H2O" && nd.state == BlockState::LIQUID) return true;
        }
    }
    return false;
}

bool PhysicsEngine::hasAdjacentOxygen(const VoxelOctree& world, int x, int y, int z) const {
    const int offsets[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    for (int i = 0; i < 6; i++) {
        int nx = x + offsets[i][0];
        int ny = y + offsets[i][1];
        int nz = z + offsets[i][2];
        VoxelData nd;
        if (world.getBlock(nx, ny, nz, nd) && nd.type != BlockType::AIR) {
            if (nd.props.chemical.composition == "O2") return true;
        }
    }
    return false;
}

void PhysicsEngine::processCorrosion(VoxelOctree& world, int x, int y, int z, float dt) {
    VoxelData data;
    if (!world.getBlock(x, y, z, data)) return;
    if (data.type == BlockType::AIR) return;
    if (!isCorrodable(data.props)) return;
    if (data.props.chemical.corrosionRate <= 0.0f) return;

    uint64_t key = posHash(x, y, z);
    bool nearWater = hasAdjacentWater(world, x, y, z);
    bool nearOxygen = hasAdjacentOxygen(world, x, y, z);

    if (!nearWater || !nearOxygen) {
        corrosionTimers.erase(key);
        return;
    }

    auto it = corrosionTimers.find(key);
    if (it == corrosionTimers.end()) {
        corrosionTimers[key] = 0.0f;
        it = corrosionTimers.find(key);
    }

    float corrosionThreshold = 1.0f / data.props.chemical.corrosionRate;
    it->second += dt;

    float currentHealth = data.props.health.currentHealth - data.props.chemical.corrosionRate * dt * 0.1f;
    if (currentHealth <= 0.0f || it->second >= corrosionThreshold) {
        std::string composition = data.props.chemical.composition;
        std::string rustComp;
        std::string rustColor;

        if (composition == "Fe" || composition == "Fe-C alloy") {
            rustComp = "Fe2O3";
            rustColor = "#8B4513";
        } else if (composition == "Cu") {
            rustComp = "CuCO3";
            rustColor = "#2E8B57";
        } else {
            rustComp = composition + "_oxide";
            rustColor = "#696969";
        }

        MaterialProps rustProps;
        rustProps.general.mass = data.props.general.mass * 0.7f;
        rustProps.general.density = data.props.general.density * 0.6f;
        rustProps.general.hardness = data.props.general.hardness * 0.3f;
        rustProps.general.elasticity = data.props.general.elasticity * 0.5f;
        rustProps.mechanical.tensileStrength = data.props.mechanical.tensileStrength * 0.4f;
        rustProps.mechanical.compressiveStrength = data.props.mechanical.compressiveStrength * 0.3f;
        rustProps.mechanical.shearStrength = data.props.mechanical.shearStrength * 0.3f;
        rustProps.mechanical.fractureToughness = data.props.mechanical.fractureToughness * 0.5f;
        rustProps.thermal.thermalConductivity = data.props.thermal.thermalConductivity * 0.3f;
        rustProps.thermal.specificHeat = data.props.thermal.specificHeat * 1.2f;
        rustProps.thermal.meltingPoint = 1800.0f;
        rustProps.thermal.boilingPoint = 3000.0f;
        rustProps.chemical.composition = rustComp;
        rustProps.chemical.corrosionRate = 0.0f;
        rustProps.chemical.chemicalResistance = 0.9f;
        rustProps.health.maxHealth = data.props.health.maxHealth * 0.5f;
        rustProps.health.currentHealth = data.props.health.maxHealth * 0.5f;
        rustProps.visual.baseColor = rustColor;
        rustProps.visual.roughness = 0.8f;
        rustProps.visual.metallicness = 0.1f;

        float currentTemp = data.currentTemperature;
        world.setBlock(x, y, z, BlockType::CUSTOM, rustProps);
        world.setBlockTemperature(x, y, z, currentTemp);
        corrosionTimers.erase(key);
    } else {
        MaterialProps updatedProps = data.props;
        updatedProps.health.currentHealth = currentHealth;
        float currentTemp = data.currentTemperature;
        world.setBlock(x, y, z, data.type, updatedProps);
        world.setBlockTemperature(x, y, z, currentTemp);
    }
}

bool PhysicsEngine::isAcid(const MaterialProps& mp) const {
    const std::string& comp = mp.chemical.composition;
    return comp == "ACID" || comp == "H2SO4" || comp == "HCl" || comp == "HNO3" || comp == "CH3COOH";
}

bool PhysicsEngine::isBase(const MaterialProps& mp) const {
    const std::string& comp = mp.chemical.composition;
    return comp == "BASE" || comp == "NaOH" || comp == "KOH" || comp == "Ca(OH)2";
}

void PhysicsEngine::neutralizeAcid(VoxelOctree& world, int x, int y, int z, int nx, int ny, int nz) {
    MaterialProps waterProps;
    waterProps.general.mass = 1.0f;
    waterProps.general.density = 1000.0f;
    waterProps.thermal.specificHeat = 4186.0f;
    waterProps.thermal.thermalConductivity = 0.6f;
    waterProps.thermal.meltingPoint = 273.15f;
    waterProps.thermal.boilingPoint = 373.15f;
    waterProps.chemical.composition = "H2O";
    waterProps.health.maxHealth = 100.0f;
    waterProps.health.currentHealth = 100.0f;
    waterProps.visual.baseColor = "#4682B4";

    MaterialProps saltProps;
    saltProps.general.mass = 0.5f;
    saltProps.general.density = 2160.0f;
    saltProps.general.hardness = 2.5f;
    saltProps.thermal.specificHeat = 880.0f;
    saltProps.chemical.composition = "SALT";
    saltProps.chemical.chemicalResistance = 0.95f;
    saltProps.health.maxHealth = 50.0f;
    saltProps.health.currentHealth = 50.0f;
    saltProps.visual.baseColor = "#FFFFFF";

    world.setBlock(x, y, z, BlockType::WATER, waterProps);
    world.setBlockState(x, y, z, BlockState::LIQUID);
    world.setBlockTemperature(x, y, z, 310.0f);

    world.setBlock(nx, ny, nz, BlockType::CUSTOM, saltProps);
    world.setBlockTemperature(nx, ny, nz, 310.0f);

    dissolutionTimers.erase(posHash(x, y, z));
    dissolutionTimers.erase(posHash(nx, ny, nz));
}

void PhysicsEngine::processAcidReactions(VoxelOctree& world, int x, int y, int z, float dt) {
    VoxelData data;
    if (!world.getBlock(x, y, z, data)) return;
    if (data.type == BlockType::AIR) return;
    if (!isAcid(data.props)) return;

    float acidAmount = data.props.health.currentHealth;
    if (acidAmount <= 0.0f) {
        world.setBlock(x, y, z, BlockType::AIR);
        return;
    }

    const int offsets[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    for (int i = 0; i < 6; i++) {
        int nx = x + offsets[i][0];
        int ny = y + offsets[i][1];
        int nz = z + offsets[i][2];

        VoxelData nd;
        if (!world.getBlock(nx, ny, nz, nd)) continue;
        if (nd.type == BlockType::AIR) continue;

        if (isBase(nd.props)) {
            neutralizeAcid(world, x, y, z, nx, ny, nz);
            return;
        }

        if (nd.props.chemical.chemicalResistance >= 1.0f) continue;

        uint64_t nkey = posHash(nx, ny, nz);
        auto it = dissolutionTimers.find(nkey);
        if (it == dissolutionTimers.end()) {
            dissolutionTimers[nkey] = 0.0f;
            it = dissolutionTimers.find(nkey);
        }

        float resistance = nd.props.chemical.chemicalResistance;
        float dissolveRate = (1.0f - resistance) * 2.0f;
        float dissolveThreshold = 1.0f / dissolveRate;

        it->second += dt;

        float acidConsumption = dissolveRate * dt * 0.5f;
        acidAmount -= acidConsumption;

        if (it->second >= dissolveThreshold) {
            world.setBlock(nx, ny, nz, BlockType::AIR);
            dissolutionTimers.erase(nkey);
        }

        if (acidAmount <= 0.0f) {
            world.setBlock(x, y, z, BlockType::AIR);
            dissolutionTimers.erase(posHash(x, y, z));
            return;
        }
    }

    if (acidAmount != data.props.health.currentHealth) {
        MaterialProps updatedProps = data.props;
        updatedProps.health.currentHealth = acidAmount;
        float currentTemp = data.currentTemperature;
        world.setBlock(x, y, z, data.type, updatedProps);
        world.setBlockTemperature(x, y, z, currentTemp);
    }
}

void PhysicsEngine::tickChemistry(VoxelOctree& world, float dt) {
    for (int x = 0; x < scanRange; x++) {
        for (int y = 0; y < scanRange; y++) {
            for (int z = 0; z < scanRange; z++) {
                VoxelData data;
                if (!world.getBlock(x, y, z, data)) continue;
                if (data.type == BlockType::AIR) continue;

                PhysicsData& pd = physicsDataMap[posHash(x, y, z)];
                if (pd.health == 0.0f) pd.health = data.props.health.currentHealth;
                pd.isOnFire = false;

                if (isBurning(x, y, z)) {
                    processCombustion(world, x, y, z, dt);
                    spreadFire(world, x, y, z, dt);
                    VoxelData rd;
                    if (world.getBlock(x, y, z, rd) && rd.type != BlockType::ASH) {
                        pd.isOnFire = true;
                    }
                    continue;
                }

                if (checkDetonationConditions(world, x, y, z)) {
                    triggerExplosion(world, x, y, z);
                    continue;
                }

                checkReactions(world, x, y, z);

                float ctemp = ambientTemperature;
                VoxelData td;
                if (world.getBlock(x, y, z, td)) {
                    ctemp = td.currentTemperature;
                }
                pd.isOnFire = (ctemp > data.props.chemical.combustionPoint && data.props.chemical.flammability > 0);

                igniteBlock(world, x, y, z, pd, data.props);
                processCorrosion(world, x, y, z, dt);
                processAcidReactions(world, x, y, z, dt);
            }
        }
    }
}

void PhysicsEngine::tickPlantGrowth(VoxelOctree& world, int x, int y, int z, PhysicsData& pd, const MaterialProps& mp, float dt) {
    if (!mp.biological.isOrganic) return;

    bool sunlight = false;
    for (int sy = y + 1; sy < scanRange && sy < y + 20; sy++) {
        if (isSolidBlock(world, x, sy, z)) break;
        sunlight = true;
    }
    pd.hasSunlight = sunlight;

    bool waterNear = false;
    const int offsets[8][3] = {{1,0,0},{-1,0,0},{0,0,1},{0,0,-1},{1,0,1},{-1,0,-1},{1,0,-1},{-1,0,1}};
    for (int i = 0; i < 8; i++) {
        if (isWaterBlock(world, x + offsets[i][0], y + offsets[i][1], z + offsets[i][2]) ||
            isWaterBlock(world, x + offsets[i][0], y - 1, z + offsets[i][2])) {
            waterNear = true;
            break;
        }
    }
    pd.hasWater = waterNear;

    bool soilBelow = isSolidBlock(world, x, y - 1, z);
    pd.hasSoil = soilBelow;

    if (!pd.hasSunlight || !pd.hasWater || !pd.hasSoil) {
        pd.bioStage = BiologicalStage::DEAD;
        return;
    }

    pd.growthTimer += mp.biological.growthRate * dt * 0.1f;

    if (pd.growthTimer >= pd.growthThreshold) {
        pd.growthTimer = 0.0f;
        switch (pd.bioStage) {
            case BiologicalStage::SEED:
                pd.bioStage = BiologicalStage::SPROUT;
                break;
            case BiologicalStage::SPROUT:
                pd.bioStage = BiologicalStage::PLANT;
                break;
            case BiologicalStage::PLANT:
                pd.bioStage = BiologicalStage::FRUIT;
                break;
            case BiologicalStage::FRUIT:
                MaterialProps fruitProps;
                fruitProps.chemical.composition = "Organic";
                fruitProps.general.mass = 0.5f;
                fruitProps.visual.baseColor = "#FF4500";
                int fx = x + std::uniform_int_distribution<int>(-2, 2)(rng);
                int fz = z + std::uniform_int_distribution<int>(-2, 2)(rng);
                if (fx >= 0 && fx < scanRange && fz >= 0 && fz < scanRange) {
                    world.setBlock(fx, y + 1, fz, BlockType::CUSTOM, fruitProps);
                }
                pd.bioStage = BiologicalStage::PLANT;
                break;
        }
    }
}

void PhysicsEngine::processEcosystem(VoxelOctree& world, float dt) {
    for (auto& agent : agents) {
        if (!agent.isAlive) continue;

        float fx = agent.x + agent.vx * dt;
        float fy = agent.y + agent.vy * dt;
        float fz = agent.z + agent.vz * dt;

        if (fx >= 0 && fx < scanRange && fy >= 0 && fy < scanRange && fz >= 0 && fz < scanRange) {
            if (!isSolidBlock(world, static_cast<int>(fx), static_cast<int>(fy), static_cast<int>(fz))) {
                agent.x = fx;
                agent.y = fy;
                agent.z = fz;
            } else {
                agent.vx *= -0.5f;
                agent.vz *= -0.5f;
            }
        }

    }
}

void PhysicsEngine::addDisease(const Disease& disease) {
    diseases.push_back(disease);
}

void PhysicsEngine::processDiseaseSpread(VoxelOctree& world, float dt) {
    for (auto& agent : agents) {
        if (!agent.isAlive) continue;
        if (agent.diseaseState == DiseaseState::INCUBATING ||
            agent.diseaseState == DiseaseState::SYMPTOMATIC) {
            const Disease* disease = nullptr;
            for (const auto& d : diseases) {
                if (d.diseaseID == agent.infectedDiseaseID) {
                    disease = &d;
                    break;
                }
            }
            if (disease) {
                progressDisease(agent, *disease, dt);
            }
        }
    }

    for (auto& source : agents) {
        if (!source.isAlive) continue;
        if (source.diseaseState != DiseaseState::INCUBATING &&
            source.diseaseState != DiseaseState::SYMPTOMATIC) continue;

        const Disease* disease = nullptr;
        for (const auto& d : diseases) {
            if (d.diseaseID == source.infectedDiseaseID) {
                disease = &d;
                break;
            }
        }
        if (!disease) continue;

        for (auto& target : agents) {
            if (&target == &source || !target.isAlive) continue;
            if (target.diseaseState != DiseaseState::HEALTHY) continue;
            transmitDisease(source, target, *disease, dt);
        }
    }
}

void PhysicsEngine::transmitDisease(Agent& source, Agent& target, const Disease& disease, float dt) {
    float dx = source.x - target.x;
    float dy = source.y - target.y;
    float dz = source.z - target.z;
    float distSq = dx * dx + dy * dy + dz * dz;
    float range = disease.transmissionRange;

    if (disease.transmissionMode == TransmissionMode::CONTACT) {
        range = 1.5f;
    }

    if (distSq > range * range) return;

    float resistance = target.immunityLevel * (1.0f - disease.immunityGain);
    float chance = disease.infectivity * (1.0f - resistance) * dt * 0.01f;

    if (disease.transmissionMode == TransmissionMode::AIRBORNE) {
        chance *= 1.5f;
    }

    if (std::uniform_real_distribution<float>(0.0f, 1.0f)(rng) < chance) {
        target.diseaseState = DiseaseState::INCUBATING;
        target.infectedDiseaseID = disease.diseaseID;
        target.infectionTimer = disease.incubationPeriod;
        target.symptomTimer = 0.0f;
        target.isDiseased = true;
    }
}

void PhysicsEngine::progressDisease(Agent& agent, const Disease& disease, float dt) {
    if (agent.diseaseState == DiseaseState::INCUBATING) {
        agent.infectionTimer -= dt;
        if (agent.infectionTimer <= 0.0f) {
            agent.diseaseState = DiseaseState::SYMPTOMATIC;
            agent.symptomTimer = disease.symptomDuration;
        }
    }

    if (agent.diseaseState == DiseaseState::SYMPTOMATIC) {
        agent.symptomTimer -= dt;
        agent.health -= disease.healthDrainPerTick * disease.severity * dt;
        agent.hunger += disease.hungerIncreasePerTick * disease.severity * dt;

        if (std::uniform_real_distribution<float>(0.0f, 1.0f)(rng) < disease.mortalityRate * dt * 0.01f) {
            agent.health -= disease.severity * 10.0f * dt;
        }

        if (agent.health <= 0.0f) {
            agent.health = 0.0f;
            agent.isAlive = false;
            return;
        }

        if (agent.symptomTimer <= 0.0f) {
            agent.diseaseState = DiseaseState::RECOVERED;
            agent.immunityLevel = disease.immunityGain;
            agent.isDiseased = false;
        }
    }
}

void PhysicsEngine::processDecay(VoxelOctree& world, int x, int y, int z, PhysicsData& pd, float dt) {
    VoxelData data;
    if (!world.getBlock(x, y, z, data)) return;
    if (data.type == BlockType::AIR) return;
    if (!data.props.biological.isOrganic) return;

    if (pd.isOnFire) {
        pd.health -= 5.0f * dt;
        if (pd.health <= 0.0f) {
            world.setBlock(x, y, z, BlockType::ASH);
        }
        return;
    }

    uint64_t key = posHash(x, y, z);
    float& decayTimer = decayTimers[key];

    float rate = calculateDecayRate(world, x, y, z, data.props);
    decayTimer += rate * dt;

    if (decayTimer >= data.props.biological.decayThreshold) {
        spawnDecayProducts(world, x, y, z, data.type);
        world.setBlock(x, y, z, BlockType::AIR);
        decayTimers.erase(key);
    }
}

float PhysicsEngine::calculateDecayRate(const VoxelOctree& world, int x, int y, int z, const MaterialProps& mp) const {
    float baseRate = 1.0f;

    VoxelData td;
    float temp = 293.15f;
    if (world.getBlock(x, y, z, td)) {
        temp = td.currentTemperature;
    }
    float tempC = temp - 273.15f;
    float tempModifier = 1.0f;
    if (tempC < 0.0f) {
        tempModifier = 0.1f;
    } else if (tempC < 10.0f) {
        tempModifier = 0.5f;
    } else if (tempC > 40.0f) {
        tempModifier = 2.0f;
    } else {
        tempModifier = 0.5f + (tempC / 40.0f) * 1.5f;
    }

    float moistureModifier = 1.0f;
    bool hasWater = false;
    const int offsets[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    for (int i = 0; i < 6; i++) {
        if (isWaterBlock(world, x + offsets[i][0], y + offsets[i][1], z + offsets[i][2])) {
            hasWater = true;
            break;
        }
    }
    if (hasWater) moistureModifier = 1.5f;

    float oxygenModifier = 1.0f;
    bool hasOxygen = false;
    for (int i = 0; i < 6; i++) {
        int nx = x + offsets[i][0], ny = y + offsets[i][1], nz = z + offsets[i][2];
        VoxelData nd;
        if (world.getBlock(nx, ny, nz, nd)) {
            if (nd.type == BlockType::AIR || nd.props.chemical.composition == "O2") {
                hasOxygen = true;
                break;
            }
        }
    }
    if (!hasOxygen) {
        oxygenModifier = 0.5f;
    } else {
        oxygenModifier = 1.5f;
    }

    return baseRate * tempModifier * moistureModifier * oxygenModifier;
}

void PhysicsEngine::spawnDecayProducts(VoxelOctree& world, int x, int y, int z, BlockType originalType) {
    MaterialProps dirtProps;
    dirtProps.general.mass = 1.5f;
    dirtProps.general.density = 1200.0f;
    dirtProps.general.hardness = 2.0f;
    dirtProps.mechanical.tensileStrength = 10.0f;
    dirtProps.mechanical.compressiveStrength = 20.0f;
    dirtProps.mechanical.shearStrength = 8.0f;
    dirtProps.mechanical.fractureToughness = 2.0f;
    dirtProps.thermal.thermalConductivity = 0.5f;
    dirtProps.thermal.specificHeat = 800.0f;
    dirtProps.thermal.meltingPoint = 1000.0f;
    dirtProps.thermal.boilingPoint = 1500.0f;
    dirtProps.chemical.composition = "SiO2-organic";
    dirtProps.visual.baseColor = "#8B4513";
    dirtProps.health.maxHealth = 100.0f;
    dirtProps.health.currentHealth = 100.0f;

    MaterialProps compostProps = dirtProps;
    compostProps.chemical.composition = "compost";
    compostProps.visual.baseColor = "#3D2B1F";

    MaterialProps gasProps;
    gasProps.general.mass = 0.01f;
    gasProps.general.density = 1.2f;
    gasProps.chemical.composition = "CO2";
    gasProps.visual.baseColor = "#CCCCCC";
    gasProps.health.maxHealth = 10.0f;
    gasProps.health.currentHealth = 10.0f;

    const int offsets[3][3] = {{0,1,0},{1,0,0},{-1,0,0}};
    int productIndex = 0;

    switch (originalType) {
        case BlockType::WOOD: {
            VoxelData below;
            if (world.getBlock(x, y - 1, z, below) && below.type == BlockType::AIR) {
                world.setBlock(x, y - 1, z, BlockType::DIRT, dirtProps);
            } else {
                for (int i = 0; i < 3 && productIndex == 0; i++) {
                    int px = x + offsets[i][0], py = y + offsets[i][1], pz = z + offsets[i][2];
                    VoxelData pd;
                    if (!world.getBlock(px, py, pz, pd) || pd.type == BlockType::AIR) {
                        world.setBlock(px, py, pz, BlockType::DIRT, dirtProps);
                        productIndex = 1;
                        break;
                    }
                }
            }
            for (int i = 0; i < 3; i++) {
                int gx = x + offsets[i][0], gy = y + offsets[i][1] + 1, gz = z + offsets[i][2];
                VoxelData gd;
                if (!world.getBlock(gx, gy, gz, gd) || gd.type == BlockType::AIR) {
                    world.setBlock(gx, gy, gz, BlockType::CUSTOM, gasProps);
                    break;
                }
            }
            break;
        }
        case BlockType::LEAVES: {
            VoxelData below;
            if (world.getBlock(x, y - 1, z, below) && below.type == BlockType::AIR) {
                world.setBlock(x, y - 1, z, BlockType::CUSTOM, compostProps);
            } else {
                for (int i = 0; i < 3; i++) {
                    int px = x + offsets[i][0], py = y + offsets[i][1], pz = z + offsets[i][2];
                    VoxelData pd;
                    if (!world.getBlock(px, py, pz, pd) || pd.type == BlockType::AIR) {
                        world.setBlock(px, py, pz, BlockType::CUSTOM, compostProps);
                        break;
                    }
                }
            }
            break;
        }
        case BlockType::CUSTOM: {
            VoxelData below;
            if (world.getBlock(x, y - 1, z, below) && below.type == BlockType::AIR) {
                world.setBlock(x, y - 1, z, BlockType::DIRT, dirtProps);
            }
            for (int i = 0; i < 3; i++) {
                int gx = x + offsets[i][0], gy = y + offsets[i][1] + 1, gz = z + offsets[i][2];
                VoxelData gd;
                if (!world.getBlock(gx, gy, gz, gd) || gd.type == BlockType::AIR) {
                    world.setBlock(gx, gy, gz, BlockType::CUSTOM, gasProps);
                    break;
                }
            }
            break;
        }
        default: {
            VoxelData below;
            if (world.getBlock(x, y - 1, z, below) && below.type == BlockType::AIR) {
                world.setBlock(x, y - 1, z, BlockType::DIRT, dirtProps);
            }
            break;
        }
    }
}

bool PhysicsEngine::hasSunlight(const VoxelOctree& world, int x, int y, int z) const {
    for (int sy = y + 1; sy < scanRange && sy < y + 20; sy++) {
        if (isSolidBlock(world, x, sy, z)) return false;
    }
    return true;
}

bool PhysicsEngine::hasWaterNearby(const VoxelOctree& world, int x, int y, int z) const {
    const int offsets[8][3] = {{1,0,0},{-1,0,0},{0,0,1},{0,0,-1},{1,0,1},{-1,0,-1},{1,0,-1},{-1,0,1}};
    for (int i = 0; i < 8; i++) {
        if (isWaterBlock(world, x + offsets[i][0], y, z + offsets[i][2])) return true;
        if (isWaterBlock(world, x + offsets[i][0], y - 1, z + offsets[i][2])) return true;
    }
    return false;
}

bool PhysicsEngine::hasSoilBelow(const VoxelOctree& world, int x, int y, int z) const {
    VoxelData below;
    if (!world.getBlock(x, y - 1, z, below)) return false;
    if (below.type == BlockType::AIR) return false;
    if (below.props.biological.soilType == "LOAMY_SOIL") return true;
    if (below.props.biological.soilType.empty() && below.type == BlockType::DIRT) return true;
    return false;
}

void PhysicsEngine::produceFruit(VoxelOctree& world, int x, int y, int z) {
    MaterialProps fruitProps;
    fruitProps.chemical.composition = "Organic";
    fruitProps.general.mass = 0.5f;
    fruitProps.general.density = 500.0f;
    fruitProps.general.hardness = 1.0f;
    fruitProps.mechanical.tensileStrength = 5.0f;
    fruitProps.mechanical.compressiveStrength = 3.0f;
    fruitProps.mechanical.shearStrength = 2.0f;
    fruitProps.mechanical.fractureToughness = 1.0f;
    fruitProps.thermal.thermalConductivity = 0.2f;
    fruitProps.thermal.specificHeat = 2000.0f;
    fruitProps.thermal.meltingPoint = 350.0f;
    fruitProps.thermal.boilingPoint = 450.0f;
    fruitProps.visual.baseColor = "#FF4500";
    fruitProps.health.maxHealth = 20.0f;
    fruitProps.health.currentHealth = 20.0f;

    const int offsets[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    std::uniform_int_distribution<int> dist(0, 5);
    for (int attempt = 0; attempt < 6; attempt++) {
        int idx = dist(rng);
        int fx = x + offsets[idx][0];
        int fy = y + offsets[idx][1];
        int fz = z + offsets[idx][2];
        if (fx >= 0 && fx < scanRange && fy >= 0 && fy < scanRange && fz >= 0 && fz < scanRange) {
            VoxelData existing;
            if (!world.getBlock(fx, fy, fz, existing) || existing.type == BlockType::AIR) {
                world.setBlock(fx, fy, fz, BlockType::CUSTOM, fruitProps);
                return;
            }
        }
    }
}

void PhysicsEngine::processPlantGrowth(VoxelOctree& world, int x, int y, int z, float dt) {
    VoxelData data;
    if (!world.getBlock(x, y, z, data)) return;
    if (data.type == BlockType::AIR) return;

    bool isSeed = (data.type == BlockType::CUSTOM && data.props.biological.isOrganic);

    uint64_t key = posHash(x, y, z);
    auto regIt = plantRegistry.find(key);
    bool isPlant = (regIt != plantRegistry.end() && regIt->second.state != PlantState::NONE && regIt->second.state != PlantState::SEED);
    if (!isSeed && !isPlant) return;

    PlantData& pd = plantRegistry[key];

    if (pd.state == PlantState::NONE) {
        pd.state = PlantState::SEED;
        pd.growthTimer = 0.0f;
        pd.fruitTimer = 0.0f;
    }

    bool sunlight = hasSunlight(world, x, y, z);
    bool water = hasWaterNearby(world, x, y, z);
    bool soil = hasSoilBelow(world, x, y, z);
    pd.conditionsMet = sunlight && water && soil;

    if (!pd.conditionsMet) return;

    pd.growthTimer += dt;

    if (pd.growthTimer >= pd.stageThreshold) {
        pd.growthTimer = 0.0f;
        switch (pd.state) {
            case PlantState::SEED:
                pd.state = PlantState::SPROUT;
                break;
            case PlantState::SPROUT:
                pd.state = PlantState::PLANT;
                break;
            case PlantState::PLANT:
                pd.state = PlantState::FRUIT;
                break;
            case PlantState::FRUIT:
                break;
            default:
                break;
        }
    }

    if (pd.state == PlantState::FRUIT) {
        pd.fruitTimer += dt;
        if (pd.fruitTimer >= pd.fruitInterval) {
            pd.fruitTimer = 0.0f;
            produceFruit(world, x, y, z);
        }
    }
}

PlantState PhysicsEngine::getPlantState(int x, int y, int z) const {
    uint64_t key = posHash(x, y, z);
    auto it = plantRegistry.find(key);
    if (it == plantRegistry.end()) return PlantState::NONE;
    return it->second.state;
}

const PlantData* PhysicsEngine::getPlantData(int x, int y, int z) const {
    uint64_t key = posHash(x, y, z);
    auto it = plantRegistry.find(key);
    if (it == plantRegistry.end()) return nullptr;
    return &it->second;
}

void PhysicsEngine::tickBiology(VoxelOctree& world, float dt) {
    for (int x = 0; x < scanRange; x++) {
        for (int y = 0; y < scanRange; y++) {
            for (int z = 0; z < scanRange; z++) {
                VoxelData data;
                if (!world.getBlock(x, y, z, data)) continue;
                if (data.type == BlockType::AIR) continue;

                PhysicsData& pd = physicsDataMap[posHash(x, y, z)];
                if (pd.bioStage == BiologicalStage{}) {
                    pd.bioStage = BiologicalStage::SEED;
                    pd.growthTimer = data.props.biological.growthRate * 50.0f;
                }
                if (pd.health == 0.0f) pd.health = data.props.health.currentHealth;

                tickPlantGrowth(world, x, y, z, pd, data.props, dt);
                processPlantGrowth(world, x, y, z, dt);
                processDecay(world, x, y, z, pd, dt);
            }
        }
    }

    processEcosystem(world, dt);
    processDiseaseSpread(world, dt);
    processMetabolism(world, dt);
    processPredatorPrey(world, dt);
    processTemperaturePerception(world, dt);
}

void PhysicsEngine::applyOrbitalGravity(VoxelOctree& world, float dt) {
    if (celestialBodies.empty() && !celestialScanDone) {
        scanCelestialBodies(world);
        if (celestialBodies.empty()) {
            celestialScanDone = true;
        }
    }

    for (auto& agent : agents) {
        if (!agent.isAlive) continue;

        float ax = 0.0f, ay = 0.0f, az = 0.0f;

        for (const auto& body : celestialBodies) {
            if (!body.isActive) continue;

            float dx = body.x - agent.x;
            float dy = body.y - agent.y;
            float dz = body.z - agent.z;
            float distSq = dx * dx + dy * dy + dz * dz;
            float minDist = body.radius + 0.5f;
            if (distSq < minDist * minDist) distSq = minDist * minDist;

            float dist = std::sqrt(distSq);
            float forceMag = gravitationalConstant * body.mass / distSq;

            ax += forceMag * dx / dist;
            ay += forceMag * dy / dist;
            az += forceMag * dz / dist;
        }

        agent.vx += ax * dt;
        agent.vy += ay * dt;
        agent.vz += az * dt;

        float altitude = agent.y - SEA_LEVEL;
        float atmoDensity = getAtmosphereDensity(altitude);
        agent.vy -= gravity * dt;
        agent.vx += windX * atmoDensity * dt;
        agent.vz += windZ * atmoDensity * dt;
    }

    for (size_t i = 0; i < celestialBodies.size(); i++) {
        for (size_t j = i + 1; j < celestialBodies.size(); j++) {
            auto& a = celestialBodies[i];
            auto& b = celestialBodies[j];
            if (!a.isActive || !b.isActive) continue;

            float dx = b.x - a.x;
            float dy = b.y - a.y;
            float dz = b.z - a.z;
            float distSq = dx * dx + dy * dy + dz * dz;
            float minDist = a.radius + b.radius;
            if (distSq < minDist * minDist) distSq = minDist * minDist;

            float dist = std::sqrt(distSq);
            float forceMag = gravitationalConstant * a.mass * b.mass / distSq;

            float fx = forceMag * dx / dist;
            float fy = forceMag * dy / dist;
            float fz = forceMag * dz / dist;

            if (!a.isFixed) {
                a.vx += fx / a.mass * dt;
                a.vy += fy / a.mass * dt;
                a.vz += fz / a.mass * dt;
            }
            if (!b.isFixed) {
                b.vx -= fx / b.mass * dt;
                b.vy -= fy / b.mass * dt;
                b.vz -= fz / b.mass * dt;
            }
        }
    }

    for (auto& body : celestialBodies) {
        if (!body.isActive || body.isFixed) continue;

        body.x += body.vx * dt;
        body.y += body.vy * dt;
        body.z += body.vz * dt;

        body.x = std::max(0.0f, std::min(255.0f, body.x));
        body.y = std::max(0.0f, std::min(255.0f, body.y));
        body.z = std::max(0.0f, std::min(255.0f, body.z));
    }
}

void PhysicsEngine::scanCelestialBodies(const VoxelOctree& world) {
    std::unordered_map<uint64_t, int> bodyMap;
    for (int i = 0; i < static_cast<int>(celestialBodies.size()); i++) {
        uint64_t key = posHash(celestialBodies[i].blockX, celestialBodies[i].blockY, celestialBodies[i].blockZ);
        bodyMap[key] = i;
    }

    std::unordered_map<uint64_t, bool> found;

    for (int x = 0; x < scanRange; x++) {
        for (int y = 0; y < scanRange; y++) {
            for (int z = 0; z < scanRange; z++) {
                VoxelData data;
                if (!world.getBlock(x, y, z, data)) continue;
                if (!data.isCelestialBody) continue;
                if (data.gravitationalMass <= 0.0f) continue;

                uint64_t key = posHash(x, y, z);
                found[key] = true;

                auto it = bodyMap.find(key);
                if (it != bodyMap.end()) {
                    auto& body = celestialBodies[it->second];
                    body.x = static_cast<float>(x) + 0.5f;
                    body.y = static_cast<float>(y) + 0.5f;
                    body.z = static_cast<float>(z) + 0.5f;
                    body.mass = data.gravitationalMass;
                } else {
                    GravitationalBody body;
                    body.x = static_cast<float>(x) + 0.5f;
                    body.y = static_cast<float>(y) + 0.5f;
                    body.z = static_cast<float>(z) + 0.5f;
                    body.mass = data.gravitationalMass;
                    body.radius = std::cbrt(data.gravitationalMass) * 0.5f;
                    body.blockX = x;
                    body.blockY = y;
                    body.blockZ = z;
                    body.isFixed = data.gravitationalMass > 1000.0f;
                    celestialBodies.push_back(body);
                }
            }
        }
    }

    for (int i = static_cast<int>(celestialBodies.size()) - 1; i >= 0; i--) {
        uint64_t key = posHash(celestialBodies[i].blockX, celestialBodies[i].blockY, celestialBodies[i].blockZ);
        if (found.find(key) == found.end()) {
            celestialBodies.erase(celestialBodies.begin() + i);
        }
    }
}

float PhysicsEngine::calculateEscapeVelocity(float centralMass, float distance) const {
    if (distance <= 0.0f) return 0.0f;
    return std::sqrt(2.0f * gravitationalConstant * centralMass / distance);
}

void PhysicsEngine::addCelestialBody(const GravitationalBody& body) {
    celestialBodies.push_back(body);
}

void PhysicsEngine::removeCelestialBody(int index) {
    if (index >= 0 && index < static_cast<int>(celestialBodies.size())) {
        celestialBodies.erase(celestialBodies.begin() + index);
    }
}

std::vector<GravitationalBody>& PhysicsEngine::getCelestialBodies() {
    return celestialBodies;
}

void PhysicsEngine::setGravitationalConstant(float g) {
    gravitationalConstant = g;
}

float PhysicsEngine::getAtmosphereDensity(float altitude) const {
    if (altitude < 0.0f) return AIR_DENSITY * 1.2f;
    return AIR_DENSITY * std::exp(-altitude / ATMOSPHERE_SCALE_HEIGHT);
}

float PhysicsEngine::getOxygenLevel(float altitude) const {
    float density = getAtmosphereDensity(altitude);
    float oxygenFraction = 0.21f;
    for (const auto& body : celestialBodies) {
        if (body.hasAtmosphere) {
            return body.atmosphere.oxygenAtAltitude(altitude);
        }
    }
    return density * oxygenFraction / AIR_DENSITY;
}

float PhysicsEngine::getAtmosphereDragFactor(float altitude, float velocity) const {
    float density = getAtmosphereDensity(altitude);
    float dragCoeff = 0.47f;
    float referenceArea = 1.0f;
    return 0.5f * density * dragCoeff * referenceArea * velocity;
}

float PhysicsEngine::getReentryHeat(float altitude, float velocity) const {
    float density = getAtmosphereDensity(altitude);
    float speed = std::abs(velocity);
    float heatFactor = density * speed * speed * speed * 1e-4f;
    return heatFactor;
}

void PhysicsEngine::applyAtmosphericDrag(VoxelOctree& world, float dt) {
    for (auto& agent : agents) {
        if (!agent.isAlive) continue;

        float altitude = agent.y - SEA_LEVEL;
        float density = getAtmosphereDensity(altitude);
        float speed = std::sqrt(agent.vx * agent.vx + agent.vy * agent.vy + agent.vz * agent.vz);

        if (speed > 0.01f && density > 1e-6f) {
            float dragCoeff = 0.47f;
            float dragForce = 0.5f * density * dragCoeff * speed;
            float dragAx = -(agent.vx / speed) * dragForce * dt;
            float dragAy = -(agent.vy / speed) * dragForce * dt;
            float dragAz = -(agent.vz / speed) * dragForce * dt;
            agent.vx += dragAx;
            agent.vy += dragAy;
            agent.vz += dragAz;
        }

        float heat = getReentryHeat(altitude, speed);
        if (heat > 0.1f) {
            float heatDamage = heat * dt * 0.1f;
            agent.health -= heatDamage;
            if (agent.health <= 0.0f) {
                agent.isAlive = false;
            }
        }

        float oxygen = getOxygenLevel(altitude);
        if (oxygen < 0.05f && altitude > 0.0f) {
            agent.energy -= (0.05f - oxygen) * 2.0f * dt;
        }
    }
}

void PhysicsEngine::addRocketStage(const RocketStage& stage) {
    rocketStages.push_back(stage);
}

std::vector<RocketStage>& PhysicsEngine::getRocketStages() {
    return rocketStages;
}

void PhysicsEngine::setRocketThrottle(int stageIndex, float throttle) {
    if (stageIndex >= 0 && stageIndex < static_cast<int>(rocketStages.size())) {
        rocketStages[stageIndex].throttle = std::max(0.0f, std::min(1.0f, throttle));
    }
}

float PhysicsEngine::calculateDeltaV(float Isp, float m0, float m1) const {
    if (m1 <= 0.0f) return 0.0f;
    const float g0 = 9.81f;
    return Isp * g0 * std::log(m0 / m1);
}

void PhysicsEngine::detachStage(VoxelOctree& world, int stageIndex) {
    if (stageIndex < 0 || stageIndex >= static_cast<int>(rocketStages.size())) return;
    auto& stage = rocketStages[stageIndex];
    if (!stage.isActive || stage.isDetached) return;

    for (auto& pos : stage.blockPositions) {
        world.setBlock(pos[0], pos[1], pos[2], BlockType::AIR, MaterialProps());
    }

    stage.isDetached = true;
    stage.isActive = false;
}

void PhysicsEngine::processRocketPhysics(VoxelOctree& world, float dt) {
    static constexpr float G0 = 9.81f;

    for (auto& stage : rocketStages) {
        if (!stage.isActive || stage.isDetached) continue;

        float totalMass = stage.dryMass + stage.currentFuel;
        if (totalMass <= stage.dryMass * 1.001f) {
            stage.currentFuel = 0.0f;
            continue;
        }

        float fuelConsumed = stage.fuelBurnRate * stage.throttle * dt;
        if (fuelConsumed > stage.currentFuel) {
            fuelConsumed = stage.currentFuel;
        }
        stage.currentFuel -= fuelConsumed;

        float exhaustVelocity = stage.Isp * G0;
        float thrustForce = stage.thrust * stage.throttle;
        float acceleration = thrustForce / totalMass;

        float deltaV = 0.0f;
        float newTotalMass = stage.dryMass + stage.currentFuel;
        if (newTotalMass > 0.0f && totalMass > 0.0f) {
            deltaV = exhaustVelocity * std::log(totalMass / newTotalMass);
        }

        stage.totalMass = newTotalMass;

        for (auto& agent : agents) {
            if (!agent.isAlive) continue;
            float agentSpeed = std::sqrt(agent.vx * agent.vx + agent.vy * agent.vy + agent.vz * agent.vz);
            if (agentSpeed > 0.001f) {
                agent.vx += (agent.vx / agentSpeed) * deltaV;
                agent.vy += (agent.vy / agentSpeed) * deltaV;
                agent.vz += (agent.vz / agentSpeed) * deltaV;
            } else {
                agent.vy += acceleration * dt;
            }
        }

        if (stage.currentFuel <= 0.0f) {
            stage.currentFuel = 0.0f;
        }
    }
}

void PhysicsEngine::emitSound(float x, float y, float z, float level, float range, const std::string& type) {
    SoundSource source;
    source.x = x;
    source.y = y;
    source.z = z;
    source.soundLevel = level;
    source.soundRange = range;
    source.soundType = type;
    source.life = 1.0f;
    source.isActive = true;
    soundSources.push_back(source);
}

std::vector<SoundSource>& PhysicsEngine::getSoundSources() {
    return soundSources;
}

float PhysicsEngine::getSoundIntensity(const VoxelOctree& world, float sx, float sy, float sz,
                                       float ax, float ay, float az, const SoundSource& source) const {
    float dx = ax - sx;
    float dy = ay - sy;
    float dz = az - sz;
    float distSq = dx * dx + dy * dy + dz * dz;
    float dist = std::sqrt(distSq);

    if (dist < 0.1f) dist = 0.1f;
    if (dist > source.soundRange) return 0.0f;

    float intensity = source.soundLevel / (dist * dist);

    int steps = static_cast<int>(dist);
    if (steps < 1) steps = 1;
    float stepX = dx / steps;
    float stepY = dy / steps;
    float stepZ = dz / steps;

    float absorption = 1.0f;
    for (int i = 1; i < steps; i++) {
        int bx = static_cast<int>(sx + stepX * i);
        int by = static_cast<int>(sy + stepY * i);
        int bz = static_cast<int>(sz + stepZ * i);

        VoxelData vd;
        if (world.getBlock(bx, by, bz, vd)) {
            if (vd.type != BlockType::AIR && vd.type != BlockType::WATER) {
                float blockAbsorption = vd.props.chemical.absorptionCoefficient;
                if (blockAbsorption < 0.01f) blockAbsorption = 0.8f;
                absorption *= (1.0f - blockAbsorption);
                if (absorption < 0.01f) return 0.0f;
            }
        }
    }

    return intensity * absorption;
}

void PhysicsEngine::propagateSound(VoxelOctree& world, float dt) {
    for (auto& source : soundSources) {
        if (!source.isActive) continue;
        source.life -= dt * 0.1f;
        if (source.life <= 0.0f) {
            source.isActive = false;
            continue;
        }

        if (source.reflections.empty() && source.soundLevel > 0.01f) {
            calculateReverb(world, source);
            calculateEcho(world, source);
        }

        for (auto& agent : agents) {
            if (!agent.isAlive) continue;

            float dist = std::sqrt((agent.x - source.x) * (agent.x - source.x) +
                                   (agent.y - source.y) * (agent.y - source.y) +
                                   (agent.z - source.z) * (agent.z - source.z));

            if (dist > source.soundRange || dist > agent.hearingRange) continue;

            float intensity = getSoundIntensity(world, source.x, source.y, source.z,
                                                agent.x, agent.y, agent.z, source);

            float reflectedTotal = 0.0f;
            for (auto& sr : source.reflections) {
                float rdist = std::sqrt((agent.x - sr.x) * (agent.x - sr.x) +
                                        (agent.y - sr.y) * (agent.y - sr.y) +
                                        (agent.z - sr.z) * (agent.z - sr.z));
                if (rdist < 20.0f) {
                    reflectedTotal += sr.intensity / (1.0f + rdist * rdist);
                }
            }
            intensity += reflectedTotal * source.reverb.wetDryMix;

            if (intensity > 0.01f) {
                bool alreadyHeard = false;
                for (auto& hs : agent.heardSounds) {
                    if (hs.soundType == source.soundType) {
                        if (intensity > hs.intensity) {
                            hs.intensity = intensity;
                            hs.sourceX = source.x;
                            hs.sourceY = source.y;
                            hs.sourceZ = source.z;
                            hs.timer = 5.0f;
                        }
                        alreadyHeard = true;
                        break;
                    }
                }
                if (!alreadyHeard) {
                    HeardSound hs;
                    hs.intensity = intensity;
                    hs.soundType = source.soundType;
                    hs.sourceX = source.x;
                    hs.sourceY = source.y;
                    hs.sourceZ = source.z;
                    hs.timer = 5.0f;
                    agent.heardSounds.push_back(hs);
                }
            }
        }
    }

    for (auto& agent : agents) {
        for (auto it = agent.heardSounds.begin(); it != agent.heardSounds.end(); ) {
            it->timer -= dt;
            it->intensity *= 0.95f;
            if (it->timer <= 0.0f || it->intensity < 0.001f) {
                it = agent.heardSounds.erase(it);
            } else {
                ++it;
            }
        }
    }
}

float PhysicsEngine::getReflectionCoefficient(int x, int y, int z, const VoxelOctree& world) const {
    VoxelData vd;
    if (!world.getBlock(x, y, z, vd)) return 0.0f;
    if (vd.type == BlockType::AIR) return 0.0f;

    float base = 0.3f;
    switch (vd.type) {
        case BlockType::STONE: base = 0.85f; break;
        case BlockType::STEEL: base = 0.95f; break;
        case BlockType::IRON: base = 0.90f; break;
        case BlockType::COPPER: base = 0.88f; break;
        case BlockType::GOLD: base = 0.92f; break;
        case BlockType::GLASS: base = 0.80f; break;
        case BlockType::WOOD: base = 0.40f; break;
        case BlockType::DIRT: base = 0.20f; break;
        case BlockType::SAND: base = 0.25f; break;
        case BlockType::WATER: base = 0.10f; break;
        case BlockType::LEAVES: base = 0.15f; break;
        default: base = 0.50f; break;
    }

    float absorption = vd.props.chemical.absorptionCoefficient;
    return base * (1.0f - absorption);
}

void PhysicsEngine::calculateReverb(const VoxelOctree& world, SoundSource& source) {
    source.reflections.clear();
    source.reflectedIntensity = 0.0f;

    const float speedOfSound = 343.0f;
    const int maxBounces = 6;

    for (int dir = 0; dir < 6; dir++) {
        float dx = 0, dy = 0, dz = 0;
        switch (dir) {
            case 0: dx = 1; break;
            case 1: dx = -1; break;
            case 2: dy = 1; break;
            case 3: dy = -1; break;
            case 4: dz = 1; break;
            case 5: dz = -1; break;
        }

        float cx = source.x, cy = source.y, cz = source.z;
        float intensity = source.soundLevel * source.reverb.wetDryMix;
        int totalBounces = 0;

        for (int bounce = 0; bounce < maxBounces; bounce++) {
            float dist = 0;
            bool hitSurface = false;
            float normDx = dx, normDy = dy, normDz = dz;

            for (float step = 0.5f; step < source.soundRange; step += 0.5f) {
                int bx = static_cast<int>(cx + dx * step);
                int by = static_cast<int>(cy + dy * step);
                int bz = static_cast<int>(cz + dz * step);

                float refl = getReflectionCoefficient(bx, by, bz, world);
                if (refl > 0.01f) {
                    dist = step;
                    hitSurface = true;
                    float absorption = 1.0f - refl;
                    float dampingFactor = std::pow(source.reverb.damping, static_cast<float>(bounce + 1));
                    intensity *= refl * dampingFactor;
                    intensity *= std::exp(-dist / source.reverb.roomSize);

                    float delay = (2.0f * dist) / speedOfSound;

                    SoundReflection sr;
                    sr.x = bx;
                    sr.y = by;
                    sr.z = bz;
                    sr.intensity = intensity;
                    sr.delay = delay;
                    sr.damping = source.reverb.damping;
                    sr.bounceCount = bounce + 1;
                    source.reflections.push_back(sr);
                    source.reflectedIntensity += intensity;

                    float dot = normDx * dx + normDy * dy + normDz * dz;
                    normDx = dx - 2.0f * dot * dx;
                    normDy = dy - 2.0f * dot * dy;
                    normDz = dz - 2.0f * dot * dz;
                    cx = bx + normDx * 0.5f;
                    cy = by + normDy * 0.5f;
                    cz = bz + normDz * 0.5f;
                    dx = normDx;
                    dy = normDy;
                    dz = normDz;
                    totalBounces++;
                    break;
                }
            }
            if (!hitSurface || intensity < 0.001f) break;
        }
    }
}

void PhysicsEngine::calculateEcho(const VoxelOctree& world, SoundSource& source) {
    const float speedOfSound = 343.0f;

    for (int dir = 0; dir < 6; dir++) {
        float dx = 0, dy = 0, dz = 0;
        switch (dir) {
            case 0: dx = 1; break;
            case 1: dx = -1; break;
            case 2: dy = 1; break;
            case 3: dy = -1; break;
            case 4: dz = 1; break;
            case 5: dz = -1; break;
        }

        for (float step = 1.0f; step < source.soundRange; step += 1.0f) {
            int bx = static_cast<int>(source.x + dx * step);
            int by = static_cast<int>(source.y + dy * step);
            int bz = static_cast<int>(source.z + dz * step);

            float refl = getReflectionCoefficient(bx, by, bz, world);
            if (refl > 0.5f) {
                float dist = step;
                float intensity = source.soundLevel * refl * std::exp(-dist / source.reverb.roomSize);
                float delay = (2.0f * dist) / speedOfSound;

                bool alreadyAdded = false;
                for (auto& sr : source.reflections) {
                    float sd = std::sqrt((sr.x - bx) * (sr.x - bx) +
                                         (sr.y - by) * (sr.y - by) +
                                         (sr.z - bz) * (sr.z - bz));
                    if (sd < 2.0f) {
                        alreadyAdded = true;
                        break;
                    }
                }

                if (!alreadyAdded && intensity > 0.01f) {
                    SoundReflection sr;
                    sr.x = bx;
                    sr.y = by;
                    sr.z = bz;
                    sr.intensity = intensity;
                    sr.delay = delay;
                    sr.damping = 0.8f;
                    sr.bounceCount = 1;
                    source.reflections.push_back(sr);
                    source.reflectedIntensity += intensity;
                }
                break;
            }
        }
    }
}

void PhysicsEngine::emitSoundWithReverb(float x, float y, float z, float level, float range, const std::string& type, const ReverbParams& reverb) {
    SoundSource source;
    source.x = x;
    source.y = y;
    source.z = z;
    source.soundLevel = level;
    source.soundRange = range;
    source.soundType = type;
    source.life = 1.0f;
    source.isActive = true;
    source.reverb = reverb;
    source.directIntensity = level;
    soundSources.push_back(source);
}

void PhysicsEngine::emitLight(float x, float y, float z, float radius, float intensity, const std::string& color) {
    LightSource ls;
    ls.x = x;
    ls.y = y;
    ls.z = z;
    ls.lightRadius = radius;
    ls.lightIntensity = intensity;
    ls.lightColor = color;
    ls.isDirectional = false;
    ls.isActive = true;
    ls.flickerRate = 0.0f;
    ls.flickerAmount = 0.0f;
    lightSources.push_back(ls);
}

void PhysicsEngine::emitDirectionalLight(float x, float y, float z, float dx, float dy, float dz, float intensity, const std::string& color) {
    LightSource ls;
    ls.x = x;
    ls.y = y;
    ls.z = z;
    ls.lightRadius = 999.0f;
    ls.lightIntensity = intensity;
    ls.lightColor = color;
    ls.isDirectional = true;
    ls.dirX = dx;
    ls.dirY = dy;
    ls.dirZ = dz;
    ls.isActive = true;
    lightSources.push_back(ls);
}

std::vector<LightSource>& PhysicsEngine::getLightSources() {
    return lightSources;
}

float PhysicsEngine::getLightLevel(const VoxelOctree& world, int x, int y, int z) const {
    float totalLight = 0.0f;

    for (const auto& ls : lightSources) {
        if (!ls.isActive) continue;

        if (ls.isDirectional) {
            float dot = (static_cast<float>(x) - ls.x) * ls.dirX +
                        (static_cast<float>(y) - ls.y) * ls.dirY +
                        (static_cast<float>(z) - ls.z) * ls.dirZ;
            if (dot > 0.0f) {
                float dist = std::sqrt((static_cast<float>(x) - ls.x) * (static_cast<float>(x) - ls.x) +
                                       (static_cast<float>(y) - ls.y) * (static_cast<float>(y) - ls.y) +
                                       (static_cast<float>(z) - ls.z) * (static_cast<float>(z) - ls.z));
                bool blocked = false;
                int steps = static_cast<int>(dist);
                if (steps > 0) {
                    float stepX = (static_cast<float>(x) - ls.x) / steps;
                    float stepY = (static_cast<float>(y) - ls.y) / steps;
                    float stepZ = (static_cast<float>(z) - ls.z) / steps;
                    for (int i = 1; i < steps; i++) {
                        int bx = static_cast<int>(ls.x + stepX * i);
                        int by = static_cast<int>(ls.y + stepY * i);
                        int bz = static_cast<int>(ls.z + stepZ * i);
                        VoxelData vd;
                        if (world.getBlock(bx, by, bz, vd) && vd.type != BlockType::AIR && vd.type != BlockType::WATER) {
                            float absorption = vd.props.chemical.lightAbsorption;
                            if (absorption > 0.5f) {
                                blocked = true;
                                break;
                            }
                        }
                    }
                }
                if (!blocked) {
                    totalLight += ls.lightIntensity;
                }
            }
        } else {
            float dx = static_cast<float>(x) - ls.x;
            float dy = static_cast<float>(y) - ls.y;
            float dz = static_cast<float>(z) - ls.z;
            float distSq = dx * dx + dy * dy + dz * dz;
            float dist = std::sqrt(distSq);

            if (dist > ls.lightRadius) continue;

            float intensity = ls.lightIntensity / (1.0f + distSq);

            bool blocked = false;
            int steps = static_cast<int>(dist);
            if (steps > 1) {
                float stepX = dx / steps;
                float stepY = dy / steps;
                float stepZ = dz / steps;
                float transmittance = 1.0f;
                for (int i = 1; i < steps; i++) {
                    int bx = static_cast<int>(ls.x + stepX * i);
                    int by = static_cast<int>(ls.y + stepY * i);
                    int bz = static_cast<int>(ls.z + stepZ * i);
                    VoxelData vd;
                    if (world.getBlock(bx, by, bz, vd) && vd.type != BlockType::AIR && vd.type != BlockType::WATER) {
                        float absorption = vd.props.chemical.lightAbsorption;
                        transmittance *= (1.0f - absorption);
                        if (transmittance < 0.01f) {
                            blocked = true;
                            break;
                        }
                    }
                }
                intensity *= transmittance;
            }

            if (!blocked) {
                totalLight += intensity;
            }
        }
    }

    float ambient = 0.05f;
    float sunlight = getDaylightFactor();
    return std::min(1.0f, std::max(0.0f, totalLight + ambient + sunlight * 0.5f));
}

void PhysicsEngine::propagateLight(VoxelOctree& world, float dt) {
    for (auto& ls : lightSources) {
        if (!ls.isActive) continue;
        if (ls.flickerRate > 0.0f) {
            float flicker = std::sin(currentTick * ls.flickerRate) * ls.flickerAmount;
            ls.lightIntensity = std::max(0.1f, ls.lightIntensity + flicker * dt);
        }
    }
}

void PhysicsEngine::tickAgents(VoxelOctree& world, float dt) {
    applyOrbitalGravity(world, dt);
    applyAtmosphericDrag(world, dt);
    processRocketPhysics(world, dt);
    propagateSound(world, dt);
    soundSources.erase(std::remove_if(soundSources.begin(), soundSources.end(),
        [](const auto& s) { return !s.isActive; }), soundSources.end());
    propagateLight(world, dt);
    lightSources.erase(std::remove_if(lightSources.begin(), lightSources.end(),
        [](const auto& ls) { return !ls.isActive; }), lightSources.end());

    for (auto& agent : agents) {
        if (!agent.isAlive) continue;

        agent.x += agent.vx * dt;
        agent.y += agent.vy * dt;
        agent.z += agent.vz * dt;

        agent.x = std::max(0.0f, std::min(255.0f, agent.x));
        agent.y = std::max(0.0f, std::min(255.0f, agent.y));
        agent.z = std::max(0.0f, std::min(255.0f, agent.z));

        if (isSolidBlock(world, static_cast<int>(agent.x), static_cast<int>(agent.y) - 1, static_cast<int>(agent.z))) {
            agent.vy = 0.0f;
        }
    }
}

void PhysicsEngine::tickParticles(float dt) {
    for (auto& p : particles) {
        if (!p.active) continue;

        p.life -= dt / p.maxLife;
        if (p.life <= 0.0f) {
            p.active = false;
            continue;
        }

        p.vy -= 2.0f * dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.z += p.vz * dt;
        p.size *= 0.99f;
    }
}

void PhysicsEngine::tickSnapshots(VoxelOctree& world) {
    if (currentTick % 30 != 0) return;

    WorldSnapshot snap;
    snap.tick = currentTick;
    snap.agents = agents;

    for (int x = 0; x < scanRange; x++) {
        for (int y = 0; y < scanRange; y++) {
            for (int z = 0; z < scanRange; z++) {
                VoxelData data;
                if (world.getBlock(x, y, z, data) && data.type != BlockType::AIR) {
                    snap.voxels[posHash(x, y, z)] = data;
                }
            }
        }
    }

    snapshotBuffer[snapshotIndex] = snap;
    snapshotIndex = (snapshotIndex + 1) % SNAPSHOT_BUFFER_SIZE;
    if (snapshotCount < SNAPSHOT_BUFFER_SIZE) snapshotCount++;
}

WorldSnapshot PhysicsEngine::saveSnapshot(const VoxelOctree& world) const {
    WorldSnapshot snap;
    snap.tick = currentTick;
    snap.agents = agents;

    for (int x = 0; x < scanRange; x++) {
        for (int y = 0; y < scanRange; y++) {
            for (int z = 0; z < scanRange; z++) {
                VoxelData data;
                if (world.getBlock(x, y, z, data) && data.type != BlockType::AIR) {
                    snap.voxels[posHash(x, y, z)] = data;
                }
            }
        }
    }

    return snap;
}

void PhysicsEngine::restoreSnapshot(VoxelOctree& world, const WorldSnapshot& snapshot) {
    currentTick = snapshot.tick;
    agents = snapshot.agents;

    for (auto& [hash, voxel] : snapshot.voxels) {
        int x = (hash >> 42) & 0x1FFFFF;
        int y = (hash >> 21) & 0x1FFFFF;
        int z = hash & 0x1FFFFF;
        if (x < scanRange && y < scanRange && z < scanRange) {
            world.setBlock(x, y, z, voxel.type, voxel.props);
            world.setBlockTemperature(x, y, z, voxel.currentTemperature);
            world.setBlockState(x, y, z, voxel.state);
        }
    }
}

bool PhysicsEngine::rewindTime(int ticks) {
    if (snapshotCount == 0) return false;

    uint64_t targetTick = currentTick - static_cast<uint64_t>(ticks);
    int bestIdx = -1;
    uint64_t bestDiff = UINT64_MAX;

    for (int i = 0; i < snapshotCount; i++) {
        uint64_t diff = (snapshotBuffer[i].tick > targetTick) ?
            (snapshotBuffer[i].tick - targetTick) : (targetTick - snapshotBuffer[i].tick);
        if (diff < bestDiff) {
            bestDiff = diff;
            bestIdx = i;
        }
    }

    if (bestIdx >= 0) {
        currentTick = snapshotBuffer[bestIdx].tick;
        agents = snapshotBuffer[bestIdx].agents;
        return true;
    }
    return false;
}

void PhysicsEngine::updateStats(const VoxelOctree& world) {
    stats.currentTick = currentTick;
    stats.totalBlocks = world.getBlockCount();

    int livingCount = 0;
    for (const auto& agent : agents) {
        if (agent.isAlive) livingCount++;
    }
    stats.livingEntities = livingCount;
}

bool PhysicsEngine::checkDetonationConditions(VoxelOctree& world, int x, int y, int z) {
    VoxelData data;
    if (!world.getBlock(x, y, z, data)) return false;
    if (data.type == BlockType::AIR) return false;
    if (data.props.chemical.explosivePower <= 0.0f) return false;
    if (data.currentTemperature < data.props.chemical.detonationTemperature) return false;
    return true;
}

void PhysicsEngine::triggerExplosion(VoxelOctree& world, int x, int y, int z) {
    VoxelData data;
    if (!world.getBlock(x, y, z, data)) return;
    if (data.props.chemical.explosivePower <= 0.0f) return;

    uint64_t key = posHash(x, y, z);
    if (explosionProcessed[key]) return;
    explosionProcessed[key] = true;

    float power = data.props.chemical.explosivePower;
    float radius = data.props.chemical.explosionRadius;
    if (radius <= 0.0f) radius = power * 0.15f;
    int radiusInt = static_cast<int>(std::ceil(radius));

    applyShockwave(world, x, y, z, power, radius);
    spawnFragments(world, x, y, z, static_cast<int>(power * 0.3f), power);

    world.setBlock(x, y, z, BlockType::AIR);

    for (int dx = -radiusInt; dx <= radiusInt; dx++) {
        for (int dy = -radiusInt; dy <= radiusInt; dy++) {
            for (int dz = -radiusInt; dz <= radiusInt; dz++) {
                if (dx == 0 && dy == 0 && dz == 0) continue;
                float dist = std::sqrt(static_cast<float>(dx*dx + dy*dy + dz*dz));
                if (dist > radius) continue;

                float heatDelta = power * 15.0f / (dist * dist + 1.0f);
                int tx = x + dx, ty = y + dy, tz = z + dz;
                VoxelData td;
                if (world.getBlock(tx, ty, tz, td) && td.type != BlockType::AIR) {
                    float newTemp = td.currentTemperature + heatDelta;
                    world.setBlockTemperature(tx, ty, tz, newTemp);
                }
            }
        }
    }

    int count = static_cast<int>(power * 0.5f);
    for (int i = 0; i < count; i++) {
        Particle p;
        p.x = static_cast<float>(x);
        p.y = static_cast<float>(y);
        p.z = static_cast<float>(z);
        std::uniform_real_distribution<float> dir(-1.0f, 1.0f);
        p.vx = dir(rng) * power * 0.1f;
        p.vy = std::abs(dir(rng)) * power * 0.1f;
        p.vz = dir(rng) * power * 0.1f;
        p.life = 1.0f;
        p.maxLife = 1.0f;
        p.size = 0.5f;
        p.type = "explosion";
        p.active = true;
        if (static_cast<int>(particles.size()) < MAX_PARTICLES) {
            particles.push_back(p);
        }
    }

    processChainReactions(world, x, y, z, power, 0);
}

void PhysicsEngine::applyShockwave(VoxelOctree& world, int x, int y, int z, float power, float radius) {
    int radiusInt = static_cast<int>(std::ceil(radius));

    for (int dx = -radiusInt; dx <= radiusInt; dx++) {
        for (int dy = -radiusInt; dy <= radiusInt; dy++) {
            for (int dz = -radiusInt; dz <= radiusInt; dz++) {
                if (dx == 0 && dy == 0 && dz == 0) continue;
                float dist = std::sqrt(static_cast<float>(dx*dx + dy*dy + dz*dz));
                if (dist > radius) continue;

                int tx = x + dx, ty = y + dy, tz = z + dz;
                VoxelData td;
                if (!world.getBlock(tx, ty, tz, td)) continue;
                if (td.type == BlockType::AIR) continue;

                float force = power / (dist * dist + 1.0f);

                float integrity = td.props.health.currentHealth / td.props.health.maxHealth;
                if (integrity < 0.01f) integrity = 0.01f;

                if (force > integrity * 100.0f) {
                    MaterialProps destroyedProps = td.props;
                    destroyedProps.health.currentHealth = 0.0f;
                    world.setBlock(tx, ty, tz, BlockType::AIR);
                } else {
                    float damage = force * 2.0f;
                    float newHealth = td.props.health.currentHealth - damage;
                    if (newHealth <= 0.0f) {
                        world.setBlock(tx, ty, tz, BlockType::AIR);
                    } else {
                        MaterialProps dmgProps = td.props;
                        dmgProps.health.currentHealth = newHealth;
                        world.setBlock(tx, ty, tz, td.type, dmgProps);
                        world.setBlockState(tx, ty, tz, td.state);
                        world.setBlockTemperature(tx, ty, tz, td.currentTemperature);
                    }
                }
            }
        }
    }
}

void PhysicsEngine::spawnFragments(VoxelOctree& world, int x, int y, int z, int count, float power) {
    std::uniform_real_distribution<float> dir(-1.0f, 1.0f);
    std::uniform_real_distribution<float> speedDist(0.5f, 1.5f);

    for (int i = 0; i < count && static_cast<int>(pendingFragments.size()) < 1024; i++) {
        Fragment frag;
        frag.x = x;
        frag.y = y;
        frag.z = z;
        float vx = dir(rng);
        float vy = std::abs(dir(rng));
        float vz = dir(rng);
        float len = std::sqrt(vx*vx + vy*vy + vz*vz);
        if (len < 0.01f) { vy = 1.0f; len = 1.0f; }
        float speed = power * 0.2f * speedDist(rng);
        frag.vx = (vx / len) * speed;
        frag.vy = (vy / len) * speed;
        frag.vz = (vz / len) * speed;
        frag.mass = 1.0f;
        frag.type = BlockType::STONE;
        pendingFragments.push_back(frag);
    }
}

void PhysicsEngine::processChainReactions(VoxelOctree& world, int x, int y, int z, float power, int depth) {
    static constexpr int MAX_CHAIN_DEPTH = 5;
    if (depth >= MAX_CHAIN_DEPTH) return;

    int radiusInt = 2;
    for (int dx = -radiusInt; dx <= radiusInt; dx++) {
        for (int dy = -radiusInt; dy <= radiusInt; dy++) {
            for (int dz = -radiusInt; dz <= radiusInt; dz++) {
                if (dx == 0 && dy == 0 && dz == 0) continue;
                int nx = x + dx, ny = y + dy, nz = z + dz;
                VoxelData nd;
                if (!world.getBlock(nx, ny, nz, nd)) continue;
                if (nd.type == BlockType::AIR) continue;
                if (nd.props.chemical.explosivePower <= 0.0f) continue;

                float dist = std::sqrt(static_cast<float>(dx*dx + dy*dy + dz*dz));
                float triggerForce = power / (dist * dist + 1.0f);
                float integrity = nd.props.health.currentHealth / nd.props.health.maxHealth;
                if (integrity < 0.01f) integrity = 0.01f;

                if (triggerForce > integrity * 50.0f) {
                    triggerExplosion(world, nx, ny, nz);
                }
            }
        }
    }
}

void PhysicsEngine::processPredatorPrey(VoxelOctree& world, float dt) {
    std::vector<Agent> newAgents;

    for (auto& agent : agents) {
        if (!agent.isAlive) continue;

        if (agent.isPredator) {
            agent.hunger += agent.hungerRate * dt;
            if (agent.hunger >= agent.maxHunger) {
                agent.health -= 2.0f * dt;
                if (agent.health <= 0.0f) {
                    agent.isAlive = false;
                    continue;
                }
            }
            huntPredator(world, agent, dt);
        }

        if (agent.isPrey) {
            agent.reproductionTimer += dt;
            fleePrey(world, agent, dt);
            reproducePrey(world, agent, dt, newAgents);
        }

        if (agent.attackCooldown > 0.0f) {
            agent.attackCooldown -= dt;
        }
    }

    for (auto& na : newAgents) {
        addAgent(na);
    }
}

void PhysicsEngine::huntPredator(VoxelOctree& world, Agent& predator, float dt) {
    Agent* nearestPrey = nullptr;
    float nearestDistSq = predator.visionRange * predator.visionRange;

    for (auto& other : agents) {
        if (&other == &predator || !other.isAlive || !other.isPrey) continue;
        float dx = other.x - predator.x;
        float dy = other.y - predator.y;
        float dz = other.z - predator.z;
        float distSq = dx * dx + dy * dy + dz * dz;
        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearestPrey = &other;
        }
    }

    if (nearestPrey) {
        float dx = nearestPrey->x - predator.x;
        float dy = nearestPrey->y - predator.y;
        float dz = nearestPrey->z - predator.z;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (dist < 1.5f) {
            if (predator.attackCooldown <= 0.0f) {
                float damage = predator.attackPower * (1.0f - nearestPrey->defensePower / 100.0f);
                if (damage < 1.0f) damage = 1.0f;
                nearestPrey->health -= damage;
                predator.attackCooldown = predator.attackCooldownMax;

                if (nearestPrey->health <= 0.0f) {
                    nearestPrey->isAlive = false;
                    predator.hunger -= 30.0f;
                    if (predator.hunger < 0.0f) predator.hunger = 0.0f;
                    predator.energy += 20.0f;
                    if (predator.energy > predator.maxEnergy) predator.energy = predator.maxEnergy;
                }
            }
        } else {
            float nx = dx / dist;
            float ny = dy / dist;
            float nz = dz / dist;
            predator.vx = nx * predator.speed;
            predator.vy = ny * predator.speed;
            predator.vz = nz * predator.speed;
        }
    } else {
        if (std::uniform_real_distribution<float>(0.0f, 1.0f)(rng) < 0.02f) {
            predator.vx = (std::uniform_real_distribution<float>(-1.0f, 1.0f)(rng)) * predator.speed;
            predator.vz = (std::uniform_real_distribution<float>(-1.0f, 1.0f)(rng)) * predator.speed;
        }
    }
}

void PhysicsEngine::fleePrey(VoxelOctree& world, Agent& prey, float dt) {
    Agent* nearestThreat = nullptr;
    float nearestDistSq = prey.dangerRange * prey.dangerRange;

    for (auto& other : agents) {
        if (&other == &prey || !other.isAlive || !other.isPredator) continue;
        float dx = other.x - prey.x;
        float dy = other.y - prey.y;
        float dz = other.z - prey.z;
        float distSq = dx * dx + dy * dy + dz * dz;
        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearestThreat = &other;
        }
    }

    if (nearestThreat) {
        float dx = prey.x - nearestThreat->x;
        float dy = prey.y - nearestThreat->y;
        float dz = prey.z - nearestThreat->z;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (dist > 0.01f) {
            float nx = dx / dist;
            float ny = dy / dist;
            float nz = dz / dist;
            prey.vx = nx * prey.speed * 1.2f;
            prey.vy = ny * prey.speed * 1.2f;
            prey.vz = nz * prey.speed * 1.2f;
        }
    } else {
        if (std::uniform_real_distribution<float>(0.0f, 1.0f)(rng) < 0.01f) {
            prey.vx = (std::uniform_real_distribution<float>(-1.0f, 1.0f)(rng)) * prey.speed * 0.5f;
            prey.vz = (std::uniform_real_distribution<float>(-1.0f, 1.0f)(rng)) * prey.speed * 0.5f;
        }
    }
}

void PhysicsEngine::reproducePrey(VoxelOctree& world, Agent& prey, float dt, std::vector<Agent>& newAgents) {
    if (prey.reproductionTimer < prey.reproductionInterval) return;
    if (prey.health < prey.maxHealth * 0.8f) return;
    if (prey.energy < prey.maxEnergy * 0.5f) return;
    if (static_cast<int>(agents.size() + newAgents.size()) >= MAX_AGENTS) return;

    bool hasNearbyPrey = false;
    for (const auto& other : agents) {
        if (&other == &prey || !other.isAlive || !other.isPrey) continue;
        float dx = other.x - prey.x;
        float dy = other.y - prey.y;
        float dz = other.z - prey.z;
        float distSq = dx * dx + dy * dy + dz * dz;
        if (distSq < 100.0f) {
            hasNearbyPrey = true;
            break;
        }
    }
    if (!hasNearbyPrey) return;

    Agent child;
    child.x = prey.x + (std::uniform_real_distribution<float>(-2.0f, 2.0f)(rng));
    child.y = prey.y + (std::uniform_real_distribution<float>(0.0f, 1.0f)(rng));
    child.z = prey.z + (std::uniform_real_distribution<float>(-2.0f, 2.0f)(rng));
    child.x = std::max(1.0f, std::min(254.0f, child.x));
    child.y = std::max(1.0f, std::min(254.0f, child.y));
    child.z = std::max(1.0f, std::min(254.0f, child.z));
    child.isPrey = true;
    child.isPredator = false;
    child.health = prey.maxHealth * 0.7f;
    child.maxHealth = prey.maxHealth;
    child.energy = prey.maxEnergy * 0.7f;
    child.maxEnergy = prey.maxEnergy;
    child.speed = prey.speed;
    child.attackPower = prey.attackPower;
    child.defensePower = prey.defensePower;
    child.visionRange = prey.visionRange;
    child.dangerRange = prey.dangerRange;
    child.hungerRate = prey.hungerRate;
    child.reproductionInterval = prey.reproductionInterval;

    newAgents.push_back(child);
    prey.reproductionTimer = 0.0f;
    prey.energy -= 15.0f;
    prey.health -= 5.0f;
}

void PhysicsEngine::processMetabolism(VoxelOctree& world, float dt) {
    for (auto& agent : agents) {
        if (!agent.isAlive) continue;

        agent.energy -= agent.energyDrainRate * dt;

        if (agent.isPredator) {
            agent.energy -= 0.02f * dt;
        }

        checkStarvation(agent, dt);

        if (agent.isAlive) {
            consumeFood(world, agent, dt);
        }
    }
}

void PhysicsEngine::consumeFood(VoxelOctree& world, Agent& agent, float dt) {
    if (agent.energy >= agent.maxEnergy) return;

    int ax = static_cast<int>(agent.x);
    int ay = static_cast<int>(agent.y);
    int az = static_cast<int>(agent.z);

    const int offsets[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};

    for (int i = 0; i < 6; i++) {
        int nx = ax + offsets[i][0];
        int ny = ay + offsets[i][1];
        int nz = az + offsets[i][2];

        VoxelData data;
        if (!world.getBlock(nx, ny, nz, data)) continue;
        if (data.type == BlockType::AIR) continue;

        bool isFood = false;
        float energyGain = 0.0f;

        if (data.type == BlockType::WOOD || data.type == BlockType::LEAVES) {
            isFood = true;
            energyGain = 15.0f;
        } else if (data.type == BlockType::CUSTOM && data.props.biological.isOrganic) {
            isFood = true;
            energyGain = agent.foodValue;
        } else if (data.type == BlockType::GRASS) {
            isFood = true;
            energyGain = 10.0f;
        }

        if (isFood && energyGain > 0.0f) {
            float actualGain = std::min(energyGain, agent.maxEnergy - agent.energy);
            agent.energy += actualGain;
            world.setBlock(nx, ny, nz, BlockType::AIR);
            return;
        }
    }
}

void PhysicsEngine::checkStarvation(Agent& agent, float dt) {
    if (agent.energy <= 0.0f) {
        agent.health -= 5.0f * dt;
        if (agent.health <= 0.0f) {
            agent.health = 0.0f;
            agent.isAlive = false;
        }
    } else if (agent.energy < agent.maxEnergy * 0.1f) {
        if (agent.speed > 0.5f) agent.speed = 0.5f;
    }
}

void PhysicsEngine::setTimeOfDay(float time) {
    currentTime = std::fmod(time, cycleDuration);
    if (currentTime < 0.0f) currentTime += cycleDuration;
}

float PhysicsEngine::getTimeOfDay() const {
    return currentTime;
}

void PhysicsEngine::setCycleDuration(float hours) {
    cycleDuration = hours;
}

float PhysicsEngine::getCycleDuration() const {
    return cycleDuration;
}

SunPosition PhysicsEngine::getSunPosition() const {
    SunPosition sun;
    float sunAngleRad = (currentTime / cycleDuration) * 2.0f * 3.14159265f - 3.14159265f / 2.0f;
    float halfCycle = cycleDuration / 2.0f;
    float normalizedTime = currentTime / cycleDuration;

    sun.angle = sunAngleRad;
    sun.x = sunOrbitRadius * std::cos(sunAngleRad);
    sun.y = sunOrbitRadius * std::sin(sunAngleRad);
    sun.z = 0.0f;
    sun.isAboveHorizon = sun.y > 0.0f;
    sun.isMoon = false;

    float sinVal = std::max(0.0f, std::sin(sunAngleRad));
    sun.intensity = sinVal;

    float elevation = sun.y / sunOrbitRadius;
    if (sun.isAboveHorizon) {
        float dawnDusk = 1.0f - elevation;
        sun.colorTemperature = 2700.0f + (6500.0f - 2700.0f) * (1.0f - dawnDusk * dawnDusk);
    } else {
        sun.colorTemperature = 2700.0f;
    }

    return sun;
}

SunPosition PhysicsEngine::getMoonPosition() const {
    SunPosition moon;
    float moonAngleRad = ((currentTime + cycleDuration / 2.0f) / cycleDuration) * 2.0f * 3.14159265f - 3.14159265f / 2.0f;

    moon.angle = moonAngleRad;
    moon.x = sunOrbitRadius * std::cos(moonAngleRad);
    moon.y = sunOrbitRadius * std::sin(moonAngleRad);
    moon.z = 0.0f;
    moon.isAboveHorizon = moon.y > 0.0f;
    moon.isMoon = true;

    float sinVal = std::max(0.0f, std::sin(moonAngleRad));
    moon.intensity = sinVal * 0.15f;

    moon.colorTemperature = 4100.0f;

    return moon;
}

float PhysicsEngine::getSunlightIntensity() const {
    SunPosition sun = getSunPosition();
    return sun.intensity;
}

float PhysicsEngine::getColorTemperature() const {
    SunPosition sun = getSunPosition();
    SunPosition moon = getMoonPosition();
    float sunContrib = sun.intensity;
    float moonContrib = moon.intensity;
    float total = sunContrib + moonContrib;
    if (total < 0.001f) return 4100.0f;
    return (sun.colorTemperature * sunContrib + moon.colorTemperature * moonContrib) / total;
}

float PhysicsEngine::getDaylightFactor() const {
    float intensity = getSunlightIntensity();
    return std::max(0.05f, intensity);
}

static Weather getWeatherPreset(WeatherType type) {
    Weather w;
    switch (type) {
        case WeatherType::CLEAR:
            w.precipitationRate = 0.0f;
            w.windSpeedX = 0.5f;
            w.windSpeedY = 0.0f;
            w.windSpeedZ = 0.3f;
            w.visibility = 1.0f;
            w.temperatureOffset = 0.0f;
            w.lightningChance = 0.0f;
            w.humidity = 0.3f;
            break;
        case WeatherType::RAIN:
            w.precipitationRate = 5.0f;
            w.windSpeedX = 2.0f;
            w.windSpeedY = 0.0f;
            w.windSpeedZ = 1.0f;
            w.visibility = 0.6f;
            w.temperatureOffset = -3.0f;
            w.lightningChance = 0.0f;
            w.humidity = 0.9f;
            break;
        case WeatherType::SNOW:
            w.precipitationRate = 3.0f;
            w.windSpeedX = 1.0f;
            w.windSpeedY = 0.0f;
            w.windSpeedZ = 0.5f;
            w.visibility = 0.5f;
            w.temperatureOffset = -10.0f;
            w.lightningChance = 0.0f;
            w.humidity = 0.7f;
            break;
        case WeatherType::STORM:
            w.precipitationRate = 10.0f;
            w.windSpeedX = 8.0f;
            w.windSpeedY = 0.0f;
            w.windSpeedZ = 4.0f;
            w.visibility = 0.3f;
            w.temperatureOffset = -5.0f;
            w.lightningChance = 0.1f;
            w.humidity = 0.95f;
            break;
        case WeatherType::FOG:
            w.precipitationRate = 0.5f;
            w.windSpeedX = 0.2f;
            w.windSpeedY = 0.0f;
            w.windSpeedZ = 0.1f;
            w.visibility = 0.2f;
            w.temperatureOffset = -1.0f;
            w.lightningChance = 0.0f;
            w.humidity = 0.95f;
            break;
    }
    return w;
}

void PhysicsEngine::setWeather(WeatherType type) {
    currentWeather.previousType = currentWeather.type;
    currentWeather.type = type;
    currentWeather.transitionTimer = currentWeather.transitionDuration;

    Weather preset = getWeatherPreset(type);
    currentWeather.precipitationRate = preset.precipitationRate;
    currentWeather.windSpeedX = preset.windSpeedX;
    currentWeather.windSpeedY = preset.windSpeedY;
    currentWeather.windSpeedZ = preset.windSpeedZ;
    currentWeather.visibility = preset.visibility;
    currentWeather.temperatureOffset = preset.temperatureOffset;
    currentWeather.lightningChance = preset.lightningChance;
    currentWeather.humidity = preset.humidity;
}

WeatherType PhysicsEngine::getWeatherType() const {
    return currentWeather.type;
}

const Weather& PhysicsEngine::getWeather() const {
    return currentWeather;
}

void PhysicsEngine::setWeatherTransition(WeatherType newType, float duration) {
    currentWeather.previousType = currentWeather.type;
    currentWeather.type = newType;
    currentWeather.transitionTimer = 0.0f;
    currentWeather.transitionDuration = duration;

    currentWeather.prevPrecipitationRate = currentWeather.precipitationRate;
    currentWeather.prevWindSpeedX = currentWeather.windSpeedX;
    currentWeather.prevWindSpeedY = currentWeather.windSpeedY;
    currentWeather.prevWindSpeedZ = currentWeather.windSpeedZ;
    currentWeather.prevVisibility = currentWeather.visibility;
    currentWeather.prevTemperatureOffset = currentWeather.temperatureOffset;
}

float PhysicsEngine::getPrecipitationRate() const {
    return currentWeather.precipitationRate;
}

float PhysicsEngine::getWindSpeed() const {
    return std::sqrt(currentWeather.windSpeedX * currentWeather.windSpeedX +
                     currentWeather.windSpeedY * currentWeather.windSpeedY +
                     currentWeather.windSpeedZ * currentWeather.windSpeedZ);
}

float PhysicsEngine::getVisibility() const {
    return currentWeather.visibility;
}

float PhysicsEngine::getSnowAccumulation() const {
    return currentWeather.snowAccumulation;
}

float PhysicsEngine::getRainAccumulation() const {
    return currentWeather.rainAccumulation;
}

void PhysicsEngine::transitionWeather(float dt) {
    if (currentWeather.transitionTimer >= currentWeather.transitionDuration) return;

    currentWeather.transitionTimer += dt;
    float t = std::min(1.0f, currentWeather.transitionTimer / currentWeather.transitionDuration);

    Weather target = getWeatherPreset(currentWeather.type);

    currentWeather.precipitationRate = currentWeather.prevPrecipitationRate +
        (target.precipitationRate - currentWeather.prevPrecipitationRate) * t;
    currentWeather.windSpeedX = currentWeather.prevWindSpeedX +
        (target.windSpeedX - currentWeather.prevWindSpeedX) * t;
    currentWeather.windSpeedY = currentWeather.prevWindSpeedY +
        (target.windSpeedY - currentWeather.prevWindSpeedY) * t;
    currentWeather.windSpeedZ = currentWeather.prevWindSpeedZ +
        (target.windSpeedZ - currentWeather.prevWindSpeedZ) * t;
    currentWeather.visibility = currentWeather.prevVisibility +
        (target.visibility - currentWeather.prevVisibility) * t;
    currentWeather.temperatureOffset = currentWeather.prevTemperatureOffset +
        (target.temperatureOffset - currentWeather.prevTemperatureOffset) * t;
    currentWeather.lightningChance = target.lightningChance;
    currentWeather.humidity = target.humidity;

    currentWeather.windForceX = currentWeather.windSpeedX * 0.5f;
    currentWeather.windForceY = 0.0f;
    currentWeather.windForceZ = currentWeather.windSpeedZ * 0.5f;
}

void PhysicsEngine::updateWeather(VoxelOctree& world, float dt) {
    transitionWeather(dt);

    weatherTimer += dt;
    if (weatherTimer >= weatherChangeInterval) {
        weatherTimer = 0.0f;
        int type = std::uniform_int_distribution<int>(0, 4)(rng);
        setWeatherTransition(static_cast<WeatherType>(type), 30.0f);
    }

    currentWeather.windForceX = currentWeather.windSpeedX * 0.5f;
    currentWeather.windForceY = 0.0f;
    currentWeather.windForceZ = currentWeather.windSpeedZ * 0.5f;

    if (currentWeather.precipitationRate > 0.0f) {
        int spawnCount = static_cast<int>(currentWeather.precipitationRate * dt * 10.0f);
        if (currentWeather.type == WeatherType::SNOW || currentWeather.type == WeatherType::FOG) {
            spawnSnowParticles(spawnCount, currentWeather.windSpeedX, currentWeather.windSpeedZ);
        } else {
            spawnRainParticles(spawnCount, currentWeather.windSpeedX, currentWeather.windSpeedZ);
        }
    }

    if (currentWeather.type == WeatherType::STORM && currentWeather.lightningChance > 0.0f) {
        if (std::uniform_real_distribution<float>(0.0f, 1.0f)(rng) < currentWeather.lightningChance * dt) {
            int lx = std::uniform_int_distribution<int>(0, scanRange - 1)(rng);
            int lz = std::uniform_int_distribution<int>(0, scanRange - 1)(rng);
            for (int ly = scanRange - 1; ly >= 0; ly--) {
                VoxelData vd;
                if (world.getBlock(lx, ly, lz, vd) && vd.type != BlockType::AIR) {
                    triggerExplosion(world, lx, ly, lz);
                    break;
                }
            }
        }
    }

    applyWindForce(world, dt);
    processWeatherAccumulation(world, dt);
}

void PhysicsEngine::spawnRainParticles(int count, float windX, float windZ) {
    for (int i = 0; i < count; i++) {
        Particle p;
        p.x = static_cast<float>(std::uniform_int_distribution<int>(0, scanRange - 1)(rng));
        p.y = static_cast<float>(scanRange);
        p.z = static_cast<float>(std::uniform_int_distribution<int>(0, scanRange - 1)(rng));
        p.vx = windX * 0.3f;
        p.vy = -8.0f;
        p.vz = windZ * 0.3f;
        p.life = 1.0f;
        p.maxLife = 1.0f;
        p.size = 0.2f;
        p.type = "rain";
        p.active = true;

        bool inserted = false;
        for (auto& slot : particles) {
            if (!slot.active) {
                slot = p;
                inserted = true;
                break;
            }
        }
        if (!inserted && static_cast<int>(particles.size()) < 2000) {
            particles.push_back(p);
        }
    }
}

void PhysicsEngine::spawnSnowParticles(int count, float windX, float windZ) {
    for (int i = 0; i < count; i++) {
        Particle p;
        p.x = static_cast<float>(std::uniform_int_distribution<int>(0, scanRange - 1)(rng));
        p.y = static_cast<float>(scanRange);
        p.z = static_cast<float>(std::uniform_int_distribution<int>(0, scanRange - 1)(rng));
        p.vx = windX * 0.5f + std::uniform_real_distribution<float>(-0.5f, 0.5f)(rng);
        p.vy = -2.0f;
        p.vz = windZ * 0.5f + std::uniform_real_distribution<float>(-0.5f, 0.5f)(rng);
        p.life = 1.0f;
        p.maxLife = 1.0f;
        p.size = 0.4f;
        p.type = "snow";
        p.active = true;

        bool inserted = false;
        for (auto& slot : particles) {
            if (!slot.active) {
                slot = p;
                inserted = true;
                break;
            }
        }
        if (!inserted && static_cast<int>(particles.size()) < 2000) {
            particles.push_back(p);
        }
    }
}

void PhysicsEngine::applyWindForce(VoxelOctree& world, float dt) {
    float fx = currentWeather.windForceX;
    float fy = currentWeather.windForceY;
    float fz = currentWeather.windForceZ;

    if (std::abs(fx) < 0.01f && std::abs(fz) < 0.01f) return;

    for (auto& agent : agents) {
        if (!agent.isAlive) continue;
        agent.vx += fx * dt * 0.1f;
        agent.vz += fz * dt * 0.1f;
    }

    for (auto& p : particles) {
        if (!p.active) continue;
        if (p.type == "leaf" || p.type == "debris" || p.type == "smoke") {
            p.vx += fx * dt * 0.5f;
            p.vz += fz * dt * 0.5f;
        }
    }

    for (int x = 0; x < scanRange; x++) {
        for (int z = 0; z < scanRange; z++) {
            for (int y = scanRange - 1; y >= 0; y--) {
                VoxelData data;
                if (!world.getBlock(x, y, z, data)) continue;
                if (data.type == BlockType::LEAVES) {
                    if (std::abs(fx) > 3.0f || std::abs(fz) > 3.0f) {
                        if (std::uniform_real_distribution<float>(0.0f, 1.0f)(rng) < 0.5f * dt) {
                            Particle leaf;
                            leaf.x = static_cast<float>(x);
                            leaf.y = static_cast<float>(y) + 1.0f;
                            leaf.z = static_cast<float>(z);
                            leaf.vx = fx * 0.3f;
                            leaf.vy = 0.5f;
                            leaf.vz = fz * 0.3f;
                            leaf.life = 5.0f;
                            leaf.maxLife = 5.0f;
                            leaf.size = 0.3f;
                            leaf.type = "leaf";
                            leaf.active = true;
                            for (auto& slot : particles) {
                                if (!slot.active) {
                                    slot = leaf;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void PhysicsEngine::processWeatherAccumulation(VoxelOctree& world, float dt) {
    if (currentWeather.type == WeatherType::RAIN || currentWeather.type == WeatherType::STORM) {
        float rate = currentWeather.precipitationRate * dt * 0.01f;
        currentWeather.rainAccumulation += rate;

        for (int x = 0; x < scanRange; x++) {
            for (int z = 0; z < scanRange; z++) {
                for (int y = scanRange - 1; y >= 0; y--) {
                    VoxelData data;
                    if (!world.getBlock(x, y, z, data)) continue;
                    if (data.type == BlockType::AIR) continue;

                    if (data.props.biological.growthRate > 0.0f) {
                        MaterialProps wetProps = data.props;
                        wetProps.biological.waterRequirement = 0.0f;
                        world.setBlock(x, y, z, data.type, wetProps);
                    }
                }
            }
        }
    }

    if (currentWeather.type == WeatherType::SNOW) {
        float rate = currentWeather.precipitationRate * dt * 0.005f;
        currentWeather.snowAccumulation += rate;

        for (int x = 0; x < scanRange; x++) {
            for (int z = 0; z < scanRange; z++) {
                for (int y = scanRange - 1; y >= 0; y--) {
                    VoxelData data;
                    if (!world.getBlock(x, y, z, data)) continue;
                    if (data.type == BlockType::AIR) {
                        if (currentWeather.snowAccumulation > 0.5f) {
                            MaterialProps snowProps;
                            snowProps.general.hardness = 0.2f;
                            snowProps.mechanical.tensileStrength = 0.5f;
                            snowProps.chemical.composition = "H2O";
                            snowProps.chemical.lightAbsorption = 0.05f;
                            world.setBlock(x, y, z, BlockType::SNOW, snowProps);
                            currentWeather.snowAccumulation -= 0.5f;
                        }
                        break;
                    }
                }
            }
        }
    }
}

float PhysicsEngine::calculateFeltTemperature(const Agent& agent) const {
    int bx = static_cast<int>(agent.x);
    int by = static_cast<int>(agent.y);
    int bz = static_cast<int>(agent.z);

    float blockTemp = ambientTemperature + currentWeather.temperatureOffset;

    float weatherWind = getWindSpeed();
    float physWind = std::sqrt(windX * windX + windY * windY + windZ * windZ);
    float windSpeed = std::max(weatherWind, physWind);

    float humidity = currentWeather.humidity;

    float windChill = 0.0f;
    if (windSpeed > 1.3f) {
        windChill = -0.5f * windSpeed * (1.0f - humidity * 0.3f);
    }

    float humidityEffect = 0.0f;
    if (blockTemp > 20.0f) {
        humidityEffect = humidity * (blockTemp - 20.0f) * 0.15f;
    } else if (blockTemp < 15.0f) {
        humidityEffect = -humidity * (15.0f - blockTemp) * 0.1f;
    }

    float felt = blockTemp + windChill + humidityEffect;
    return felt;
}

float PhysicsEngine::getComfortLevel(float temperature) const {
    float comfortMin = 15.0f;
    float comfortMax = 25.0f;

    if (temperature >= comfortMin && temperature <= comfortMax) {
        return 1.0f;
    }

    if (temperature < comfortMin) {
        float diff = comfortMin - temperature;
        if (temperature <= -10.0f) return 0.0f;
        return std::max(0.0f, 1.0f - diff / 25.0f);
    }

    float diff = temperature - comfortMax;
    if (temperature >= 40.0f) return 0.0f;
    return std::max(0.0f, 1.0f - diff / 15.0f);
}

void PhysicsEngine::processTemperaturePerception(VoxelOctree& world, float dt) {
    for (auto& agent : agents) {
        if (!agent.isAlive) continue;

        agent.feltTemperature = calculateFeltTemperature(agent);
        agent.comfortLevel = getComfortLevel(agent.feltTemperature);

        agent.isSeekingWarmth = agent.feltTemperature < 15.0f;
        agent.isSeekingShade = agent.feltTemperature > 25.0f;

        float speedMultiplier = 0.5f + agent.comfortLevel * 0.5f;
        agent.speed = speedMultiplier;

        if (agent.feltTemperature < -10.0f) {
            float severity = std::abs(agent.feltTemperature + 10.0f);
            float damage = severity * 0.5f * dt;
            agent.health -= damage;
            if (agent.health <= 0.0f) {
                agent.health = 0.0f;
                agent.isAlive = false;
            }
        } else if (agent.feltTemperature > 40.0f) {
            float severity = agent.feltTemperature - 40.0f;
            float damage = severity * 0.5f * dt;
            agent.health -= damage;
            if (agent.health <= 0.0f) {
                agent.health = 0.0f;
                agent.isAlive = false;
            }
        }

        if (agent.isSeekingWarmth) {
            float bestWarmth = -999.0f;
            int bestX = static_cast<int>(agent.x);
            int bestY = static_cast<int>(agent.y);
            int bestZ = static_cast<int>(agent.z);

            int range = static_cast<int>(agent.visionRange);
            for (int dx = -range; dx <= range; dx++) {
                for (int dy = -range; dy <= range; dy++) {
                    for (int dz = -range; dz <= range; dz++) {
                        int nx = static_cast<int>(agent.x) + dx;
                        int ny = static_cast<int>(agent.y) + dy;
                        int nz = static_cast<int>(agent.z) + dz;
                        if (nx < 0 || ny < 0 || nz < 0 || nx >= scanRange || ny >= scanRange || nz >= scanRange) continue;

                        VoxelData data;
                        if (!world.getBlock(nx, ny, nz, data)) continue;

                        float blockTemp = data.props.thermal.heatOutput;
                        if (blockTemp > bestWarmth) {
                            float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy + dz * dz));
                            if (dist <= agent.visionRange) {
                                bestWarmth = blockTemp;
                                bestX = nx;
                                bestY = ny;
                                bestZ = nz;
                            }
                        }
                    }
                }
            }

            if (bestWarmth > 0.0f) {
                float dirX = static_cast<float>(bestX) - agent.x;
                float dirY = static_cast<float>(bestY) - agent.y;
                float dirZ = static_cast<float>(bestZ) - agent.z;
                float len = std::sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
                if (len > 0.1f) {
                    agent.vx += (dirX / len) * agent.speed * dt;
                    agent.vy += (dirY / len) * agent.speed * dt;
                    agent.vz += (dirZ / len) * agent.speed * dt;
                }
            }
        }

        if (agent.isSeekingShade) {
            float bestShade = 999.0f;
            int bestX = static_cast<int>(agent.x);
            int bestY = static_cast<int>(agent.y);
            int bestZ = static_cast<int>(agent.z);

            int range = static_cast<int>(agent.visionRange);
            for (int dx = -range; dx <= range; dx++) {
                for (int dy = -range; dy <= range; dy++) {
                    for (int dz = -range; dz <= range; dz++) {
                        int nx = static_cast<int>(agent.x) + dx;
                        int ny = static_cast<int>(agent.y) + dy;
                        int nz = static_cast<int>(agent.z) + dz;
                        if (nx < 0 || ny < 0 || nz < 0 || nx >= scanRange || ny >= scanRange || nz >= scanRange) continue;

                        VoxelData data;
                        if (!world.getBlock(nx, ny, nz, data)) continue;

                        if (data.type == BlockType::LEAVES || data.type == BlockType::WOOD) {
                            float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy + dz * dz));
                            if (dist <= agent.visionRange && dist < bestShade) {
                                bestShade = dist;
                                bestX = nx;
                                bestY = ny;
                                bestZ = nz;
                            }
                        }
                    }
                }
            }

            if (bestShade < 999.0f) {
                float dirX = static_cast<float>(bestX) - agent.x;
                float dirY = static_cast<float>(bestY) - agent.y;
                float dirZ = static_cast<float>(bestZ) - agent.z;
                float len = std::sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
                if (len > 0.1f) {
                    agent.vx += (dirX / len) * agent.speed * dt;
                    agent.vy += (dirY / len) * agent.speed * dt;
                    agent.vz += (dirZ / len) * agent.speed * dt;
                }
            }
        }
    }
}

}
