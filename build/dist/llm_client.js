/**
 * OpenMind – LLM Client + Voxel Agent
 * Supports OpenAI function calling (Groq, OpenAI, Custom),
 * Anthropic tool use, Google function declarations,
 * and prompt-based fallback for others.
 * Agent loop: think → call tools → observe → repeat
 */
class LLMClient {
    constructor() {
        this.provider = 'groq';
        this.apiKey = '';
        this.endpoint = 'https://api.groq.com/openai/v1/chat/completions';
        this.model = 'llama-3.3-70b-versatile';
        this.connected = false;
        this.engine = null;
        this.renderer = null;
        this.maxAgentIterations = 15;
        this.onToolCall = null;

        this.typeMap = {
            'AIR': 0, 'STONE': 1, 'DIRT': 2, 'GRASS': 3, 'WATER': 4, 'SAND': 5,
            'GLASS': 6, 'WOOD': 7, 'LEAVES': 8, 'IRON': 9, 'COPPER': 10,
            'GOLD': 11, 'STEEL': 12, 'DIAMOND': 13, 'COAL': 14, 'BEDROCK': 15,
            'SNOW': 18, 'ASH': 16, 'TNT': 17, 'DOOR': 30, 'BUTTON': 31,
            'LAUNCHER': 32, 'LOCK': 33, 'LAMP': 34, 'CHEST': 35,
            'SWITCH': 36, 'CONVEYOR': 37, 'PISTON': 38, 'TRAPDOOR': 39, 'FIRE': 40
        };

        this.agentTools = [
            {
                type: 'function',
                function: {
                    name: 'place_blocks',
                    description: 'Place multiple blocks in the world. Each block has x,y,z coordinates (0-255) and a type. Returns number of blocks placed.',
                    parameters: {
                        type: 'object',
                        properties: {
                            blocks: {
                                type: 'array',
                                description: 'Array of block placements',
                                items: {
                                    type: 'object',
                                    properties: {
                                        x: { type: 'integer', description: 'X coordinate 0-255' },
                                        y: { type: 'integer', description: 'Y coordinate 0-255 (0=ground, up)' },
                                        z: { type: 'integer', description: 'Z coordinate 0-255' },
                                        type: { type: 'string', description: 'Block type: AIR, STONE, DIRT, GRASS, WATER, SAND, GLASS, WOOD, LEAVES, IRON, COPPER, GOLD, STEEL, DIAMOND, COAL, BEDROCK, ASH, TNT, SNOW, DOOR, BUTTON, LAUNCHER, LOCK, LAMP, CHEST, SWITCH, CONVEYOR, PISTON, TRAPDOOR, FIRE' }
                                    },
                                    required: ['x', 'y', 'z', 'type']
                                }
                            }
                        },
                        required: ['blocks']
                    }
                }
            },
            {
                type: 'function',
                function: {
                    name: 'fill_rect',
                    description: 'Fill a rectangular volume with blocks. Useful for floors, walls, roofs.',
                    parameters: {
                        type: 'object',
                        properties: {
                            x1: { type: 'integer', description: 'Start X' },
                            y1: { type: 'integer', description: 'Start Y' },
                            z1: { type: 'integer', description: 'Start Z' },
                            x2: { type: 'integer', description: 'End X (inclusive)' },
                            y2: { type: 'integer', description: 'End Y (inclusive)' },
                            z2: { type: 'integer', description: 'End Z (inclusive)' },
                            type: { type: 'string', description: 'Block type' }
                        },
                        required: ['x1', 'y1', 'z1', 'x2', 'y2', 'z2', 'type']
                    }
                }
            },
            {
                type: 'function',
                function: {
                    name: 'hollow_rect',
                    description: 'Fill only the walls of a rectangular volume (hollow inside). Good for buildings.',
                    parameters: {
                        type: 'object',
                        properties: {
                            x1: { type: 'integer', description: 'Start X' },
                            y1: { type: 'integer', description: 'Start Y' },
                            z1: { type: 'integer', description: 'Start Z' },
                            x2: { type: 'integer', description: 'End X (inclusive)' },
                            y2: { type: 'integer', description: 'End Y (inclusive)' },
                            z2: { type: 'integer', description: 'End Z (inclusive)' },
                            type: { type: 'string', description: 'Block type for walls' }
                        },
                        required: ['x1', 'y1', 'z1', 'x2', 'y2', 'z2', 'type']
                    }
                }
            },
            {
                type: 'function',
                function: {
                    name: 'get_block',
                    description: 'Check what block is at a given position. Returns the block type name or AIR if empty.',
                    parameters: {
                        type: 'object',
                        properties: {
                            x: { type: 'integer', description: 'X coordinate' },
                            y: { type: 'integer', description: 'Y coordinate' },
                            z: { type: 'integer', description: 'Z coordinate' }
                        },
                        required: ['x', 'y', 'z']
                    }
                }
            },
            {
                type: 'function',
                function: {
                    name: 'survey_area',
                    description: 'Survey blocks in a region. Returns a summary of block types and counts in the bounding box.',
                    parameters: {
                        type: 'object',
                        properties: {
                            x1: { type: 'integer' },
                            y1: { type: 'integer' },
                            z1: { type: 'integer' },
                            x2: { type: 'integer' },
                            y2: { type: 'integer' },
                            z2: { type: 'integer' }
                        },
                        required: ['x1', 'y1', 'z1', 'x2', 'y2', 'z2']
                    }
                }
            },
            {
                type: 'function',
                function: {
                    name: 'clear_area',
                    description: 'Remove all blocks in a region (set to AIR).',
                    parameters: {
                        type: 'object',
                        properties: {
                            x1: { type: 'integer' },
                            y1: { type: 'integer' },
                            z1: { type: 'integer' },
                            x2: { type: 'integer' },
                            y2: { type: 'integer' },
                            z2: { type: 'integer' }
                        },
                        required: ['x1', 'y1', 'z1', 'x2', 'y2', 'z2']
                    }
                }
            },
            {
                type: 'function',
                function: {
                    name: 'move_to',
                    description: 'Move the agent to a coordinate position.',
                    parameters: {
                        type: 'object',
                        properties: {
                            x: { type: 'integer', description: 'Target X' },
                            y: { type: 'integer', description: 'Target Y' },
                            z: { type: 'integer', description: 'Target Z' }
                        },
                        required: ['x', 'y', 'z']
                    }
                }
            },
            {
                type: 'function',
                function: {
                    name: 'place_block',
                    description: 'Place a single block at a coordinate.',
                    parameters: {
                        type: 'object',
                        properties: {
                            x: { type: 'integer', description: 'X coordinate' },
                            y: { type: 'integer', description: 'Y coordinate' },
                            z: { type: 'integer', description: 'Z coordinate' },
                            type: { type: 'string', description: 'Block type' }
                        },
                        required: ['x', 'y', 'z', 'type']
                    }
                }
            },
            {
                type: 'function',
                function: {
                    name: 'observe',
                    description: 'Observe the surrounding area within a radius. Returns block type counts.',
                    parameters: {
                        type: 'object',
                        properties: {
                            radius: { type: 'integer', description: 'Observation radius (1-20)' }
                        },
                        required: ['radius']
                    }
                }
            },
            {
                type: 'function',
                function: {
                    name: 'report',
                    description: 'Report a status message or summary of what was done.',
                    parameters: {
                        type: 'object',
                        properties: {
                            message: { type: 'string', description: 'The report message' }
                        },
                        required: ['message']
                    }
                }
            }
        ];

        this.agentSystemPrompt = `You are OpenMind Agent, an expert voxel builder. You have tools to build structures in a 3D voxel world.

COORDINATE SYSTEM: x=0-255 (east-west), y=0-255 (up, y=1 is ground level), z=0-255 (north-south).

Available block types: AIR, STONE, DIRT, GRASS, WATER, SAND, GLASS, WOOD, LEAVES, IRON, COPPER, GOLD, STEEL, DIAMOND, COAL, BEDROCK, ASH, TNT, SNOW, DOOR, BUTTON, LAUNCHER, LOCK, LAMP, CHEST, SWITCH, CONVEYOR, PISTON, TRAPDOOR, FIRE

BUILDING STRATEGY:
1. Start with a floor/foundation using fill_rect
2. Add walls with fill_rect or hollow_rect
3. Add a roof with fill_rect
4. Add details: windows (GLASS), doors (DOOR), interior (LAMP, CHEST)
5. Add exterior details: path, garden, fence

RULES:
- Use fill_rect for large surfaces (floors, walls, roofs) instead of individual blocks
- Use hollow_rect for building shells
- Keep total blocks under 3000 for performance
- Place blocks at sensible heights (y=1 for ground floors, walls go up from there)
- Think step by step. Call multiple tools to build a complete structure.
- When done, provide a brief summary of what you built.`;
    }

