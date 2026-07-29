#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#include "VoxelOctree.h"
#include "PhysicsEngine.h"
#include "MaterialProperties.h"
#include <string>
#include <sstream>
#include <vector>
#include <cstring>

using namespace OpenMind;

static VoxelOctree* g_world = nullptr;
static PhysicsEngine* g_engine = nullptr;

static std::string floatToStr(float v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.4f", v);
    return std::string(buf);
}

static std::string intToStr(int v) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", v);
    return std::string(buf);
}

static std::string u64ToStr(uint64_t v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%llu", (unsigned long long)v);
    return std::string(buf);
}

static float jsonGetFloat(const std::string& json, const std::string& key, float def) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return def;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return def;
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    if (pos >= json.size()) return def;
    if (json[pos] == '"') {
        pos++;
        size_t end = json.find('"', pos);
        if (end == std::string::npos) return def;
        return std::stof(json.substr(pos, end - pos));
    }
    size_t end = pos;
    while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != ']') end++;
    return std::stof(json.substr(pos, end - pos));
}

static std::string jsonGetString(const std::string& json, const std::string& key, const std::string& def) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return def;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return def;
    pos++;
    while (pos < json.size() && json[pos] == ' ') pos++;
    if (pos >= json.size() || json[pos] != '"') return def;
    pos++;
    size_t end = json.find('"', pos);
    if (end == std::string::npos) return def;
    return json.substr(pos, end - pos);
}

static bool jsonGetBool(const std::string& json, const std::string& key, bool def) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return def;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return def;
    pos++;
    while (pos < json.size() && json[pos] == ' ') pos++;
    if (json.substr(pos, 4) == "true") return true;
    if (json.substr(pos, 5) == "false") return false;
    return def;
}

static int jsonGetInt(const std::string& json, const std::string& key, int def) {
    return static_cast<int>(jsonGetFloat(json, key, static_cast<float>(def)));
}

static BlockType blockTypeFromName(const std::string& name) {
    if (name == "DIRT") return BlockType::DIRT;
    if (name == "GRASS") return BlockType::GRASS;
    if (name == "STONE") return BlockType::STONE;
    if (name == "WOOD") return BlockType::WOOD;
    if (name == "LEAVES") return BlockType::LEAVES;
    if (name == "WATER") return BlockType::WATER;
    if (name == "SAND") return BlockType::SAND;
    if (name == "GLASS") return BlockType::GLASS;
    if (name == "STEEL") return BlockType::STEEL;
    if (name == "IRON") return BlockType::IRON;
    if (name == "COPPER") return BlockType::COPPER;
    if (name == "GOLD") return BlockType::GOLD;
    if (name == "DIAMOND") return BlockType::DIAMOND;
    if (name == "COAL") return BlockType::COAL;
    if (name == "BEDROCK") return BlockType::BEDROCK;
    return BlockType::CUSTOM;
}

