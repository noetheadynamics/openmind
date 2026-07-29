# Changelog

All notable changes to OpenMind are documented here.

## [0.1.0] - 2026-07-27

### Added

#### Core Engine (57 Features)
- VoxelOctree sparse octree storage (max depth 8, 256³)
- PhysicsEngine with 26+ subsystems
- Structural stress simulation with force propagation
- Material tensile strength and block breaking
- Thermal conduction and specific heat capacity
- Wind system with direction and speed
- Air pressure simulation
- Cloud formation and precipitation
- Humidity tracking
- Radiation simulation
- Magnetism system
- Chemical reactions between blocks
- Acid corrosion mechanics
- Combustion and fire spread
- Metallurgy (melting, smelting, alloying)
- Plant growth system
- Decay and decomposition
- Disease system for biological entities
- Predator-prey dynamics
- Agent metabolism
- Sound propagation and reverb
- Lighting engine with day/night cycle
- Time of day and sun position
- Weather system (Clear, Rain, Snow, Storm, Fog)
- Block type system (19 types + CUSTOM)
- Material properties (mass, density, hardness, etc.)

#### LLM Connector (5 Providers)
- OpenAI client (GPT-4, GPT-4o, GPT-4o Mini)
- Anthropic client (Claude 3 Opus/Sonnet/Haiku)
- Google Gemini client (Pro/Ultra/Nano)
- Ollama client (Llama 3, Mistral, Code Llama)
- OpenAI-Compatible client (Groq, Together, any endpoint)
- Mock LLM client for testing
- HttpClient (WinHTTP for Windows)
- JSON validator with injection protection
- Async request manager with thread pool
- 5 prompt templates (world gen, materials, agents, commands, dialogue)

#### Agent Cognitive Layer
- CognitiveAgent with perceive→think→act loop
- MemorySystem (short-term + long-term, importance decay)
- GoalManager (priority queue, sub-goals, progress tracking)
- PerceptionSystem (vision range scanning, agent detection)
- AgentToolCalling (8 tools: move_to, place_block, break_block, search_for, talk_to, scan_inventory, craft, build_structure)
- AgentCommunication (LLM-powered dialogue, gossip propagation)
- 49 tests passing

#### Omni-Console Frontend
- Glassmorphism design with backdrop-filter blur
- Dark/Light mode toggle
- 9 panels: AI Prompt, Time Controls, Visual Overlays, Agent Dashboard, Environment Presets, Material Forge, Export Hub, Teleport, AI Brain Switcher
- AI Prompt with autocomplete, history, command parsing
- Time controls with play/pause, rewind/forward, speed slider
- Visual overlays: stress, temperature, radiation, thoughts, ecosystem, density
- Agent dashboard with inspector panel
- Environment presets: Earth, Moon, Mars, Ocean, Space, Custom
- Material Forge with AI generation
- Export Hub (GLTF, FBX, OBJ, IFC, STEP, DWG, CSV, JSON, VTK, MP4, MOV, OMW, OMM)
- Teleport with bookmarks
- AI Brain Switcher (Cloud/Local LLM)
- Floating stats overlay
- Console log

#### Build System
- Emscripten WASM build script
- Native build script (Linux/macOS)
- Web build script (asset copying)
- Test runner script
- Deploy script
- CMakeLists.txt

#### Documentation
- README.md with full feature list
- ARCHITECTURE.md with system diagrams
- API_REFERENCE.md with all functions
- USER_MANUAL.md with usage guide
- BUILD_GUIDE.md with compilation instructions
- CHANGELOG.md (this file)

#### Project Structure
- Organized src/ directory (engine, llm, agents, ui, bridge)
- tests/ directory for all test files
- docs/ directory for documentation
- build/ directory for scripts and output
- demo_world.json for demonstration
- package.json

### Fixed
- PerceptionSystem block detection in VoxelOctree
- MemorySystem long-term eviction thresholds
- Agent tool searchFor return values
- Test assertion expectations

### Changed
- Default scan range from 256 to runtime-configurable via setScanRange()
- BlockType enum expanded to include SNOW (18) and CUSTOM (255)
- VoxelData constructor defaults state to SOLID
- isWaterBlock() requires state == LIQUID + composition "H2O"

### Known Issues
- Emscripten SDK not installed — WASM build blocked
- Full engine.tick() is slow at large scan ranges
- Particle slot reuse requires scanning for inactive slots
- Product blocks in checkReactions() need proper tensileStrength
