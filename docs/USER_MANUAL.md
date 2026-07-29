# OpenMind User Manual

## Getting Started

### Opening OpenMind

1. Open `web/index.html` in a modern browser (Chrome, Firefox, Edge)
2. The loading screen appears while the WASM engine initializes
3. Once loaded, you see the Omni-Console interface

### First Steps

1. **Type a command** in the AI Prompt panel:
   - "Create a 10x10 stone platform"
   - "Place a diamond block at 5,5,5"
   - "Set weather to rain"

2. **Click Generate** or press Ctrl+Enter to execute

3. **Explore the panels** using the top bar buttons

---

## AI Prompt Panel

### Basic Commands

| Command | Effect |
|---------|--------|
| `weather clear` | Set weather to clear |
| `weather rain` | Set weather to rain |
| `weather snow` | Set weather to snow |
| `weather storm` | Set weather to storm |
| `weather fog` | Set weather to fog |
| `time 12` | Set time to noon |
| `pause` | Pause simulation |
| `resume` | Resume simulation |
| `speed 10` | Set speed to 10x |
| `help` | Show available commands |

### Natural Language

Type any description to generate a world:
- "A small village with wooden houses and a stone wall"
- "An alien landscape with glowing crystals"
- "A medieval castle on a hill"

### Keyboard Shortcuts

| Key | Action |
|-----|--------|
| Ctrl+Enter | Send prompt |
| Tab | Autocomplete suggestion |
| Up Arrow | Previous prompt |
| Down Arrow | Next prompt |

---

## Time Controls Panel

### Time Display
Shows current time (HH:MM:SS) and tick count.

### Play/Pause
Click the play/pause button to toggle simulation.

### Speed Control
Use the slider to set speed:
- 0.1x — Very slow (debugging)
- 0.25x — Slow motion
- 0.5x — Half speed
- 1x — Normal speed
- 2x — Double speed
- 5x — Fast forward
- 10x — Very fast
- 100x — Ultra fast

### Rewind/Forward
- **⏪ 10s** — Rewind 10 seconds
- **⏪ 1m** — Rewind 1 minute
- **⏪ 5m** — Rewind 5 minutes
- **10s ⏩** — Fast-forward 10 seconds
- **1m ⏩** — Fast-forward 1 minute
- **5m ⏩** — Fast-forward 5 minutes

### Time Presets
Quick-set time to:
- Midnight (0:00)
- Sunrise (6:00)
- Noon (12:00)
- Sunset (18:00)

### Cycle Duration
Adjust how long one full day/night cycle takes (1 minute to 2 hours).

---

## Visual Overlays Panel

Toggle overlays to visualize different data:

| Overlay | Description |
|---------|-------------|
| **Structural Stress** | Red = high stress, green = stable |
| **Temperature** | Blue = cold, red = hot |
| **Radiation** | Purple = high radiation |
| **Agent Thoughts** | Speech bubbles above agents |
| **Ecosystem Graph** | Live population chart |
| **Density Map** | Block density visualization |

---

## Agent Dashboard Panel

### Agent List
Shows all active agents with:
- Name
- Role
- Health (HP)

### Agent Inspector
Click an agent to view detailed information:

- **Health/Hunger/Energy** bars
- **Current Goal** — What the agent is trying to do
- **Recent Memories** — Last 5 memory entries
- **Thoughts** — Latest LLM reasoning
- **Relationships** — Friends and enemies
- **Inventory** — Items the agent is carrying

---

## Environment Presets Panel

### Presets

| Preset | Gravity | Air | Temp | Description |
|--------|---------|-----|------|-------------|
| 🌍 Earth | 9.81 | 1.225 | 293K | Default Earth conditions |
| 🌙 Moon | 1.62 | 0 | 100K | No atmosphere |
| 🔴 Mars | 3.71 | 0.020 | 210K | Thin atmosphere |
| 🌊 Ocean | 9.81 | 1025 | 277K | Underwater |
| 🚀 Space | 0 | 0 | 2.7K | Zero gravity |

### Custom
Select "Custom" to adjust:
- Gravity (0-20 m/s²)
- Air Density (0-5 kg/m³)
- Temperature (0-600 K)
- Humidity (0-100%)

Click **Apply Preset** to activate.

---

## Material Forge Panel

### Generate Materials

1. Describe the material in the text box:
   - "A lightweight, fireproof, translucent alien glass that glows blue"
   - "A super-strong metal alloy that conducts electricity"
   - "A biodegradable plastic that decomposes in water"

2. Click **Generate Material**

3. View the generated properties:
   - Color swatch
   - Mass, Hardness, Melting Point
   - Density, Tensile Strength

4. Click **Add to World** to use it

### Recent Materials
View your last 5 generated materials. Click to re-inspect.

---

## Export Hub Panel

### 3D Models
- **.GLTF** — Web standard 3D format
- **.FBX** — Autodesk format
- **.OBJ** — Universal 3D format

### Architecture
- **.IFC** — Building information model
- **.STEP** — CAD exchange format
- **.DWG** — AutoCAD format

### Scientific Data
- **.CSV** — Spreadsheet data
- **.JSON** — Raw voxel data
- **.VTK** — Scientific visualization

### Video
- **.MP4** — Compressed video
- **.MOV** — QuickTime video

### World Data
- **.OMW** — OpenMind World save
- **.OMM** — OpenMind Material pack

---

## Teleport & Navigation Panel

### Teleport
1. Enter X, Y, Z coordinates
2. Click **Go** to teleport camera

### Bookmarks
1. Position camera at a location
2. Click **+ Save Current Position**
3. Click a bookmark to teleport back
4. Click ✕ to delete a bookmark

### Freefly Camera
Toggle to detach camera from agents and fly freely.

---

## AI Brain Switcher Panel

### Cloud Mode
Select a cloud LLM provider:
1. Choose provider (OpenAI, Anthropic, Google)
2. Choose model (GPT-4, Claude 3, Gemini Pro)
3. Enter API key
4. Click **Test Connection**

### Local Mode
Use a local LLM:
1. Select Ollama or LM Studio
2. Choose model (Llama 3, Mistral)
3. Set endpoint (default: http://localhost:11434)
4. Click **Test Connection**

### Connection Status
- 🟢 Green = Connected
- 🔴 Red = Disconnected
- 🟡 Yellow = Testing...

---

## Dark/Light Mode

Click the ◐ button in the top-right corner to toggle between dark and light themes.

---

## Tips

1. **Use Ctrl+Enter** to send prompts quickly
2. **Tab autocomplete** saves typing
3. **Speed 10x** is good for watching physics play out
4. **Temperature overlay** helps find heat sources
5. **Agent Dashboard** auto-refreshes every 2 seconds
6. **Material Forge** generates mock data without LLM connection
7. **Bookmarks** save interesting camera positions
8. **Console log** (bottom-right) shows all engine messages
