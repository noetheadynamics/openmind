# OpenMind API Reference

## JavaScript API (via WASM Module)

All functions are called via `Module.ccall()` from JavaScript.

---

### World Functions

#### `initWorld()`
Initializes the voxel world with default terrain.

```javascript
Module.ccall('initWorld', null, [], []);
```

#### `setBlock(x, y, z, type, propsJson)`
Places a block with full material properties.

| Parameter | Type | Description |
|-----------|------|-------------|
| x, y, z | number | World coordinates |
| type | number | BlockType enum (0-255) |
| propsJson | string | JSON string of MaterialProps |

```javascript
const props = JSON.stringify({
    mass: 7.85, density: 7850, hardness: 8.0,
    tensileStrength: 400, baseColor: "#71797E"
});
Module.ccall('setBlock', 'number', ['number','number','number','number','string'], [5, 2, 5, 12, props]);
```

#### `setBlockSimple(x, y, z, type)`
Places a block with default properties.

```javascript
Module.ccall('setBlockSimple', 'number', ['number','number','number','number'], [5, 2, 5, 1]);
```

#### `getBlock(x, y, z)`
Returns JSON string of block data at position.

```javascript
const json = Module.ccall('getBlock', 'string', ['number','number','number'], [5, 2, 5]);
const block = JSON.parse(json);
// { type: 12, state: 0, occupied: true, temperature: 293.15, stress: 0 }
```

#### `removeBlock(x, y, z)`
Removes block at position.

```javascript
Module.ccall('removeBlock', null, ['number','number','number'], [5, 2, 5]);
```

#### `tickPhysicsDelta(delta)`
Advances physics simulation by delta seconds.

```javascript
Module.ccall('tickPhysicsDelta', 'number', ['number'], [1/60]);
```

#### `getWorldStats()`
Returns JSON string of world statistics.

```javascript
const json = Module.ccall('getWorldStats', 'string', [], []);
const stats = JSON.parse(json);
// { totalBlocks, currentTick, timeScale, averageTemperature, livingEntities }
```

---

### Time Functions

#### `setTimeOfDay(hours)`
Sets time of day (0-24 hours).

```javascript
Module.ccall('setTimeOfDay', null, ['number'], [12]); // Noon
```

#### `getTimeOfDay()`
Returns current time of day in hours.

```javascript
const hours = Module.ccall('getTimeOfDay', 'number', [], []);
```

#### `getSunlightIntensity()`
Returns sun intensity (0.0 - 1.0).

```javascript
const intensity = Module.ccall('getSunlightIntensity', 'number', [], []);
```

#### `setCycleDuration(seconds)`
Sets duration of one full day/night cycle.

```javascript
Module.ccall('setCycleDuration', null, ['number'], [600]); // 10 minutes
```

#### `rewindTime(seconds)`
Rewinds time by specified seconds.

```javascript
Module.ccall('rewindTime', 'number', ['number'], [60]); // Rewind 1 minute
```

---

### Weather Functions

#### `setWeather(type)`
Sets weather type.

| Type | Value | Description |
|------|-------|-------------|
| CLEAR | 0 | Clear sky |
| RAIN | 1 | Rain |
| SNOW | 2 | Snow |
| STORM | 3 | Thunderstorm |
| FOG | 4 | Fog |

```javascript
Module.ccall('setWeather', null, ['number'], [1]); // Rain
```

#### `getWeather()`
Returns JSON string of weather data.

```javascript
const json = Module.ccall('getWeather', 'string', [], []);
const weather = JSON.parse(json);
// { type: 1, visibility: 0.8, windSpeed: 5.2 }
```

#### `setTimeScale(scale)`
Sets simulation speed multiplier.

```javascript
Module.ccall('setTimeScale', null, ['number'], [10]); // 10x speed
```

---

### Agent Functions

#### `getAgentCount()`
Returns number of active agents.

```javascript
const count = Module.ccall('getAgentCount', 'number', [], []);
```

