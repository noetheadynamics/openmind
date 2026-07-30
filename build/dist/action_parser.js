/**
 * OpenMind – Action Parser
 * Parses LLM responses into structured agent actions
 */
(function() {
    'use strict';

    const VALID_TOOLS = new Set([
        'move_to', 'place_block', 'place_blocks', 'fill_rect', 'hollow_rect',
        'get_block', 'survey_area', 'clear_area', 'observe', 'report'
    ]);

    const VALID_BLOCK_TYPES = new Set([
        'AIR','STONE','DIRT','GRASS','WATER','SAND','GLASS','WOOD','LEAVES',
        'IRON','COPPER','GOLD','STEEL','DIAMOND','COAL','BEDROCK','ASH','TNT','SNOW',
        'DOOR','BUTTON','LAUNCHER','LOCK','LAMP','CHEST','SWITCH','CONVEYOR','PISTON','TRAPDOOR','FIRE'
    ]);

    class ActionParser {
        constructor() {
            this.errors = [];
        }

        parse(response) {
            this.errors = [];
            if (!response) return { actions: [], text: '', errors: ['Empty response'] };

            if (typeof response === 'string') {
                return this.parseText(response);
            }

            if (response.tool_calls && Array.isArray(response.tool_calls)) {
                return this.parseToolCalls(response.tool_calls);
            }

            return { actions: [], text: response.text || '', errors: ['Unrecognized response format'] };
        }

        parseText(text) {
            const cleaned = text.replace(/```json\s*/g, '').replace(/```\s*/g, '').trim();
            try {
                const parsed = JSON.parse(cleaned);
                if (parsed.tool_calls) return this.parseToolCalls(parsed.tool_calls);
                if (Array.isArray(parsed)) return this.parseActionArray(parsed);
            } catch (e) {
                const match = cleaned.match(/\{[\s\S]*\}/);
                if (match) {
                    try {
                        const parsed = JSON.parse(match[0]);
                        if (parsed.tool_calls) return this.parseToolCalls(parsed.tool_calls);
                    } catch {}
                }
            }
            return { actions: [], text: text, errors: [], summary: text };
        }

        parseToolCalls(toolCalls) {
            const actions = [];
            const errors = [];

            for (const tc of toolCalls) {
                const fnName = tc.function?.name || tc.name;
                let fnArgs;
                try {
                    fnArgs = typeof tc.function?.arguments === 'string'
                        ? JSON.parse(tc.function.arguments)
                        : (tc.function?.arguments || tc.input || {});
                } catch (e) {
                    errors.push(`Invalid JSON in arguments for ${fnName}: ${e.message}`);
                    continue;
                }

                const validation = this.validateAction(fnName, fnArgs);
                if (validation.valid) {
                    actions.push({ tool: fnName, args: fnArgs, id: tc.id || null });
                } else {
                    errors.push(...validation.errors);
                }
            }

            return { actions, text: '', errors };
        }

        parseActionArray(arr) {
            const actions = [];
            const errors = [];

            for (const item of arr) {
                const tool = item.tool || item.action || item.type;
                const args = item.args || item.arguments || item.params || item;
                const validation = this.validateAction(tool, args);
                if (validation.valid) {
                    actions.push({ tool, args, id: null });
                } else {
                    errors.push(...validation.errors);
                }
            }

            return { actions, text: '', errors };
        }

        validateAction(toolName, args) {
            const errors = [];
            if (!toolName || typeof toolName !== 'string') {
                errors.push('Missing or invalid tool name');
                return { valid: false, errors };
            }
            if (!VALID_TOOLS.has(toolName)) {
                errors.push(`Unknown tool: "${toolName}". Valid: ${[...VALID_TOOLS].join(', ')}`);
                return { valid: false, errors };
            }

            switch (toolName) {
                case 'move_to':
                    if (typeof args.x !== 'number' || typeof args.z !== 'number') {
                        errors.push('move_to requires x and z coordinates');
                    }
                    break;
                case 'place_block':
                    if (typeof args.x !== 'number' || typeof args.y !== 'number' || typeof args.z !== 'number') {
                        errors.push('place_block requires x, y, z coordinates');
                    }
                    if (args.type && !VALID_BLOCK_TYPES.has(args.type.toUpperCase())) {
                        errors.push(`Invalid block type: "${args.type}"`);
                    }
                    break;
                case 'place_blocks':
                    if (!Array.isArray(args.blocks)) {
                        errors.push('place_blocks requires a blocks array');
                    }
                    break;
                case 'fill_rect':
                case 'hollow_rect':
                case 'clear_area':
                    if (typeof args.x1 !== 'number' || typeof args.y1 !== 'number' || typeof args.z1 !== 'number' ||
                        typeof args.x2 !== 'number' || typeof args.y2 !== 'number' || typeof args.z2 !== 'number') {
                        errors.push(`${toolName} requires x1,y1,z1,x2,y2,z2`);
                    }
                    break;
                case 'get_block':
                    if (typeof args.x !== 'number' || typeof args.y !== 'number' || typeof args.z !== 'number') {
                        errors.push('get_block requires x, y, z');
                    }
                    break;
                case 'survey_area':
                    if (typeof args.x1 !== 'number' || typeof args.y1 !== 'number' || typeof args.z1 !== 'number' ||
                        typeof args.x2 !== 'number' || typeof args.y2 !== 'number' || typeof args.z2 !== 'number') {
                        errors.push('survey_area requires x1,y1,z1,x2,y2,z2');
                    }
                    break;
                case 'report':
                    if (!args.message || typeof args.message !== 'string') {
                        errors.push('report requires a message string');
                    }
                    break;
            }

            return { valid: errors.length === 0, errors };
        }
    }

    window.AgentActionParser = ActionParser;
})();
