#pragma once

#include "VoxelOctree.h"
#include "PhysicsEngine.h"
#include <string>

namespace OpenMind {

class OpenMindEngine {
public:
    OpenMindEngine();
    ~OpenMindEngine();

    void init();
    void shutdown();

    void tick(float dt);

    void setBlock(int x, int y, int z, BlockType type, const MaterialProps& props);
    void setBlockSimple(int x, int y, int z, BlockType type);
    bool getBlock(int x, int y, int z, VoxelData& data) const;

    void setTimeScale(float scale);
    float getTimeScale() const;
    void setGravity(float g);
    void setTemperature(float ambient);
    void setWind(float wx, float wy, float wz);
    void setScanRange(int range);

    void setWeather(WeatherType type);
    WeatherType getWeatherType() const;
    float getPrecipitationRate() const;
    float getWindSpeed() const;
    float getVisibility() const;

    void setTimeOfDay(float time);
    float getTimeOfDay() const;
    void setCycleDuration(float hours);
    float getSunlightIntensity() const;

    void addAgent(const Agent& agent);
    int getAgentCount() const;
    const Agent& getAgent(int index) const;

    const WorldStats& getStats() const;

    bool rewind(int ticks);

    VoxelOctree& getWorld();
    PhysicsEngine& getEngine();
    const VoxelOctree& getWorld() const;
    const PhysicsEngine& getEngine() const;

private:
    VoxelOctree world;
    PhysicsEngine engine;
    bool initialized = false;
};

} // namespace OpenMind