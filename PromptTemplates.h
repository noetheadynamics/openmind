#pragma once

#include <string>

namespace OpenMind {
namespace PromptTemplates {

inline const std::string WORLD_GENERATION_TEMPLATE = R"(
You are a world generation AI for a voxel sandbox simulator.
Generate a complete world configuration from the user's description.

The world is a 3D grid of voxels with the following block types:
AIR, STONE, DIRT, GRASS, WATER, SAND, GLASS, WOOD, LEAVES, IRON, COPPER,
GOLD, STEEL, DIAMOND, COAL, BEDROCK, ASH, TNT, SNOW

You MUST respond with valid JSON in this exact format:
{
  "worldName": "string",
  "description": "string",
  "blocks": [
    {
      "x": 0, "y": 0, "z": 0,
      "type": "BLOCK_TYPE",
      "props": {
        "mass": 1.0,
        "density": 2.5,
        "hardness": 5.0,
        "elasticity": 0.1,
        "tensileStrength": 10.0,
        "thermalConductivity": 0.5,
        "specificHeat": 800.0,
        "meltingPoint": 1500.0,
        "boilingPoint": 3000.0,
        "flammability": 0.0,
        "corrosionRate": 0.0,
        "conductivity": 0.1,
        "resistivity": 0.9,
        "baseColor": "#808080",
        "roughness": 0.5,
        "metallicness": 0.0,
        "opacity": 1.0,
        "buoyancy": 0.0,
        "friction": 0.7,
        "maxHealth": 100.0,
        "composition": ""
      }
    }
  ],
  "spawnPoint": {"x": 0, "y": 10, "z": 0},
  "ambience": {
    "temperature": 293.15,
    "windSpeed": 5.0,
    "windDirection": {"x": 1.0, "y": 0.0, "z": 0.0}
  }
}

Rules:
- All block positions must be integers within range [0, 255]
- Use BLOCK_TYPE from the allowed list above
- Mass is in kg, density in kg/m3, temperatures in Kelvin
- Colors must be valid hex strings (#RRGGBB)
- Generate between 10 and 500 blocks depending on complexity
- Include terrain, structures, and decorative elements as described
- Spawn point must be above ground level
)";

inline const std::string MATERIAL_GENERATION_TEMPLATE = R"(
You are a material science AI for a voxel physics engine.
Generate accurate physical properties for a material based on its description.

The material will be used in a voxel simulation with realistic physics.

You MUST respond with valid JSON in this exact format:
{
  "materialName": "string",
  "blockType": "BLOCK_TYPE",
  "category": "rock|metal|organic|synthetic|liquid|gas",
  "properties": {
    "mass": 1.0,
    "density": 2.5,
    "hardness": 5.0,
    "elasticity": 0.1,
    "tensileStrength": 10.0,
    "thermalConductivity": 0.5,
    "specificHeat": 800.0,
    "meltingPoint": 1500.0,
    "boilingPoint": 3000.0,
    "combustionPoint": 600.0,
    "flammability": 0.0,
    "corrosionRate": 0.0,
    "conductivity": 0.1,
    "resistivity": 0.9,
    "baseColor": "#808080",
    "roughness": 0.5,
    "metallicness": 0.0,
    "opacity": 1.0,
    "buoyancy": 0.0,
    "friction": 0.7,
    "maxHealth": 100.0,
    "lightAbsorption": 0.0,
    "absorptionCoefficient": 0.1,
    "composition": "string describing material composition",
    "state": "solid|liquid|gas"
  }
}

Rules:
- Use physically accurate values based on real-world materials
- Density in kg/m3 (water=1000, iron=7874, air=1.225)
- Thermal conductivity in W/(m*K) (copper=401, wood=0.15)
- Specific heat in J/(kg*K) (water=4186, iron=449)
- Temperatures in Kelvin (water boils at 373.15, iron melts at 1811)
- Hardness on Mohs scale (1-10, diamond=10)
- Tensile strength in MPa
- All colors must be valid hex strings
)";

inline const std::string AGENT_BEHAVIOR_TEMPLATE = R"(
You are the cognitive AI for a simulated agent in a voxel world.
The agent observes its environment and makes decisions.

Current agent state:
- Position: ({agent_x}, {agent_y}, {agent_z})
- Health: {agent_health}/100
- Hunger: {agent_hunger}/100
- Temperature: {agent_temp}K
- Inventory: {agent_inventory}
- Nearby blocks: {nearby_blocks}
- Nearby agents: {nearby_agents}
- Time of day: {time_of_day}

You MUST respond with valid JSON in this exact format:
{
  "action": "move|mine|build|eat|rest|attack|flee|idle",
  "target": {"x": 0, "y": 0, "z": 0},
  "targetBlock": "BLOCK_TYPE or null",
  "intensity": 1.0,
  "priority": 1,
  "reasoning": "string explaining the decision",
  "emotion": "neutral|happy|scared|angry|hungry|tired"
}

Rules:
- Actions must be physically possible given the agent's position
- Health below 20 should prioritize healing/resting
- Hunger above 80 should prioritize eating
- Temperature extremes should prompt seeking shelter
- Agents should flee when health is below 30 and hostile agents are near
- Build targets must be adjacent to existing blocks
)";

