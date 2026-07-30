/**
 * OpenMind – Agent Management
 * Multi-agent manager with IndexedDB persistence
 */
(function() {
    'use strict';

    class Agent {
        constructor(data) {
            this.id = data.id || crypto.randomUUID().substring(0, 8);
            this.name = data.name || 'Agent';
            this.role = data.role || 'builder';
            this.personality = data.personality || 'helpful';
            this.x = data.x || 128;
            this.y = data.y || 2;
            this.z = data.z || 128;
            this.health = data.health ?? 100;
            this.energy = data.energy ?? 100;
            this.hunger = data.hunger ?? 0;
            this.memories = data.memories || [];
            this.inventory = data.inventory || {};
            this.currentGoal = data.currentGoal || null;
            this.state = data.state || 'idle';
            this.lastActive = data.lastActive || Date.now();
            this.createdAt = data.createdAt || Date.now();
            this.thinkLoop = null;
        }

        addMemory(type, text) {
            this.memories.push({ type, text, time: Date.now() });
            if (this.memories.length > 50) this.memories = this.memories.slice(-50);
        }

        toJSON() {
            return {
                id: this.id, name: this.name, role: this.role, personality: this.personality,
                x: this.x, y: this.y, z: this.z, health: this.health, energy: this.energy,
                hunger: this.hunger, memories: this.memories, inventory: this.inventory,
                currentGoal: this.currentGoal, state: this.state,
                lastActive: this.lastActive, createdAt: this.createdAt
            };
        }
    }

    class AgentManager {
        constructor() {
            this.agents = new Map();
            this.dbName = 'openmind_agents';
            this.storeName = 'agents';
            this.db = null;
            this.llm = null;
            this.engine = null;
            this.renderer = null;
            this.onAgentUpdate = null;
            this.onAgentMessage = null;
        }

        async init() {
            try {
                this.db = await new Promise((resolve, reject) => {
                    const req = indexedDB.open(this.dbName, 1);
                    req.onupgradeneeded = (e) => {
                        const db = e.target.result;
                        if (!db.objectStoreNames.contains(this.storeName)) {
                            db.createObjectStore(this.storeName, { keyPath: 'id' });
                        }
                    };
                    req.onsuccess = (e) => resolve(e.target.result);
                    req.onerror = (e) => reject(e.target.error);
                });
            } catch (e) {
                console.warn('[AgentManager] IndexedDB unavailable:', e.message);
            }
            await this.loadAll();
        }

        async save(agent) {
            this.agents.set(agent.id, agent);
            if (this.db) {
                try {
                    const tx = this.db.transaction(this.storeName, 'readwrite');
                    tx.objectStore(this.storeName).put(agent.toJSON());
                } catch (e) {}
            }
        }

        async delete(agentId) {
            const agent = this.agents.get(agentId);
            if (agent && agent.thinkLoop) agent.thinkLoop.stop();
            this.agents.delete(agentId);
            if (this.db) {
                try {
                    const tx = this.db.transaction(this.storeName, 'readwrite');
                    tx.objectStore(this.storeName).delete(agentId);
                } catch (e) {}
            }
        }

        async loadAll() {
            if (!this.db) return;
            try {
                const tx = this.db.transaction(this.storeName, 'readonly');
                const req = tx.objectStore(this.storeName).getAll();
                const data = await new Promise((resolve) => {
                    req.onsuccess = () => resolve(req.result);
                    req.onerror = () => resolve([]);
                });
                for (const d of data) {
                    this.agents.set(d.id, new Agent(d));
                }
            } catch (e) {}
        }

        async saveAll() {
            if (!this.db) return;
            try {
                const tx = this.db.transaction(this.storeName, 'readwrite');
                const store = tx.objectStore(this.storeName);
                for (const [, agent] of this.agents) {
                    store.put(agent.toJSON());
                }
            } catch (e) {}
        }

        spawn(config) {
            const agentData = config.toAgentData();
            const agent = new Agent(agentData);
            this.agents.set(agent.id, agent);
            this.save(agent);
            return agent;
        }

        startAgent(agentId) {
            const agent = this.agents.get(agentId);
            if (!agent || !this.llm || !this.engine) return false;

            if (agent.thinkLoop) agent.thinkLoop.stop();

            agent.thinkLoop = new AgentThinkLoop(agent, this.llm, this.engine);
            agent.thinkLoop.maxIterations = agent.maxIterations || 15;
            agent.thinkLoop.intervalMs = agent.thinkIntervalMs || 3000;

            agent.thinkLoop.onAction = (action, result) => {
                agent.state = 'acting';
                agent.lastActive = Date.now();
                this.save(agent);
                if (this.onAgentMessage) this.onAgentMessage(agentId, `Action: ${action.tool} → ${result.substring(0, 100)}`);
            };

            agent.thinkLoop.onThought = (thought) => {
                agent.lastActive = Date.now();
                if (this.onAgentMessage) this.onAgentMessage(agentId, `Thought: ${thought.substring(0, 200)}`);
            };

            agent.thinkLoop.onComplete = (reason) => {
                agent.state = 'idle';
                agent.addMemory('complete', reason);
                this.save(agent);
                if (this.onAgentUpdate) this.onAgentUpdate(agentId);
                if (this.onAgentMessage) this.onAgentMessage(agentId, `Completed: ${reason}`);
            };

            agent.thinkLoop.onError = (error) => {
                agent.addMemory('error', error);
                this.save(agent);
                if (this.onAgentMessage) this.onAgentMessage(agentId, `Error: ${error}`);
            };

            agent.state = 'thinking';
            agent.thinkLoop.start();
            this.save(agent);
            if (this.onAgentUpdate) this.onAgentUpdate(agentId);
            return true;
        }

        stopAgent(agentId) {
            const agent = this.agents.get(agentId);
            if (!agent) return;
            if (agent.thinkLoop) agent.thinkLoop.stop();
            agent.state = 'idle';
            this.save(agent);
            if (this.onAgentUpdate) this.onAgentUpdate(agentId);
        }

        stopAll() {
            for (const [, agent] of this.agents) {
                if (agent.thinkLoop) agent.thinkLoop.stop();
                agent.state = 'idle';
            }
            this.saveAll();
        }

        getAll() { return [...this.agents.values()]; }
        get(agentId) { return this.agents.get(agentId); }
        getCount() { return this.agents.size; }

        setLLM(llm) { this.llm = llm; }
        setEngine(engine) { this.engine = engine; }
        setRenderer(renderer) { this.renderer = renderer; }
    }

    window.Agent = Agent;
    window.AgentManager = AgentManager;
})();
