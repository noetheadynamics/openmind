/**
 * OpenMind – Agent Configuration
 * Role, personality, goals, and presets for autonomous agents
 */
(function() {
    'use strict';

    const ROLES = {
        builder: { label: 'Builder', description: 'Constructs buildings and structures', icon: '🔨' },
        farmer: { label: 'Farmer', description: 'Plants crops and manages food', icon: '🌾' },
        explorer: { label: 'Explorer', description: 'Scouts the world and maps terrain', icon: '🧭' },
        miner: { label: 'Miner', description: 'Digs tunnels and gathers resources', icon: '⛏️' },
        guard: { label: 'Guard', description: 'Protects areas and patrols', icon: '🛡️' },
        artist: { label: 'Artist', description: 'Creates sculptures and decorations', icon: '🎨' }
    };

    const PERSONALITIES = {
        helpful: { label: 'Helpful', description: 'Eager to assist and complete tasks' },
        cautious: { label: 'Cautious', description: 'Plans carefully before acting' },
        creative: { label: 'Creative', description: 'Finds unique solutions' },
        efficient: { label: 'Efficient', description: 'Optimizes for speed and minimal blocks' },
        thorough: { label: 'Thorough', description: 'Takes time to do things right' }
    };

    const GOAL_PRESETS = {
        build_house: {
            label: 'Build a House',
            description: 'Build a complete wooden house with walls, roof, door, and windows',
            substeps: [
                { description: 'Survey the area for a good building spot', done: false },
                { description: 'Build a wooden floor foundation', done: false },
                { description: 'Build walls with hollow_rect', done: false },
                { description: 'Add a roof with fill_rect', done: false },
                { description: 'Add door and glass windows', done: false },
                { description: 'Report completion', done: false }
            ],
            constraints: 'Use WOOD for walls and roof, GLASS for windows. Keep under 2000 blocks.'
        },
        build_tower: {
            label: 'Build a Tower',
            description: 'Build a tall stone tower',
            substeps: [
                { description: 'Build a stone foundation', done: false },
                { description: 'Build tower shaft upward', done: false },
                { description: 'Add a platform at the top', done: false },
                { description: 'Report completion', done: false }
            ],
            constraints: 'Use STONE. Height should be 20+ blocks.'
        },
        build_garden: {
            label: 'Build a Garden',
            description: 'Create a garden with grass, flowers, and a path',
            substeps: [
                { description: 'Clear a flat area', done: false },
                { description: 'Place grass blocks', done: false },
                { description: 'Create a stone path', done: false },
                { description: 'Add leaf hedges', done: false },
                { description: 'Report completion', done: false }
            ],
            constraints: 'Use GRASS, LEAVES, STONE for path. Keep natural looking.'
        },
        build_bridge: {
            label: 'Build a Bridge',
            description: 'Build a bridge over a gap or water',
            substeps: [
                { description: 'Survey area to find water or gap', done: false },
                { description: 'Build support pillars', done: false },
                { description: 'Build the bridge deck', done: false },
                { description: 'Add railings', done: false },
                { description: 'Report completion', done: false }
            ],
            constraints: 'Use WOOD or STONE. Must span the gap.'
        },
        custom: {
            label: 'Custom Task',
            description: 'User-defined task'
        }
    };

    const DEFAULT_CONFIG = {
        name: 'Agent',
        role: 'builder',
        personality: 'helpful',
        goalPreset: 'custom',
        customGoal: '',
        startX: 128,
        startY: 2,
        startZ: 128,
        thinkIntervalMs: 3000,
        maxIterations: 15,
        enabled: true
    };

    class AgentConfig {
        constructor() {
            this.config = { ...DEFAULT_CONFIG };
        }

        static getRoles() { return ROLES; }
        static getPersonalities() { return PERSONALITIES; }
        static getGoalPresets() { return GOAL_PRESETS; }
        static getDefaults() { return { ...DEFAULT_CONFIG }; }

        apply(overrides) {
            Object.assign(this.config, overrides);
            return this.config;
        }

        buildGoal() {
            const preset = GOAL_PRESETS[this.config.goalPreset] || GOAL_PRESETS.custom;
            if (this.config.goalPreset === 'custom' && this.config.customGoal) {
                return {
                    description: this.config.customGoal,
                    substeps: [],
                    constraints: ''
                };
            }
            return {
                description: preset.description,
                substeps: (preset.substeps || []).map(s => ({ ...s })),
                constraints: preset.constraints || ''
            };
        }

        toAgentData() {
            return {
                name: this.config.name,
                role: this.config.role,
                personality: this.config.personality,
                x: this.config.startX,
                y: this.config.startY,
                z: this.config.startZ,
                health: 100,
                energy: 100,
                hunger: 0,
                memories: [],
                inventory: {},
                currentGoal: this.buildGoal(),
                thinkIntervalMs: this.config.thinkIntervalMs,
                maxIterations: this.config.maxIterations
            };
        }
    }

    window.AgentConfig = AgentConfig;
    window.AGENT_ROLES = ROLES;
    window.AGENT_PERSONALITIES = PERSONALITIES;
    window.AGENT_GOAL_PRESETS = GOAL_PRESETS;
})();
