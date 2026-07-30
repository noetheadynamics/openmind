class PromptBridge {
    constructor(llm, engine, options = {}) {
        this.llm = llm;
        this.engine = engine;
        this.console = options.console || null;
        this.commandCount = 0;
        this.errorCount = 0;
        this.lastResult = null;

        this.envMap = { EARTH: 0, MOON: 1, MARS: 2, OCEAN: 3, SPACE: 4, CUSTOM: 5 };
        this.weatherMap = { CLEAR: 0, RAIN: 1, SNOW: 2, STORM: 3, FOG: 4 };
        this.typeMap = {
            AIR: 0, STONE: 1, DIRT: 2, GRASS: 3, WATER: 4, SAND: 5,
            GLASS: 6, WOOD: 7, LEAVES: 8, IRON: 9, COPPER: 10,
            GOLD: 11, STEEL: 12, DIAMOND: 13, COAL: 14, BEDROCK: 15,
            ASH: 16, TNT: 17, SNOW: 18, DOOR: 30, BUTTON: 31,
            LAUNCHER: 32, LOCK: 33, LAMP: 34, CHEST: 35,
            SWITCH: 36, CONVEYOR: 37, PISTON: 38, TRAPDOOR: 39, FIRE: 40
        };

        this.commandHandlers = {
            place_block: (cmd) => this.handlePlaceBlock(cmd),
            break_block: (cmd) => this.handleBreakBlock(cmd),
            spawn_agent: (cmd) => this.handleSpawnAgent(cmd),
            set_environment: (cmd) => this.handleSetEnvironment(cmd),
            set_time_scale: (cmd) => this.handleSetTimeScale(cmd),
            generate_world: (cmd) => this.handleGenerateWorld(cmd),
            generate_material: (cmd) => this.handleGenerateMaterial(cmd)
        };

        this.systemPrompt = `You are a voxel world command bridge. Your job is to translate natural language requests into JSON commands.

Available commands:
1. place_block: Place a single block. {"action":"place_block","x":int,"y":int,"z":int,"type":"BLOCK_TYPE"}
2. break_block: Remove a block. {"action":"break_block","x":int,"y":int,"z":int}
3. spawn_agent: Spawn an AI agent. {"action":"spawn_agent","name":"string","role":"builder|explorer|miner|farmer|guard|artist","x":int,"y":int,"z":int}
4. set_environment: Change environment preset. {"action":"set_environment","type":"EARTH|MOON|MARS|OCEAN|SPACE"}
5. set_time_scale: Set simulation speed. {"action":"set_time_scale","scale":float}
6. generate_world: Generate a world from description. {"action":"generate_world","prompt":"string","size":int,"baseType":"BLOCK_TYPE"} 
7. generate_material: Generate a material definition. {"action":"generate_material","prompt":"string","name":"string","color":"#hex","mass":float,"hardness":float,"density":float,"tensileStrength":float,"meltingPoint":float}

Block types: AIR, STONE, DIRT, GRASS, WATER, SAND, GLASS, WOOD, LEAVES, IRON, COPPER, GOLD, STEEL, DIAMOND, COAL, BEDROCK, ASH, TNT, SNOW, DOOR, BUTTON, LAUNCHER, LOCK, LAMP, CHEST, SWITCH, CONVEYOR, PISTON, TRAPDOOR, FIRE

Important rules:
- Coordinates must be 0-255. Y=0 is ground level.
- Return ONLY valid JSON. No explanation, no markdown, no code blocks.
- If the request is ambiguous, choose the most likely interpretation.
- For multi-step requests, return an array of commands: {"commands":[...]}`;

        this.parsePrompt = `Parse the following user request into one or more JSON commands.
Return ONLY a JSON object with either "action" (single command) or "commands" (array of commands).
Never include explanations or markdown.`;
    }

    log(msg) {
        if (this.console && this.console.addChatMessage) {
            this.console.addChatMessage('assistant', msg);
        }
    }

    async process(text) {
        this.commandCount++;
        try {
            if (!this.llm.apiKey) {
                return { success: false, error: 'No API key configured. Add your key in the Brain panel.' };
            }

            const messages = [
                { role: 'system', content: this.systemPrompt },
                { role: 'user', content: `${this.parsePrompt}\n\nUser request: ${text}` }
            ];

            let responseText = '';
            const provider = this.llm.provider;

            if (provider === 'anthropic') {
                const resp = await this.rawAnthropic(messages);
                responseText = resp;
            } else if (provider === 'google') {
                const resp = await this.rawGoogle(messages);
                responseText = resp;
            } else {
                const resp = await this.rawOpenAI(messages);
                responseText = resp;
            }

            if (!responseText) {
                return { success: false, error: 'Empty response from LLM' };
            }

            const commands = this.parseResponse(responseText);
            if (!commands || commands.length === 0) {
                this.errorCount++;
                return { success: false, error: 'No valid commands parsed from LLM response', raw: responseText };
            }

            const results = [];
            for (const cmd of commands) {
                const handler = this.commandHandlers[cmd.action];
                if (handler) {
                    const result = await handler(cmd);
                    results.push(result);
                } else {
                    results.push({ success: false, action: cmd.action, error: `Unknown command: ${cmd.action}` });
                }
            }

            const successCount = results.filter(r => r.success).length;
            const failCount = results.filter(r => !r.success).length;
            const summary = results.map(r => r.message || r.error || 'ok').join('; ');

            this.lastResult = { success: failCount === 0, results, summary, raw: responseText };
            return this.lastResult;
        } catch (e) {
            this.errorCount++;
            return { success: false, error: e.message };
        }
    }

    async rawOpenAI(messages) {
        const controller = new AbortController();
        const t = setTimeout(() => controller.abort(), 30000);
        try {
            const resp = await fetch(this.llm.endpoint, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json', 'Authorization': 'Bearer ' + this.llm.apiKey },
                signal: controller.signal,
                body: JSON.stringify({
                    model: this.llm.model,
                    messages: messages,
                    temperature: 0.1,
                    max_tokens: 2048
                })
            });
            clearTimeout(t);
            if (!resp.ok) {
                const err = await resp.text();
                throw new Error('API error ' + resp.status + ': ' + err.substring(0, 200));
            }
            const data = await resp.json();
            return data.choices?.[0]?.message?.content || '';
        } catch (e) {
            clearTimeout(t);
            throw e;
        }
    }

    async rawAnthropic(messages) {
        const controller = new AbortController();
        const t = setTimeout(() => controller.abort(), 30000);
        try {
            const sysMsg = messages.find(m => m.role === 'system');
            const chatMsgs = messages.filter(m => m.role !== 'system').map(m => ({
                role: m.role === 'assistant' ? 'assistant' : 'user',
                content: m.content
            }));
            const resp = await fetch(this.llm.endpoint, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'x-api-key': this.llm.apiKey,
                    'anthropic-version': '2023-06-01',
                    'anthropic-dangerous-direct-browser-access': 'true'
                },
                signal: controller.signal,
                body: JSON.stringify({
                    model: this.llm.model,
                    max_tokens: 2048,
                    system: sysMsg?.content || '',
                    messages: chatMsgs
                })
            });
            clearTimeout(t);
            if (!resp.ok) {
                const err = await resp.text();
                throw new Error('API error ' + resp.status + ': ' + err.substring(0, 200));
            }
            const data = await resp.json();
            return data.content?.map(b => b.text).filter(Boolean).join('') || '';
        } catch (e) {
            clearTimeout(t);
            throw e;
        }
    }

    async rawGoogle(messages) {
        const controller = new AbortController();
        const t = setTimeout(() => controller.abort(), 30000);
        try {
            const sysMsg = messages.find(m => m.role === 'system');
            const contents = messages.filter(m => m.role !== 'system').map(m => ({
                role: m.role === 'assistant' ? 'model' : 'user',
                parts: [{ text: m.content }]
            }));
            const url = this.llm.endpoint + '/' + this.llm.model + ':generateContent';
            const resp = await fetch(url, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json', 'x-goog-api-key': this.llm.apiKey },
                signal: controller.signal,
                body: JSON.stringify({
                    contents: contents,
                    systemInstruction: sysMsg ? { parts: [{ text: sysMsg.content }] } : undefined,
                    generationConfig: { temperature: 0.1, maxOutputTokens: 2048 }
                })
            });
            clearTimeout(t);
            if (!resp.ok) {
                const err = await resp.text();
                throw new Error('API error ' + resp.status + ': ' + err.substring(0, 200));
            }
            const data = await resp.json();
            return data.candidates?.[0]?.content?.parts?.map(p => p.text).filter(Boolean).join('') || '';
        } catch (e) {
            clearTimeout(t);
            throw e;
        }
    }

    parseResponse(text) {
        text = text.trim();
        if (text.startsWith('```')) {
            text = text.replace(/```(?:json)?\s*/g, '').trim();
        }
        try {
            const parsed = JSON.parse(text);
            if (parsed.commands && Array.isArray(parsed.commands)) return parsed.commands;
            if (parsed.action) return [parsed];
            if (Array.isArray(parsed)) return parsed;
            return null;
        } catch (e) {
            const match = text.match(/\{(?:[^{}]|"[^"]*")*\}/g);
            if (match) {
                for (const m of match) {
                    try {
                        const parsed = JSON.parse(m);
                        if (parsed.action) return [parsed];
                        if (parsed.commands) return parsed.commands;
                    } catch (_) {}
                }
            }
            return null;
        }
    }

    resolveType(name) {
        const upper = (name + '').toUpperCase();
        if (this.typeMap[upper] !== undefined) return this.typeMap[upper];
        const num = parseInt(name);
        if (!isNaN(num) && num >= 0 && num <= 255) return num;
        return 1;
    }

    handlePlaceBlock(cmd) {
        const x = Math.round(cmd.x), y = Math.round(cmd.y), z = Math.round(cmd.z);
        if (x < 0 || x > 255 || y < 0 || y > 255 || z < 0 || z > 255) {
            return { success: false, action: 'place_block', error: `Coordinates out of range: (${x},${y},${z})` };
        }
        const type = this.resolveType(cmd.type);
        const result = this.engine.setBlock(x, y, z, type);
        if (result !== null) {
            this.engine.tick(0.016);
            return { success: true, action: 'place_block', message: `Placed ${cmd.type} at (${x},${y},${z})`, x, y, z, type };
        }
        return { success: false, action: 'place_block', error: 'Engine failed to place block' };
    }

    handleBreakBlock(cmd) {
        const x = Math.round(cmd.x), y = Math.round(cmd.y), z = Math.round(cmd.z);
        if (x < 0 || x > 255 || y < 0 || y > 255 || z < 0 || z > 255) {
            return { success: false, action: 'break_block', error: `Coordinates out of range: (${x},${y},${z})` };
        }
        const result = this.engine.setBlock(x, y, z, 0);
        if (result !== null) {
            this.engine.tick(0.016);
            return { success: true, action: 'break_block', message: `Broken block at (${x},${y},${z})`, x, y, z };
        }
        return { success: false, action: 'break_block', error: 'Engine failed to break block' };
    }

    async handleSpawnAgent(cmd) {
        if (typeof window.AgentManager === 'undefined' || typeof window.AgentConfig === 'undefined') {
            return { success: false, action: 'spawn_agent', error: 'Agent system not loaded' };
        }
        const name = cmd.name || 'Agent';
        const role = cmd.role || 'builder';
        const x = Math.round(cmd.x || 0), y = Math.round(cmd.y || 1), z = Math.round(cmd.z || 0);
        try {
            let manager = window._agentManager;
            if (!manager) {
                manager = new AgentManager();
                manager.setLLM(this.llm);
                manager.setEngine(this.engine);
                await manager.init();
                window._agentManager = manager;
            }
            const cfg = new AgentConfig();
            cfg.apply({ name, role, goalPreset: 'custom', customGoal: `Build something at (${x},${y},${z})` });
            const agent = manager.spawn(cfg);
            if (agent) {
                agent.x = x; agent.y = y; agent.z = z;
                return { success: true, action: 'spawn_agent', message: `Spawned agent "${name}" (${role}) at (${x},${y},${z})`, agentId: agent.id };
            }
            return { success: false, action: 'spawn_agent', error: 'Failed to spawn agent' };
        } catch (e) {
            return { success: false, action: 'spawn_agent', error: e.message };
        }
    }

    handleSetEnvironment(cmd) {
        const envType = (cmd.type || '').toUpperCase();
        const presets = this.engine.envPresets;
        if (presets && presets[envType.toLowerCase()]) {
            const p = presets[envType.toLowerCase()];
            if (p.gravity !== undefined) this.engine.setGravity(p.gravity);
            if (p.temperature !== undefined) this.engine.setAmbientTemperature(p.temperature);
            return { success: true, action: 'set_environment', message: `Environment set to ${envType}` };
        }
        return { success: false, action: 'set_environment', error: `Unknown environment: ${envType}` };
    }

    handleSetTimeScale(cmd) {
        const scale = parseFloat(cmd.scale);
        if (isNaN(scale) || scale <= 0) {
            return { success: false, action: 'set_time_scale', error: `Invalid scale: ${cmd.scale}` };
        }
        this.engine.setTimeScale(scale);
        return { success: true, action: 'set_time_scale', message: `Time scale set to ${scale}x` };
    }

    handleGenerateWorld(cmd) {
        if (!this.engine || !this.engine.wasmReady) {
            return { success: false, action: 'generate_world', error: 'WASM engine not ready' };
        }
        const prompt = cmd.prompt || '';
        const size = Math.min(Math.max(cmd.size || 16, 1), 64);
        const baseType = this.resolveType(cmd.baseType || 'DIRT');
        let placed = 0;

        const flat = prompt.toLowerCase().includes('flat') || prompt.toLowerCase().includes('platform');
        const water = prompt.toLowerCase().includes('water') || prompt.toLowerCase().includes('river') || prompt.toLowerCase().includes('lake') || prompt.toLowerCase().includes('ocean');
        const hill = prompt.toLowerCase().includes('hill') || prompt.toLowerCase().includes('mountain') || prompt.toLowerCase().includes('rocky');
        const forest = prompt.toLowerCase().includes('forest') || prompt.toLowerCase().includes('tree') || prompt.toLowerCase().includes('wood');
        const desert = prompt.toLowerCase().includes('desert') || prompt.toLowerCase().includes('sand') || prompt.toLowerCase().includes('arid');
        const snow = prompt.toLowerCase().includes('snow') || prompt.toLowerCase().includes('ice') || prompt.toLowerCase().includes('frozen') || prompt.toLowerCase().includes('tundra');
        const cave = prompt.toLowerCase().includes('cave') || prompt.toLowerCase().includes('underground') || prompt.toLowerCase().includes('cavern');
        const city = prompt.toLowerCase().includes('city') || prompt.toLowerCase().includes('town') || prompt.toLowerCase().includes('village') || prompt.toLowerCase().includes('house');

        const floorType = desert ? this.resolveType('SAND') : snow ? this.resolveType('SNOW') : baseType;
        const fillType = cave ? this.resolveType('STONE') : floorType;

        for (let x = 0; x < size; x++) {
            for (let z = 0; z < size; z++) {
                this.engine.setBlock(x, 0, z, this.resolveType('BEDROCK'));
                placed++;
                if (!cave || (x + z) % 4 !== 0) {
                    this.engine.setBlock(x, 1, z, fillType);
                    placed++;
                }
            }
        }

        if (hill) {
            const cx = Math.floor(size / 2), cz = Math.floor(size / 2);
            const peakHeight = Math.min(Math.floor(size / 2), 12);
            const hillType = snow && peakHeight > 4 ? this.resolveType('SNOW') : this.resolveType('STONE');
            for (let y = 2; y <= peakHeight; y++) {
                const r = Math.max(1, Math.floor((peakHeight - y) * size / peakHeight / 2));
                for (let dx = -r; dx <= r; dx++) {
                    for (let dz = -r; dz <= r; dz++) {
                        if (dx * dx + dz * dz <= r * r) {
                            const bx = cx + dx, bz = cz + dz;
                            if (bx >= 0 && bx < size && bz >= 0 && bz < size) {
                                this.engine.setBlock(bx, y, bz, y > peakHeight * 0.7 ? hillType : this.resolveType('STONE'));
                                placed++;
                            }
                        }
                    }
                }
            }
        }

        if (water) {
            const wSize = Math.floor(size * 0.4);
            const wx = Math.floor((size - wSize) / 2);
            for (let x = wx; x < wx + wSize; x++) {
                for (let z = 0; z < size; z++) {
                    for (let y = 1; y <= 2; y++) {
                        this.engine.setBlock(x, y, z, this.resolveType('WATER'));
                        placed++;
                    }
                }
            }
        }

        if (forest) {
            const treeType = this.resolveType('WOOD');
            const leafType = this.resolveType('LEAVES');
            for (let i = 0; i < Math.floor(size / 3); i++) {
                const tx = Math.floor(Math.random() * size);
                const tz = Math.floor(Math.random() * size);
                const trunkH = 2 + Math.floor(Math.random() * 3);
                for (let y = 2; y <= 2 + trunkH; y++) {
                    this.engine.setBlock(tx, y, tz, treeType);
                    placed++;
                }
                for (let dx = -2; dx <= 2; dx++) {
                    for (let dz = -2; dz <= 2; dz++) {
                        for (let dy = -1; dy <= 1; dy++) {
                            const ly = 2 + trunkH + dy;
                            if (ly >= 2 && Math.abs(dx) + Math.abs(dz) <= 2 + dy) {
                                const lx = tx + dx, lz = tz + dz;
                                if (lx >= 0 && lx < size && lz >= 0 && lz < size) {
                                    this.engine.setBlock(lx, ly, lz, leafType);
                                    placed++;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (city) {
            const wallType = this.resolveType('STONE');
            const roofType = this.resolveType('WOOD');
            for (let i = 0; i < Math.floor(size / 5); i++) {
                const bx = 2 + Math.floor(Math.random() * (size - 6));
                const bz = 2 + Math.floor(Math.random() * (size - 6));
                const bw = 3 + Math.floor(Math.random() * 3);
                const bh = 2 + Math.floor(Math.random() * 3);
                for (let x = bx; x < bx + bw; x++) {
                    for (let z = bz; z < bz + bw; z++) {
                        for (let y = 1; y <= bh; y++) {
                            if (y === bh) {
                                this.engine.setBlock(x, y, z, roofType);
                            } else if (x === bx || x === bx + bw - 1 || z === bz || z === bz + bw - 1) {
                                this.engine.setBlock(x, y, z, wallType);
                            }
                            placed++;
                        }
                    }
                }
            }
        }

        this.engine.tick(0.1);
        return { success: true, action: 'generate_world', message: `Generated world "${prompt.substring(0, 40)}" with ${placed} blocks`, blocksPlaced: placed };
    }

    handleGenerateMaterial(cmd) {
        const colorHex = cmd.color || '#808080';
        const colorNum = parseInt(colorHex.replace('#', ''), 16);
        const mapping = {};
        if (cmd.name) mapping.name = cmd.name;
        if (cmd.mass) mapping.mass = parseFloat(cmd.mass);
        if (cmd.hardness) mapping.hardness = parseFloat(cmd.hardness);
        if (cmd.density) mapping.density = parseFloat(cmd.density);
        if (cmd.tensileStrength) mapping.tensileStrength = parseFloat(cmd.tensileStrength);
        if (cmd.meltingPoint) mapping.meltingPoint = parseFloat(cmd.meltingPoint);
        if (cmd.color) mapping.color = colorNum;
        if (this.llm.renderer) {
            this.llm.renderer.setCustomBlockColor(255, colorNum);
        }
        return { success: true, action: 'generate_material', message: `Material "${cmd.name || 'custom'}" generated with color ${cmd.color || '#808080'}`, material: mapping };
    }
}

window.PromptBridge = PromptBridge;
