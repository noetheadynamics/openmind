#include "openmind_engine.h"

namespace OpenMind {

OpenMindEngine::OpenMindEngine() {}

OpenMindEngine::~OpenMindEngine() {
    shutdown();
}

void OpenMindEngine::init() {
    if (initialized) return;
    engine.setGravity(9.81f);
    engine.setTimeScale(1.0f);
    engine.setTemperature(293.15f);
    engine.setScanRange(256);
    engine.setTimeOfDay(6.0f);
    engine.setCycleDuration(24.0f);
    initialized = true;
}

void OpenMindEngine::shutdown() {
    initialized = false;
}

void OpenMindEngine::tick(float dt) {
    if (!initialized) return;
    engine.tick(world, dt);
}

void OpenMindEngine::setBlock(int x, int y, int z, BlockType type, const MaterialProps& props) {
    world.setBlock(x, y, z, type, props);
}

void OpenMindEngine::setBlockSimple(int x, int y, int z, BlockType type) {
    world.setBlock(x, y, z, type);
}

bool OpenMindEngine::getBlock(int x, int y, int z, VoxelData& data) const {
    return world.getBlock(x, y, z, data);
}

void OpenMindEngine::setTimeScale(float scale) {
    engine.setTimeScale(scale);
}

float OpenMindEngine::getTimeScale() const {
    return engine.getTimeScale();
}

void OpenMindEngine::setGravity(float g) {
    engine.setGravity(g);
}

void OpenMindEngine::setTemperature(float ambient) {
    engine.setTemperature(ambient);
}

void OpenMindEngine::setWind(float wx, float wy, float wz) {
    engine.setWind(wx, wy, wz);
}

void OpenMindEngine::setScanRange(int range) {
    engine.setScanRange(range);
}

void OpenMindEngine::setWeather(WeatherType type) {
    engine.setWeather(type);
}

WeatherType OpenMindEngine::getWeatherType() const {
    return engine.getWeatherType();
}

float OpenMindEngine::getPrecipitationRate() const {
    return engine.getPrecipitationRate();
}

float OpenMindEngine::getWindSpeed() const {
    return engine.getWindSpeed();
}

float OpenMindEngine::getVisibility() const {
    return engine.getVisibility();
}

void OpenMindEngine::setTimeOfDay(float time) {
    engine.setTimeOfDay(time);
}

float OpenMindEngine::getTimeOfDay() const {
    return engine.getTimeOfDay();
}

void OpenMindEngine::setCycleDuration(float hours) {
    engine.setCycleDuration(hours);
}

float OpenMindEngine::getSunlightIntensity() const {
    return engine.getSunlightIntensity();
}

void OpenMindEngine::addAgent(const Agent& agent) {
    engine.addAgent(agent);
}

int OpenMindEngine::getAgentCount() const {
    return static_cast<int>(engine.getAgents().size());
}

const Agent& OpenMindEngine::getAgent(int index) const {
    static Agent emptyAgent{};
    const auto& agents = engine.getAgents();
    if (index < 0 || index >= static_cast<int>(agents.size())) return emptyAgent;
    return agents[index];
}

const WorldStats& OpenMindEngine::getStats() const {
    return engine.getStats();
}

bool OpenMindEngine::rewind(int ticks) {
    return engine.rewindTime(ticks);
}

VoxelOctree& OpenMindEngine::getWorld() { return world; }
PhysicsEngine& OpenMindEngine::getEngine() { return engine; }
const VoxelOctree& OpenMindEngine::getWorld() const { return world; }
const PhysicsEngine& OpenMindEngine::getEngine() const { return engine; }

} // namespace OpenMind