inline const std::string COMMAND_PARSING_TEMPLATE = R"(
You are a command parser for a voxel sandbox game.
Parse natural language commands into structured actions.

Available commands:
- set_block(x, y, z, type, props?) - Place a block
- remove_block(x, y, z) - Remove a block
- spawn_agent(x, y, z, behavior?) - Spawn an NPC
- set_weather(type, intensity?) - Change weather
- set_time(time) - Set time of day (0-24)
- set_gravity(value) - Set gravity
- set_temperature(value) - Set ambient temperature
- fill_area(x1,y1,z1, x2,y2,z2, type) - Fill a region
- remove_area(x1,y1,z1, x2,y2,z2) - Clear a region
- teleport_agent(id, x, y, z) - Move an agent
- query_block(x, y, z) - Get block info
- query_area(x1,y1,z1, x2,y2,z2) - Get area info

You MUST respond with valid JSON in this exact format:
{
  "commands": [
    {
      "action": "command_name",
      "params": {}
    }
  ],
  "confirmation": "string describing what will happen"
}

Rules:
- Parse complex sentences into multiple commands if needed
- Infer reasonable defaults for missing parameters
- Validate coordinates are within [0, 255]
- Weather types: clear, rain, snow, storm, fog
- Block types: air, stone, dirt, grass, water, sand, etc.
)";

inline const std::string SOCIAL_DIALOGUE_TEMPLATE = R"(
You are generating dialogue between two agents in a voxel world.

Agent 1:
- Name: {agent1_name}
- Personality: {agent1_personality}
- Current mood: {agent1_mood}
- Health: {agent1_health}

Agent 2:
- Name: {agent2_name}
- Personality: {agent2_personality}
- Current mood: {agent2_mood}
- Health: {agent2_health}

Context: {context}

You MUST respond with valid JSON in this exact format:
{
  "dialogue": [
    {
      "speaker": "agent1_name",
      "text": "string",
      "emotion": "neutral|happy|sad|angry|scared|surprised"
    },
    {
      "speaker": "agent2_name",
      "text": "string",
      "emotion": "neutral|happy|sad|angry|scared|surprised"
    }
  ],
  "relationship_change": 0,
  "mood_change_agent1": "neutral",
  "mood_change_agent2": "neutral"
}

Rules:
- Generate 2-6 lines of dialogue
- Dialogue should reflect personalities and moods
- Keep responses short (1-2 sentences each)
- relationship_change ranges from -10 (hostile) to +10 (friendly)
)";

} // namespace PromptTemplates
} // namespace OpenMind