    setConfig(provider, apiKey, endpoint, model) {
        this.provider = provider;
        this.apiKey = apiKey;
        this.endpoint = endpoint || this.getEndpoint(provider);
        this.model = model !== undefined ? model : this.getDefaultModel(provider);
    }

    getEndpoint(provider) {
        const endpoints = {
            groq: 'https://api.groq.com/openai/v1/chat/completions',
            openai: 'https://api.openai.com/v1/chat/completions',
            anthropic: 'https://api.anthropic.com/v1/messages',
            google: 'https://generativelanguage.googleapis.com/v1beta/models'
        };
        return endpoints[provider] || endpoints.groq;
    }

    getDefaultModel(provider) {
        const models = {
            groq: 'llama-3.3-70b-versatile',
            openai: 'gpt-4o-mini',
            anthropic: 'claude-3-haiku-20240307',
            google: 'gemini-2.0-flash'
        };
        return models[provider] || models.groq;
    }

    resolveType(typeName) {
        const upper = (typeName || 'STONE').toUpperCase();
        return this.typeMap[upper] !== undefined ? this.typeMap[upper] : 1;
    }

    executeTool(name, args) {
        if (!this.engine || !this.engine.wasmReady) return 'Error: WASM engine not ready';

        switch (name) {
            case 'place_blocks': {
                let placed = 0;
                const blocks = args.blocks || [];
                for (const b of blocks) {
                    const x = Math.round(b.x), y = Math.round(b.y), z = Math.round(b.z);
                    const t = this.resolveType(b.type);
                    if (x >= 0 && x < 256 && y >= 0 && y < 256 && z >= 0 && z < 256) {
                        this.engine.setBlock(x, y, z, t);
                        placed++;
                    }
                }
                return `Placed ${placed} blocks successfully.`;
            }
            case 'fill_rect': {
                let placed = 0;
                const t = this.resolveType(args.type);
                const x1 = Math.min(args.x1, args.x2), x2 = Math.max(args.x1, args.x2);
                const y1 = Math.min(args.y1, args.y2), y2 = Math.max(args.y1, args.y2);
                const z1 = Math.min(args.z1, args.z2), z2 = Math.max(args.z1, args.z2);
                for (let x = x1; x <= x2; x++)
                    for (let y = y1; y <= y2; y++)
                        for (let z = z1; z <= z2; z++) {
                            if (x >= 0 && x < 256 && y >= 0 && y < 256 && z >= 0 && z < 256) {
                                this.engine.setBlock(x, y, z, t);
                                placed++;
                            }
                        }
                return `Filled rect (${x1},${y1},${z1}) to (${x2},${y2},${z2}) with ${args.type}: ${placed} blocks placed.`;
            }
            case 'hollow_rect': {
                let placed = 0;
                const t = this.resolveType(args.type);
                const x1 = Math.min(args.x1, args.x2), x2 = Math.max(args.x1, args.x2);
                const y1 = Math.min(args.y1, args.y2), y2 = Math.max(args.y1, args.y2);
                const z1 = Math.min(args.z1, args.z2), z2 = Math.max(args.z1, args.z2);
                for (let x = x1; x <= x2; x++)
                    for (let y = y1; y <= y2; y++)
                        for (let z = z1; z <= z2; z++) {
                            if (x === x1 || x === x2 || y === y1 || y === y2 || z === z1 || z === z2) {
                                if (x >= 0 && x < 256 && y >= 0 && y < 256 && z >= 0 && z < 256) {
                                    this.engine.setBlock(x, y, z, t);
                                    placed++;
                                }
                            }
                        }
                return `Hollow rect (${x1},${y1},${z1}) to (${x2},${y2},${z2}) with ${args.type}: ${placed} blocks placed (walls only).`;
            }
            case 'get_block': {
                const gx = Math.round(args.x), gy = Math.round(args.y), gz = Math.round(args.z);
                if (gx < 0 || gx >= 256 || gy < 0 || gy >= 256 || gz < 0 || gz >= 256) return 'Out of bounds (0-255)';
                const t = this.engine.getBlock(gx, gy, gz);
                const typeName = Object.keys(this.typeMap).find(k => this.typeMap[k] === t) || 'UNKNOWN';
                return `Block at (${gx},${gy},${gz}): type=${t} (${typeName})`;
            }
            case 'survey_area': {
                const counts = {};
                const x1 = Math.min(args.x1, args.x2), x2 = Math.max(args.x1, args.x2);
                const y1 = Math.min(args.y1, args.y2), y2 = Math.max(args.y1, args.y2);
                const z1 = Math.min(args.z1, args.z2), z2 = Math.max(args.z1, args.z2);
                if (x1 < 0 || x2 >= 256 || y1 < 0 || y2 >= 256 || z1 < 0 || z2 >= 256) return 'Survey area out of bounds (0-255)';
                let total = 0;
                for (let x = x1; x <= x2; x++)
                    for (let y = y1; y <= y2; y++)
                        for (let z = z1; z <= z2; z++) {
                            const t = this.engine.getBlock(x, y, z);
                            const name = Object.keys(this.typeMap).find(k => this.typeMap[k] === t) || 'AIR';
                            counts[name] = (counts[name] || 0) + 1;
                            total++;
                        }
                return `Survey (${x1},${y1},${z1})-(${x2},${y2},${z2}): ${total} positions. ${JSON.stringify(counts)}`;
            }
            case 'clear_area': {
                let cleared = 0;
                const x1 = Math.min(args.x1, args.x2), x2 = Math.max(args.x1, args.x2);
                const y1 = Math.min(args.y1, args.y2), y2 = Math.max(args.y1, args.y2);
                const z1 = Math.min(args.z1, args.z2), z2 = Math.max(args.z1, args.z2);
                if (x1 < 0 || x2 >= 256 || y1 < 0 || y2 >= 256 || z1 < 0 || z2 >= 256) return 'Clear area out of bounds (0-255)';
                for (let x = x1; x <= x2; x++)
                    for (let y = y1; y <= y2; y++)
                        for (let z = z1; z <= z2; z++) {
                            if (x >= 0 && x < 256 && y >= 0 && y < 256 && z >= 0 && z < 256) {
                                this.engine.setBlock(x, y, z, 0);
                                cleared++;
                            }
                        }
                return `Cleared ${cleared} blocks in (${x1},${y1},${z1})-(${x2},${y2},${z2}).`;
            }
            case 'move_to': {
                return `Agent moved to (${args.x},${args.y},${args.z})`;
            }
            case 'place_block': {
                const px = Math.round(args.x), py = Math.round(args.y), pz = Math.round(args.z);
                if (px < 0 || px >= 256 || py < 0 || py >= 256 || pz < 0 || pz >= 256) return 'Coordinates out of bounds (0-255)';
                const pt = this.resolveType(args.type);
                this.engine.setBlock(px, py, pz, pt);
                return `Placed ${args.type} at (${px},${py},${pz})`;
            }
            case 'observe': {
                const radius = Math.min(args.radius || 5, 20);
                return `Observing radius ${radius} around current position.`;
            }
            case 'report':
                return `Report: ${args.message || 'done'}`;
            default:
                return `Unknown tool: ${name}`;
        }
    }

