#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
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
#include <map>
#include <mutex>

using namespace OpenMind;

static VoxelOctree* g_world = nullptr;
static PhysicsEngine* g_engine = nullptr;
static float g_cameraX = 0, g_cameraY = 10, g_cameraZ = 0;
static std::map<std::string, int> g_overlays;
static std::string g_lastExportCSV;
static std::string g_lastExportGLTF;
static std::mutex g_mutex;

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
    if (pos == std::string::npos || pos + 1 >= json.size()) return def;
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    if (pos >= json.size()) return def;
    try {
        if (json[pos] == '"') {
            pos++;
            size_t end = json.find('"', pos);
            if (end == std::string::npos || end == pos) return def;
            return std::stof(json.substr(pos, end - pos));
        }
        size_t end = pos;
        while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != ']') end++;
        if (end == pos) return def;
        return std::stof(json.substr(pos, end - pos));
    } catch (...) { return def; }
}

static std::string jsonGetString(const std::string& json, const std::string& key, const std::string& def) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return def;
    pos = json.find(':', pos);
    if (pos == std::string::npos || pos + 1 >= json.size()) return def;
    pos++;
    while (pos < json.size() && json[pos] == ' ') pos++;
    if (pos >= json.size() || json[pos] != '"') return def;
    pos++;
    size_t end = json.find('"', pos);
    if (end == std::string::npos || end == pos) return def;
    return json.substr(pos, end - pos);
}

