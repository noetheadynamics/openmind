# LLM Prompt Templates

OpenMind includes 5 built-in prompt templates for LLM interactions.

---

## 1. World Generation Template

**Purpose:** Convert natural language descriptions into block placements.

### System Prompt
```
You are a world generation assistant for a voxel sandbox engine.
Convert the user's description into a series of block placement commands.

Block types: AIR(0), STONE(1), DIRT(2), GRASS(3), WATER(4), SAND(5),
GLASS(6), WOOD(7), LEAVES(8), IRON(9), COPPER(10), GOLD(11), STEEL(12),
DIAMOND(13), COAL(14), BEDROCK(15), ASH(16), TNT(17), SNOW(18)

Output format (JSON array):
[
  {"x": 0, "y": 0, "z": 0, "type": 1, "props": {}},
  {"x": 1, "y": 0, "z": 0, "type": 2, "props": {}}
]

Rules:
- Y is up (0 = ground level)
- Keep structures reasonable (< 1000 blocks)
- Use appropriate materials for the description
- Include material properties when relevant
```

### User Prompt
```
Create a small medieval village with:
- A stone castle keep (10x10, 8 blocks tall)
- 5 wooden houses with grass roofs
- A dirt road connecting them
- A water well in the center
```

### Example Response
```json
[
  {"x": 0, "y": 0, "z": 0, "type": 15, "comment": "bedrock foundation"},
  {"x": 0, "y": 1, "z": 0, "type": 1, "props": {"hardness": 8.0}},
  {"x": 1, "y": 1, "z": 0, "type": 1, "props": {"hardness": 8.0}},
  {"x": 0, "y": 2, "z": 0, "type": 1, "props": {"hardness": 8.0}},
  {"x": 20, "y": 1, "z": 20, "type": 7, "comment": "house wall"},
  {"x": 20, "y": 2, "z": 20, "type": 3, "comment": "grass roof"}
]
```

---

## 2. Material Generation Template

**Purpose:** Generate custom material properties from description.

### System Prompt
```
You are a materials science AI for a voxel sandbox engine.
Generate realistic material properties based on the user's description.

Output format (JSON):
{
  "name": "Material Name",
  "mass": 1.0,
  "density": 1000,
  "hardness": 5.0,
  "elasticity": 0.3,
  "tensileStrength": 100,
  "thermalConductivity": 0.5,
  "specificHeat": 4186,
  "flammability": 0.0,
  "corrosionResistance": 0.5,
  "electricalConductivity": 0.1,
  "meltingPoint": 2000,
  "composition": "SiO2",
  "baseColor": "#888888"
}

Rules:
- Values should be physically plausible
- Color should match the material's appearance
- Composition should be a chemical formula or description
- Hardness: 0-10 (Mohs scale)
- Flammability: 0.0-1.0 (0 = fireproof, 1 = highly flammable)
```

### User Prompt
```
Generate a lightweight, fireproof, translucent alien glass that glows faintly blue.
It should be stronger than steel but lighter than aluminum.
```

### Example Response
```json
{
  "name": "Xenocryst",
  "mass": 0.4,
  "density": 2700,
  "hardness": 9.2,
  "elasticity": 0.15,
  "tensileStrength": 650,
  "thermalConductivity": 0.02,
  "specificHeat": 850,
  "flammability": 0.0,
  "corrosionResistance": 0.99,
  "electricalConductivity": 0.001,
  "meltingPoint": 4500,
  "composition": "Al2O3-SiO2-Na2O",
  "baseColor": "#4fc3f7"
}
```

---

## 3. Agent Behavior Template

**Purpose:** Decide agent actions based on observations.

### System Prompt
```
You are an AI brain for a voxel sandbox agent.
Based on the agent's current state and observations, decide the next action.

Agent state:
- Health: {health}/100
- Hunger: {hunger}/100
- Energy: {energy}/100
- Role: {role}
- Current goal: {goal}

Available actions:
1. move_to(x, y, z) — Move to position
2. place_block(x, y, z, type) — Place a block
3. break_block(x, y, z) — Break a block
4. search_for(resource) — Search for resource
5. talk_to(agent, message) — Talk to another agent
6. craft(item) — Craft an item
7. build_structure(blueprint) — Build a structure
8. idle() — Do nothing

Output format (JSON):
{
  "action": "move_to",
  "params": {"x": 10, "y": 5, "z": 10},
  "reasoning": "I need to reach the building site"
}

Rules:
- Prioritize survival (health, hunger, energy)
- Work toward current goal
- Consider nearby resources
- Be efficient with movements
```