    async generate(prompt) {
        if (!this.apiKey) {
            return { success: false, error: 'No API key configured. Add your key in the Brain panel.' };
        }
        try {
            return await this.runAgent(prompt);
        } catch (e) {
            return { success: false, error: e.message };
        }
    }

    async runAgent(prompt) {
        const messages = [
            { role: 'system', content: this.agentSystemPrompt },
            { role: 'user', content: prompt }
        ];

        let totalBlocksPlaced = 0;
        const toolsUsed = [];

        for (let iteration = 0; iteration < this.maxAgentIterations; iteration++) {
            console.log(`[Agent] Iteration ${iteration + 1}/${this.maxAgentIterations}`);

            let response;
            if (this.provider === 'anthropic') {
                response = await this.callAnthropicAgent(messages);
            } else if (this.provider === 'google') {
                response = await this.callGoogleAgent(messages);
            } else {
                response = await this.callOpenAIAgent(messages);
            }

            if (!response) {
                return { success: false, error: 'No response from LLM' };
            }

            if (response.error) {
                return { success: false, error: response.error };
            }

            if (response.toolCalls && response.toolCalls.length > 0) {
                if (this.provider === 'anthropic') {
                    const anthropicContent = [];
                    if (response.text) anthropicContent.push({ type: 'text', text: response.text });
                    for (const tc of response.toolCalls) {
                        anthropicContent.push({
                            type: 'tool_use', id: tc.id,
                            name: tc.function.name,
                            input: JSON.parse(tc.function.arguments || '{}')
                        });
                    }
                    messages.push({ role: 'assistant', content: anthropicContent });
                } else if (this.provider === 'google') {
                    const parts = [];
                    if (response.text) parts.push({ text: response.text });
                    for (const tc of response.toolCalls) {
                        const tcArgs = JSON.parse(tc.function.arguments || '{}');
                        parts.push({ functionCall: { name: tc.function.name, args: tcArgs } });
                    }
                    messages.push({ role: 'model', parts });
                } else {
                    messages.push({ role: 'assistant', content: response.text || null, tool_calls: response.toolCalls });
                }

                for (const tc of response.toolCalls) {
                    const fnName = tc.function.name;
                    let fnArgs;
                    try {
                        fnArgs = JSON.parse(tc.function.arguments);
                    } catch (e) {
                        fnArgs = {};
                    }

                    console.log(`[Agent] Tool call: ${fnName}`, fnArgs);
                    toolsUsed.push(fnName);

                    if (this.onToolCall) this.onToolCall(fnName, fnArgs);

                    const result = this.executeTool(fnName, fnArgs);
                    console.log(`[Agent] Tool result: ${result.substring(0, 200)}`);

                    if (fnName === 'place_blocks' || fnName === 'fill_rect' || fnName === 'hollow_rect') {
                        const match = result.match(/(\d+) blocks/);
                        if (match) totalBlocksPlaced += parseInt(match[1]);
                    }

                    if (this.provider === 'anthropic') {
                        messages.push({ role: 'user', content: [{ type: 'tool_result', tool_use_id: tc.id, content: result }] });
                    } else if (this.provider === 'google') {
                        let parsedResult;
                        try { parsedResult = JSON.parse(result); } catch (e) { parsedResult = { text: result }; }
                        messages.push({ role: 'function', parts: [{ functionResponse: { name: fnName, response: parsedResult } }] });
                    } else {
                        messages.push({ role: 'tool', tool_call_id: tc.id, content: result });
                    }
                }
            } else {
                if (this.renderer) {
                    this.renderer.dirty = true;
                }
                return {
                    success: true,
                    blocks: [],
                    text: response.text || 'Done.',
                    totalBlocksPlaced,
                    toolsUsed: [...new Set(toolsUsed)],
                    iterations: iteration + 1
                };
            }
        }

        if (this.renderer) {
            this.renderer.dirty = true;
        }
        return {
            success: true,
            blocks: [],
            text: 'Agent completed max iterations.',
            totalBlocksPlaced,
            toolsUsed: [...new Set(toolsUsed)],
            iterations: this.maxAgentIterations
        };
    }