static bool jsonGetBool(const std::string& json, const std::string& key, bool def) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return def;
    pos = json.find(':', pos);
    if (pos == std::string::npos || pos + 1 >= json.size()) return def;
    pos++;
    while (pos < json.size() && json[pos] == ' ') pos++;
    if (pos + 4 <= json.size() && json.substr(pos, 4) == "true") return true;
    if (pos + 5 <= json.size() && json.substr(pos, 5) == "false") return false;
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
    if (name == "ASH") return BlockType::ASH;
    if (name == "TNT") return BlockType::TNT;
    if (name == "SNOW") return BlockType::SNOW;
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
int setBlock(int x, int y, int z, int blockTypeInt, const char* propsJsonPtr) {
    if (!g_world) return 0;
    if (x < 0 || x > 255 || y < 0 || y > 255 || z < 0 || z > 255) return 0;
    if (blockTypeInt < 0 || blockTypeInt > 255) return 0;

    BlockType bt = static_cast<BlockType>(blockTypeInt);
    MaterialProps mp;

    std::string propsJson(propsJsonPtr ? propsJsonPtr : "");
    if (!propsJson.empty()) {
        mp.general.mass = jsonGetFloat(propsJson, "mass", mp.general.mass);
        mp.general.density = jsonGetFloat(propsJson, "density", mp.general.density);
        mp.general.hardness = jsonGetFloat(propsJson, "hardness", mp.general.hardness);
        mp.general.elasticity = jsonGetFloat(propsJson, "elasticity", mp.general.elasticity);
        mp.mechanical.tensileStrength = jsonGetFloat(propsJson, "tensileStrength", mp.mechanical.tensileStrength);
        mp.mechanical.compressiveStrength = jsonGetFloat(propsJson, "compressiveStrength", mp.mechanical.compressiveStrength);
        mp.mechanical.shearStrength = jsonGetFloat(propsJson, "shearStrength", mp.mechanical.shearStrength);
        mp.mechanical.fractureToughness = jsonGetFloat(propsJson, "fractureToughness", mp.mechanical.fractureToughness);
        mp.thermal.thermalConductivity = jsonGetFloat(propsJson, "thermalConductivity", mp.thermal.thermalConductivity);
        mp.thermal.specificHeat = jsonGetFloat(propsJson, "specificHeat", mp.thermal.specificHeat);
        mp.thermal.meltingPoint = jsonGetFloat(propsJson, "meltingPoint", mp.thermal.meltingPoint);
        mp.thermal.boilingPoint = jsonGetFloat(propsJson, "boilingPoint", mp.thermal.boilingPoint);
        mp.thermal.thermalSofteningPoint = jsonGetFloat(propsJson, "thermalSofteningPoint", mp.thermal.thermalSofteningPoint);
        mp.chemical.composition = jsonGetString(propsJson, "composition", mp.chemical.composition);
        mp.chemical.flammability = jsonGetFloat(propsJson, "flammability", mp.chemical.flammability);
        mp.chemical.combustionPoint = jsonGetFloat(propsJson, "combustionPoint", mp.chemical.combustionPoint);
        mp.chemical.corrosionRate = jsonGetFloat(propsJson, "corrosionRate", mp.chemical.corrosionRate);
        mp.chemical.chemicalResistance = jsonGetFloat(propsJson, "chemicalResistance", mp.chemical.chemicalResistance);
        mp.biological.isOrganic = jsonGetBool(propsJson, "isOrganic", mp.biological.isOrganic);
        mp.biological.isBiological = jsonGetBool(propsJson, "isBiological", mp.biological.isBiological);
        mp.biological.growthRate = jsonGetFloat(propsJson, "growthRate", mp.biological.growthRate);
        mp.biological.sunlightRequirement = jsonGetFloat(propsJson, "sunlightRequirement", mp.biological.sunlightRequirement);
        mp.biological.waterRequirement = jsonGetFloat(propsJson, "waterRequirement", mp.biological.waterRequirement);
        mp.electrical.conductivity = jsonGetFloat(propsJson, "conductivity", mp.electrical.conductivity);
        mp.electrical.resistivity = jsonGetFloat(propsJson, "resistivity", mp.electrical.resistivity);
        mp.visual.baseColor = jsonGetString(propsJson, "baseColor", mp.visual.baseColor);
        mp.visual.roughness = jsonGetFloat(propsJson, "roughness", mp.visual.roughness);
        mp.visual.metallicness = jsonGetFloat(propsJson, "metallicness", mp.visual.metallicness);
        mp.visual.opacity = jsonGetFloat(propsJson, "opacity", mp.visual.opacity);
        mp.visual.textureStyle = jsonGetString(propsJson, "textureStyle", mp.visual.textureStyle);
        mp.environmental.buoyancy = jsonGetFloat(propsJson, "buoyancy", mp.environmental.buoyancy);
        mp.environmental.permeability = jsonGetFloat(propsJson, "permeability", mp.environmental.permeability);
        mp.environmental.friction = jsonGetFloat(propsJson, "friction", mp.environmental.friction);
        mp.layering.layerAbove = jsonGetString(propsJson, "layerAbove", mp.layering.layerAbove);
        mp.layering.layerBelow = jsonGetString(propsJson, "layerBelow", mp.layering.layerBelow);
        mp.health.maxHealth = jsonGetFloat(propsJson, "maxHealth", mp.health.maxHealth);
        mp.health.currentHealth = jsonGetFloat(propsJson, "currentHealth", mp.health.currentHealth);
    }

    g_world->setBlock(x, y, z, bt, mp);
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int setBlockSimple(int x, int y, int z, int blockTypeInt) {
    if (!g_world) return 0;
    if (x < 0 || x > 255 || y < 0 || y > 255 || z < 0 || z > 255) return 0;
    if (blockTypeInt < 0 || blockTypeInt > 255) return 0;
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
int tickPhysicsDelta(float deltaSec) {
    if (!g_world || !g_engine) return 0;
    g_engine->tick(*g_world, deltaSec);
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int removeBlock(int x, int y, int z) {
    if (!g_world) return 0;
    if (x < 0 || x > 255 || y < 0 || y > 255 || z < 0 || z > 255) return 0;
    g_world->setBlock(x, y, z, BlockType::AIR);
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int setAgentPosition(int index, float x, float y, float z) {
    if (!g_engine) return 0;
    auto& agents = g_engine->getAgents();
    if (index < 0 || index >= (int)agents.size()) return 0;
    agents[index].x = x;
    agents[index].y = y;
    agents[index].z = z;
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int teleportCamera(float x, float y, float z) {
    g_cameraX = x;
    g_cameraY = y;
    g_cameraZ = z;
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int setOverlay(const char* typePtr, int enabled) {
    std::string type(typePtr ? typePtr : "");
    g_overlays[type] = enabled;
    return 1;
}

EMSCRIPTEN_KEEPALIVE
const char* saveWorld() {
    static std::string result;
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_world) { result = "{}"; return result.c_str(); }

    int count = g_world->getBlockCount();
    if (count == 0) {
        result = "{\"blocks\":[],\"agentCount\":" + intToStr(g_engine ? (int)g_engine->getAgents().size() : 0) + "}";
        return result.c_str();
    }

    result.reserve(count * 80 + 32);
    result = "{\"blocks\":[";
    bool first = true;
    g_world->traverse([&](const VoxelData& data, int x, int y, int z, int) -> bool {
        if (!data.occupied || data.type == BlockType::AIR) return true;
        if (!first) result += ",";
        first = false;
        result += "{\"x\":" + intToStr(x) + ",\"y\":" + intToStr(y) + ",\"z\":" + intToStr(z) + ",\"type\":" + intToStr(static_cast<int>(data.type)) + "}";
        return true;
    });
    result += "],\"agentCount\":" + intToStr(g_engine ? (int)g_engine->getAgents().size() : 0) + "}";
    return result.c_str();
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
    if (!g_world) {
        result = "{\"error\":\"no world\"}";
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
int rewindTime(float seconds) {
    if (!g_world || !g_engine) return 0;
    int ticks = (int)(seconds * 60.0f);
    return g_engine->rewindTime(ticks) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int saveSnapshot() {
    if (!g_world || !g_engine) return 0;
    g_engine->tick(*g_world, 0.0f);
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
const char* exportCSV(const char* filenamePtr) {
    static std::string result;
    std::string filename(filenamePtr ? filenamePtr : "");
    if (!g_world || filename.empty()) { result = "error: no world"; return result.c_str(); }

    std::lock_guard<std::mutex> lock(g_mutex);

    int count = g_world->getBlockCount();
    if (count == 0) {
        result = "{\"filename\":\"" + filename + "\",\"rows\":0,\"csvLength\":0}";
        return result.c_str();
    }

    std::string header = "x,y,z,blockType,mass,density,hardness,elasticity,tensileStrength,"
        "thermalConductivity,specificHeat,meltingPoint,boilingPoint,"
        "composition,flammability,combustionPoint,corrosionRate,"
        "conductivity,resistivity,baseColor,roughness,metallicness,"
        "opacity,buoyancy,friction,maxHealth,currentHealth\n";

    std::string csv = header;
    csv.reserve(count * 400 + header.size());
    int rowCount = 0;
    g_world->traverse([&](const VoxelData& data, int x, int y, int z, int) -> bool {
        if (!data.occupied || data.type == BlockType::AIR) return true;

        std::string row = intToStr(x) + "," + intToStr(y) + "," + intToStr(z) + ","
            + intToStr(static_cast<int>(data.type)) + ","
            + floatToStr(data.props.general.mass) + ","
            + floatToStr(data.props.general.density) + ","
            + floatToStr(data.props.general.hardness) + ","
            + floatToStr(data.props.general.elasticity) + ","
            + floatToStr(data.props.mechanical.tensileStrength) + ","
            + floatToStr(data.props.thermal.thermalConductivity) + ","
            + floatToStr(data.props.thermal.specificHeat) + ","
            + floatToStr(data.props.thermal.meltingPoint) + ","
            + floatToStr(data.props.thermal.boilingPoint) + ","
            + data.props.chemical.composition + ","
            + floatToStr(data.props.chemical.flammability) + ","
            + floatToStr(data.props.chemical.combustionPoint) + ","
            + floatToStr(data.props.chemical.corrosionRate) + ","
            + floatToStr(data.props.electrical.conductivity) + ","
            + floatToStr(data.props.electrical.resistivity) + ","
            + data.props.visual.baseColor + ","
            + floatToStr(data.props.visual.roughness) + ","
            + floatToStr(data.props.visual.metallicness) + ","
            + floatToStr(data.props.visual.opacity) + ","
            + floatToStr(data.props.environmental.buoyancy) + ","
            + floatToStr(data.props.environmental.friction) + ","
            + floatToStr(data.props.health.maxHealth) + ","
            + floatToStr(data.props.health.currentHealth) + "\n";
        csv += row;
        rowCount++;
        return true;
    });

    result = "{\"filename\":\"" + filename + "\",\"rows\":" + intToStr(rowCount) + ",\"csvLength\":" + intToStr((int)csv.size()) + "}";
    g_lastExportCSV = csv;
    return result.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* exportGLTF(const char* filenamePtr) {
    static std::string result;
    std::string filename(filenamePtr ? filenamePtr : "");
    if (!g_world || filename.empty()) { result = "error: no world"; return result.c_str(); }

    std::lock_guard<std::mutex> lock(g_mutex);

    int count = g_world->getBlockCount();
    if (count == 0) {
        result = "{\"filename\":\"" + filename + "\",\"vertices\":0,\"gltfLength\":0}";
        return result.c_str();
    }

    struct Vert { float x, y, z; };
    std::vector<Vert> vertices;
    std::vector<uint32_t> indices;
    vertices.reserve(count * 8);
    indices.reserve(count * 12);

    g_world->traverse([&](const VoxelData& data, int x, int y, int z, int) -> bool {
        if (!data.occupied || data.type == BlockType::AIR) return true;

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
        return true;
    });

    std::string gltf = "{\n";
    gltf += "  \"asset\": {\"version\": \"2.0\", \"generator\": \"OpenMind\"},\n";
    gltf += "  \"scene\": 0,\n";
    gltf += "  \"scenes\": [{\"nodes\": [0]}],\n";
    gltf += "  \"nodes\": [{\"mesh\": 0}],\n";
    gltf += "  \"meshes\": [{\"primitives\": [{\"attributes\": {\"POSITION\": 0}, \"indices\": 1}]}],\n";
    gltf += "  \"accessors\": [\n";
    gltf += "    {\"bufferView\": 0, \"componentType\": 5126, \"count\": " + intToStr((int)vertices.size()) + ", \"type\": \"VEC3\",";
    gltf += "     \"max\": [256,256,256], \"min\": [0,0,0]},\n";
    gltf += "    {\"bufferView\": 1, \"componentType\": 5125, \"count\": " + intToStr((int)indices.size()) + ", \"type\": \"SCALAR\"}\n";
    gltf += "  ],\n";
    gltf += "  \"bufferViews\": [\n";
    gltf += "    {\"buffer\": 0, \"byteOffset\": 0, \"byteLength\": " + intToStr((int)(vertices.size() * 12)) + ", \"target\": 34962},\n";
    gltf += "    {\"buffer\": 0, \"byteOffset\": " + intToStr((int)(vertices.size() * 12)) + ", \"byteLength\": " + intToStr((int)(indices.size() * 4)) + ", \"target\": 34963}\n";
    gltf += "  ],\n";
    gltf += "  \"buffers\": [{\"byteLength\": " + intToStr((int)(vertices.size() * 12 + indices.size() * 4)) + "}]\n";
    gltf += "}\n";

    g_lastExportGLTF = gltf;
    result = "{\"filename\":\"" + filename + "\",\"vertices\":" + intToStr((int)vertices.size()) + ",\"gltfLength\":" + intToStr((int)gltf.size()) + "}";
    return result.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* getLastExportCSV() {
    return g_lastExportCSV.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* getLastExportGLTF() {
    return g_lastExportGLTF.c_str();
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
    if (weatherType < 0 || weatherType > 4) return;
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
void setCycleDuration(float seconds) {
    if (!g_engine) return;
    float hours = seconds / 3600.0f;
    g_engine->setCycleDuration(hours);
}

EMSCRIPTEN_KEEPALIVE
int generateFromPrompt(const char* promptPtr) {
    if (!g_world) return 0;

    std::string p(promptPtr ? promptPtr : "");
    for (auto& c : p) c = tolower(c);

    int size = 10;
    if (p.find("small") != std::string::npos) size = 5;
    if (p.find("large") != std::string::npos) size = 20;
    if (p.find("big") != std::string::npos) size = 20;
    if (size < 1) size = 1;
    if (size > 64) size = 64;

    BlockType baseType = BlockType::STONE;
    if (p.find("dirt") != std::string::npos) baseType = BlockType::DIRT;
    if (p.find("grass") != std::string::npos) baseType = BlockType::GRASS;
    if (p.find("wood") != std::string::npos) baseType = BlockType::WOOD;
    if (p.find("sand") != std::string::npos) baseType = BlockType::SAND;
    if (p.find("iron") != std::string::npos) baseType = BlockType::IRON;
    if (p.find("gold") != std::string::npos) baseType = BlockType::GOLD;
    if (p.find("diamond") != std::string::npos) baseType = BlockType::DIAMOND;
    if (p.find("water") != std::string::npos) baseType = BlockType::WATER;
    if (p.find("snow") != std::string::npos) baseType = BlockType::SNOW;

    for (int x = 0; x < size; x++) {
        for (int z = 0; z < size; z++) {
            g_world->setBlock(x, 1, z, baseType);
        }
    }

    g_cameraX = size / 2.0f;
    g_cameraY = size + 5;
    g_cameraZ = size + 10;

    if (p.find("house") != std::string::npos || p.find("wall") != std::string::npos) {
        BlockType wallType = BlockType::WOOD;
        if (p.find("stone") != std::string::npos) wallType = BlockType::STONE;
        int h = 4;
        for (int i = 0; i < size; i++) {
            g_world->setBlock(i, 2, 0, wallType);
            g_world->setBlock(i, 3, 0, wallType);
            g_world->setBlock(i, 4, 0, wallType);
            g_world->setBlock(i, 2, size-1, wallType);
            g_world->setBlock(i, 3, size-1, wallType);
            g_world->setBlock(i, 4, size-1, wallType);
            g_world->setBlock(0, 2, i, wallType);
            g_world->setBlock(0, 3, i, wallType);
            g_world->setBlock(0, 4, i, wallType);
            g_world->setBlock(size-1, 2, i, wallType);
            g_world->setBlock(size-1, 3, i, wallType);
            g_world->setBlock(size-1, 4, i, wallType);
        }
    }

    return 1;
}

}
