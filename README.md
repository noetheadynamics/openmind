# OpenMind

**In-browser voxel simulation engine with LLM-powered autonomous agents.**

[![MIT License](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.2.0-blue)](#)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-orange)](src/)
[![WebAssembly](https://img.shields.io/badge/WebAssembly-supported-yellow)](#)
[![57 Physics Modules](https://img.shields.io/badge/Physics-57%20Modules-brightgreen)](#)

---

## Table of Contents

- [Overview](#overview)
- [What You Can Build With OpenMind](#what-you-can-build-with-openmind)
- [Features](#features)
  - [Physics Engine (57 Modules)](#physics-engine-57-modules)
  - [LLM Integration (6 Providers)](#llm-integration-6-providers)
  - [Autonomous Agent System](#autonomous-agent-system)
  - [Frontend](#frontend)
  - [Building Tools](#building-tools)
  - [Multiplayer](#multiplayer)
  - [PWA & Mobile](#pwa--mobile)
- [Quick Start](#quick-start)
- [Project Structure](#project-structure)
- [API Reference](#api-reference)
- [Building from Source](#building-from-source)
- [Desktop App (Tauri)](#desktop-app-tauri)
- [Testing](#testing)
- [Roadmap](#roadmap)
- [License](#license)

---

## Overview

OpenMind is a real-time voxel physics engine compiled to **WebAssembly**, paired with a **multi-provider LLM integration layer** and **autonomous agent cognitive system** — all running directly in the browser with no server-side dependencies.

The engine simulates **57 physical, chemical, biological, and cognitive subsystems** within a 256³ voxel world. Users interact through natural language (via LLM), direct manipulation (point-and-click / touch), or by deploying autonomous AI agents that perceive, reason, and act on their own.

### How It Works

```
User Input (text / click / touch)
        │
        ▼
┌─────────────────────────────────────┐
│      OmniConsole Frontend (JS)       │
│  AI Prompt · Time Controls · Panels  │
│  Block Editor · Building Tools       │
└────────────┬────────────────────────┘
             │ Module.ccall() bridge
┌────────────▼────────────────────────┐
│      WASM Bridge (openmind_bridge)   │
│      C++ → JavaScript interop        │
└────────────┬────────────────────────┘
             │
┌────────────▼────────────────────────┐
│      PhysicsEngine (C++17)           │
│  VoxelOctree · 57 subsystems         │
│  Deterministic tick-based simulation │
└────────────┬────────────────────────┘
             │
┌────────────▼────────────────────────┐
│      AI Layer                        │
│  ┌──────────────────────────────┐    │
│  │  LLM Connector (6 providers) │    │
│  │  Groq · OpenAI · Anthropic   │    │
│  │  Google · Ollama · Custom    │    │
│  └──────────────────────────────┘    │
│  ┌──────────────────────────────┐    │
│  │  Agent Cognitive Layer        │    │
│  │  Memory · Goals · Perception  │    │
│  │  Tools · Communication        │    │
│  └──────────────────────────────┘    │
└──────────────────────────────────────┘
```

The system follows a **Rules + Properties** pattern:
- **Rules** define behaviors — physics laws, LLM reasoning, agent cognition
- **Properties** define state — block types, material properties, agent memories

This separation makes the engine deterministic (same rules + same properties = same outcome) and extensible (new rules can be added without modifying existing ones).

---

## What You Can Build With OpenMind

### Education & Teaching

| Domain | Example |
|--------|---------|
| **Physics** | Drop objects under variable gravity, observe structural stress, simulate fluid dynamics in real-time |
| **Chemistry** | Set up chemical reactions between block types, watch combustion and corrosion |
| **Biology** | Spawn agents and observe predator-prey dynamics, plant growth, disease spread |
| **Environmental Science** | Change weather patterns, trigger erosion, simulate climate effects on terrain |
| **Astronomy** | Switch environment to Moon or Mars gravity, observe orbital mechanics |

### Game Development

| Domain | Example |
|--------|---------|
| **Prototyping** | Rapidly iterate on voxel game mechanics without compiling |
| **Level Design** | Build environments using copy/paste, symmetry, blueprints, and fill patterns |
| **AI NPCs** | Deploy LLM-powered agents with goals, memory, and tool-use capabilities |
| **Interactive Worlds** | Add doors, buttons, sensors, pistons, and computers with signal wiring |

### AI Research

| Domain | Example |
|--------|---------|
| **Agent Cognition** | Study how agents form memories, set goals, and make decisions in a physical world |
| **Multi-Agent Systems** | Observe emergent behavior from multiple agents communicating and competing |
| **LLM Grounding** | Test how language models understand and act within a 3D environment |
| **Tool-Use Benchmarking** | Evaluate LLMs on their ability to use 8+ tools in sequence to achieve goals |

### Architecture & Design

| Domain | Example |
|--------|---------|
| **Structural Visualization** | Design buildings and watch stress propagate through the structure |
| **Material Testing** | Create custom materials in the Forge and test their properties |
| **Urban Planning** | Simulate environmental effects (wind, temperature, radiation) on structures |
| **Interior Design** | Place furniture and interactive objects in physics-enabled rooms |

### Creative & Generative Art

| Domain | Example |
|--------|---------|
| **Text-to-World** | Describe a scene in natural language and watch it materialize block by block |
| **Voxel Sculpting** | Use building tools with symmetry and patterns for detailed creations |
| **Interactive Installations** | Build responsive environments with sensors, timers, and triggers |
| **Collaborative Worlds** | Invite others to build together with permission controls |

### Simulation & Training

| Domain | Example |
|--------|---------|
| **Disaster Response** | Simulate earthquakes, floods, and fires for training scenarios |
| **Emergency Planning** | Test evacuation routes and safety measures in a risk-free environment |
| **Product Testing** | Place virtual products in physics environments and observe behavior |

---

## Features

### Physics Engine — 57 Modules

| # | Module | Description |
|---|--------|-------------|
| 1 | **VoxelOctree** | Sparse octree storage, max depth 8, 256³ world |
| 2 | **Block Properties** | Mass, density, hardness, thermal, electrical, biological per block |
| 3 | **Gravity System** | Configurable gravity with orbital mechanics (Earth, Moon, Mars, Space, Custom) |
| 4 | **Lighting Engine** | Block-based light propagation with color temperature |
| 5 | **Water Simulation** | Fluid dynamics with flow, pressure, and buoyancy |
| 6 | **Lava System** | High-temperature fluid with heat radiation |
| 7 | **Fire System** | Ignition, spread, heat generation, oxygen-dependent |
| 8 | **Smoke System** | Particle-based smoke with diffusion and wind |
| 9 | **Explosion System** | Blast radius, debris ejection, chain reactions |
| 10 | **Erosion System** | Weather-based terrain deformation over time |
| 11 | **Sediment Transport** | Material displacement by water and wind |
| 12 | **Time of Day** | Sun position, day/night cycle, configurable duration |
| 13 | **Structural Stress** | Force propagation through connected blocks |
| 14 | **Material Tensile Strength** | Blocks break when stress exceeds threshold |
| 15 | **Block Breaking** | Stress-induced destruction with fragment generation |
| 16 | **Stress Propagation** | Force distribution across connected structures |
| 17 | **Buoyancy** | Float/sink based on density comparison |
| 18 | **Pressure System** | Block-based pressure simulation for fluids and gases |
| 19 | **Convection** | Heat-driven air currents |
| 20 | **Gas Diffusion** | Gas spread through atmosphere |
| 21 | **Phase Changes** | Melt, freeze, boil, condense, sublimate, deposit |
| 22 | **Dissolution** | Material dissolving in compatible liquids |
| 23 | **Evaporation** | Liquid to gas transition based on temperature |
| 24 | **Condensation** | Gas to liquid transition on cooling |
| 25 | **Sublimation** | Solid to gas transition (e.g., dry ice) |
| 26 | **Deposition** | Gas to solid transition (e.g., frost) |
| 27 | **Thermal Conduction** | Heat transfer between adjacent blocks |
| 28 | **Specific Heat Capacity** | Temperature change rate per material |
| 29 | **Thermal Radiation** | Heat emission and absorption across distance |
| 30 | **Wind System** | Directional wind with speed and gusts |
| 31 | **Air Pressure** | Atmospheric pressure simulation per column |
| 32 | **Cloud Formation** | Moisture-based cloud generation at altitude |
| 33 | **Precipitation** | Rain and snow from saturated clouds |
| 34 | **Humidity** | Moisture tracking per block with diffusion |
| 35 | **Temperature** | Ambient and block temperature with heat sources |
| 36 | **Humidify/Dehumidify** | Moisture modification by systems |
| 37 | **Weather Transitions** | Smooth transitions between 5 weather states |
| 38 | **Atmosphere Layer** | Air composition (O₂, CO₂, N₂) and density by altitude |
| 39 | **Oxygen System** | Oxygen consumption for combustion and respiration |
| 40 | **Radiation Simulation** | Radiation spread, shielding, and decay |
| 41 | **Magnetism** | Magnetic field interactions between magnetic blocks |
| 42 | **Chemical Reactions** | Block-to-block reactions with product/output |
| 43 | **Acid Corrosion** | Material degradation by acidic substances |
| 44 | **Combustion** | Fire and fuel consumption with oxygen requirement |
| 45 | **Metallurgy** | Melting, smelting, alloying of metal types |
| 46 | **Crystal Growth** | Mineral crystallization over time |
| 47 | **Electricity** | Power generation, transmission, and consumption |
| 48 | **Plant Growth** | Biological growth simulation with light/water needs |
| 49 | **Decay System** | Decomposition of organic materials over time |
| 50 | **Disease System** | Biological entity infections with spread mechanics |
| 51 | **Predator-Prey** | Agent population dynamics with hunting and reproduction |
| 52 | **Metabolism** | Agent energy consumption and food requirements |
| 53 | **Sound Propagation** | 3D sound travel through blocks with attenuation |
| 54 | **Reverb Simulation** | Acoustic environment modeling per space |
| 55 | **Orbital Mechanics** | Satellite and orbit simulation with gravity |
| 56 | **Fragments** | Block debris and particle generation on destruction |
| 57 | **Agent Cognition** | Memory, goals, perception, tool-calling, communication |

### LLM Integration — 6 Providers

| Provider | Models | When to Use |
|----------|--------|-------------|
| **Groq** | Llama 3.3 70B, Llama 3.1 8B, Mixtral 8x7B | Fastest inference, default for agent thinking |
| **OpenAI** | GPT-4o, GPT-4o Mini, GPT-4 Turbo | Highest quality reasoning, complex tasks |
| **Anthropic** | Claude 3 Opus, Sonnet, Haiku | Long-context, safety-critical applications |
| **Google Gemini** | Gemini 2.0 Flash, 1.5 Pro, 1.5 Flash | Multimodal, Google ecosystem |
| **Ollama** | Any model (local) | Offline use, privacy-sensitive, no API costs |
| **Custom** | Any OpenAI-compatible endpoint | vLLM, Together AI, Fireworks AI, DeepSeek, etc. |

All providers are configurable through the **AI Brain Switcher** panel — no code changes needed to switch between providers mid-session.

### Autonomous Agent System

| Component | Description |
|-----------|-------------|
| **Memory System** | Dual-store architecture: short-term (recent experiences) and long-term (important patterns consolidated via sleep). Supports recall, forgetting, and importance scoring |
| **Goal Manager** | Priority queue with hierarchical decomposition. Agents can break high-level goals ("build a house") into sub-goals ("find wood", "place walls", "craft roof") with progress tracking |
| **Perception System** | Configurable vision range (default 8 blocks). Agents scan their environment, detect block types, entities, and changes. Supports attention-filtering to avoid information overload |
| **Tool Calling** | 8 tools exposed to LLM decision-making: `move_to`, `place_block`, `break_block`, `search_for`, `talk_to`, `scan_inventory`, `craft`, `build_structure`. Each tool has structured parameters and validation |
| **Agent Communication** | LLM-powered dialogue between agents. Supports gossip propagation (information spreads through social networks), negotiation, and collaborative task coordination |
| **Configuration** | 6 roles (Builder, Farmer, Explorer, Miner, Guard, Artist), 5 personalities (Helpful, Cautious, Creative, Efficient, Thorough), custom goals |
| **Full Loop** | Observe → Think → Decide → Act. Each cycle: perceive environment, retrieve relevant memories, evaluate goals, select action via LLM, execute tool call, observe result, store in memory |
| **Multi-Agent Management** | Create, inspect, pause, resume, and delete agents. Per-agent state persistence in IndexedDB. Dashboard shows health, energy, position, current goal, and thought history |

### Frontend

| Panel | What It Does |
|-------|-------------|
| **AI Command Console** | Send natural language prompts to generate worlds, place blocks, spawn agents, change weather/time. Autocomplete suggestions, chat history, markdown rendering. Processes commands locally when offline and routes to LLM when connected |
| **Time Controls** | Play/pause simulation, fast-forward/rewind (10s, 1m, 5m), variable speed slider (0.1x to 100x), day/night cycle duration, time presets (midnight, sunrise, noon, sunset) |
| **Visual Overlays** | Toggle structural stress heatmap, temperature distribution, radiation levels, ecosystem population graph, density map, agent thoughts. Post-processing controls for bloom, fog, saturation, contrast |
| **Agent Dashboard** | Spawn agents with role/personality/goal selection. Real-time inspector showing health, energy, position, state, current goal, thought history. Start/stop/delete controls |
| **Environment Presets** | One-click switch between Earth, Moon, Mars, Ocean, Space, or Custom environments. Adjustable gravity, air density, temperature, humidity with live preview |
| **Material Forge** | Describe a material in natural language ("a lightweight fireproof translucent alien glass that glows faintly blue") and generate complete material properties. Preview and add to world |
| **Export Hub** | Export world as GLTF (3D model), CSV (data), JSON, OMW (world file), or OMM (materials). Progress tracking during export |
| **Interactive Objects** | Create 15 types of physics-enabled objects (door, button, lever, switch, chest, lamp, piston, conveyor, trapdoor, fire, lock, launcher, sensor, timer, computer) with signal wiring |
| **Inventory** | 36-slot grid with item management. Drag-to-rearrange. Hotbar slots 1-9 for quick access |
| **Crafting** | Recipe-based crafting system with AI-assisted material generation. Browse and craft from discovered recipes |
| **Building Tools** | Selection box/brush/paint modes, copy/paste with rotation and flip, X/Y/Z symmetry, fill with 7 patterns, blueprint save/load, undo/redo history |

### Building Tools

| Tool | Description |
|------|-------------|
| **Selection** | Three modes: Box (drag to select rectangular volume), Brush (freeform placement), Paint (replace block types) |
| **Copy/Paste** | Copy selection to clipboard with 4×90° rotation and X/Y/Z flip. Ghost preview shows placement location. Ctrl+C/Ctrl+V shortcuts |
| **Symmetry** | Mirror block placement across X, Y, or Z axis with visual plane indicators. Useful for symmetric building |
| **Fill Patterns** | 7 fill types: Solid, Checkerboard, Stripes, Random, Gradient (by height), Line, Circle, Sphere. Fill replaces blocks matching selected type within a selection |
| **Blueprints** | Save selections as named blueprints in IndexedDB library. Export/import `.BP` files for sharing. Library browser with load/delete |
| **Undo/Redo** | Full operation history with keyboard shortcuts (Ctrl+Z/Ctrl+Y). History list browser |

### Multiplayer

- **WebRTC peer-to-peer** — No central server required. Host creates a room, clients join via host key
- **Real-time sync** — Block changes, agent states, and environment settings synchronized across all connected clients
- **Role-based permissions** — Admin, Builder, and Guest roles with configurable capabilities
- **Protected areas** — Define rectangular zones with custom permission overrides
- **Player entities** — Represented with colored name tags and customizable colors
- **Chat** — Built-in chat for inter-player communication
- **Multiplayer UI** — Connection panel with host key display, client join interface, player list

### PWA & Mobile

- **Touch controls** — On-screen joystick for movement, tap-to-place, double-tap-to-break, pinch-to-zoom, two-finger drag-to-look. Action buttons for jump, place, break, inventory. 9-slot hotbar
- **Responsive layout** — Adapts to desktop, tablet, and phone screens with three breakpoints (768px, 480px, 360px). 44px minimum touch targets
- **Installable (PWA)** — manifest.json with standalone display, 192px and 512px icons, theme-color support
- **Offline capable** — Service worker caches 45+ core assets on first load. Works without network after initial visit. API calls pass through for online functionality
- **Mobile performance** — 300ms sim loop vs 200ms desktop, requestIdleCallback for non-critical updates, reduced chunk render distance

---

## Quick Start

### Prerequisites

| Component | Required | Purpose |
|-----------|----------|---------|
| Python 3.6+ | Yes | Local HTTP server |
| Git | Yes | Clone repository |
| Emscripten SDK | Only for WASM rebuild | Compile C++ to WebAssembly |
| C++17 compiler | Only for native build | Run tests natively |
| CMake 3.10+ | Only for native build | Build system |

### Option 1: Run Immediately (Pre-built)

```bash
# Clone
git clone https://github.com/noetheadynamics/openmind.git
cd openmind

# Serve
python -m http.server 8080 -d build/dist

# Open in browser
start http://localhost:8080    # Windows
open http://localhost:8080     # macOS
xdg-open http://localhost:8080 # Linux
```

That's it. The `build/dist/` directory contains everything needed:
- Pre-compiled WASM engine (`openmind.wasm` — 253KB)
- All frontend JavaScript, CSS, and HTML
- PWA manifest and service worker
- Icons for "Add to Home Screen"

### Option 2: Rebuild WASM from C++ Source

```bash
# Linux/macOS
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
cd ../openmind
bash build/build_wasm.sh
cp build/wasm/openmind.{js,wasm} build/dist/
python -m http.server 8080 -d build/dist
```

```batch
REM Windows
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
.\emsdk install latest
.\emsdk activate latest
cd ..\openmind
build\build_wasm.bat
copy build\wasm\openmind.* build\dist\
python -m http.server 8080 -d build\dist
```

### Option 3: Native Build (for Running C++ Tests)

```bash
# Linux/macOS
cd openmind
mkdir build-native && cd build-native
cmake ../src -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
ctest --output-on-failure
```

```batch
REM Windows (VS 2022 Developer Command Prompt)
cd openmind
build.bat
```

### First-Time Setup

1. Open `http://localhost:8080` — the WASM engine loads automatically
2. **To use AI features**, configure your LLM provider:
   - Click the **Brain** panel (lightbulb icon in the top bar)
   - Select a provider (Groq is the fastest — free API key at https://console.groq.com)
   - Enter your API key and click "Test Connection"
3. Start typing in the **AI Command Console** — try: _"Create a 10x10 stone platform"_
4. Or explore manually: click blocks to place (left-click) / break (right-click), use the Time Controls to advance the day, spawn agents from the Dashboard

---

## Project Structure

```
openmind/
├── src/                              # C++ source code
│   ├── engine/                       #   Physics engine core
│   │   ├── PhysicsEngine.h/cpp       #     Main engine — 57 subsystems
│   │   ├── VoxelOctree.h/cpp         #     Sparse octree (256³, depth 8)
│   │   ├── MaterialProperties.h      #     Block material database
│   │   ├── PhysicsTypes.h            #     Type definitions
│   │   └── openmind_engine.h/cpp     #     Engine wrapper API
│   ├── llm/                          #   LLM provider connectors
│   │   ├── LLMInterface.h            #     Abstract interface
│   │   ├── OpenAIClient.h/cpp        #     OpenAI GPT models
│   │   ├── AnthropicClient.h/cpp     #     Anthropic Claude models
│   │   ├── GoogleClient.h/cpp        #     Google Gemini models
│   │   ├── OllamaClient.h/cpp        #     Local Ollama (offline)
│   │   ├── OpenAICompatibleClient.h/cpp  # Custom endpoint
│   │   ├── HttpClient.h/cpp          #     HTTP abstraction layer
│   │   ├── JSONValidator.h/cpp       #     JSON parsing + injection protection
│   │   ├── AsyncRequestManager.h/cpp #     Async request handling
│   │   ├── PromptTemplates.h         #     5 prompt templates
│   │   └── MockLLMClient.h           #     Test mock
│   ├── agents/                       #   Autonomous agent cognitive layer
│   │   ├── AgentCognitive.h          #     Base agent class
│   │   ├── CognitiveAgent.h          #     Full perceive→think→act loop
│   │   ├── MemorySystem.h            #     Short/long-term memory with consolidation
│   │   ├── GoalManager.h             #     Priority goal scheduling
│   │   ├── PerceptionSystem.h        #     Vision scanning + analysis
│   │   ├── AgentToolCalling.h         #     8 tool implementations
│   │   └── AgentCommunication.h      #     Inter-agent dialogue + gossip
│   ├── bridge/                       #   WASM interop layer
│   │   └── openmind_bridge.cpp       #     EMSCRIPTEN_KEEPALIVE exports
│   └── ui/                           #   Frontend source (JavaScript/CSS/HTML)
│       ├── index.html                #     Entry point
│       ├── omni_console.js           #     Core UI controller
│       ├── omni_console.css          #     Styles
│       ├── llm_client.js             #     JavaScript LLM client
│       ├── voxel_renderer.js         #     Three.js 3D renderer
│       ├── world_editor.js           #     Block place/break with raycasting
│       ├── touch_controls.js         #     Mobile touch input
│       ├── bridge.js                 #     PromptBridge — LLM→Engine commands
│       ├── world_io.js               #     IndexedDB persistence
│       ├── stats.js                  #     Live statistics display
│       ├── agent_think.js            #     Agent think loop (JS-side)
│       ├── agent_management.js       #     Multi-agent manager
│       ├── agent_config.js           #     Agent role/personality configs
│       ├── agent_prompt_builder.js   #     Agent context builder
│       ├── action_parser.js          #     LLM response parser
│       ├── interactive_objects.js    #     15 physics-enabled object types
│       ├── state_machine.js          #     State machine engine
│       ├── particle_system.js        #     9 particle effect presets
│       ├── skybox.js                 #     Dynamic sky rendering
│       ├── water_renderer.js         #     GLSL water shader
│       ├── sound_system.js           #     WebAudio engine
│       ├── copy_paste.js             #     Copy/paste with rotation/flip
│       ├── blueprints.js             #     Blueprint library + .BP files
│       ├── symmetry.js               #     Mirror tools
│       ├── pattern_tools.js          #     7 fill patterns
│       ├── building_history.js       #     Undo/redo system
│       ├── crafting_system.js        #     Recipe crafting
│       ├── inventory_system.js       #     36-slot inventory
│       ├── post_processing.js        #     Bloom/fog/color effects
│       ├── shortcuts.js              #     Keyboard shortcuts
│       ├── settings_panel.js         #     User settings
│       ├── notifications.js          #     Toast notifications
│       ├── error_handler.js          #     Error handling
│       ├── tutorial.js               #     Interactive tutorial
│       ├── loading_screen.js         #     Loading overlay
│       ├── ui_connection.js          #     WASM bridge wrapper
│       ├── ui_animations.js          #     Animation helpers
│       ├── utils.js                  #     Utility functions
│       ├── network.js                #     WebRTC networking
│       ├── host.js                   #     Multiplayer host
│       ├── client.js                 #     Multiplayer client
│       ├── multiplayer_ui.js         #     Multiplayer interface
│       ├── permissions.js            #     Role-based permissions
│       ├── chat.js                   #     Chat system
│       ├── player_entities.js        #     Player entity management
│       ├── manifest.json             #     PWA manifest
│       └── service-worker.js         #     PWA service worker
├── build/                            # Build system and output
│   ├── dist/                         #   Deployable frontend (run from here)
│   │   ├── index.html                #     Entry point
│   │   ├── openmind.js               #     WASM loader
│   │   ├── openmind.wasm             #     Compiled engine (253KB)
│   │   ├── omni_console.js           #     Core UI
│   │   ├── ... (60+ files)           #     All frontend assets
│   │   ├── manifest.json             #     PWA manifest
│   │   ├── service-worker.js         #     Service worker
│   │   └── icons/                    #     App icons (192px, 512px)
│   ├── wasm/                         #   WASM build artifacts (gitignored)
│   ├── build_wasm.bat                #   Windows WASM build
│   ├── build_wasm.sh                 #   Linux/macOS WASM build
│   ├── build_web.sh                  #   Asset copy script
│   ├── build.bat                     #   Windows native build
│   ├── build_native.sh               #   Linux/macOS native build
│   ├── run_tests.sh                  #   Test runner
│   ├── CMakeLists.txt                #   CMake config
│   └── deploy.sh                     #   Deploy script
├── tests/                            # C++ unit tests (26 files)
│   ├── test_engine_wrapper.cpp       #   Engine API
│   ├── test_agent.cpp                #   Agent cognition
│   ├── test_weather.cpp              #   Weather system
│   ├── test_temperature.cpp          #   Thermal
│   ├── test_combustion.cpp           #   Fire
│   ├── test_corrosion.cpp            #   Acid
│   ├── test_plant.cpp                #   Biology
│   ├── test_disease.cpp              #   Disease
│   ├── test_sound.cpp                #   Audio
│   ├── test_explosion.cpp            #   Explosion
│   ├── test_lighting.cpp             #   Lighting
│   ├── test_atmosphere.cpp           #   Atmosphere
│   ├── test_metabolism.cpp           #   Metabolism
│   ├── test_predator_prey.cpp        #   Population dynamics
│   ├── test_decay.cpp                #   Decomposition
│   ├── test_reverb.cpp               #   Acoustics
│   ├── test_step.cpp                 #   Step response
│   ├── test_orbital.cpp              #   Orbital mechanics
│   ├── test_llm.cpp                  #   LLM integration
│   ├── test_melting_only.cpp         #   Phase changes
│   └── test_melting_debug.cpp        #   Melting diagnostics
├── docs/                             # Documentation
│   ├── ARCHITECTURE.md               #   System architecture
│   ├── API_REFERENCE.md              #   Full API docs
│   ├── BUILD_GUIDE.md                #   Build instructions
│   ├── CHANGELOG.md                  #   Version history
│   ├── USER_MANUAL.md                #   User guide
│   └── LLM_PROMPT_TEMPLATES.md       #   Prompt engineering
├── *.js, *.css, *.html               # Root-level dev copies
├── *.cpp, *.h                        # Root-level C++ modules
├── *.bat, *.sh                       # Root-level build scripts
├── CMakeLists.txt                    # Root CMake config
├── package.json                      # Project metadata
├── LICENSE                           # MIT License
└── README.md                         # This file
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
Module.ccall('setWeather', null, ['number'], [type]);  // 0=Clear, 1=Rain, 2=Snow, 3=Storm, 4=Fog
Module.ccall('getWeather', 'string', [], []);
Module.ccall('setTimeScale', null, ['number'], [scale]);

// Agents
Module.ccall('getAgentCount', 'number', [], []);
Module.ccall('getAgentData', 'string', ['number'], [index]);
Module.ccall('addAgent', 'number', ['number','number','number'], [x, y, z]);

// Camera & navigation
Module.ccall('teleportCamera', null, ['number','number','number'], [x, y, z]);

// Overlays
Module.ccall('setOverlay', null, ['string','number'], [type, enabled]);

// Generation & persistence
Module.ccall('generateFromPrompt', null, ['string'], [prompt]);
Module.ccall('saveWorld', 'string', [], []);
Module.ccall('exportCSV', 'string', [], []);
Module.ccall('exportGLTF', 'string', [], []);
```

### Block Types

| ID | Name | Properties |
|----|------|-----------|
| 0 | AIR | Empty, passable |
| 1 | STONE | Dense, high strength |
| 2 | DIRT | Soft, organic |
| 3 | GRASS | Living, supports plants |
| 4 | WATER | Fluid, transparent |
| 5 | SAND | Granular, erodible |
| 6 | GLASS | Transparent, brittle |
| 7 | WOOD | Organic, flammable |
| 8 | LEAVES | Foliage, decayable |
| 9 | IRON | Metal, conductive |
| 10 | COPPER | Conductive, corrosion-resistant |
| 11 | GOLD | Soft, valuable |
| 12 | STEEL | Strong alloy |
| 13 | DIAMOND | Hardest |
| 14 | COAL | Fuel, burnable |
| 15 | BEDROCK | Indestructible |
| 16 | ASH | Fire residue |
| 17 | TNT | Explosive |
| 18 | SNOW | Cold precipitation |
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

### Environment Presets

| Preset | Gravity | Air Density | Temperature | Notes |
|--------|---------|-------------|-------------|-------|
| Earth | 9.81 m/s² | 1.225 kg/m³ | 293.15 K | Standard conditions |
| Moon | 1.62 m/s² | 0 kg/m³ | 100 K | Vacuum, no atmosphere |
| Mars | 3.71 m/s² | 0.020 kg/m³ | 210 K | Thin CO₂ atmosphere |
| Ocean | 9.81 m/s² | 1025 kg/m³ | 277 K | Underwater |
| Space | 0 m/s² | 0 kg/m³ | 2.7 K | Deep vacuum |
| Custom | Configurable | Configurable | Configurable | User-defined |

---

## Building from Source

### Native Build (for C++ Testing)

```bash
# Linux/macOS
mkdir build-native && cd build-native
cmake ../src -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
ctest --output-on-failure
```

```batch
REM Windows (VS 2022 Developer Command Prompt)
cd openmind
build.bat
```

### WASM Build (for Web Deployment)

```bash
# Linux/macOS
bash build/build_wasm.sh
cp build/wasm/openmind.{js,wasm} build/dist/
```

```batch
REM Windows
build\build_wasm.bat
copy build\wasm\openmind.* build\dist\
```

**Output:** `build/dist/openmind.js` (WASM loader) + `build/dist/openmind.wasm` (compiled engine, ~253KB)

---

## Desktop App (Tauri)

OpenMind can be packaged as a lightweight desktop application using [Tauri](https://tauri.app/) — delivering native performance with a ~14MB binary vs Electron's ~100MB+.

**Prerequisites:**
- [Rust](https://www.rust-lang.org/tools/install) (rustup + stable-msvc toolchain)
- [Node.js](https://nodejs.org/) (v18+)
- [Tauri CLI](https://tauri.app/start/) (`npm install -D @tauri-apps/cli`)
- [WebView2 Runtime](https://developer.microsoft.com/en-us/microsoft-edge/webview2/) (pre-installed on Windows 10/11)

**Build the Desktop App:**

```bash
# Install dependencies
npm install

# Run in development mode (starts dev server on :8080 + opens native window)
npm run tauri:dev

# Build for production
npm run tauri:build
```

**Output:**

| Platform | Installer |
|----------|-----------|
| Windows (MSI) | `src-tauri/target/release/bundle/msi/OpenMind_1.0.0_x64_en-US.msi` |
| Windows (NSIS) | `src-tauri/target/release/bundle/nsis/OpenMind_1.0.0_x64-setup.exe` |
| macOS | `src-tauri/target/release/bundle/dmg/OpenMind.app` |
| Linux | `src-tauri/target/release/bundle/appimage/OpenMind.AppImage` |

**Automatic Updates:**

The desktop app checks GitHub Releases hourly and prompts to install new versions automatically.

```bash
# Push a new update (bumps version, builds, creates GitHub release)
npm run publish:update -- 1.1.0
# or
publish.bat 1.1.0
```

Installed apps will then:
1. Detect the new release on GitHub
2. Show an "Update Available" notification
3. Download the new installer
4. Install it silently and relaunch automatically

**Why Tauri?**
- **Small Bundle:** ~14MB (vs Electron's ~100MB+)
- **Native Performance:** Runs with system WebView, no bundled Chromium
- **Same UI:** All HTML/JS/CSS and WASM engine remain unchanged
- **Local File Access:** Save/load worlds directly to your file system via native dialogs
- **Cross-Platform:** Windows, macOS, and Linux from the same codebase

---

## Testing

### C++ Unit Tests (26 files)

```bash
# Run all tests
bash build/run_tests.sh

# Or via ctest
cd build-native
ctest --output-on-failure
```

**Test coverage:** Engine API, agent cognition (memory, goals, perception, tools, communication), weather, temperature, combustion, corrosion, plant growth, disease, sound, explosion, lighting, atmosphere, orbital mechanics, LLM integration, phase changes, melting diagnostics, predator-prey dynamics, metabolism, decay, acoustics.

### Web Test Pages

Serve `build/dist/` and navigate to:

| Page | URL | Tests |
|------|-----|-------|
| Frontend | `/test_frontend.html` | UI components, panel system |
| Interactive Objects | `/test_interactive.html` | 15 object types, signals |
| Building Tools | `/test_building.html` | Selection, copy/paste, patterns |
| Visual Effects | `/test_visual.html` | Particles, water, sky, post-processing |
| Mobile | `/test_mobile.html` | Touch controls, responsive layout |
| Multiplayer | `/test_multiplayer.html` | WebRTC, sync, chat, permissions |
| WASM Bridge | `/test_bridge.html` | Engine connection, block ops |

---

## Roadmap

### v0.2.0 (Current)
- ✅ 57-module physics engine compiled to WASM
- ✅ 6 LLM providers with custom endpoint support
- ✅ Autonomous agent cognitive layer (7 components)
- ✅ Complete web frontend (60+ files)
- ✅ Mobile touch controls + PWA support
- ✅ Building tools (copy/paste, symmetry, blueprints, patterns)
- ✅ Interactive objects (15 types with signal wiring)
- ✅ World persistence via IndexedDB
- ✅ Multiplayer via WebRTC

### v1.0.0 (Planned)
- 🔄 Improved performance at scale (larger worlds)
- 🔄 Agent personality system and enhanced behaviors
- 🔄 Collaborative editing tools
- 🔄 Export/import improvements

### Future
- 📋 VR/AR support
- 📋 Plugin system for custom physics modules
- 📋 Cloud deployment and hosting
- 📋 Advanced AI agent hierarchies and teamwork

---

## License

MIT License. See [LICENSE](LICENSE).

---

## Acknowledgments

[Emscripten](https://emscripten.org) — WebAssembly compilation
· [Three.js](https://threejs.org) — 3D rendering
· [Groq](https://groq.com) — Fast LLM inference
· [OpenAI](https://openai.com) — GPT models
· [Anthropic](https://anthropic.com) — Claude models
· [Google AI](https://ai.google.dev) — Gemini models
· [Ollama](https://ollama.ai) — Local LLM serving
