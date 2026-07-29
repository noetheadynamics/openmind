# OpenMind Architecture

## Overview

OpenMind follows the **Rules + Properties** architecture pattern:

- **Rules** define behaviors (how physics laws work, how LLMs reason, how agents think)
- **Properties** define state (block positions, material properties, agent memories)

This separation allows the engine to be deterministic (same rules + same properties = same outcome) while remaining extensible (new rules can be added without changing existing ones).

---

## System Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    OMNI-CONSOLE (UI)                        │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐          │
│  │  Prompt  │ │  Time   │ │ Overlays│ │ Agents  │          │
│  │  Input   │ │ Controls│ │  Panel  │ │Dashboard│          │
│  └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘          │
│       └───────────┴───────────┴───────────┘                │
│                           │                                 │
│              ┌────────────┴────────────┐                   │
│              │    OmniConsole (JS)     │                   │
│              │    + EngineConnection   │                   │
│              └────────────┬────────────┘                   │
└───────────────────────────┼────────────────────────────────┘
                            │ Module.ccall()
┌───────────────────────────┼────────────────────────────────┐
│                    WASM BRIDGE                              │
│              openmind_bridge.cpp                            │
│         Exported functions for JS↔C++                       │
└───────────────────────────┼────────────────────────────────┘
                            │
┌───────────────────────────┼────────────────────────────────┐
│                  PHYSICS ENGINE                             │
│  ┌────────────────────────┴────────────────────────┐       │
│  │              openmind_engine.cpp                 │       │
│  │           (Unified Wrapper Layer)                │       │
│  └──────┬──────────┬──────────┬──────────┬─────────┘       │
│         │          │          │          │                  │
│  ┌──────┴───┐ ┌────┴────┐ ┌──┴───┐ ┌───┴──────┐          │
│  │  Voxel   │ │Thermal  │ │Stress│ │ Weather  │          │
│  │  Octree  │ │ System  │ │System│ │ System   │          │
│  │          │ │         │ │      │ │          │          │
│  │ Blocks   │ │Temp     │ │Force │ │Rain/Snow │          │
│  │ Chunks   │ │Conduct  │ │Break │ │Wind/Press│          │
│  │ Spatial  │ │Specific │ │Prop  │ │Clouds    │          │
│  │  Hash    │ │Heat     │ │      │ │Humidity  │          │
│  └──────────┘ └─────────┘ └──────┘ └──────────┘          │
│                                                            │
│  ┌──────────┐ ┌─────────┐ ┌──────┐ ┌──────────┐          │
│  │ Lighting │ │ Sound   │ │Bio   │ │Chemistry │          │
│  │ Engine   │ │Propagat │ │System│ │Reactions │          │
│  │          │ │Reverb   │ │Plants│ │Acid      │          │
│  │ Day/Night│ │         │ │Decay │ │Combust   │          │
│  │ Shadows  │ │         │ │Disease│ │Metalurgy │          │
│  └──────────┘ └─────────┘ └──────┘ └──────────┘          │
│                                                            │
│  ┌──────────┐ ┌─────────┐ ┌──────┐                       │
│  │ Radiation│ │Magnetism│ │Rocket│                       │
│  │Simulat   │ │         │ │Orbit │                       │
│  └──────────┘ └─────────┘ └──────┘                       │
└────────────────────────────────────────────────────────────┘
                            │
┌───────────────────────────┼────────────────────────────────┐
│                 LLM CONNECTOR                               │
│              LLMInterface (Abstract)                        │
│  ┌──────────┬──────────┬──────────┬──────────┐             │
│  │ OpenAI   │Anthropic │ Google   │ Ollama   │             │
│  │ Client   │ Client   │ Client   │ Client   │             │
│  └──────────┴──────────┴──────────┴──────────┘             │
│  ┌──────────┬──────────┬──────────┐                        │
│  │OpenAI    │ JSON     │ Async    │                        │
│  │Compat.   │ Validator│ Request  │                        │
│  │ Client   │          │ Manager  │                        │
│  └──────────┴──────────┴──────────┘                        │
└────────────────────────────────────────────────────────────┘
                            │