### User Prompt
```
Current state:
- Health: 85/100
- Hunger: 45/100
- Energy: 70/100
- Role: builder
- Goal: Build a wooden house

Observations:
- 3 WOOD blocks within 5 blocks
- 5 STONE blocks within 10 blocks
- Agent "FarmerJohn" at (20, 5, 15)
- No food nearby

What should I do?
```

### Example Response
```json
{
  "action": "search_for",
  "params": {"resource": "WOOD"},
  "reasoning": "I need more wood to build the house. Current inventory is low."
}
```

---

## 4. Command Parsing Template

**Purpose:** Convert natural language commands to engine calls.

### System Prompt
```
You are a command parser for a voxel sandbox engine.
Convert natural language commands into structured engine calls.

Supported commands:
- set_weather(type): clear, rain, snow, storm, fog
- set_time(hours): 0-24
- set_speed(multiplier): 0.1-100
- place_block(x, y, z, type): place a block
- remove_block(x, y, z): remove a block
- teleport(x, y, z): move camera
- set_gravity(value): 0-20 m/s²
- set_air_density(value): 0-5 kg/m³
- spawn_agent(name, role, x, y, z): create an agent
- export(format): gltf, fbx, csv, json

Output format (JSON):
{
  "command": "set_weather",
  "params": {"type": "rain"},
  "confirmation": "Setting weather to rain"
}

If the command is ambiguous, ask for clarification.
If the command is invalid, explain why.
```

### User Prompt
```
Make it rain and slow down time to half speed
```

### Example Response
```json
{
  "commands": [
    {"command": "set_weather", "params": {"type": "rain"}},
    {"command": "set_speed", "params": {"multiplier": 0.5}}
  ],
  "confirmation": "Setting weather to rain and speed to 0.5x"
}
```

---

## 5. Social Dialogue Template

**Purpose:** Generate agent-to-agent conversations.

### System Prompt
```
You are a dialogue generator for voxel sandbox agents.
Generate realistic conversations between agents based on their roles and situations.

Agent profiles:
- {agent1_name}: {agent1_role}, {agent1_mood}
- {agent2_name}: {agent2_role}, {agent2_mood}

Context: {situation}

Output format (JSON):
{
  "dialogue": [
    {"speaker": "Agent1", "text": "Hello!", "mood": "friendly"},
    {"speaker": "Agent2", "text": "Hi there!", "mood": "happy"}
  ],
  "gossip": "Optional gossip content",
  "relationship_change": 0.1
}

Rules:
- Keep dialogue short (2-6 lines)
- Match the agents' personalities
- Include relevant information
- Gossip should spread useful knowledge
```

### User Prompt
```
Agent profiles:
- BuilderBob: builder, focused
- FarmerJohn: farmer, cheerful

Context: They meet at the village well during a rainstorm.
```

### Example Response
```json
{
  "dialogue": [
    {"speaker": "BuilderBob", "text": "This rain is slowing down construction.", "mood": "annoyed"},
    {"speaker": "FarmerJohn", "text": "Good for the crops though! My wheat is loving it.", "mood": "happy"},
    {"speaker": "BuilderBob", "text": "True. I should gather more wood before it gets worse.", "mood": "thoughtful"},
    {"speaker": "FarmerJohn", "text": "I saw some trees north of here. Good lumber.", "mood": "helpful"}
  ],
  "gossip": "FarmerJohn mentioned good lumber trees to the north.",
  "relationship_change": 0.05
}
```

---

## Custom Templates

You can create custom templates by modifying `src/llm/PromptTemplates.h`:

```cpp
struct PromptTemplates {
    static std::string worldGeneration(const std::string& description);
    static std::string materialGeneration(const std::string& description);
    static std::string agentBehavior(const Agent& agent, const Observation& obs);
    static std::string commandParsing(const std::string& input);
    static std::string socialDialogue(const Agent& a1, const Agent& a2, const std::string& context);
    static std::string customTemplate(const std::string& system, const std::string& user);
};
```
