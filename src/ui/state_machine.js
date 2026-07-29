/**
 * OpenMind – State Machine System
 * Tracks state of every interactive object, supports transitions, conditions, animations
 */
(function() {
    'use strict';

    const State = Object.freeze({
        // Door
        DOOR_CLOSED: 'DOOR_CLOSED',
        DOOR_OPENING: 'DOOR_OPENING',
        DOOR_OPEN: 'DOOR_OPEN',
        DOOR_CLOSING: 'DOOR_CLOSING',
        DOOR_LOCKED: 'DOOR_LOCKED',
        // Button
        BUTTON_IDLE: 'BUTTON_IDLE',
        BUTTON_PRESSED: 'BUTTON_PRESSED',
        BUTTON_RELEASING: 'BUTTON_RELEASING',
        // Lever
        LEVER_UP: 'LEVER_UP',
        LEVER_DOWN: 'LEVER_DOWN',
        LEVER_MOVING: 'LEVER_MOVING',
        // Switch
        SWITCH_OFF: 'SWITCH_OFF',
        SWITCH_ON: 'SWITCH_ON',
        // Chest
        CHEST_CLOSED: 'CHEST_CLOSED',
        CHEST_OPEN: 'CHEST_OPEN',
        CHEST_LOCKED: 'CHEST_LOCKED',
        // Lamp
        LAMP_OFF: 'LAMP_OFF',
        LAMP_ON: 'LAMP_ON',
        // Piston
        PISTON_RETRACTED: 'PISTON_RETRACTED',
        PISTON_EXTENDING: 'PISTON_EXTENDING',
        PISTON_EXTENDED: 'PISTON_EXTENDED',
        PISTON_RETRACTING: 'PISTON_RETRACTING',
        // Conveyor
        CONVEYOR_OFF: 'CONVEYOR_OFF',
        CONVEYOR_ON: 'CONVEYOR_ON',
        // Trapdoor
        TRAPDOOR_CLOSED: 'TRAPDOOR_CLOSED',
        TRAPDOOR_OPENING: 'TRAPDOOR_OPENING',
        TRAPDOOR_OPEN: 'TRAPDOOR_OPEN',
        TRAPDOOR_CLOSING: 'TRAPDOOR_CLOSING',
        // Fire
        FIRE_BURNING: 'FIRE_BURNING',
        FIRE_EXTINGUISHED: 'FIRE_EXTINGUISHED',
        // Lock
        LOCK_LOCKED: 'LOCK_LOCKED',
        LOCK_UNLOCKED: 'LOCK_UNLOCKED',
        // Launcher
        LAUNCHER_IDLE: 'LAUNCHER_IDLE',
        LAUNCHER_CHARGING: 'LAUNCHER_CHARGING',
        LAUNCHER_LAUNCHED: 'LAUNCHER_LAUNCHED',
        LAUNCHER_COOLDOWN: 'LAUNCHER_COOLDOWN',
        // Sensor
        SENSOR_IDLE: 'SENSOR_IDLE',
        SENSOR_TRIGGERED: 'SENSOR_TRIGGERED',
        // Timer
        TIMER_STOPPED: 'TIMER_STOPPED',
        TIMER_RUNNING: 'TIMER_RUNNING',
        // Computer
        COMPUTER_OFF: 'COMPUTER_OFF',
        COMPUTER_BOOTING: 'COMPUTER_BOOTING',
        COMPUTER_ON: 'COMPUTER_ON'
    });

    const Transitions = {
        DOOR: {
            DOOR_CLOSED: { on: 'DOOR_OPENING', off: null, key: null },
            DOOR_OPENING: { complete: 'DOOR_OPEN', duration: 0.3 },
            DOOR_OPEN: { on: 'DOOR_CLOSING', off: null, key: null },
            DOOR_CLOSING: { complete: 'DOOR_CLOSED', duration: 0.3 },
            DOOR_LOCKED: { unlock: 'DOOR_CLOSED' }
        },
        BUTTON: {
            BUTTON_IDLE: { press: 'BUTTON_PRESSED' },
            BUTTON_PRESSED: { complete: 'BUTTON_RELEASING', duration: 0.15, signal: true },
            BUTTON_RELEASING: { complete: 'BUTTON_IDLE', duration: 0.1 }
        },
        LEVER: {
            LEVER_UP: { toggle: 'LEVER_MOVING', signal: 'off' },
            LEVER_DOWN: { toggle: 'LEVER_MOVING', signal: 'on' },
            LEVER_MOVING: { complete: null, duration: 0.2, getNext: (prev) => prev === 'LEVER_UP' ? 'LEVER_DOWN' : 'LEVER_UP' }
        },
        SWITCH: {
            SWITCH_OFF: { toggle: 'SWITCH_ON', signal: 'on' },
            SWITCH_ON: { toggle: 'SWITCH_OFF', signal: 'off' }
        },
        CHEST: {
            CHEST_CLOSED: { open: 'CHEST_OPEN', key: null },
            CHEST_OPEN: { close: 'CHEST_CLOSED' },
            CHEST_LOCKED: { unlock: 'CHEST_CLOSED' }
        },
        LAMP: {
            LAMP_OFF: { toggle: 'LAMP_ON', signal: 'on' },
            LAMP_ON: { toggle: 'LAMP_OFF', signal: 'off' }
        },
        PISTON: {
            PISTON_RETRACTED: { extend: 'PISTON_EXTENDING', signal: 'on' },
            PISTON_EXTENDING: { complete: 'PISTON_EXTENDED', duration: 0.25 },
            PISTON_EXTENDED: { retract: 'PISTON_RETRACTING', signal: 'off' },
            PISTON_RETRACTING: { complete: 'PISTON_RETRACTED', duration: 0.25 }
        },
        CONVEYOR: {
            CONVEYOR_OFF: { toggle: 'CONVEYOR_ON', signal: 'on' },
            CONVEYOR_ON: { toggle: 'CONVEYOR_OFF', signal: 'off' }
        },
        TRAPDOOR: {
            TRAPDOOR_CLOSED: { open: 'TRAPDOOR_OPENING' },
            TRAPDOOR_OPENING: { complete: 'TRAPDOOR_OPEN', duration: 0.25 },
            TRAPDOOR_OPEN: { close: 'TRAPDOOR_CLOSING' },
            TRAPDOOR_CLOSING: { complete: 'TRAPDOOR_CLOSED', duration: 0.25 }
        },
        FIRE: {
            FIRE_BURNING: { extinguish: 'FIRE_EXTINGUISHED', signal: 'off' },
            FIRE_EXTINGUISHED: { light: 'FIRE_BURNING', signal: 'on' }
        },
        LOCK: {
            LOCK_LOCKED: { unlock: 'LOCK_UNLOCKED', signal: 'unlocked', requires: 'key' },
            LOCK_UNLOCKED: { lock: 'LOCK_LOCKED', signal: 'locked' }
        },
        LAUNCHER: {
            LAUNCHER_IDLE: { activate: 'LAUNCHER_CHARGING' },
            LAUNCHER_CHARGING: { complete: 'LAUNCHER_LAUNCHED', duration: 1.0, signal: 'launch' },
            LAUNCHER_LAUNCHED: { complete: 'LAUNCHER_COOLDOWN', duration: 2.0 },
            LAUNCHER_COOLDOWN: { complete: 'LAUNCHER_IDLE', duration: 1.0 }
        },
        SENSOR: {
            SENSOR_IDLE: { detect: 'SENSOR_TRIGGERED', signal: 'on' },
            SENSOR_TRIGGERED: { clear: 'SENSOR_IDLE', signal: 'off', autoClear: true, duration: 0.5 }
        },
        TIMER: {
            TIMER_STOPPED: { start: 'TIMER_RUNNING', signal: 'start' },
            TIMER_RUNNING: { complete: 'TIMER_STOPPED', signal: 'end' }
        },
        COMPUTER: {
            COMPUTER_OFF: { power: 'COMPUTER_BOOTING' },
            COMPUTER_BOOTING: { complete: 'COMPUTER_ON', duration: 2.0 },
            COMPUTER_ON: { power: 'COMPUTER_OFF' }
        }
    };

    class StateMachine {
        constructor() {
            this.states = new Map();
            this.listeners = new Map();
            this.conditions = new Map();
            this.chains = [];
            this.timers = new Map();
            this.animating = new Map();
            this.dirty = false;
        }

        register(id, type, initialState, props) {
            const t = Transitions[type];
            if (!t) return false;
            const st = initialState || Object.keys(t)[0];
            this.states.set(id, {
                id, type, state: st, props: props || {},
                lastSignal: null, lastTick: Date.now(), animProgress: 0
            });
            this.dirty = true;
            return true;
        }

        unregister(id) {
            this.states.delete(id);
            this.timers.delete(id);
            this.animating.delete(id);
            this.dirty = true;
        }

        get(id) { return this.states.get(id) || null; }

        getAll() { return Array.from(this.states.values()); }

        getByType(type) {
            return this.getAll().filter(s => s.type === type);
        }

        transition(id, action) {
            const entry = this.states.get(id);
            if (!entry) return null;

            const tDef = Transitions[entry.type];
            if (!tDef) return null;

            const currentTrans = tDef[entry.state];
            if (!currentTrans) return null;

            if (currentTrans.requires) {
                if (currentTrans.requires === 'key' && !entry.props.hasKey) {
                    this.emit(id, 'locked', { reason: 'requires_key' });
                    return null;
                }
            }

            let nextState;
            if (action === 'complete' && currentTrans.complete) {
                nextState = currentTrans.complete;
            } else if (currentTrans[action]) {
                if (currentTrans[action].getNext) {
                    nextState = currentTrans[action].getNext(entry.state);
                } else {
                    nextState = currentTrans[action];
                }
            } else {
                return null;
            }

            if (!nextState) return null;

            const oldState = entry.state;
            entry.state = nextState;
            entry.lastTick = Date.now();
            entry.animProgress = 0;
            this.dirty = true;

            const newTrans = tDef[nextState];
            if (newTrans && newTrans.duration) {
                this.animating.set(id, {
                    duration: newTrans.duration * 1000,
                    start: Date.now(),
                    from: oldState,
                    to: nextState
                });
            }

            const signal = currentTrans[action] && currentTrans[action].signal;
            if (signal !== undefined && signal !== null) {
                entry.lastSignal = signal;
                this.emit(id, 'signal', { signal, from: oldState, to: nextState });
            }

            this.emit(id, 'transition', { from: oldState, to: nextState, action });

            this.evaluateConditions(id, nextState);
            this.evaluateChains(id, nextState);

            return { from: oldState, to: nextState, signal };
        }

        addCondition(sourceId, targetId, sourceState, targetAction) {
            if (!this.conditions.has(sourceId)) this.conditions.set(sourceId, []);
            this.conditions.get(sourceId).push({ targetId, sourceState, targetAction });
        }

        evaluateConditions(sourceId, newState) {
            const conds = this.conditions.get(sourceId);
            if (!conds) return;
            for (const c of conds) {
                if (c.sourceState === newState || c.sourceState === '*') {
                    this.transition(c.targetId, c.targetAction);
                }
            }
        }

        addChain(chain) {
            this.chains.push(chain);
        }

        evaluateChains(sourceId, newState) {
            for (const chain of this.chains) {
                for (let i = 0; i < chain.length; i++) {
                    if (chain[i].id === sourceId && chain[i].state === newState) {
                        if (i + 1 < chain.length) {
                            const next = chain[i + 1];
                            this.transition(next.id, next.action);
                        }
                        break;
                    }
                }
            }
        }

        addTimer(id, durationMs, callback) {
            this.timers.set(id, { duration: durationMs, start: Date.now(), callback, done: false });
        }

        updateTimers() {
            const now = Date.now();
            for (const [id, timer] of this.timers) {
                if (timer.done) continue;
                if (now - timer.start >= timer.duration) {
                    timer.done = true;
                    timer.callback(id);
                    this.timers.delete(id);
                }
            }
        }

        updateAnimations() {
            const now = Date.now();
            for (const [id, anim] of this.animating) {
                const elapsed = now - anim.start;
                anim.progress = Math.min(elapsed / anim.duration, 1);
                if (anim.progress >= 1) {
                    this.animating.delete(id);
                    this.transition(id, 'complete');
                }
            }
        }

        getAnimation(id) {
            return this.animating.get(id) || null;
        }

        on(id, event, fn) {
            if (!this.listeners.has(id)) this.listeners.set(id, {});
            const l = this.listeners.get(id);
            if (!l[event]) l[event] = [];
            l[event].push(fn);
        }

        off(id, event, fn) {
            const l = this.listeners.get(id);
            if (l && l[event]) {
                l[event] = l[event].filter(f => f !== fn);
            }
        }

        emit(id, event, data) {
            const l = this.listeners.get(id);
            if (l && l[event]) {
                for (const fn of l[event]) fn(data);
            }
            const global = this.listeners.get('*');
            if (global && global[event]) {
                for (const fn of global[event]) fn({ id, ...data });
            }
        }

        serialize() {
            const out = {};
            for (const [id, entry] of this.states) {
                out[id] = {
                    type: entry.type,
                    state: entry.state,
                    props: entry.props,
                    lastSignal: entry.lastSignal
                };
            }
            return out;
        }

        deserialize(data) {
            for (const [id, entry] of Object.entries(data)) {
                if (this.states.has(id)) {
                    const s = this.states.get(id);
                    s.state = entry.state;
                    s.props = entry.props || {};
                    s.lastSignal = entry.lastSignal;
                }
            }
            this.dirty = true;
        }

        tick(dt) {
            this.updateAnimations();
            this.updateTimers();
        }
    }

    window.StateMachine = StateMachine;
    window.ObjectState = State;
})();
