/**
 * OpenMind – Agent Prompt Builder
 * Constructs comprehensive prompts for autonomous agent reasoning
 */
(function() {
    'use strict';

    const BLOCK_TYPES = [
        'AIR','STONE','DIRT','GRASS','WATER','SAND','GLASS','WOOD','LEAVES',
        'IRON','COPPER','GOLD','STEEL','DIAMOND','COAL','BEDROCK','ASH','TNT','SNOW'
    ];

    const AGENT_TOOLS = [
        {
            type: 'function',
            function: {
                name: 'move_to',
                description: 'Move the agent to a target position. Returns arrival status.',
                parameters: {
                    type: 'object',
                    properties: {
                        x: { type: 'integer', description: 'Target X (0-255)' },
                        y: { type: 'integer', description: 'Target Y (0-255)' },
                        z: { type: 'integer', description: 'Target Z (0-255)' }
                    },
                    required: ['x', 'y', 'z']
                }
            }
        },
        {
            type: 'function',
            function: {
                name: 'place_block',
                description: 'Place a single block at coordinates. Returns success/failure.',
                parameters: {
                    type: 'object',
                    properties: {
                        x: { type: 'integer', description: 'X coordinate (0-255)' },
                        y: { type: 'integer', description: 'Y coordinate (0-255)' },
                        z: { type: 'integer', description: 'Z coordinate (0-255)' },
                        type: { type: 'string', description: 'Block type: STONE, DIRT, GRASS, WOOD, GLASS, SAND, WATER, LEAVES, IRON, COPPER, GOLD, STEEL, DIAMOND, COAL, SNOW', enum: BLOCK_TYPES.slice(1) }
                    },
                    required: ['x', 'y', 'z', 'type']
                }
            }
        },
        {
            type: 'function',
            function: {
                name: 'place_blocks',
                description: 'Place multiple blocks at once. Efficient for bulk operations.',
                parameters: {
                    type: 'object',
                    properties: {
                        blocks: {
                            type: 'array',
                            items: {
                                type: 'object',
                                properties: {
                                    x: { type: 'integer' }, y: { type: 'integer' }, z: { type: 'integer' },
                                    type: { type: 'string', enum: BLOCK_TYPES.slice(1) }
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
                description: 'Fill a rectangular volume with blocks. For floors, walls, roofs.',
                parameters: {
                    type: 'object',
                    properties: {
                        x1: { type: 'integer' }, y1: { type: 'integer' }, z1: { type: 'integer' },
                        x2: { type: 'integer' }, y2: { type: 'integer' }, z2: { type: 'integer' },
                        type: { type: 'string', enum: BLOCK_TYPES.slice(1) }
                    },
                    required: ['x1', 'y1', 'z1', 'x2', 'y2', 'z2', 'type']
                }
            }
        },
        {
            type: 'function',
            function: {
                name: 'hollow_rect',
                description: 'Create walls of a rectangular volume (hollow inside). For building shells.',
                parameters: {
                    type: 'object',
                    properties: {
                        x1: { type: 'integer' }, y1: { type: 'integer' }, z1: { type: 'integer' },
                        x2: { type: 'integer' }, y2: { type: 'integer' }, z2: { type: 'integer' },
                        type: { type: 'string', enum: BLOCK_TYPES.slice(1) }
                    },
                    required: ['x1', 'y1', 'z1', 'x2', 'y2', 'z2', 'type']
                }
            }
        },
        {
            type: 'function',
            function: {
                name: 'get_block',
                description: 'Check what block is at a position. Returns type name or AIR.',
                parameters: {
                    type: 'object',
                    properties: { x: { type: 'integer' }, y: { type: 'integer' }, z: { type: 'integer' } },
                    required: ['x', 'y', 'z']
                }
            }
        },
        {
            type: 'function',
            function: {
                name: 'survey_area',
                description: 'Survey a region. Returns block type counts.',
                parameters: {
                    type: 'object',
                    properties: {
                        x1: { type: 'integer' }, y1: { type: 'integer' }, z1: { type: 'integer' },
                        x2: { type: 'integer' }, y2: { type: 'integer' }, z2: { type: 'integer' }
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
                        x1: { type: 'integer' }, y1: { type: 'integer' }, z1: { type: 'integer' },
                        x2: { type: 'integer' }, y2: { type: 'integer' }, z2: { type: 'integer' }
                    },
                    required: ['x1', 'y1', 'z1', 'x2', 'y2', 'z2']
                }
            }
        },
        {
            type: 'function',
            function: {
                name: 'observe',
                description: 'Look around the agent. Returns nearby blocks within a radius.',
                parameters: {
                    type: 'object',
                    properties: {
                        radius: { type: 'integer', description: 'Look radius (1-20, default 5)' }
                    }
                }
            }
        },
        {
            type: 'function',
            function: {
                name: 'report',
                description: 'Report status or findings. Used to communicate with the user.',
                parameters: {
                    type: 'object',
                    properties: {
                        message: { type: 'string', description: 'Report message' }
                    },
                    required: ['message']
                }
            }
        }
    ];

    class AgentPromptBuilder {
        constructor() {
            this.maxMemories = 20;
        }

        buildSystemPrompt(agent) {
            const personality = agent.personality || 'helpful';
            const role = agent.role || 'builder';

            return `You are "${agent.name || 'Agent'}", an autonomous agent in a 3D voxel world.

ROLE: ${role.charAt(0).toUpperCase() + role.slice(1)}
PERSONALITY: ${personality.charAt(0).toUpperCase() + personality.slice(1)}

You have tools to interact with the world. Think step by step, plan your actions, and execute them using tool calls.

COORDINATE SYSTEM:
- x: 0-255 (east-west)
- y: 0-255 (vertical, y=1 is ground level, y increases upward)
- z: 0-255 (north-south)

BLOCK TYPES: STONE, DIRT, GRASS, WATER, SAND, GLASS, WOOD, LEAVES, IRON, COPPER, GOLD, STEEL, DIAMOND, COAL, SNOW

BUILDING RULES:
- Use fill_rect for large surfaces (floors, walls, roofs)
- Use hollow_rect for building shells
- Plan structure: foundation → walls → roof → details
- Keep blocks under 3000 per task for performance
- Think before acting: survey the area first, then plan, then build

BEHAVIOR:
- Be methodical: break complex tasks into smaller steps
- Report your progress using the report tool
- If something fails, try a different approach
- When your goal is complete, report what you built`;
        }

        buildWorldContext(engine) {
            if (!engine || !engine.wasmReady) return 'World: Not loaded';

            try {
                const stats = engine.getWorldStats();
                let ctx = `World State:\n`;
                ctx += `- Total blocks: ${stats.totalBlocks || 0}\n`;
                ctx += `- Temperature: ${(stats.temperature || 293).toFixed(0)}K\n`;
                ctx += `- Weather: ${stats.weather || 'clear'}\n`;
                ctx += `- Time of day: ${stats.timeOfDay || 12}\n`;
                return ctx;
            } catch (e) {
                return 'World State: Unable to read';
            }
        }

        buildAgentState(agent) {
            let state = `Agent State:\n`;
            state += `- Position: (${Math.round(agent.x)}, ${Math.round(agent.y)}, ${Math.round(agent.z)})\n`;
            state += `- Health: ${Math.round(agent.health || 100)}%\n`;
            state += `- Energy: ${Math.round(agent.energy || 100)}%\n`;
            if (agent.inventory) {
                const items = Object.entries(agent.inventory).filter(([,v]) => v > 0);
                if (items.length > 0) {
                    state += `- Inventory: ${items.map(([k,v]) => `${k}:${v}`).join(', ')}\n`;
                }
            }
            return state;
        }

        buildMemoryContext(memories) {
            if (!memories || memories.length === 0) return 'Memory: Empty (first task)\n';
            const recent = memories.slice(-this.maxMemories);
            let ctx = `Recent Memory (last ${recent.length} entries):\n`;
            for (const m of recent) {
                ctx += `- [${m.type || 'info'}] ${m.text || JSON.stringify(m)}\n`;
            }
            return ctx;
        }

        buildGoalContext(goal) {
            if (!goal) return 'Goal: None assigned\n';
            let ctx = `Current Goal: ${goal.description}\n`;
            if (goal.substeps && goal.substeps.length > 0) {
                ctx += `Substeps:\n`;
                for (let i = 0; i < goal.substeps.length; i++) {
                    const s = goal.substeps[i];
                    ctx += `  ${i + 1}. ${s.description || s} ${s.done ? '[DONE]' : '[TODO]'}\n`;
                }
            }
            if (goal.constraints) {
                ctx += `Constraints: ${goal.constraints}\n`;
            }
            return ctx;
        }

        buildFullPrompt(agent, engine, memories, goal) {
            const system = this.buildSystemPrompt(agent);
            const world = this.buildWorldContext(engine);
            const agentState = this.buildAgentState(agent);
            const memory = this.buildMemoryContext(memories);
            const goalCtx = this.buildGoalContext(goal);

            const userMessage = `${world}\n\n${agentState}\n\n${memory}\n\n${goalCtx}\n\nWhat should you do next? Use tools to take action.`;

            return {
                messages: [
                    { role: 'system', content: system },
                    { role: 'user', content: userMessage }
                ],
                tools: AGENT_TOOLS
            };
        }
    }

    window.AgentPromptBuilder = AgentPromptBuilder;
    window.AGENT_TOOLS = AGENT_TOOLS;
    window.BLOCK_TYPES = BLOCK_TYPES;
})();
