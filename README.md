# OpenMind

**In-browser voxel simulation engine with LLM-powered autonomous agents.**

[![MIT License](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.2.0-blue)](#)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-orange)](src/)
[![WebAssembly](https://img.shields.io/badge/WebAssembly-supported-yellow)](#)
[![57 Physics Modules](https://img.shields.io/badge/Physics-57%20Modules-brightgreen)](#)

---

## Overview

OpenMind is a real-time voxel physics engine compiled to WebAssembly, paired with a multi-provider LLM integration layer and autonomous agent cognitive system — all running directly in the browser.

The engine simulates structural stress, thermodynamics, fluid dynamics, weather, chemistry, biology, electricity, magnetism, sound propagation, orbital mechanics, and agent cognition within a 256³ voxel world. The frontend provides a complete environment for world building, real-time simulation control, AI-assisted creation, and multiplayer collaboration.

### Architecture

```
User → JavaScript Frontend → WASM Bridge → PhysicsEngine (C++17)
                                         → LLM Connector (6 providers)
                                         → Agent Cognitive Layer
```

The system follows a **Rules + Properties** pattern: rules define behaviors (physics laws, LLM reasoning, agent cognition), while properties define state (block types, material properties, agent memories). This separation ensures determinism and extensibility.

---

## Key Capabilities

### Physics Engine — 57 Modules

| Domain | Modules |
|--------|---------|
| **Core** | VoxelOctree sparse storage (256³, depth 8), Block Properties (mass, density, hardness), Gravity with orbital mechanics, Lighting propagation |
| **Fluids** | Water simulation, Lava, Buoyancy, Pressure System, Convection, Gas Diffusion |
| **Thermal** | Thermal Conduction, Specific Heat, Thermal Radiation, Phase Changes (melt/freeze/boil), Evaporation, Condensation |
| **Weather** | Wind, Air Pressure, Cloud Formation, Precipitation, Humidity, Temperature, Weather Transitions, Atmosphere Layer |
| **Chemistry** | Chemical Reactions, Acid Corrosion, Combustion, Metallurgy, Crystal Growth, Dissolution |
| **Biology** | Plant Growth, Decay, Disease, Predator-Prey, Metabolism |
| **Physics** | Structural Stress, Tensile Strength, Stress Propagation, Erosion, Sediment Transport, Fragments |
| **Electromagnetism** | Electricity, Magnetism, Radiation |
| **Audio** | Sound Propagation, Reverb Simulation |
| **Space** | Orbital Mechanics |
| **Systems** | Fire, Smoke, Explosion, Lightning, Time of Day, Oxygen |
| **AI** | Agent Cognition (memory, goals, perception, tools, communication) |

### LLM Integration — 6 Providers

| Provider | Models | Use Case |
|----------|--------|----------|
| **Groq** | Llama 3.3 70B, Llama 3.1 8B, Mixtral 8x7B | Fast inference (default) |
| **OpenAI** | GPT-4o, GPT-4o Mini, GPT-4 Turbo | High-quality reasoning |
| **Anthropic** | Claude 3 Opus, Sonnet, Haiku | Long-context tasks |
| **Google Gemini** | Gemini 2.0 Flash, 1.5 Pro, 1.5 Flash | Multimodal |
| **Ollama** | Any local model | Offline / private |
| **Custom** | OpenAI-compatible endpoint (vLLM, Together, DeepSeek, etc.) | Bring your own |

### Autonomous Agent System

- **Memory System**: Short-term + long-term memory with importance-based consolidation
- **Goal Manager**: Priority queue with sub-goal decomposition and progress tracking
- **Perception System**: Block scanning, environment analysis, entity detection
- **Tool Calling**: 8 tools — move_to, place_block, break_block, search_for, talk_to, scan_inventory, craft, build_structure
- **Agent Communication**: LLM-powered dialogue and gossip propagation
- **Full Cognitive Loop**: Observe → Think → Decide → Act at configurable intervals

### Frontend

| Feature | Description |
|---------|-------------|
| AI Command Console | Natural language world generation, material creation, agent control |
| Simulation Controls | Play/pause, speed slider, rewind, day/night cycle |
| Visual Overlays | Structural stress, temperature, radiation, ecosystem, moisture |
| Environment Presets | Earth, Moon, Mars, Ocean, Space, Custom (gravity, air density, temperature) |
| Material Forge | AI-generated block materials with custom properties |
| Building Tools | Selection, copy/paste with rotation/flip, symmetry, fill patterns, blueprints, undo/redo |
| Interactive Objects | 15 types: doors, buttons, levers, chests, pistons, sensors, timers, computers |
| Inventory & Crafting | 36-slot grid, recipe-based crafting with AI assistance |
| Export Hub | GLTF, CSV, JSON, OMW world format |
| Multiplayer | WebRTC peer-to-peer, real-time sync, role-based permissions |

### PWA & Mobile

- Touch-optimized controls (joystick, tap/double-tap, pinch zoom, two-finger look)
- Installable via "Add to Home Screen"
- Offline support via service worker caching
- Responsive layout across desktop, tablet, and phone

---

## Quick Start

### Prerequisites

- **Python 3.6+** (for local HTTP server)
- **Emscripten SDK** (optional, for WASM rebuild)

### Run (no build required)

```bash
git clone https://github.com/noetheadynamics/openmind.git
cd openmind
python -m http.server 8080 -d build/dist
```

Open [http://localhost:8080](http://localhost:8080) — pre-built WASM is included.

### Rebuild WASM from Source

```bash
# Install Emscripten: https://emscripten.org/docs/getting_started/downloads.html

# Linux/macOS
bash build/build_wasm.sh
cp build/wasm/openmind.{js,wasm} build/dist/

# Windows
build\build_wasm.bat
copy build\wasm\openmind.* build\dist\
```

---

## Project Structure

```
openmind/
├── src/                  # C++ source
│   ├── engine/           #   PhysicsEngine, VoxelOctree, material system
│   ├── llm/              #   6 provider clients, HTTP, JSON, async
│   ├── agents/           #   Memory, goals, perception, tools, communication
│   ├── bridge/           #   WASM EMSCRIPTEN_KEEPALIVE exports
│   └── ui/               #   Frontend source (JS, CSS, HTML)
├── build/
│   ├── dist/             #   Deployable frontend (run from here)
│   ├── wasm/             #   WASM build output (gitignored)
│   ├── build_wasm.bat    #   Windows WASM build
│   ├── build_wasm.sh     #   Linux/macOS WASM build
│   └── build.bat         #   Windows native build
├── tests/                # 26 C++ unit tests
├── docs/                 # Architecture, API, build guide, changelog
├── CMakeLists.txt        # Root CMake config
└── LICENSE               # MIT
```

---

## API

The WASM module exposes the engine via `Module.ccall()`:

```
initWorld()              setBlock(x,y,z,type,props)     getBlockData(x,y,z)
tickPhysicsDelta(dt)     getWorldStats()                setTimeOfDay(hours)
getTimeOfDay()           setWeather(type)               getWeather()
getAgentCount()          getAgentData(index)            addAgent(x,y,z)
teleportCamera(x,y,z)    generateFromPrompt(prompt)     saveWorld()
```

See [API_REFERENCE.md](docs/API_REFERENCE.md) for full documentation.

---

## Building from Source

| Platform | Native Build | WASM Build |
|----------|-------------|------------|
| **Linux/macOS** | `cmake ../src && make` | `bash build/build_wasm.sh` |
| **Windows** | `build.bat` (VS 2022) | `build\build_wasm.bat` |

### Native Build

```bash
mkdir build-native && cd build-native
cmake ../src -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)          # Linux/macOS
ctest --output-on-failure
```

---

## Testing

- **26 C++ unit tests**: Engine API, agents, weather, thermal, combustion, corrosion, biology, disease, sound, explosion, lighting, atmosphere, orbital mechanics, LLM integration
- **8 web test pages**: Frontend, interactive objects, building tools, visual effects, UX, mobile, multiplayer, WASM bridge

```bash
# C++ tests
bash build/run_tests.sh

# Web — serve build/dist/ and open /test_*.html
```

---

## License

MIT License. See [LICENSE](LICENSE).

---

## Acknowledgments

[Emscripten](https://emscripten.org) · [Three.js](https://threejs.org) · [Groq](https://groq.com) · [OpenAI](https://openai.com) · [Anthropic](https://anthropic.com) · [Google AI](https://ai.google.dev) · [Ollama](https://ollama.ai)
