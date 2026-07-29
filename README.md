# OpenMind

**Universal Voxel-Based Sandbox Emulator with AI-Powered Agents**

![MIT License](https://img.shields.io/badge/license-MIT-green)
![Version](https://img.shields.io/badge/version-1.0.0-blue)
![C++17](https://img.shields.io/badge/C%2B%2B-17-orange)
![WebAssembly](https://img.shields.io/badge/WebAssembly-supported-yellow)
![57 Physics Modules](https://img.shields.io/badge/Physics-57%20Modules-brightgreen)

---

## Overview

OpenMind is a real-time voxel physics engine with an LLM-powered AI brain, autonomous agent cognitive layer, and a glassmorphism web frontend. It simulates structural stress, temperature, weather, biology, chemistry, orbital mechanics, and agent reasoning — all compiled to WebAssembly and running directly in the browser.

The engine follows a **Rules + Properties** architecture: rules define behaviors (physics laws, LLM reasoning, agent cognition) while properties define state (block positions, material properties, agent memories). This separation makes the system deterministic, extensible, and testable.

OpenMind is designed for **educators** teaching physics and chemistry through interactive simulation, **game developers** prototyping voxel mechanics, **AI researchers** exploring agent cognition and LLM grounding, **architects** visualizing 3D building designs, and **anyone** who wants to build collaborative voxel worlds from natural language.

---

## Key Features

### Physics Engine (57 Modules)

| # | Module | Description |
|---|--------|-------------|
| 1 | VoxelOctree | Sparse octree storage, max depth 8, 256³ world |
| 2 | Block Properties | Mass, density, hardness, thermal, electrical, biological |
| 3 | Gravity System | Configurable gravity with orbital mechanics |
| 4 | Lighting Engine | Block-based light propagation with color |
| 5 | Water Simulation | Fluid dynamics, flow, pressure |
| 6 | Lava System | High-temperature fluid with heat radiation |
| 7 | Fire System | Ignition, spread, heat generation |
| 8 | Smoke System | Particle-based smoke with diffusion |
| 9 | Explosion System | Blast radius, debris, chain reactions |
| 10 | Erosion System | Weather-based terrain deformation |
| 11 | Sediment Transport | Material displacement by water/wind |
| 12 | Time of Day | Sun position, day/night cycle |
| 13 | Structural Stress | Force propagation through blocks |
| 14 | Material Tensile Strength | Breaking under load |
| 15 | Block Breaking | Stress-induced destruction |
| 16 | Stress Propagation | Force distribution across structures |
| 17 | Buoyancy | Float/sink based on density |
| 18 | Pressure System | Block-based pressure simulation |
| 19 | Convection | Heat-driven air currents |
| 20 | Gas Diffusion | Gas spread through atmosphere |
| 21 | Phase Changes | Melt, freeze, boil, condense |
| 22 | Dissolution | Material dissolving in liquids |
| 23 | Evaporation | Liquid to gas transition |
| 24 | Condensation | Gas to liquid transition |
| 25 | Sublimation | Solid to gas transition |
| 26 | Deposition | Gas to solid transition |
| 27 | Thermal Conduction | Heat transfer between blocks |
| 28 | Specific Heat Capacity | Material-specific thermal properties |
| 29 | Thermal Radiation | Heat emission and absorption |
| 30 | Wind System | Directional wind with speed |
| 31 | Air Pressure | Atmospheric pressure simulation |
| 32 | Cloud Formation | Moisture-based cloud generation |
| 33 | Precipitation | Rain and snow |
| 34 | Humidity | Moisture tracking per block |
| 35 | Temperature | Ambient and block temperature |
| 36 | Humidify/Dehumidify | Moisture modification |
| 37 | Weather Transitions | Smooth weather state changes |
| 38 | Atmosphere Layer | Air composition and density |
| 39 | Oxygen System | Breathing and combustion support |
| 40 | Radiation Simulation | Radiation spread and shielding |
| 41 | Magnetism | Magnetic field interactions |
| 42 | Chemical Reactions | Block-to-block reactions |
| 43 | Acid Corrosion | Material degradation |
| 44 | Combustion | Fire and fuel consumption |
| 45 | Metallurgy | Melting, smelting, alloying |
| 46 | Crystal Growth | Mineral crystallization |
| 47 | Electricity | Power generation and transmission |
| 48 | Plant Growth | Biological growth simulation |
| 49 | Decay System | Decomposition over time |
| 50 | Disease System | Biological entity infections |
| 51 | Predator-Prey | Agent population dynamics |
| 52 | Metabolism | Agent energy consumption |
| 53 | Sound Propagation | 3D sound through blocks |
| 54 | Reverb Simulation | Acoustic environment modeling |
| 55 | Orbital Mechanics | Satellite and orbit simulation |
| 56 | Fragments | Block debris and particles |
| 57 | Agent Cognition | Memory, goals, perception, tool-calling |

### LLM Integration (6+ Providers)

| Provider | Models | Status |
|----------|--------|--------|
| Groq | Llama 3.3 70B, Llama 3.1 8B, Mixtral 8x7B | ✅ |
| OpenAI | GPT-4o, GPT-4o Mini, GPT-4 Turbo | ✅ |
| Anthropic | Claude 3 Opus, Sonnet, Haiku | ✅ |
| Google Gemini | Gemini 2.0 Flash, 1.5 Pro, 1.5 Flash | ✅ |
| Ollama (Local) | Any model via local server | ✅ |
| Custom (OpenAI-compatible) | Any endpoint, any model name | ✅ |

### Agent Cognitive System

| Component | Tests | Description |
|-----------|-------|-------------|
| Memory System | 8 | Short/long-term memory, recall, consolidation |
| Goal Manager | 10 | Priority-based goal scheduling |
| Perception System | 5 | Block scanning, environment analysis |
| Tool Calling | 8 | setBlock, removeBlock, placeBlock, scanArea, talkTo, buildStructure, craft, scanInventory |
| Agent Communication | 7 | Inter-agent dialogue and messaging |
| Full Cognitive Loop | 11 | Observe → Think → Decide → Act cycle |

### Omni-Console Frontend

| Panel | Capabilities |
|-------|-------------|
| AI Prompt | Natural language world generation, autocomplete, history |
| Time Controls | Play/Pause, Fast-Forward, Rewind, speed slider |
| Visual Overlays | Stress, temperature, radiation, ecosystem, moisture, pressure |
| Agent Dashboard | Inspector, memories, goals, inventory, relationships |
| Environment Presets | Earth, Moon, Mars, Ocean, Space, Custom |
| Material Forge | AI-powered material property generation |
| Export Hub | GLTF, CSV, JSON, OMW format, download |
| Teleport | Coordinate input, bookmarks, navigation |
| AI Brain Switcher | Cloud/Local LLM toggle, provider configuration |
| Interactive Objects | Create, edit, manage physics-enabled objects |
| Inventory | 36-slot grid with item management |
| Crafting | Recipe-based crafting with AI assistance |
| Building Tools | Selection, copy/paste, symmetry, fill, blueprints, undo/redo |

### Multiplayer & Collaboration

- WebRTC peer-to-peer connections
- Real-time block synchronization
- Role-based permissions (admin, builder, guest)
- Protected areas with customizable boundaries
- Player entities with name tags and colors

### Mobile Support

- Touch-optimized controls
- Responsive glassmorphism layout
- Gyroscope camera controls
- Pinch-to-zoom

### Building Tools

- **Selection**: Rectangular and freeform block selection
- **Copy/Paste**: Clipboard with offset positioning
- **Symmetry**: Mirror X, Y, Z axes
- **Fill**: Replace blocks in selection by type
- **Blueprints**: Save and load building templates
- **Undo/Redo**: Full history with keyboard shortcuts

---

## Real-World Use Cases

### Education

| Domain | Use Case |
|--------|----------|
| Physics | Demonstrate gravity, pressure, fluid dynamics in real-time |
| Chemistry | Visualize chemical reactions, combustion, corrosion |
| Biology | Simulate plant growth, predator-prey dynamics, ecosystems |
| Environmental Science | Model weather patterns, erosion, climate effects |

### Game Development

| Domain | Use Case |
|--------|----------|
| Prototyping | Rapidly test voxel game mechanics |
| Level Design | Build and iterate on game environments |
| AI NPCs | Test agent behavior with LLM-powered reasoning |

### AI Research

| Domain | Use Case |
|--------|----------|
| Agent Cognition | Study memory, goal-setting, decision-making |
| Multi-Agent Systems | Observe emergent behavior from agent interactions |
| LLM Grounding | Test language models in physical environments |

### Architecture & Design

| Domain | Use Case |
|--------|----------|
| Building Design | Visualize structures in 3D with physics validation |
| Urban Planning | Simulate environmental effects on building materials |
| Interior Design | Place and test furniture in physics-enabled rooms |

### Creative & Entertainment

| Domain | Use Case |
|--------|----------|
| Generative Art | Create voxel sculptures from text prompts |
| Interactive Installations | Build responsive environments for exhibitions |
| Collaborative Worlds | Multi-user building and exploration |
| Digital Twins | Model real-world environments for simulation |

### Training & Simulation

| Domain | Use Case |
|--------|----------|
| Disaster Response | Simulate earthquakes, floods, fires |
| Military Training | Model terrain and environmental effects |
| Emergency Planning | Test evacuation routes and safety measures |

### Research & Science

| Domain | Use Case |
|--------|----------|
| Material Science | Test material properties under stress |
| Geology | Simulate erosion, sedimentation, tectonic effects |
| Meteorology | Model weather systems and climate |

### Prototyping

| Domain | Use Case |
|--------|----------|
| Concept Visualization | Turn ideas into 3D worlds from text |
| Product Testing | Simulate product behavior in physics environments |
| Rapid Iteration | Build, test, modify in real-time |

---

## Screenshots

| Feature | Preview |
|---------|---------|
| Omni-Console Interface | `docs/screenshots/omni_console.png` |
| Physics Simulation | `docs/screenshots/physics.png` |
| Agent Dashboard | `docs/screenshots/agents.png` |
| Material Forge | `docs/screenshots/forge.png` |
| Multiplayer | `docs/screenshots/multiplayer.png` |

---

## Quick Start

### Prerequisites

| Component | Version | Purpose |
|-----------|---------|---------|
| Python 3 | 3.6+ | HTTP server for web version |
| Emscripten SDK | Latest | WASM compilation (optional) |
| C++17 Compiler | GCC 9+, Clang 10+, MSVC 19.20+ | Native build (optional) |
| CMake | 3.10+ | Build system (optional) |

### Option 1: Web (No Build Required)

```bash
# Clone the repository
git clone https://github.com/noetheadynamics/openmind.git
cd openmind

# Serve the frontend
python -m http.server 8080 -d web

# Open http://localhost:8080 in your browser
```

### Option 2: WASM Build

```bash
# Install Emscripten SDK
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# Build OpenMind
cd ..
bash build/build_wasm.sh

# Serve
python -m http.server 8080 -d web
```

### Option 3: Native Build (Testing)

```bash
# Linux/macOS
mkdir build-native && cd build-native
cmake ../src -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run tests
ctest --output-on-failure

# Windows (MSVC)
build.bat
```

---

## Architecture

### System Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                      OMNI-CONSOLE (UI)                          │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐          │
│  │  Prompt   │ │   Time   │ │ Overlays │ │  Agents  │          │
│  │  Input    │ │ Controls │ │  Panel   │ │Dashboard │          │
│  └─────┬────┘ └─────┬────┘ └─────┬────┘ └─────┬────┘          │
│        └────────────┴────────────┴────────────┘                 │
│                          │                                      │
│              ┌───────────┴───────────┐                         │
│              │   OmniConsole (JS)    │                         │
│              │   + EngineConnection  │                         │
│              └───────────┬───────────┘                         │
└──────────────────────────┼──────────────────────────────────────┘
                           │ Module.ccall()
┌──────────────────────────┼──────────────────────────────────────┐
│                    WASM BRIDGE                                  │
│              ┌───────────┴───────────┐                         │
│              │  openmind_bridge.cpp  │                         │
│              │  EMSCRIPTEN_KEEPALIVE │                         │
│              └───────────┬───────────┘                         │
└──────────────────────────┼──────────────────────────────────────┘
                           │
┌──────────────────────────┼──────────────────────────────────────┐
│                    ENGINE CORE                                  │
│  ┌───────────────────────┴───────────────────────┐             │
│  │              PhysicsEngine                     │             │
│  │  VoxelOctree │ Thermal │ Stress │ Weather      │             │
│  │  Chemistry   │ Biology │ Sound  │ Orbital      │             │
│  └───────────────────────┬───────────────────────┘             │
└──────────────────────────┼──────────────────────────────────────┘
                           │
┌──────────────────────────┼──────────────────────────────────────┐
│                    AI LAYER                                     │
│  ┌───────────────────────┴───────────────────────┐             │
│  │           LLM Connector (6 Providers)          │             │
│  │  Groq │ OpenAI │ Anthropic │ Google │ Custom   │             │
│  └───────────────────────┬───────────────────────┘             │
│  ┌───────────────────────┴───────────────────────┐             │
│  │           Agent Cognitive Layer                │             │
│  │  Memory │ Goals │ Perception │ Tools │ Comms   │             │
│  └───────────────────────────────────────────────┘             │
└─────────────────────────────────────────────────────────────────┘
```

### Rules + Properties Pattern

**Rules** define behaviors:
- Physics laws (gravity, thermal conduction, fluid dynamics)
- LLM reasoning (prompt parsing, response generation)
- Agent cognition (memory formation, goal evaluation, action selection)

**Properties** define state:
- Block positions and types in the VoxelOctree
- Material properties (mass, density, hardness, thermal conductivity)
- Agent state (position, health, memories, goals)

This separation ensures determinism (same rules + same properties = same outcome) and extensibility (new rules can be added without modifying existing ones).

### Data Flow

```
User Input → LLM → Block Instructions → Engine → VoxelOctree → Renderer → Display
     ↑                                                              │
     └──────────────────── Agent Cognition ←────────────────────────┘
```

---

## Project Structure

```
openmind/
├── src/                    # C++ source code (35 files)
│   ├── engine/             # Physics engine (8 files)
│   │   ├── PhysicsEngine.h/cpp      # Main engine with 57 subsystems
│   │   ├── VoxelOctree.h/cpp        # Sparse octree storage
│   │   ├── MaterialProperties.h     # Material property database
│   │   ├── PhysicsTypes.h           # Type definitions
│   │   └── openmind_engine.h/cpp    # Engine wrapper API
│   ├── llm/                # LLM connector (19 files)
│   │   ├── LLMInterface.h           # Abstract LLM interface
│   │   ├── OpenAIClient.h/cpp       # OpenAI API client
│   │   ├── AnthropicClient.h/cpp    # Anthropic API client
│   │   ├── GoogleClient.h/cpp       # Google Gemini client
│   │   ├── OllamaClient.h/cpp       # Local Ollama client
│   │   ├── OpenAICompatibleClient.h/cpp  # Custom endpoint client
│   │   ├── HttpClient.h/cpp         # HTTP abstraction
│   │   ├── JSONValidator.h/cpp      # JSON parsing/validation
│   │   ├── AsyncRequestManager.h/cpp # Async request handling
│   │   ├── PromptTemplates.h        # 5 prompt templates
│   │   └── MockLLMClient.h          # Test mock
│   ├── agents/             # Agent cognitive layer (7 files)
│   │   ├── AgentCognitive.h         # Base agent class
│   │   ├── CognitiveAgent.h         # Full cognitive loop
│   │   ├── MemorySystem.h           # Memory management
│   │   ├── GoalManager.h            # Goal scheduling
│   │   ├── PerceptionSystem.h       # Environment scanning
│   │   ├── AgentToolCalling.h       # 8 tool implementations
│   │   └── AgentCommunication.h     # Inter-agent messaging
│   └── bridge/             # WASM bridge (1 file)
│       └── openmind_bridge.cpp      # EMSCRIPTEN_KEEPALIVE exports
├── web/                    # Frontend (50 files)
│   ├── index.html          # Main entry point
│   ├── omni_console.js     # Core UI logic
│   ├── omni_console.css    # Glassmorphism styles
│   ├── llm_client.js       # LLM API client
│   ├── network.js          # WebRTC networking
│   ├── host.js             # Multiplayer host
│   ├── client.js           # Multiplayer client
│   ├── voxel_renderer.js   # Three.js 3D renderer
│   ├── physics_visuals.js  # Overlay visualization
│   ├── particle_system.js  # Particle effects
│   ├── sound_system.js     # Audio engine
│   ├── interactive_objects.js  # Physics objects
│   ├── crafting_system.js  # Crafting recipes
│   ├── inventory_system.js # Inventory management
│   ├── building_history.js # Undo/redo
│   ├── blueprints.js       # Building templates
│   ├── selection.js        # Block selection
│   ├── copy_paste.js       # Clipboard operations
│   ├── symmetry.js         # Mirror tools
│   ├── import_export_building.js  # Import/export
│   ├── world_io.js         # Save/load with IndexedDB
│   ├── touch_controls.js   # Mobile controls
│   ├── tutorial.js         # Interactive tutorial
│   ├── loading_screen.js   # Loading screen
│   ├── settings_panel.js   # User settings
│   ├── shortcuts.js        # Keyboard shortcuts
│   ├── notifications.js    # Toast notifications
│   ├── error_handler.js    # Global error handling
│   ├── ui_animations.js    # Animation utilities
│   ├── ui_connection.js    # WASM bridge connection
│   ├── permissions.js      # Role-based access
│   ├── multiplayer_ui.js   # Multiplayer interface
│   ├── chat.js             # Chat system
│   ├── stats.js            # Statistics display
│   ├── post_processing.js  # Visual effects
│   ├── skybox.js           # Sky rendering
│   ├── water_renderer.js   # Water effects
│   ├── physics_visuals.js  # Debug overlays
│   ├── state_machine.js    # State management
│   ├── pattern_tools.js    # Pattern recognition
│   ├── undo_redo.js        # History management
│   ├── demo_world.json     # Demo world definition
│   ├── manifest.json       # PWA manifest
│   ├── icons/              # PWA icons
│   └── test_*.html         # 7 test pages
├── tests/                  # C++ unit tests (26 files)
│   ├── test_engine_wrapper.cpp  # Engine API tests
│   ├── test_agent.cpp           # Agent cognition tests
│   ├── test_weather.cpp         # Weather system tests
│   ├── test_temperature.cpp     # Thermal tests
│   ├── test_combustion.cpp      # Fire tests
│   ├── test_corrosion.cpp       # Acid tests
│   ├── test_plant.cpp           # Biology tests
│   ├── test_disease.cpp         # Disease tests
│   ├── test_sound.cpp           # Audio tests
│   └── ... (17 more)
├── docs/                   # Documentation (6 files)
│   ├── ARCHITECTURE.md     # System architecture
│   ├── API_REFERENCE.md    # API documentation
│   ├── BUILD_GUIDE.md      # Build instructions
│   ├── CHANGELOG.md        # Version history
│   ├── USER_MANUAL.md      # User guide
│   └── LLM_PROMPT_TEMPLATES.md  # Prompt templates
├── build/                  # Build scripts
│   ├── build_wasm.bat      # Windows WASM build
│   ├── build_wasm.sh       # Linux/macOS WASM build
│   ├── build.bat           # Windows native build
│   ├── build_native.sh     # Linux/macOS native build
│   ├── run_tests.sh        # Test runner
│   ├── CMakeLists.txt      # CMake configuration
│   └── deploy.sh           # Deployment script
├── CMakeLists.txt          # Root CMake configuration
├── package.json            # Project metadata
├── manifest.json           # PWA manifest
├── LICENSE                 # MIT License
└── README.md               # This file
```

---

## API Reference

### JavaScript API (via WASM Module)

```javascript
// World manipulation
Module.ccall('initWorld', null, [], []);
Module.ccall('setBlockSimple', 'number', ['number','number','number','number'], [x, y, z, type]);
Module.ccall('setBlock', 'number', ['number','number','number','number','string'], [x, y, z, type, propsJson]);
Module.ccall('getBlockData', 'string', ['number','number','number'], [x, y, z]);
Module.ccall('tickPhysicsDelta', 'number', ['number'], [delta]);
Module.ccall('getWorldStats', 'string', [], []);

// Time control
Module.ccall('setTimeOfDay', null, ['number'], [hours]);
Module.ccall('getTimeOfDay', 'number', [], []);
Module.ccall('getSunlightIntensity', 'number', [], []);
Module.ccall('setCycleDuration', null, ['number'], [seconds]);
Module.ccall('rewindTime', 'number', ['number'], [seconds]);

// Weather
Module.ccall('setWeather', null, ['number'], [type]); // 0=Clear,1=Rain,2=Snow,3=Storm,4=Fog
Module.ccall('getWeather', 'string', [], []);
Module.ccall('setTimeScale', null, ['number'], [scale]);

// Agents
Module.ccall('getAgentCount', 'number', [], []);
Module.ccall('getAgentData', 'string', ['number'], [index]);
Module.ccall('addAgent', 'number', ['number','number','number'], [x, y, z]);
Module.ccall('setAgentPosition', null, ['number','number','number','number'], [idx, x, y, z]);

// Camera
Module.ccall('teleportCamera', null, ['number','number','number'], [x, y, z]);

// Overlays
Module.ccall('setOverlay', null, ['string','number'], [type, enabled]);

// Generation
Module.ccall('generateFromPrompt', null, ['string'], [prompt]);
Module.ccall('saveWorld', 'string', [], []);

// Export
Module.ccall('exportCSV', 'string', [], []);
Module.ccall('exportGLTF', 'string', [], []);
```

### Parameters

| Function | Parameters | Return | Description |
|----------|-----------|--------|-------------|
| `initWorld` | none | void | Initialize world with default terrain |
| `setBlockSimple` | x, y, z, type | number | Place block by type ID |
| `setBlock` | x, y, z, type, propsJson | number | Place block with properties |
| `getBlockData` | x, y, z | string | Get block data as JSON |
| `tickPhysicsDelta` | delta | number | Advance physics by delta time |
| `getWorldStats` | none | string | Get world statistics as JSON |
| `setTimeOfDay` | hours | void | Set time (0-24) |
| `getTimeOfDay` | none | number | Get current time |
| `setWeather` | type | void | Set weather type |
| `getWeather` | none | string | Get weather data as JSON |
| `getAgentCount` | none | number | Get number of agents |
| `getAgentData` | index | string | Get agent data as JSON |
| `teleportCamera` | x, y, z | void | Move camera to position |
| `generateFromPrompt` | prompt | void | Generate world from text |
| `saveWorld` | none | string | Save world to JSON |

---

## Block Types

| ID | Name | Description |
|----|------|-------------|
| 0 | AIR | Empty space |
| 1 | STONE | Basic rock material |
| 2 | DIRT | Soil material |
| 3 | GRASS | Living grass block |
| 4 | WATER | Fluid block |
| 5 | SAND | Granular material |
| 6 | GLASS | Transparent solid |
| 7 | WOOD | Organic building material |
| 8 | LEAVES | Plant foliage |
| 9 | IRON | Metal ore |
| 10 | COPPER | Conductive metal |
| 11 | GOLD | Precious metal |
| 12 | STEEL | Alloy metal |
| 13 | DIAMOND | hardest material |
| 14 | COAL | Fuel material |
| 15 | BEDROCK | Indestructible base |
| 16 | ASH | Fire residue |
| 17 | TNT | Explosive material |
| 18 | SNOW | Frozen precipitation |
| 255 | CUSTOM | User-defined material |

### Material Properties Schema

```json
{
  "mass": 7.85,
  "density": 7850,
  "hardness": 8.0,
  "tensileStrength": 400,
  "thermalConductivity": 50.0,
  "specificHeat": 450.0,
  "meltingPoint": 1811,
  "electricalConductivity": 0.7,
  "magneticPermeability": 0.0,
  "baseColor": "#71797E",
  "isOrganic": false,
  "flammable": false,
  "corrosionResistance": 0.9
}
```

---

## Environment Presets

| Preset | Gravity (m/s²) | Air Density (kg/m³) | Temperature (K) | Description |
|--------|----------------|---------------------|------------------|-------------|
| Earth | 9.81 | 1.225 | 293.15 | Standard Earth conditions |
| Moon | 1.62 | 0 | 100 | Lunar vacuum |
| Mars | 3.71 | 0.020 | 210 | Thin Martian atmosphere |
| Ocean | 9.81 | 1025 | 277 | Underwater environment |
| Space | 0 | 0 | 2.7 | Deep space vacuum |
| Custom | Configurable | Configurable | Configurable | User-defined |

---

## LLM Configuration

### Cloud Providers

1. **Groq** (Default, fastest)
   - Endpoint: `https://api.groq.com/openai/v1/chat/completions`
   - Model: `llama-3.3-70b-versatile`
   - Get API key: https://console.groq.com

2. **OpenAI**
   - Endpoint: `https://api.openai.com/v1/chat/completions`
   - Model: `gpt-4o-mini`
   - Get API key: https://platform.openai.com

3. **Anthropic**
   - Endpoint: `https://api.anthropic.com/v1/messages`
   - Model: `claude-3-haiku-20240307`
   - Get API key: https://console.anthropic.com

4. **Google Gemini**
   - Endpoint: `https://generativelanguage.googleapis.com/v1beta/models`
   - Model: `gemini-2.0-flash`
   - Get API key: https://aistudio.google.com

### Local Providers

5. **Ollama**
   - Endpoint: `http://localhost:11434`
   - Install: https://ollama.ai
   - Run: `ollama pull llama3`

6. **LM Studio**
   - Endpoint: `http://localhost:1234`
   - Install: https://lmstudio.ai

### Custom Endpoint

7. **Custom (OpenAI-compatible)**
   - Select "Custom" in the Brain panel
   - Enter your endpoint URL
   - Enter your model name
   - Enter your API key
   - Works with: vLLM, Together AI, Fireworks AI, DeepSeek, and any OpenAI-compatible API

### Prompt Templates

The engine includes 5 built-in templates:

1. **World Generation** — Parse natural language into block placements
2. **Material Generation** — Create custom material properties
3. **Agent Behavior** — Decide agent actions from observations
4. **Command Parsing** — Convert user commands to engine calls
5. **Social Dialogue** — Generate agent-to-agent conversations

---

## Building from Source

### Native Build (Linux/macOS)

```bash
# Install dependencies
sudo apt-get install build-essential cmake  # Ubuntu/Debian
# or
brew install cmake  # macOS

# Build
mkdir build-native && cd build-native
cmake ../src -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run tests
ctest --output-on-failure
```

### Native Build (Windows)

```batch
# Open Developer Command Prompt for VS 2022
build.bat

# Or manually
mkdir build-native && cd build-native
cmake ../src -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### WASM Build

```bash
# Install Emscripten SDK
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# Build
cd ..
bash build/build_wasm.sh

# Output: web/openmind.js + web/openmind.wasm
```

---

## Testing

### C++ Tests

```bash
# Run all 26 tests
bash build/run_tests.sh

# Or manually
cd build-native
ctest --output-on-failure
```

| Test File | Coverage |
|-----------|----------|
| test_engine_wrapper.cpp | Engine API, block operations |
| test_agent.cpp | Agent cognition, memory, goals |
| test_weather.cpp | Weather system, precipitation |
| test_temperature.cpp | Thermal conduction, heat |
| test_combustion.cpp | Fire, fuel, oxygen |
| test_corrosion.cpp | Acid, material degradation |
| test_plant.cpp | Growth, biology |
| test_disease.cpp | Infection, spread |
| test_sound.cpp | Propagation, reverb |
| test_explosion.cpp | Blast, debris |
| test_lighting.cpp | Light propagation |
| test_atmosphere.cpp | Air pressure, wind |
| test_disease.cpp | Pathogen simulation |
| test_metabolism.cpp | Energy, hunger |
| test_predator_prey.cpp | Population dynamics |
| test_decay.cpp | Decomposition |
| test_reverb.cpp | Acoustic modeling |
| test_step.cpp | Step responses |
| test_orbital.cpp | Orbital mechanics |
| test_llm.cpp | LLM integration |
| test_reverb.cpp | Sound reflection |
| test_lighting.cpp | Illumination |
| test_atmosphere.cpp | Air composition |
| test_melting_only.cpp | Phase changes |
| test_melting_debug.cpp | Melting diagnostics |
| test_isolate.cpp | System isolation |

### Web Test Pages

| Page | Purpose |
|------|---------|
| test_frontend.html | UI component testing |
| test_interactive.html | Interactive objects |
| test_building.html | Building tools |
| test_visual.html | Visual effects |
| test_ux.html | User experience |
| test_mobile.html | Mobile controls |
| test_multiplayer.html | Multiplayer networking |

---

## Contributing

### Workflow

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Create a Pull Request

### Code Style

- **C++**: Follow Google C++ Style Guide, use `std::` prefix
- **JavaScript**: Use ES6+ features, avoid `var`, use `const`/`let`
- **CSS**: Use CSS custom properties, follow BEM naming
- **Tests**: Add tests for new features, maintain >80% coverage

### Development Setup

```bash
# Clone your fork
git clone https://github.com/yourusername/openmind.git
cd openmind

# Install dependencies
# (No npm dependencies required)

# Start development server
python -m http.server 8080 -d web

# Make changes and refresh browser
```

---

## Roadmap

### v1.0.0 (Current)
- ✅ 57-module physics engine
- ✅ 6 LLM providers with custom endpoint support
- ✅ Agent cognitive system with 49 tests
- ✅ Glassmorphism web frontend
- ✅ Multiplayer via WebRTC
- ✅ Mobile touch controls
- ✅ WASM compilation

### v1.1.0 (Planned)
- 🔄 Advanced particle effects
- 🔄 Procedural terrain generation
- 🔄 Enhanced agent personalities
- 🔄 Collaborative editing improvements
- 🔄 Performance optimizations

### v1.2.0 (Future)
- 📋 VR/AR support
- 📋 Plugin system for custom physics
- 📋 Advanced AI agent hierarchies
- 📋 Real-time collaboration
- 📋 Cloud deployment tools

---

## License

MIT License

Copyright (c) 2026 OpenMind

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---

## Acknowledgments

- [Emscripten](https://emscripten.org) — WebAssembly compilation
- [Three.js](https://threejs.org) — 3D rendering
- [Groq](https://groq.com) — Fast LLM inference
- [OpenAI](https://openai.com) — GPT models
- [Anthropic](https://anthropic.com) — Claude models
- [Google](https://ai.google.dev) — Gemini models
- [Ollama](https://ollama.ai) — Local LLM serving
- The voxel engine community for inspiration
- All contributors and testers

---

**Built with ❤️ by OpenMind Team**