┌───────────────────────────┼────────────────────────────────┐
│              AGENT COGNITIVE LAYER                          │
│              CognitiveAgent (perceive→think→act)            │
│  ┌──────────┬──────────┬──────────┬──────────┐             │
│  │ Memory   │  Goal    │Perception│  Tools   │             │
│  │ System   │ Manager  │ System   │ (8 tools)│             │
│  │          │          │          │          │             │
│  │Short-term│ Priority │ Vision   │move_to   │             │
│  │Long-term │ Sub-goals│ Hearing  │place_blk │             │
│  │ Decay    │ Progress │ Detect   │break_blk │             │
│  │Summary   │ Status   │ Blocks   │search    │             │
│  └──────────┴──────────┴──────────┴──────────┘             │
│  ┌──────────────────────────────────────────┐              │
│  │        Agent Communication               │              │
│  │  LLM Dialogue │ Gossip │ Relationships   │              │
│  └──────────────────────────────────────────┘              │
└────────────────────────────────────────────────────────────┘
```

---

## Core Components

### 1. VoxelOctree (`VoxelOctree.h/cpp`)

**Purpose:** Sparse octree storage for 3D voxel data.

- **Max depth:** 8 (256³ voxels)
- **Key function:** `posHash(x,y,z)` for map lookups
- **Data structure:** `VoxelData` stores block type, state (SOLID/LIQUID/GAS), material props
- **Operations:** `setBlock()`, `getBlock()`, `removeBlock()`, `setBlockTemperature()`

### 2. PhysicsEngine (`PhysicsEngine.h/cpp`)

**Purpose:** All physics simulations (~3600+ lines).

**Subsystems:**
- **Structural Stress:** Force propagation, tensile strength, block breaking
- **Thermal:** Conduction, specific heat, temperature equilibrium
- **Atmospheric:** Wind, pressure, humidity, clouds, precipitation
- **Chemical:** Reactions, acid corrosion, combustion, metallurgy
- **Biological:** Plant growth, decay, disease, predator-prey, metabolism
- **Acoustic:** Sound propagation, reverb, heard sounds
- **Radiation:** Radiation simulation, magnetism
- **Orbital:** Gravity, orbital mechanics, rocket physics
- **Lighting:** Sun position, day/night cycle, light intensity

### 3. OpenMindEngine (`openmind_engine.h/cpp`)

**Purpose:** Unified wrapper that ties all subsystems together.

- **Methods:** `initWorld()`, `setBlock()`, `tick()`, `getWorldStats()`
- **Lifecycle:** Creates VoxelOctree + PhysicsEngine, calls `tick()` each frame
- **State management:** Tracks time of day, weather, tick count

### 4. LLM Connector (`LLMInterface.h`)

**Purpose:** Abstract interface for LLM providers.

**Architecture:**
- `LLMInterface` — Abstract base class with `generate()`, `setConfig()`
- `LLMConfig` — Model, API key, endpoint, temperature, max tokens
- `LLMResponse` — Text content, tokens used, finish reason
- Provider implementations: OpenAI, Anthropic, Google, Ollama, OpenAI-Compatible

**Supporting modules:**
- `HttpClient` — WinHTTP for Windows native builds
- `JSONValidator` — JSON parsing, schema validation, injection protection
- `AsyncRequestManager` — Thread pool with priority queue and rate limiting
- `PromptTemplates` — 5 templates for world gen, materials, agents, commands, dialogue

### 5. Agent Cognitive Layer (`AgentCognitive.h`)

**Purpose:** Autonomous agent reasoning and action.

**Cognitive Loop:**
```
perceive() → think() → act()
    │            │          │
    ▼            ▼          ▼
Observation  LLM Call   Tool Execution
    │            │          │
    ▼            ▼          ▼
Perception  AgentAction  ToolResult
    │            │          │
    ▼            ▼          ▼
  Memory     Goals      Memory
```

**Modules:**
- **MemorySystem:** Short-term (20 items) + long-term memory, importance decay, summarization
- **GoalManager:** Priority queue, sub-goals, progress tracking
- **PerceptionSystem:** World observation, block counting, agent detection
- **AgentToolCalling:** 8 tools (move_to, place_block, break_block, search_for, talk_to, scan_inventory, craft, build_structure)
- **AgentCommunication:** LLM-powered dialogue, conversation recording, gossip propagation

---

## Data Flow

### Frame Update
```
1. User Input (prompt, time control, overlay toggle)
        │
2. OmniConsole.js → EngineConnection.call()
        │
3. WASM Module.ccall() → openmind_bridge.cpp
        │
4. OpenMindEngine.tick()
        │
5. PhysicsEngine.tick()
   ├── StressSystem.update()
   ├── ThermalSystem.update()
   ├── AtmosphericSystem.update()
   ├── ChemicalSystem.update()
   ├── BiologicalSystem.update()
   ├── SoundSystem.update()
   └── OrbitalSystem.update()
        │
6. Agent CognitiveAgent.think()
   ├── perceive() → scan voxels
   ├── think() → call LLM
   └── act() → execute tool
        │
7. Render (if 3D viewer attached)
```

### LLM Request Flow
```
1. Agent needs decision → call LLM
        │
2. CognitiveAgent formats prompt
   (Memory + Goal + Perception → Prompt)
        │
3. LLMInterface.generate()
   ├── OpenAIClient → HTTP POST → OpenAI API
   ├── AnthropicClient → HTTP POST → Anthropic API
   ├── GoogleClient → HTTP POST → Google API
   ├── OllamaClient → HTTP POST → localhost:11434
   └── OpenAICompatibleClient → any endpoint
        │
4. LLMResponse returned
        │
5. CognitiveAgent parses action
        │
6. Tool execution (place_block, move_to, etc.)
        │
7. Memory updated with action + result
```

---

## Rules + Properties Pattern

### Rules (Behaviors)
```cpp
// Physics Rule: Thermal Conduction
void ThermalSystem::update(VoxelOctree& world) {
    for each block at (x,y,z):
        float temp = block.temperature;
        for each neighbor:
            float delta = (neighbor.temp - temp) * conductivity;
            block.temp += delta;
            neighbor.temp -= delta;
}
```

### Properties (State)
```cpp
// Block Properties
struct VoxelData {
    BlockType type;        // STONE, WOOD, WATER...
    BlockState state;      // SOLID, LIQUID, GAS
    MaterialProps props;   // mass, density, hardness...
    float temperature;     // Current temperature
    float stress;          // Current stress level
};
```

This pattern ensures:
- **Determinism:** Same inputs → same outputs
- **Testability:** Rules can be tested independently
- **Extensibility:** New rules can be added without changing existing code
- **Performance:** Properties can be cached, rules can be parallelized