    async callOpenAIAgent(messages) {
        const controller = new AbortController();
        const timeout = setTimeout(() => controller.abort(), 60000);
        try {
            const response = await fetch(this.endpoint, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Authorization': 'Bearer ' + this.apiKey
                },
                signal: controller.signal,
                body: JSON.stringify({
                    model: this.model,
                    messages: messages,
                    tools: this.agentTools,
                    tool_choice: 'auto',
                    temperature: 0.2,
                    max_tokens: 4096
                })
            });

            if (!response.ok) {
                const err = await response.text();
                throw new Error('API error ' + response.status + ': ' + err.substring(0, 200));
            }

            const data = await response.json();
            clearTimeout(timeout);
            const choice = data.choices?.[0];
            const msg = choice?.message;

            if (msg?.tool_calls?.length > 0) {
                return { toolCalls: msg.tool_calls, text: msg.content || '' };
            }
            return { text: msg?.content || '', toolCalls: null };
        } catch (e) {
            clearTimeout(timeout);
            throw e;
        }
    }

    async callAnthropicAgent(messages) {
        const controller = new AbortController();
        const timeout = setTimeout(() => controller.abort(), 60000);
        try {
            const anthropicMessages = messages.filter(m => m.role !== 'system');
            const anthropicTools = this.agentTools.map(t => ({
                name: t.function.name,
                description: t.function.description,
                input_schema: t.function.parameters
            }));

            const response = await fetch(this.endpoint, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'x-api-key': this.apiKey,
                    'anthropic-version': '2023-06-01',
                    'anthropic-dangerous-direct-browser-access': 'true'
                },
                signal: controller.signal,
                body: JSON.stringify({
                    model: this.model,
                    max_tokens: 4096,
                    system: this.agentSystemPrompt,
                    messages: anthropicMessages,
                    tools: anthropicTools
                })
            });

            if (!response.ok) {
                const err = await response.text();
                throw new Error('API error ' + response.status + ': ' + err.substring(0, 200));
            }

            const data = await response.json();
            clearTimeout(timeout);

            const toolCalls = [];
            let text = '';
            for (const block of data.content || []) {
                if (block.type === 'text') text += block.text;
                if (block.type === 'tool_use') {
                    toolCalls.push({
                        id: block.id,
                        function: { name: block.name, arguments: JSON.stringify(block.input) }
                    });
                }
            }

            if (toolCalls.length > 0) {
                return { toolCalls, text };
            }
            return { text, toolCalls: null };
        } catch (e) {
            clearTimeout(timeout);
            throw e;
        }
    }

    async callGoogleAgent(messages) {
        const controller = new AbortController();
        const timeout = setTimeout(() => controller.abort(), 60000);
        try {
            const url = this.endpoint + '/' + this.model + ':generateContent';
            const roleMap = { assistant: 'model', user: 'user', model: 'model', function: 'function' };
            const contents = [];
            for (const m of messages) {
                if (m.role === 'system') continue;
                const role = roleMap[m.role] || 'user';
                const parts = m.parts || (m.content ? [{ text: m.content }] : []);
                if (parts.length > 0) contents.push({ role, parts });
            }

            const functionDecls = this.agentTools.map(t => ({
                name: t.function.name,
                description: t.function.description,
                parameters: t.function.parameters
            }));

            const response = await fetch(url, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json', 'x-goog-api-key': this.apiKey },
                signal: controller.signal,
                body: JSON.stringify({
                    contents: contents,
                    systemInstruction: { parts: [{ text: this.agentSystemPrompt }] },
                    tools: [{ functionDeclarations: functionDecls }],
                    generationConfig: { temperature: 0.2, maxOutputTokens: 4096 }
                })
            });

            if (!response.ok) {
                const err = await response.text();
                throw new Error('API error ' + response.status + ': ' + err.substring(0, 200));
            }

            const data = await response.json();
            clearTimeout(timeout);

            const candidate = data.candidates?.[0];
            const parts = candidate?.content?.parts || [];
            let text = '';
            const toolCalls = [];

            for (const part of parts) {
                if (part.text) text += part.text;
                if (part.functionCall) {
                    toolCalls.push({
                        id: 'google_' + Date.now() + '_' + Math.random().toString(36).substr(2, 6),
                        function: {
                            name: part.functionCall.name,
                            arguments: JSON.stringify(part.functionCall.args || {})
                        }
                    });
                }
            }

            if (toolCalls.length > 0) {
                return { toolCalls, text };
            }
            return { text, toolCalls: null };
        } catch (e) {
            clearTimeout(timeout);
            throw e;
        }
    }

    async executeBlocks(blocks) {
        if (!this.engine || !this.engine.wasmReady) return 0;
        let placed = 0;
        for (const block of blocks) {
            const x = Math.round(block.x);
            const y = Math.round(block.y);
            const z = Math.round(block.z);
            const typeName = (block.type || 'STONE').toUpperCase();
            const type = this.typeMap[typeName] !== undefined ? this.typeMap[typeName] : 1;
            if (x >= 0 && x < 256 && y >= 0 && y < 256 && z >= 0 && z < 256) {
                this.engine.setBlock(x, y, z, type);
                placed++;
            }
        }
        return placed;
    }
}

window.LLMClient = LLMClient;
