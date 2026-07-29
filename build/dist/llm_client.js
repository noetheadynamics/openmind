class LLMClient {
    constructor() {
        this.provider = 'groq';
        this.apiKey = '';
        this.endpoint = 'https://api.groq.com/openai/v1/chat/completions';
        this.model = 'llama-3.3-70b-versatile';
        this.connected = false;
        this.engine = null;
        this.renderer = null;

        this.systemPrompt = `You are OpenMind, an AI that generates voxel worlds. When given a prompt, output ONLY a JSON array of block placement instructions. No explanation, no markdown, no code fences.

Each instruction is an object: {"x":0,"y":0,"z":0,"type":"STONE"}
Valid types: STONE, DIRT, GRASS, WATER, SAND, GLASS, WOOD, LEAVES, IRON, COPPER, GOLD, STEEL, DIAMOND, COAL, BEDROCK, SNOW

Rules:
- Coordinates are 0-255
- Y=0 is ground level, Y increases upward
- Generate structures that make sense (platforms, walls, floors, roofs, etc.)
- Use appropriate materials (WOOD for walls, GLASS for windows, WATER for moats)
- Keep total blocks under 5000 for performance

Examples:
- "10x10 stone platform" → [{"x":0,"y":1,"z":0,"type":"STONE"}, ...100 blocks at y=1 forming a 10x10 grid]
- "small wooden house" → platform + walls + roof + floor
- "pond" → depression with WATER blocks`;
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

    async generate(prompt) {
        if (!this.apiKey) {
            return { success: false, error: 'No API key configured. Add your key in the Brain panel.' };
        }

        try {
            let blocks;
            if (this.provider === 'anthropic') {
                blocks = await this.callAnthropic(prompt);
            } else if (this.provider === 'google') {
                blocks = await this.callGoogle(prompt);
            } else {
                blocks = await this.callOpenAICompatible(prompt);
            }

            if (!Array.isArray(blocks)) {
                return { success: false, error: 'Invalid response format from LLM' };
            }

            return { success: true, blocks };
        } catch (e) {
            return { success: false, error: e.message };
        }
    }

    async callOpenAICompatible(prompt) {
        const controller = new AbortController();
        const timeout = setTimeout(() => controller.abort(), 30000);
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
                messages: [
                    { role: 'system', content: this.systemPrompt },
                    { role: 'user', content: prompt }
                ],
                temperature: 0.3,
                max_tokens: 4096
            })
        });

        if (!response.ok) {
            const err = await response.text();
            throw new Error('API error ' + response.status + ': ' + err.substring(0, 200));
        }

        const data = await response.json();
        const content = data.choices?.[0]?.message?.content || '';
        clearTimeout(timeout);
        return this.parseBlocks(content);
        } finally { clearTimeout(timeout); }
    }

    async callAnthropic(prompt) {
        const controller = new AbortController();
        const timer = setTimeout(() => controller.abort(), 30000);
        try {
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
                system: this.systemPrompt,
                messages: [{ role: 'user', content: prompt }]
            })
        });

        if (!response.ok) {
            const err = await response.text();
            throw new Error('API error ' + response.status + ': ' + err.substring(0, 200));
        }

        const data = await response.json();
        const content = data.content?.[0]?.text || '';
        return this.parseBlocks(content);
        } finally { clearTimeout(timer); }
    }

    async callGoogle(prompt) {
        const controller = new AbortController();
        const timer = setTimeout(() => controller.abort(), 30000);
        try {
        const url = this.endpoint + '/' + this.model + ':generateContent';
        const response = await fetch(url, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json', 'x-goog-api-key': this.apiKey },
            signal: controller.signal,
            body: JSON.stringify({
                contents: [{ parts: [{ text: this.systemPrompt + '\n\nUser: ' + prompt }] }],
                generationConfig: { temperature: 0.3, maxOutputTokens: 4096 }
            })
        });

        if (!response.ok) {
            const err = await response.text();
            throw new Error('API error ' + response.status + ': ' + err.substring(0, 200));
        }

        const data = await response.json();
        const content = data.candidates?.[0]?.content?.parts?.[0]?.text || '';
        return this.parseBlocks(content);
        } finally { clearTimeout(timer); }
    }

    parseBlocks(content) {
        console.log('[LLM] Raw response:', content.substring(0, 500));
        let cleaned = content.trim();
        cleaned = cleaned.replace(/```json\s*/g, '').replace(/```\s*/g, '');
        cleaned = cleaned.replace(/^[^\[{]*([\[\s\S]*\])[^}\]]*$/, '$1');

        try {
            const parsed = JSON.parse(cleaned);
            if (Array.isArray(parsed)) return parsed;
            if (parsed.blocks && Array.isArray(parsed.blocks)) return parsed.blocks;
            return [];
        } catch (e) {
            const arrayMatch = cleaned.match(/\[[\s\S]*\]/);
            if (arrayMatch) {
                try { return JSON.parse(arrayMatch[0]); } catch {}
            }
            const lines = content.split('\n');
            const jsonLines = [];
            let inArray = false;
            for (const line of lines) {
                const trimmed = line.trim();
                if (trimmed.startsWith('[')) inArray = true;
                if (inArray) jsonLines.push(trimmed);
                if (trimmed.endsWith(']')) break;
            }
            if (jsonLines.length > 0) {
                try { return JSON.parse(jsonLines.join('\n')); } catch {}
            }
            return [];
        }
    }

    async executeBlocks(blocks) {
        if (!this.engine || !this.engine.wasmReady) return 0;
        let placed = 0;

        const typeMap = {
            'STONE': 1, 'DIRT': 2, 'GRASS': 3, 'WATER': 4, 'SAND': 5,
            'GLASS': 6, 'WOOD': 7, 'LEAVES': 8, 'IRON': 9, 'COPPER': 10,
            'GOLD': 11, 'STEEL': 12, 'DIAMOND': 13, 'COAL': 14, 'BEDROCK': 15,
            'SNOW': 18, 'ASH': 16, 'TNT': 17
        };

        for (const block of blocks) {
            const x = Math.round(block.x);
            const y = Math.round(block.y);
            const z = Math.round(block.z);
            const typeName = (block.type || 'STONE').toUpperCase();
            const type = typeMap[typeName] !== undefined ? typeMap[typeName] : 1;

            if (x >= 0 && x < 256 && y >= 0 && y < 256 && z >= 0 && z < 256) {
                this.engine.setBlock(x, y, z, type);
                placed++;
            }
        }

        return placed;
    }
}

window.LLMClient = LLMClient;
