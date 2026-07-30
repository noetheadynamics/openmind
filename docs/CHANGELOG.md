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

## [0.2.0] - 2026-07-30

### Added

#### Phase 5.5a – WASM Bridge Layer
- `ui_connection.js`: EngineConnection class wrapping all Module.ccall() calls
- Robust connection status tracking with 3s retry interval
- Tick, weather, time, stats, and agent data APIs
- `test_bridge.html`: Integration test page

#### Phase 5.5b – Renderer ↔ Engine Sync
- `voxel_renderer.js`: Full Three.js InstancedMesh chunk renderer with WASM sync
- Raycasting for block hit detection with face normals
- Dirty flag-driven mesh rebuild on world changes
- Update loop pulls block data from WASM at 200ms intervals

#### Phase 5.5c – Block Editing
- `world_editor.js`: WorldEditor with right-click break and ghost preview
- Semi-transparent ghost mesh snaps to grid with face detection
- Place/break via raycast coordinates + face normals
- `contextmenu` prevention for right-click handling

#### Phase 5.5d – Prompt Bridge (LLM → Engine)
- `bridge.js`: PromptBridge with 7 command handlers
  - `place_block`, `break_block`, `spawn_agent`, `set_environment`
  - `set_time_scale`, `generate_world`, `generate_material`
- Raw fetch() calls to OpenAI/Anthropic/Google endpoints
- JSON parsing with regex fallback for malformed responses
- Commands dispatched to engine/renderer/agent manager

#### Phase 5.5e – World I/O
- `world_io.js`: IndexedDB-based persistence
- Auto-save every 30s with configurable interval
- `.OMW` export/import with file download/upload
- World listing with timestamps and delete capability
- Ctrl+S shortcut for quick save

#### Phase 5.5f – Live Stats
- `stats.js`: LiveStats class polling engine + renderer every 1s
- Displays 7 stat elements: blocks, tick, FPS, temperature, weather, time, agents
- Ecosystem graph with predator/prey population tracking via rAF

#### Phase 5.5g – Agent Loop
- `agent_think.js`: 15-iteration think loop with 12 tools
- `agent_management.js`: Multi-agent manager with IndexedDB persistence
- `agent_config.js`: 6 roles, 5 personalities, goal presets
- `agent_prompt_builder.js`: Dynamic world state + memory context builder
- `action_parser.js`: JSON validation + tool call extraction
- 3 API providers (Groq, OpenAI, Anthropic) with per-agent model selection

#### Phase 5.5h – Interactive Objects
- `interactive_objects.js`: 15 object types (door, button, lever, switch, chest, lamp, piston, conveyor, trapdoor, fire, lock, launcher, sensor, timer, computer)
- Signal propagation system with configurable wiring
- Serialization/deserialization for save/load support
- `state_machine.js`: 50+ states with transitions and animations

#### Phase 5.5i – Visual Features
- `particle_system.js`: 9 particle presets with pool-based allocation
- `water_renderer.js`: Custom GLSL shaders with waves, reflections, Fresnel
- `skybox.js`: Dynamic sky with sun/moon/stars/clouds + CSS gradient
- `sound_system.js`: Audio context with weather ambient support

#### Phase 5.5j – Building Tools
- `copy_paste.js`: Copy/paste with rotation (4×90°) and X/Y/Z flip, ghost preview
- `blueprints.js`: IndexedDB blueprint library with `.BP` export/import
- `symmetry.js`: X/Y/Z mirroring with visual plane meshes
- `pattern_tools.js`: 7 fill patterns (solid, checker, stripe, gradient_h, random, line, circle, sphere)
- Ctrl+C/Ctrl+V keyboard shortcuts for copy/paste

#### Phase 5.5k – Mobile Polish
- `touch_controls.js`: Full mobile input — joystick, tap/double-tap, pinch zoom, two-finger look
- 9-slot hotbar with touch selection, action buttons (jump/place/break/inventory)
- Mobile detection in OmniConsole (`_isMobile`) with auto-init of TouchControls
- `manifest.json`: PWA config with standalone display, icons (192+512)
- `service-worker.js`: Caches 45 core assets, API pass-through, offline fallback
- Responsive CSS at 768px/480px/360px breakpoints with 44px touch targets
- Mobile performance throttles: 300ms sim loop, `requestIdleCallback` for sky/water

### Fixed
- Right-click context menu prevented in voxel renderer for block breaking
- WASM bridge retry logic prevents infinite loop on load failure
- Agent loop uses `_thinking` flag to prevent concurrent think cycles

### Known Issues
- Full engine.tick() is slow at large scan ranges
- Particle slot reuse requires scanning for inactive slots
- Product blocks in checkReactions() need proper tensileStrength
