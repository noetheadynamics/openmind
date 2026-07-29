# OpenMind

**Universal Voxel-Based Sandbox Emulator with AI-Powered Agents**

OpenMind is a real-time voxel physics engine with an LLM-powered AI brain, autonomous agent cognitive layer, and a glassmorphism web frontend. It simulates structural stress, temperature, weather, biology, orbital mechanics, and agent reasoning — all in WebAssembly.

## Features

### Physics Engine (57 Modules)
Structural stress, thermal conduction, wind, precipitation, chemical reactions, combustion, plant growth, disease, predator-prey dynamics, sound propagation, lighting, and more.

### LLM Connector (5+ Providers)
OpenAI, Anthropic, Google Gemini, Groq, Ollama, and any OpenAI-compatible endpoint via the Custom provider option.

### Agent Cognitive Layer
Memory system, goal manager, perception, 8 tool-calling actions, inter-agent communication, and full cognitive loop — all tested with 49 unit tests.

### Omni-Console Frontend
Glassmorphism UI with AI prompt, time controls, visual overlays, agent dashboard, environment presets, material forge, export hub, multiplayer, and mobile support.

## Quick Start

### Web (No Build)
```bash
# Serve the frontend
python -m http.server 8080 -d web
# Open http://localhost:8080
```

### WASM Build
```bash
# Install Emscripten SDK
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk && ./emsdk install latest && ./emsdk activate latest
source ./emsdk_env.sh

# Build
bash build/build_wasm.sh

# Serve
python -m http.server 8080 -d web
```

### Native Build (Testing)
```bash
bash build/build_native.sh
bash build/run_tests.sh
```

## Project Structure

```
openmind/
├── src/                    # C++ source code
│   ├── engine/             # Physics engine (57 features)
│   ├── llm/                # LLM connector (5+ providers)
│   ├── agents/             # Agent cognitive layer
│   ├── bridge/             # WASM bridge (EMSCRIPTEN_KEEPALIVE)
│   └── ui/                 # Legacy UI files
├── web/                    # Frontend (served directly)
│   ├── index.html          # Main entry point
│   ├── omni_console.js     # Core UI logic
│   ├── omni_console.css    # Glassmorphism styles
│   ├── *.js                # Feature modules (30+ files)
│   └── test_*.html         # Test pages
├── tests/                  # C++ unit tests (26 files)
├── docs/                   # Architecture, API reference, user manual
├── build/                  # Build scripts and CMake config
├── CMakeLists.txt          # Root CMake configuration
├── package.json            # Project metadata
└── manifest.json           # PWA manifest
```

## API

```javascript
// World manipulation
Module.ccall('initWorld', null, [], []);
Module.ccall('setBlockSimple', 'number', ['number','number','number','number'], [x, y, z, type]);
Module.ccall('getBlockData', 'string', ['number','number','number'], [x, y, z]);
Module.ccall('tickPhysicsDelta', 'number', ['number'], [delta]);
Module.ccall('getWorldStats', 'string', [], []);

// AI generation
Module.ccall('generateFromPrompt', null, ['string'], [prompt]);
Module.ccall('saveWorld', 'string', [], []);
```

## Block Types
```
0=AIR  1=STONE  2=DIRT  3=GRASS  4=WATER  5=SAND  6=GLASS
7=WOOD  8=LEAVES  9=IRON  10=COPPER  11=GOLD  12=STEEL
13=DIAMOND  14=COAL  15=BEDROCK  16=ASH  17=TNT  18=SNOW
```

## License

MIT