extern "C" {

EMSCRIPTEN_KEEPALIVE
int initWorld() {
    if (g_world) delete g_world;
    if (g_engine) delete g_engine;
    g_world = new VoxelOctree();
    g_engine = new PhysicsEngine();
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int setBlock(int x, int y, int z, int blockTypeInt, const char* propsJson) {
    if (!g_world) return 0;
    if (x < 0 || x > 255 || y < 0 || y > 255 || z < 0 || z > 255) return 0;

    BlockType bt = static_cast<BlockType>(blockTypeInt);
    MaterialProps mp;

    if (propsJson && propsJson[0] != '\0') {
        std::string json(propsJson);
        mp.general.mass = jsonGetFloat(json, "mass", mp.general.mass);
        mp.general.density = jsonGetFloat(json, "density", mp.general.density);
        mp.general.hardness = jsonGetFloat(json, "hardness", mp.general.hardness);
        mp.general.elasticity = jsonGetFloat(json, "elasticity", mp.general.elasticity);
        mp.mechanical.tensileStrength = jsonGetFloat(json, "tensileStrength", mp.mechanical.tensileStrength);
        mp.mechanical.compressiveStrength = jsonGetFloat(json, "compressiveStrength", mp.mechanical.compressiveStrength);
        mp.mechanical.shearStrength = jsonGetFloat(json, "shearStrength", mp.mechanical.shearStrength);
        mp.mechanical.fractureToughness = jsonGetFloat(json, "fractureToughness", mp.mechanical.fractureToughness);
        mp.thermal.thermalConductivity = jsonGetFloat(json, "thermalConductivity", mp.thermal.thermalConductivity);
        mp.thermal.specificHeat = jsonGetFloat(json, "specificHeat", mp.thermal.specificHeat);
        mp.thermal.meltingPoint = jsonGetFloat(json, "meltingPoint", mp.thermal.meltingPoint);
        mp.thermal.boilingPoint = jsonGetFloat(json, "boilingPoint", mp.thermal.boilingPoint);
        mp.thermal.thermalSofteningPoint = jsonGetFloat(json, "thermalSofteningPoint", mp.thermal.thermalSofteningPoint);
        mp.chemical.composition = jsonGetString(json, "composition", mp.chemical.composition);
        mp.chemical.flammability = jsonGetFloat(json, "flammability", mp.chemical.flammability);
        mp.chemical.combustionPoint = jsonGetFloat(json, "combustionPoint", mp.chemical.combustionPoint);
        mp.chemical.corrosionRate = jsonGetFloat(json, "corrosionRate", mp.chemical.corrosionRate);
        mp.chemical.chemicalResistance = jsonGetFloat(json, "chemicalResistance", mp.chemical.chemicalResistance);
        mp.biological.isOrganic = jsonGetBool(json, "isOrganic", mp.biological.isOrganic);
        mp.biological.isBiological = jsonGetBool(json, "isBiological", mp.biological.isBiological);
        mp.biological.growthRate = jsonGetFloat(json, "growthRate", mp.biological.growthRate);
        mp.biological.sunlightRequirement = jsonGetFloat(json, "sunlightRequirement", mp.biological.sunlightRequirement);
        mp.biological.waterRequirement = jsonGetFloat(json, "waterRequirement", mp.biological.waterRequirement);
        mp.electrical.conductivity = jsonGetFloat(json, "conductivity", mp.electrical.conductivity);
        mp.electrical.resistivity = jsonGetFloat(json, "resistivity", mp.electrical.resistivity);
        mp.visual.baseColor = jsonGetString(json, "baseColor", mp.visual.baseColor);
        mp.visual.roughness = jsonGetFloat(json, "roughness", mp.visual.roughness);
        mp.visual.metallicness = jsonGetFloat(json, "metallicness", mp.visual.metallicness);
        mp.visual.opacity = jsonGetFloat(json, "opacity", mp.visual.opacity);
        mp.visual.textureStyle = jsonGetString(json, "textureStyle", mp.visual.textureStyle);
        mp.environmental.buoyancy = jsonGetFloat(json, "buoyancy", mp.environmental.buoyancy);
        mp.environmental.permeability = jsonGetFloat(json, "permeability", mp.environmental.permeability);
        mp.environmental.friction = jsonGetFloat(json, "friction", mp.environmental.friction);
        mp.layering.layerAbove = jsonGetString(json, "layerAbove", mp.layering.layerAbove);
        mp.layering.layerBelow = jsonGetString(json, "layerBelow", mp.layering.layerBelow);
        mp.health.maxHealth = jsonGetFloat(json, "maxHealth", mp.health.maxHealth);
        mp.health.currentHealth = jsonGetFloat(json, "currentHealth", mp.health.currentHealth);
    }

    g_world->setBlock(x, y, z, bt, mp);
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int setBlockSimple(int x, int y, int z, int blockTypeInt) {
    if (!g_world) return 0;
    if (x < 0 || x > 255 || y < 0 || y > 255 || z < 0 || z > 255) return 0;
    g_world->setBlock(x, y, z, static_cast<BlockType>(blockTypeInt));
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int tickPhysics() {
    if (!g_world || !g_engine) return 0;
    g_engine->tick(*g_world, 0.016f);
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int tickPhysicsDelta(float deltaMs) {
    if (!g_world || !g_engine) return 0;
    g_engine->tick(*g_world, deltaMs);
    return 1;
}

EMSCRIPTEN_KEEPALIVE
const char* getBlockData(int x, int y, int z) {
    static std::string result;
    if (!g_world || x < 0 || x > 255 || y < 0 || y > 255 || z < 0 || z > 255) {
        result = "{\"exists\":false}";
        return result.c_str();
    }

    VoxelData data;
    if (!g_world->getBlock(x, y, z, data) || !data.occupied) {
        result = "{\"exists\":false}";
        return result.c_str();
    }

    result = "{";
    result += "\"exists\":true,";
    result += "\"blockType\":" + intToStr(static_cast<int>(data.type)) + ",";
    result += "\"blockTypeName\":\"" + std::string(blockTypeToString(data.type)) + "\",";
    result += "\"mass\":" + floatToStr(data.props.general.mass) + ",";
    result += "\"density\":" + floatToStr(data.props.general.density) + ",";
    result += "\"hardness\":" + floatToStr(data.props.general.hardness) + ",";
    result += "\"elasticity\":" + floatToStr(data.props.general.elasticity) + ",";
    result += "\"tensileStrength\":" + floatToStr(data.props.mechanical.tensileStrength) + ",";
    result += "\"compressiveStrength\":" + floatToStr(data.props.mechanical.compressiveStrength) + ",";
    result += "\"thermalConductivity\":" + floatToStr(data.props.thermal.thermalConductivity) + ",";
    result += "\"specificHeat\":" + floatToStr(data.props.thermal.specificHeat) + ",";
    result += "\"meltingPoint\":" + floatToStr(data.props.thermal.meltingPoint) + ",";
    result += "\"boilingPoint\":" + floatToStr(data.props.thermal.boilingPoint) + ",";
    result += "\"composition\":\"" + data.props.chemical.composition + "\",";
    result += "\"flammability\":" + floatToStr(data.props.chemical.flammability) + ",";
    result += "\"combustionPoint\":" + floatToStr(data.props.chemical.combustionPoint) + ",";
    result += "\"corrosionRate\":" + floatToStr(data.props.chemical.corrosionRate) + ",";
    result += "\"chemicalResistance\":" + floatToStr(data.props.chemical.chemicalResistance) + ",";
    result += "\"isOrganic\":" + std::string(data.props.biological.isOrganic ? "true" : "false") + ",";
    result += "\"isBiological\":" + std::string(data.props.biological.isBiological ? "true" : "false") + ",";
    result += "\"growthRate\":" + floatToStr(data.props.biological.growthRate) + ",";
    result += "\"conductivity\":" + floatToStr(data.props.electrical.conductivity) + ",";
    result += "\"resistivity\":" + floatToStr(data.props.electrical.resistivity) + ",";
    result += "\"baseColor\":\"" + data.props.visual.baseColor + "\",";
    result += "\"roughness\":" + floatToStr(data.props.visual.roughness) + ",";
    result += "\"metallicness\":" + floatToStr(data.props.visual.metallicness) + ",";
    result += "\"opacity\":" + floatToStr(data.props.visual.opacity) + ",";
    result += "\"textureStyle\":\"" + data.props.visual.textureStyle + "\",";
    result += "\"buoyancy\":" + floatToStr(data.props.environmental.buoyancy) + ",";
    result += "\"permeability\":" + floatToStr(data.props.environmental.permeability) + ",";
    result += "\"friction\":" + floatToStr(data.props.environmental.friction) + ",";
    result += "\"maxHealth\":" + floatToStr(data.props.health.maxHealth) + ",";
    result += "\"currentHealth\":" + floatToStr(data.props.health.currentHealth);
    result += "}";

    return result.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* getWorldStats() {
    static std::string result;
    if (!g_engine) {
        result = "{\"error\":\"no engine\"}";
        return result.c_str();
    }

    const WorldStats& s = g_engine->getStats();
    result = "{";
    result += "\"totalBlocks\":" + intToStr(s.totalBlocks) + ",";
    result += "\"waterBlocks\":" + intToStr(s.waterBlocks) + ",";
    result += "\"fireBlocks\":" + intToStr(s.fireBlocks) + ",";
    result += "\"livingEntities\":" + intToStr(s.livingEntities) + ",";
    result += "\"averageTemperature\":" + floatToStr(s.averageTemperature) + ",";
    result += "\"timeScale\":" + floatToStr(s.timeScale) + ",";
    result += "\"currentTick\":" + u64ToStr(s.currentTick);
    result += "}";

    return result.c_str();
}

EMSCRIPTEN_KEEPALIVE
void setTimeScale(float scale) {
    if (g_engine) g_engine->setTimeScale(scale);
}

EMSCRIPTEN_KEEPALIVE
void setGravity(float g) {
    if (g_engine) g_engine->setGravity(g);
}

EMSCRIPTEN_KEEPALIVE
void setAmbientTemperature(float temp) {
    if (g_engine) g_engine->setTemperature(temp);
}

EMSCRIPTEN_KEEPALIVE
void setWind(float wx, float wy, float wz) {
    if (g_engine) g_engine->setWind(wx, wy, wz);
}

EMSCRIPTEN_KEEPALIVE
int rewindTime(int ticks) {
    if (!g_world || !g_engine) return 0;
    return g_engine->rewindTime(ticks) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int saveSnapshot() {
    if (!g_world || !g_engine) return 0;
    return 1;
}

EMSCRIPTEN_KEEPALIVE
const char* getPendingFragments() {
    static std::string result;
    if (!g_engine) {
        result = "[]";
        return result.c_str();
    }

    auto frags = g_engine->getPendingFragments();
    result = "[";
    for (size_t i = 0; i < frags.size(); i++) {
        if (i > 0) result += ",";
        result += "{";
        result += "\"x\":" + intToStr(frags[i].x) + ",";
        result += "\"y\":" + intToStr(frags[i].y) + ",";
        result += "\"z\":" + intToStr(frags[i].z) + ",";
        result += "\"vx\":" + floatToStr(frags[i].vx) + ",";
        result += "\"vy\":" + floatToStr(frags[i].vy) + ",";
        result += "\"vz\":" + floatToStr(frags[i].vz) + ",";
        result += "\"mass\":" + floatToStr(frags[i].mass) + ",";
        result += "\"type\":" + intToStr(static_cast<int>(frags[i].type));
        result += "}";
    }
    result += "]";

    g_engine->clearPendingFragments();
    return result.c_str();
}

EMSCRIPTEN_KEEPALIVE
int addAgent(float x, float y, float z, int isPredator) {
    if (!g_engine) return 0;
    Agent a;
    a.x = x;
    a.y = y;
    a.z = z;
    a.isPredator = isPredator != 0;
    g_engine->addAgent(a);
    return 1;
}

EMSCRIPTEN_KEEPALIVE
const char* getAgentData(int index) {
    static std::string result;
    if (!g_engine || index < 0 || index >= static_cast<int>(g_engine->getAgents().size())) {
        result = "{\"exists\":false}";
        return result.c_str();
    }

    const Agent& a = g_engine->getAgents()[index];
    result = "{";
    result += "\"exists\":true,";
    result += "\"id\":" + intToStr(a.id) + ",";
    result += "\"x\":" + floatToStr(a.x) + ",";
    result += "\"y\":" + floatToStr(a.y) + ",";
    result += "\"z\":" + floatToStr(a.z) + ",";
    result += "\"vx\":" + floatToStr(a.vx) + ",";
    result += "\"vy\":" + floatToStr(a.vy) + ",";
    result += "\"vz\":" + floatToStr(a.vz) + ",";
    result += "\"energy\":" + floatToStr(a.energy) + ",";
    result += "\"isAlive\":" + std::string(a.isAlive ? "true" : "false") + ",";
    result += "\"isPredator\":" + std::string(a.isPredator ? "true" : "false") + ",";
    result += "\"isDiseased\":" + std::string(a.isDiseased ? "true" : "false") + ",";
    result += "\"health\":" + floatToStr(a.health);
    result += "}";

    return result.c_str();
}

EMSCRIPTEN_KEEPALIVE
int getAgentCount() {
    if (!g_engine) return 0;
    return static_cast<int>(g_engine->getAgents().size());
}

EMSCRIPTEN_KEEPALIVE
int exportCSV(const char* filename) {
    if (!g_world || !filename) return 0;

    std::string path(filename);
    std::ofstream out(path);
    if (!out.is_open()) return 0;

    out << "x,y,z,blockType,mass,density,hardness,elasticity,tensileStrength,";

    out << "thermalConductivity,specificHeat,meltingPoint,boilingPoint,";

    out << "composition,flammability,combustionPoint,corrosionRate,";

    out << "conductivity,resistivity,baseColor,roughness,metallicness,";

    out << "opacity,buoyancy,friction,maxHealth,currentHealth" << std::endl;

    for (int x = 0; x < 256; x++) {
        for (int y = 0; y < 256; y++) {
            for (int z = 0; z < 256; z++) {
                VoxelData data;
                if (!g_world->getBlock(x, y, z, data) || !data.occupied) continue;
                if (data.type == BlockType::AIR) continue;

                out << x << "," << y << "," << z << ","
                    << static_cast<int>(data.type) << ","
                    << data.props.general.mass << ","
                    << data.props.general.density << ","
                    << data.props.general.hardness << ","
                    << data.props.general.elasticity << ","
                    << data.props.mechanical.tensileStrength << ","
                    << data.props.thermal.thermalConductivity << ","
                    << data.props.thermal.specificHeat << ","
                    << data.props.thermal.meltingPoint << ","
                    << data.props.thermal.boilingPoint << ","
                    << data.props.chemical.composition << ","
                    << data.props.chemical.flammability << ","
                    << data.props.chemical.combustionPoint << ","
                    << data.props.chemical.corrosionRate << ","
                    << data.props.electrical.conductivity << ","
                    << data.props.electrical.resistivity << ","
                    << data.props.visual.baseColor << ","
                    << data.props.visual.roughness << ","
                    << data.props.visual.metallicness << ","
                    << data.props.visual.opacity << ","
                    << data.props.environmental.buoyancy << ","
                    << data.props.environmental.friction << ","
                    << data.props.health.maxHealth << ","
                    << data.props.health.currentHealth << std::endl;
            }
        }
    }

    return out.good() ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int exportGLTF(const char* filename) {
    if (!g_world || !filename) return 0;

    std::string path(filename);
    if (path.find(".gltf") == std::string::npos) path += ".gltf";
    std::ofstream out(path);
    if (!out.is_open()) return 0;

    struct Vert { float x, y, z; };
    std::vector<Vert> vertices;
    std::vector<uint32_t> indices;

    for (int x = 0; x < 256; x++) {
        for (int y = 0; y < 256; y++) {
            for (int z = 0; z < 256; z++) {
                VoxelData data;
                if (!g_world->getBlock(x, y, z, data) || !data.occupied) continue;
                if (data.type == BlockType::AIR) continue;

                uint32_t base = static_cast<uint32_t>(vertices.size());
                float bx = static_cast<float>(x);
                float by = static_cast<float>(y);
                float bz = static_cast<float>(z);

                vertices.push_back({bx, by, bz});
                vertices.push_back({bx+1, by, bz});
                vertices.push_back({bx+1, by+1, bz});
                vertices.push_back({bx, by+1, bz});
                vertices.push_back({bx, by, bz+1});
                vertices.push_back({bx+1, by, bz+1});
                vertices.push_back({bx+1, by+1, bz+1});
                vertices.push_back({bx, by+1, bz+1});

                uint32_t faces[12] = {
                    base,base+1,base+2, base,base+2,base+3,
                    base+4,base+6,base+5, base+4,base+7,base+6
                };
                for (int f = 0; f < 12; f++) indices.push_back(faces[f]);
            }
        }
    }

    out << "{\n";
    out << "  \"asset\": {\"version\": \"2.0\", \"generator\": \"OpenMind\"},\n";
    out << "  \"scene\": 0,\n";
    out << "  \"scenes\": [{\"nodes\": [0]}],\n";
    out << "  \"nodes\": [{\"mesh\": 0}],\n";
    out << "  \"meshes\": [{\"primitives\": [{\"attributes\": {\"POSITION\": 0}, \"indices\": 1}]}],\n";
    out << "  \"accessors\": [\n";
    out << "    {\"bufferView\": 0, \"componentType\": 5126, \"count\": " << vertices.size() << ", \"type\": \"VEC3\",";
    out << "     \"max\": [256,256,256], \"min\": [0,0,0]},\n";
    out << "    {\"bufferView\": 1, \"componentType\": 5125, \"count\": " << indices.size() << ", \"type\": \"SCALAR\"}\n";
    out << "  ],\n";
    out << "  \"bufferViews\": [\n";
    out << "    {\"buffer\": 0, \"byteOffset\": 0, \"byteLength\": " << (vertices.size() * 12) << ", \"target\": 34962},\n";
    out << "    {\"buffer\": 0, \"byteOffset\": " << (vertices.size() * 12) << ", \"byteLength\": " << (indices.size() * 4) << ", \"target\": 34963}\n";
    out << "  ],\n";
    out << "  \"buffers\": [{\"byteLength\": " << (vertices.size() * 12 + indices.size() * 4) << "}]\n";
    out << "}\n";

    return out.good() ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
void cleanup() {
    delete g_world;
    delete g_engine;
    g_world = nullptr;
    g_engine = nullptr;
}

EMSCRIPTEN_KEEPALIVE
void setWeather(int weatherType) {
    if (!g_engine) return;
    g_engine->setWeather(static_cast<WeatherType>(weatherType));
}

EMSCRIPTEN_KEEPALIVE
int getWeatherType() {
    if (!g_engine) return 0;
    return static_cast<int>(g_engine->getWeatherType());
}

EMSCRIPTEN_KEEPALIVE
const char* getWeather() {
    static std::string result;
    if (!g_engine) {
        result = "{\"error\":\"no engine\"}";
        return result.c_str();
    }

    const Weather& w = g_engine->getWeather();
    result = "{";
    result += "\"type\":" + intToStr(static_cast<int>(w.type)) + ",";
    result += "\"precipitationRate\":" + floatToStr(w.precipitationRate) + ",";
    result += "\"windSpeedX\":" + floatToStr(w.windSpeedX) + ",";
    result += "\"windSpeedY\":" + floatToStr(w.windSpeedY) + ",";
    result += "\"windSpeedZ\":" + floatToStr(w.windSpeedZ) + ",";
    result += "\"visibility\":" + floatToStr(w.visibility) + ",";
    result += "\"temperatureOffset\":" + floatToStr(w.temperatureOffset) + ",";
    result += "\"humidity\":" + floatToStr(w.humidity) + ",";
    result += "\"lightningChance\":" + floatToStr(w.lightningChance);
    result += "}";

    return result.c_str();
}

EMSCRIPTEN_KEEPALIVE
void setTimeOfDay(float hours) {
    if (!g_engine) return;
    g_engine->setTimeOfDay(hours);
}

EMSCRIPTEN_KEEPALIVE
float getTimeOfDay() {
    if (!g_engine) return 0.0f;
    return g_engine->getTimeOfDay();
}

EMSCRIPTEN_KEEPALIVE
const char* getSunPosition() {
    static std::string result;
    if (!g_engine) {
        result = "{\"error\":\"no engine\"}";
        return result.c_str();
    }

    SunPosition sun = g_engine->getSunPosition();
    result = "{";
    result += "\"angle\":" + floatToStr(sun.angle) + ",";
    result += "\"x\":" + floatToStr(sun.x) + ",";
    result += "\"y\":" + floatToStr(sun.y) + ",";
    result += "\"intensity\":" + floatToStr(sun.intensity) + ",";
    result += "\"colorTemperature\":" + floatToStr(sun.colorTemperature) + ",";
    result += "\"isAboveHorizon\":" + std::string(sun.isAboveHorizon ? "true" : "false");
    result += "}";

    return result.c_str();
}

EMSCRIPTEN_KEEPALIVE
float getSunlightIntensity() {
    if (!g_engine) return 0.0f;
    return g_engine->getSunlightIntensity();
}

EMSCRIPTEN_KEEPALIVE
void setCycleDuration(float hours) {
    if (!g_engine) return;
    g_engine->setCycleDuration(hours);
}

}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(openmind) {
    emscripten::function("initWorld", &initWorld);
    emscripten::function("setBlock", &setBlock);
    emscripten::function("setBlockSimple", &setBlockSimple);
    emscripten::function("tickPhysics", &tickPhysics);
    emscripten::function("tickPhysicsDelta", &tickPhysicsDelta);
    emscripten::function("getBlockData", &getBlockData);
    emscripten::function("getWorldStats", &getWorldStats);
    emscripten::function("setTimeScale", &setTimeScale);
    emscripten::function("setGravity", &setGravity);
    emscripten::function("setAmbientTemperature", &setAmbientTemperature);
    emscripten::function("setWind", &setWind);
    emscripten::function("rewindTime", &rewindTime);
    emscripten::function("saveSnapshot", &saveSnapshot);
    emscripten::function("getPendingFragments", &getPendingFragments);
    emscripten::function("addAgent", &addAgent);
    emscripten::function("getAgentData", &getAgentData);
    emscripten::function("getAgentCount", &getAgentCount);
    emscripten::function("exportCSV", &exportCSV);
    emscripten::function("exportGLTF", &exportGLTF);
    emscripten::function("cleanup", &cleanup);
    emscripten::function("setWeather", &setWeather);
    emscripten::function("getWeatherType", &getWeatherType);
    emscripten::function("getWeather", &getWeather);
    emscripten::function("setTimeOfDay", &setTimeOfDay);
    emscripten::function("getTimeOfDay", &getTimeOfDay);
    emscripten::function("getSunPosition", &getSunPosition);
    emscripten::function("getSunlightIntensity", &getSunlightIntensity);
    emscripten::function("setCycleDuration", &setCycleDuration);
}
#endif