#### `getAgentData(index)`
Returns JSON string of agent data.

```javascript
const json = Module.ccall('getAgentData', 'string', ['number'], [0]);
const agent = JSON.parse(json);
// { name, role, health, hunger, energy, x, y, z, goal, memories, thoughts, relationships, inventory }
```

#### `setAgentPosition(index, x, y, z)`
Moves agent to position.

```javascript
Module.ccall('setAgentPosition', null, ['number','number','number','number'], [0, 10, 5, 10]);
```

---

### Camera Functions

#### `teleportCamera(x, y, z)`
Moves camera to position.

```javascript
Module.ccall('teleportCamera', null, ['number','number','number'], [10, 20, 10]);
```

---

### Overlay Functions

#### `setOverlay(type, enabled)`
Toggles visual overlay.

| Type | Description |
|------|-------------|
| stress | Structural stress heatmap |
| temperature | Temperature heatmap |
| radiation | Radiation heatmap |
| thoughts | Agent thought bubbles |
| density | Block density map |

```javascript
Module.ccall('setOverlay', null, ['string','number'], ['stress', 1]); // Enable
Module.ccall('setOverlay', null, ['string','number'], ['stress', 0]); // Disable
```

---

### Generation Functions

#### `generateFromPrompt(prompt)`
Generates world from natural language description.

```javascript
Module.ccall('generateFromPrompt', null, ['string'], ['Create a 10x10 stone platform']);
```

#### `exportWorld(format)`
Exports world in specified format.

| Format | Description |
|--------|-------------|
| gltf | 3D model (GL Transmission Format) |
| fbx | 3D model (Autodesk FBX) |
| obj | 3D model (Wavefront OBJ) |
| csv | Spreadsheet data |
| json | Raw voxel data |
| vtk | Scientific visualization |
| mp4 | Video recording |
| omw | OpenMind World format |
| omm | OpenMind Material pack |

```javascript
const data = Module.ccall('exportWorld', 'string', ['string'], ['json']);
```

#### `saveWorld()`
Returns JSON string of complete world state.

```javascript
const json = Module.ccall('saveWorld', 'string', [], []);
```

---

## Block Types

| Value | Name | Description |
|-------|------|-------------|
| 0 | AIR | Empty space |
| 1 | STONE | Solid rock |
| 2 | DIRT | Soil |
| 3 | GRASS | Grass block |
| 4 | WATER | Liquid water |
| 5 | SAND | Granular sand |
| 6 | GLASS | Transparent glass |
| 7 | WOOD | Tree wood |
| 8 | LEAVES | Tree leaves |
| 9 | IRON | Iron ore/metal |
| 10 | COPPER | Copper ore/metal |
| 11 | GOLD | Gold ore/metal |
| 12 | STEEL | Steel alloy |
| 13 | DIAMOND | Diamond |
| 14 | COAL | Coal ore |
| 15 | BEDROCK | Indestructible base |
| 16 | ASH | Fire residue |
| 17 | TNT | Explosive |
| 18 | SNOW | Snow block |
| 255 | CUSTOM | User-defined material |

---

## MaterialProps (JSON)

```json
{
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
    "baseColor": "#888888",
    "growthRate": 0.0,
    "decayRate": 0.0
}
```

---

## Weather Types

| Value | Name | Effects |
|-------|------|---------|
| 0 | CLEAR | Normal visibility, no precipitation |
| 1 | RAIN | Reduced visibility, water blocks grow |
| 2 | SNOW | Cold temperature, snow accumulation |
| 3 | STORM | Very low visibility, lightning possible |
| 4 | FOG | Very low visibility, temperature stable |

---

## Event System

The EngineConnection class emits events:

```javascript
engine.on('log', (data) => {
    // data.msg: string
    // data.type: 'ok' | 'warn' | 'err' | ''
});

engine.on('connected', () => {
    // WASM engine loaded successfully
});
```
