/**
 * OpenMind – Agent Think Loop
 * Core reasoning loop wired to LLM
 */
(function() {
    'use strict';

    class AgentThinkLoop {
        constructor(agent, llmClient, engine) {
            this.agent = agent;
            this.llm = llmClient;
            this.engine = engine;
            this.promptBuilder = new AgentPromptBuilder();
            this.actionParser = new AgentActionParser();
            this.maxIterations = 15;
            this.running = false;
            this.intervalMs = 3000;
            this._timer = null;
            this._iteration = 0;
            this.onAction = null;
            this.onThought = null;
            this.onComplete = null;
            this.onError = null;
            this._lastReport = '';
        }

        start() {
            if (this.running) return;
            this.running = true;
            this._iteration = 0;
            this._tick();
        }

        stop() {
            this.running = false;
            if (this._timer) { clearTimeout(this._timer); this._timer = null; }
        }

        async _tick() {
            if (!this.running || !this.llm.apiKey) return;
            if (this._iteration >= this.maxIterations) {
                this.stop();
                if (this.onComplete) this.onComplete('Max iterations reached');
                return;
            }

            this._iteration++;
            try {
                await this._thinkOnce();
            } catch (e) {
                console.error('[AgentThink] Error:', e);
                if (this.onError) this.onError(e.message);
                this.agent.addMemory('error', `Thinking error: ${e.message}`);
            }
            if (this.running) {
                this._timer = setTimeout(() => this._tick(), this.intervalMs);
            }
        }

        async _thinkOnce() {
            const { messages, tools } = this.promptBuilder.buildFullPrompt(
                this.agent, this.engine, this.agent.memories, this.agent.currentGoal
            );

            if (this.onThought) this.onThought(`Thinking (step ${this._iteration})...`);

            const response = await this._callLLM(messages, tools);

            if (!response) {
                this.agent.addMemory('error', 'No response from LLM');
                return;
            }

            if (response.error) {
                this.agent.addMemory('error', `LLM error: ${response.error}`);
                if (this.onError) this.onError(response.error);
                return;
            }

            const parsed = this.actionParser.parse(response);

            if (parsed.text && parsed.text.trim()) {
                this.agent.addMemory('thought', parsed.text);
                if (this.onThought) this.onThought(parsed.text);
            }

            if (parsed.errors.length > 0) {
                console.warn('[AgentThink] Parse errors:', parsed.errors);
                this.agent.addMemory('warning', `Parse issues: ${parsed.errors.join('; ')}`);
            }

            for (const action of parsed.actions) {
                const result = this._executeAction(action);
                this.agent.addMemory('action', `${action.tool}(${JSON.stringify(action.args).substring(0, 80)}) → ${result.substring(0, 120)}`);

                if (this.onAction) this.onAction(action, result);

                if (action.tool === 'report' && action.args.message) {
                    this._lastReport = action.args.message;
                }
            }

            this.agent.x = Math.round(this.agent.x);
            this.agent.y = Math.round(this.agent.y);
            this.agent.z = Math.round(this.agent.z);
        }

        async _callLLM(messages, tools) {
            if (this.llm.provider === 'anthropic') {
                return this._callAnthropic(messages, tools);
            } else if (this.llm.provider === 'google') {
                return this._callGoogle(messages, tools);
            } else {
                return this._callOpenAI(messages, tools);
            }
        }

        async _callOpenAI(messages, tools) {
            const controller = new AbortController();
            const timeout = setTimeout(() => controller.abort(), 60000);
            try {
                const response = await fetch(this.llm.endpoint, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json', 'Authorization': 'Bearer ' + this.llm.apiKey },
                    signal: controller.signal,
                    body: JSON.stringify({
                        model: this.llm.model, messages, tools,
                        tool_choice: 'auto', temperature: 0.3, max_tokens: 4096
                    })
                });
                if (!response.ok) {
                    const err = await response.text();
                    throw new Error('API error ' + response.status + ': ' + err.substring(0, 200));
                }
                const data = await response.json();
                clearTimeout(timeout);
                return data.choices?.[0]?.message;
            } catch (e) { clearTimeout(timeout); throw e; }
        }

        async _callAnthropic(messages, tools) {
            const controller = new AbortController();
            const timeout = setTimeout(() => controller.abort(), 60000);
            try {
                const anthropicMessages = messages.filter(m => m.role !== 'system');
                const anthropicTools = tools.map(t => ({
                    name: t.function.name, description: t.function.description, input_schema: t.function.parameters
                }));
                const response = await fetch(this.llm.endpoint, {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json', 'x-api-key': this.llm.apiKey,
                        'anthropic-version': '2023-06-01', 'anthropic-dangerous-direct-browser-access': 'true'
                    },
                    signal: controller.signal,
                    body: JSON.stringify({
                        model: this.llm.model, max_tokens: 4096,
                        system: messages.find(m => m.role === 'system')?.content || '',
                        messages: anthropicMessages, tools: anthropicTools
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
                        toolCalls.push({ id: block.id, function: { name: block.name, arguments: JSON.stringify(block.input) } });
                    }
                }
                if (toolCalls.length > 0) return { tool_calls: toolCalls, content: text };
                return { content: text, tool_calls: null };
            } catch (e) { clearTimeout(timeout); throw e; }
        }

        async _callGoogle(messages, tools) {
            const controller = new AbortController();
            const timeout = setTimeout(() => controller.abort(), 60000);
            try {
                const url = this.llm.endpoint + '/' + this.llm.model + ':generateContent';
                const contents = messages.filter(m => m.role !== 'system').map(m => ({
                    role: m.role === 'assistant' ? 'model' : 'user', parts: [{ text: m.content || '' }]
                }));
                const functionDecls = tools.map(t => ({
                    name: t.function.name, description: t.function.description, parameters: t.function.parameters
                }));
                const response = await fetch(url, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json', 'x-goog-api-key': this.llm.apiKey },
                    signal: controller.signal,
                    body: JSON.stringify({
                        contents,
                        systemInstruction: { parts: [{ text: messages.find(m => m.role === 'system')?.content || '' }] },
                        tools: [{ functionDeclarations: functionDecls }],
                        generationConfig: { temperature: 0.3, maxOutputTokens: 4096 }
                    })
                });
                if (!response.ok) {
                    const err = await response.text();
                    throw new Error('API error ' + response.status + ': ' + err.substring(0, 200));
                }
                const data = await response.json();
                clearTimeout(timeout);
                const parts = data.candidates?.[0]?.content?.parts || [];
                let text = '';
                const toolCalls = [];
                for (const part of parts) {
                    if (part.text) text += part.text;
                    if (part.functionCall) {
                        toolCalls.push({
                            id: 'google_' + Date.now() + '_' + Math.random().toString(36).substr(2, 6),
                            function: { name: part.functionCall.name, arguments: JSON.stringify(part.functionCall.args || {}) }
                        });
                    }
                }
                if (toolCalls.length > 0) return { tool_calls: toolCalls, content: text };
                return { content: text, tool_calls: null };
            } catch (e) { clearTimeout(timeout); throw e; }
        }

        _executeAction(action) {
            if (!this.engine || !this.engine.wasmReady) return 'Error: Engine not ready';
            const { tool, args } = action;
            const typeMap = {
                'AIR':0,'STONE':1,'DIRT':2,'GRASS':3,'WATER':4,'SAND':5,'GLASS':6,'WOOD':7,'LEAVES':8,
                'IRON':9,'COPPER':10,'GOLD':11,'STEEL':12,'DIAMOND':13,'COAL':14,'BEDROCK':15,'ASH':16,'TNT':17,'SNOW':18
            };
            const resolveType = (t) => typeMap[(t||'').toUpperCase()] || 1;

            switch (tool) {
                case 'move_to': {
                    this.agent.x = args.x; this.agent.y = args.y !== undefined ? args.y : 2; this.agent.z = args.z;
                    return `Moved to (${args.x}, ${this.agent.y}, ${args.z})`;
                }
                case 'place_block': {
                    const t = resolveType(args.type);
                    this.engine.setBlock(args.x, args.y, args.z, t);
                    return `Placed ${args.type} at (${args.x},${args.y},${args.z})`;
                }
                case 'place_blocks': {
                    let count = 0;
                    for (const b of (args.blocks || [])) {
                        if (b.x >= 0 && b.x < 256 && b.y >= 0 && b.y < 256 && b.z >= 0 && b.z < 256) {
                            this.engine.setBlock(b.x, b.y, b.z, resolveType(b.type));
                            count++;
                        }
                    }
                    return `Placed ${count} blocks`;
                }
                case 'fill_rect': {
                    let count = 0;
                    const t = resolveType(args.type);
                    for (let x = Math.min(args.x1,args.x2); x <= Math.max(args.x1,args.x2); x++)
                        for (let y = Math.min(args.y1,args.y2); y <= Math.max(args.y1,args.y2); y++)
                            for (let z = Math.min(args.z1,args.z2); z <= Math.max(args.z1,args.z2); z++)
                                if (x>=0&&x<256&&y>=0&&y<256&&z>=0&&z<256) { this.engine.setBlock(x,y,z,t); count++; }
                    return `Filled rect with ${args.type}: ${count} blocks`;
                }
                case 'hollow_rect': {
                    let count = 0;
                    const t = resolveType(args.type);
                    for (let x = Math.min(args.x1,args.x2); x <= Math.max(args.x1,args.x2); x++)
                        for (let y = Math.min(args.y1,args.y2); y <= Math.max(args.y1,args.y2); y++)
                            for (let z = Math.min(args.z1,args.z2); z <= Math.max(args.z1,args.z2); z++)
                                if (x===Math.min(args.x1,args.x2)||x===Math.max(args.x1,args.x2)||y===Math.min(args.y1,args.y2)||y===Math.max(args.y1,args.y2)||z===Math.min(args.z1,args.z2)||z===Math.max(args.z1,args.z2))
                                    if (x>=0&&x<256&&y>=0&&y<256&&z>=0&&z<256) { this.engine.setBlock(x,y,z,t); count++; }
                    return `Hollow rect with ${args.type}: ${count} wall blocks`;
                }
                case 'get_block': {
                    const t = this.engine.getBlock(args.x, args.y, args.z);
                    const name = Object.keys(typeMap).find(k => typeMap[k] === t) || (t === 0 ? 'AIR' : 'UNKNOWN');
                    return `Block at (${args.x},${args.y},${args.z}): ${name}`;
                }
                case 'survey_area': {
                    const counts = {};
                    for (let x = Math.min(args.x1,args.x2); x <= Math.max(args.x1,args.x2); x++)
                        for (let y = Math.min(args.y1,args.y2); y <= Math.max(args.y1,args.y2); y++)
                            for (let z = Math.min(args.z1,args.z2); z <= Math.max(args.z1,args.z2); z++) {
                                const t = this.engine.getBlock(x,y,z);
                                const name = Object.keys(typeMap).find(k => typeMap[k] === t) || (t===0?'AIR':'X');
                                counts[name] = (counts[name]||0)+1;
                            }
                    return `Survey: ${JSON.stringify(counts)}`;
                }
                case 'clear_area': {
                    let count = 0;
                    for (let x = Math.min(args.x1,args.x2); x <= Math.max(args.x1,args.x2); x++)
                        for (let y = Math.min(args.y1,args.y2); y <= Math.max(args.y1,args.y2); y++)
                            for (let z = Math.min(args.z1,args.z2); z <= Math.max(args.z1,args.z2); z++)
                                if (x>=0&&x<256&&y>=0&&y<256&&z>=0&&z<256) { this.engine.setBlock(x,y,z,0); count++; }
                    return `Cleared ${count} blocks`;
                }
                case 'observe': {
                    const r = Math.min(args.radius || 5, 20);
                    const counts = {};
                    for (let x = this.agent.x-r; x <= this.agent.x+r; x++)
                        for (let y = Math.max(0,this.agent.y-r); y <= Math.min(255,this.agent.y+r); y++)
                            for (let z = this.agent.z-r; z <= this.agent.z+r; z++)
                                if (x>=0&&x<256&&z>=0&&z<256) {
                                    const t = this.engine.getBlock(x,y,z);
                                    const name = Object.keys(typeMap).find(k => typeMap[k] === t) || (t===0?'AIR':'X');
                                    counts[name] = (counts[name]||0)+1;
                                }
                    return `Observing radius ${r}: ${JSON.stringify(counts)}`;
                }
                case 'report':
                    return `Report: ${args.message}`;
                default:
                    return `Unknown tool: ${tool}`;
            }
        }

        get lastReport() { return this._lastReport; }
    }

    window.AgentThinkLoop = AgentThinkLoop;
})();
