/**
 * OpenMind – Interactive Objects System
 * All 15 object types with state, logic, visual feedback
 */
(function() {
    'use strict';

    const ObjectType = Object.freeze({
        DOOR: 'DOOR', BUTTON: 'BUTTON', LEVER: 'LEVER', SWITCH: 'SWITCH',
        CHEST: 'CHEST', LAMP: 'LAMP', PISTON: 'PISTON', CONVEYOR: 'CONVEYOR',
        TRAPDOOR: 'TRAPDOOR', FIRE: 'FIRE', LOCK: 'LOCK', LAUNCHER: 'LAUNCHER',
        SENSOR: 'SENSOR', TIMER: 'TIMER', COMPUTER: 'COMPUTER'
    });

    const VisualDefs = {
        DOOR: {
            color: 0x8B4513, openColor: 0x654321, lockedColor: 0xCC0000,
            shape: 'door', height: 2, openAngle: Math.PI / 2
        },
        BUTTON: {
            color: 0x5555FF, pressedColor: 0x8888FF,
            shape: 'button', height: 0.3, pressedHeight: 0.15
        },
        LEVER: {
            color: 0x888888, downColor: 0xAAAAAA,
            shape: 'lever', height: 1.0, downAngle: -Math.PI / 3
        },
        SWITCH: {
            color: 0x333333, onColor: 0x00FF00,
            shape: 'switch', height: 0.6, onAngle: Math.PI / 4
        },
        CHEST: {
            color: 0xA0522D, openColor: 0xCD853F, lockedColor: 0xCC0000,
            shape: 'chest', width: 1.5, height: 0.8, openAngle: Math.PI / 3
        },
        LAMP: {
            color: 0x444444, onColor: 0xFFFF00, emissive: 0xFFFF88,
            shape: 'lamp', height: 1.5, lightIntensity: 2.0, lightRadius: 8
        },
        PISTON: {
            color: 0x666666, extendedColor: 0x888888,
            shape: 'piston', height: 1.0, extendedHeight: 2.0
        },
        CONVEYOR: {
            color: 0x555555, onColor: 0x555555,
            shape: 'conveyor', height: 0.3, speed: 2.0
        },
        TRAPDOOR: {
            color: 0x8B4513, openColor: 0x654321,
            shape: 'trapdoor', height: 0.15, openAngle: Math.PI / 2
        },
        FIRE: {
            color: 0xFF4400, burningColor: 0xFF6600, extinguishedColor: 0x222222,
            shape: 'fire', height: 0.8, lightIntensity: 1.5, lightRadius: 6
        },
        LOCK: {
            color: 0x888888, lockedColor: 0xCC0000, unlockedColor: 0x00CC00,
            shape: 'lock', height: 0.5
        },
        LAUNCHER: {
            color: 0x4444FF, chargingColor: 0xFFFF00, launchedColor: 0xFF4400,
            shape: 'launcher', height: 1.5
        },
        SENSOR: {
            color: 0x00FF00, triggeredColor: 0xFF0000,
            shape: 'sensor', height: 0.5, detectionRadius: 5
        },
        TIMER: {
            color: 0x444444, runningColor: 0x0088FF,
            shape: 'timer', height: 0.6
        },
        COMPUTER: {
            color: 0x333333, onColor: 0x00AAFF, screenColor: 0x001122,
            shape: 'computer', height: 1.2
        }
    };

    class InteractiveObject {
        constructor(id, type, x, y, z, props) {
            this.id = id;
            this.type = type;
            this.x = x;
            this.y = y;
            this.z = z;
            this.props = props || {};
            this.state = null;
            this.mesh = null;
            this.emitter = null;
            this.animationAngle = 0;
            this.animationHeight = 0;
            this.pulseTime = 0;
            this.particles = [];
            this.label = null;
        }

        getColor() {
            const v = VisualDefs[this.type];
            if (!v) return 0xFFFFFF;
            if (this.props.locked) return v.lockedColor || v.color;
            switch (this.state) {
                case ObjectState.DOOR_OPEN:
                case ObjectState.CHEST_OPEN:
                case ObjectState.TRAPDOOR_OPEN:
                    return v.openColor || v.color;
                case ObjectState.LAMP_ON:
                    return v.onColor || v.color;
                case ObjectState.SWITCH_ON:
                    return v.onColor || v.color;
                case ObjectState.LEVER_DOWN:
                    return v.downColor || v.color;
                case ObjectState.BUTTON_PRESSED:
                    return v.pressedColor || v.color;
                case ObjectState.PISTON_EXTENDED:
                    return v.extendedColor || v.color;
                case ObjectState.CONVEYOR_ON:
                    return v.onColor || v.color;
                case ObjectState.FIRE_BURNING:
                    return v.burningColor || v.color;
                case ObjectState.LOCK_UNLOCKED:
                    return v.unlockedColor || v.color;
                case ObjectState.LOCK_LOCKED:
                    return v.lockedColor || v.color;
                case ObjectState.LAUNCHER_CHARGING:
                    return v.chargingColor || v.color;
                case ObjectState.LAUNCHER_LAUNCHED:
                    return v.launchedColor || v.color;
                case ObjectState.SENSOR_TRIGGERED:
                    return v.triggeredColor || v.color;
                case ObjectState.TIMER_RUNNING:
                    return v.runningColor || v.color;
                case ObjectState.COMPUTER_ON:
                    return v.onColor || v.color;
                default:
                    return v.color;
            }
        }

        getScale() {
            const anim = window._interactiveObjects?.sm?.getAnimation(this.id);
            if (!anim) return { x: 1, y: 1, z: 1 };
            const p = anim.progress;
            if (this.type === 'BUTTON' && this.state === ObjectState.BUTTON_PRESSED) {
                const h = 0.5 * (1 - p);
                return { x: 1, y: Math.max(h, 0.2), z: 1 };
            }
            if (this.type === 'PISTON') {
                if (this.state === ObjectState.PISTON_EXTENDING) {
                    return { x: 1, y: 1 + p, z: 1 };
                }
                if (this.state === ObjectState.PISTON_RETRACTING) {
                    return { x: 1, y: 2 - p, z: 1 };
                }
            }
            return { x: 1, y: 1, z: 1 };
        }

        getEmissive() {
            if (this.type === 'LAMP' && this.state === ObjectState.LAMP_ON) {
                return VisualDefs.LAMP.emissive;
            }
            if (this.type === 'FIRE' && this.state === ObjectState.FIRE_BURNING) {
                return 0xFF4400;
            }
            if (this.type === 'COMPUTER' && this.state === ObjectState.COMPUTER_ON) {
                return 0x003366;
            }
            return 0x000000;
        }

        getLabel() {
            const v = VisualDefs[this.type];
            if (!v) return '';
            let label = this.type;
            if (this.props.label) label = this.props.label;
            if (this.props.locked) label += ' [LOCKED]';
            return label;
        }

        getStatusText() {
            return this.state || 'UNKNOWN';
        }
    }

    class InteractiveObjectSystem {
        constructor() {
            this.objects = new Map();
            this.sm = new StateMachine();
            this.signals = [];
            this.listeners = [];
            this.inventory = null;
            this.crafting = null;
            this.engine = null;
            this.renderer = null;
            this.autoId = 0;
            window._interactiveObjects = this;
        }

        setEngine(e) { this.engine = e; }
        setRenderer(r) { this.renderer = r; }
        setInventory(inv) { this.inventory = inv; }
        setCrafting(cr) { this.crafting = cr; }

        nextId() { return 'obj_' + (++this.autoId); }

        create(type, x, y, z, props) {
            const id = this.nextId();
            const obj = new InteractiveObject(id, type, x, y, z, props);
            const initState = this.getInitialState(type);
            this.sm.register(id, type, initState, props);
            obj.state = initState;
            this.objects.set(id, obj);
            this.sm.on(id, 'signal', (data) => this.onSignal(id, data));
            this.sm.on(id, 'transition', (data) => this.onTransition(id, data));
            return obj;
        }

        getInitialState(type) {
            const map = {
                DOOR: 'DOOR_CLOSED', BUTTON: 'BUTTON_IDLE', LEVER: 'LEVER_UP',
                SWITCH: 'SWITCH_OFF', CHEST: 'CHEST_CLOSED', LAMP: 'LAMP_OFF',
                PISTON: 'PISTON_RETRACTED', CONVEYOR: 'CONVEYOR_OFF',
                TRAPDOOR: 'TRAPDOOR_CLOSED', FIRE: 'FIRE_BURNING',
                LOCK: 'LOCK_LOCKED', LAUNCHER: 'LAUNCHER_IDLE',
                SENSOR: 'SENSOR_IDLE', TIMER: 'TIMER_STOPPED', COMPUTER: 'COMPUTER_OFF'
            };
            return map[type] || 'IDLE';
        }

        remove(id) {
            this.objects.delete(id);
            this.sm.unregister(id);
        }

        get(id) { return this.objects.get(id); }

        getByType(type) {
            return Array.from(this.objects.values()).filter(o => o.type === type);
        }

        getAll() { return Array.from(this.objects.values()); }

        interact(id) {
            const obj = this.objects.get(id);
            if (!obj) return null;

            if (obj.props.locked) {
                if (this.inventory && this.inventory.hasItem('key_' + obj.type)) {
                    this.inventory.removeItem('key_' + obj.type);
                    obj.props.locked = false;
                    this.sm.transition(id, 'unlock');
                    this.emit('unlock', obj);
                    return { success: true, message: 'Unlocked!' };
                }
                this.sm.emit(id, 'locked', { reason: 'requires_key' });
                return { success: false, message: 'Locked. Find a key.' };
            }

            switch (obj.type) {
                case 'DOOR':
                    return this.sm.transition(id, 'on') || this.sm.transition(id, 'off');
                case 'BUTTON':
                    return this.sm.transition(id, 'press');
                case 'LEVER':
                    return this.sm.transition(id, 'toggle');
                case 'SWITCH':
                    return this.sm.transition(id, 'toggle');
                case 'CHEST':
                    if (obj.state === 'CHEST_CLOSED') {
                        this.sm.transition(id, 'open');
                        this.emit('chest_open', obj);
                        return { success: true, message: 'Chest opened' };
                    }
                    if (obj.state === 'CHEST_OPEN') {
                        this.sm.transition(id, 'close');
                        return { success: true, message: 'Chest closed' };
                    }
                    return null;
                case 'LAMP':
                    return this.sm.transition(id, 'toggle');
                case 'TRAPDOOR':
                    return this.sm.transition(id, 'open') || this.sm.transition(id, 'close');
                case 'FIRE':
                    return this.sm.transition(id, 'extinguish') || this.sm.transition(id, 'light');
                case 'LOCK':
                    if (this.inventory && this.inventory.hasItem('key')) {
                        this.inventory.removeItem('key');
                        obj.props.locked = false;
                        return this.sm.transition(id, 'unlock');
                    }
                    return this.sm.transition(id, 'lock');
                case 'LAUNCHER':
                    return this.sm.transition(id, 'activate');
                case 'SENSOR':
                    return this.sm.transition(id, 'detect');
                case 'TIMER':
                    if (obj.state === 'TIMER_STOPPED') {
                        this.sm.transition(id, 'start');
                        const duration = (obj.props.duration || 5) * 1000;
                        this.sm.addTimer(id, duration, () => {
                            this.sm.transition(id, 'complete');
                        });
                        return { success: true, message: 'Timer started' };
                    }
                    return this.sm.transition(id, 'stop');
                case 'COMPUTER':
                    return this.sm.transition(id, 'power');
                default:
                    return null;
            }
        }

        onSignal(id, data) {
            const obj = this.objects.get(id);
            if (!obj) return;
            this.signals.push({ id, ...data, time: Date.now() });

            for (const listener of this.listeners) {
                listener({ type: 'signal', id, obj, ...data });
            }

            this.propagateSignal(id, data.signal);
        }

        onTransition(id, data) {
            const obj = this.objects.get(id);
            if (!obj) return;
            obj.state = data.to;
            for (const listener of this.listeners) {
                listener({ type: 'transition', id, obj, ...data });
            }
        }

        propagateSignal(sourceId, signal) {
            const src = this.objects.get(sourceId);
            if (!src) return;

            for (const [id, obj] of this.objects) {
                if (id === sourceId) continue;
                const dist = Math.sqrt(
                    Math.pow(src.x - obj.x, 2) +
                    Math.pow(src.y - obj.y, 2) +
                    Math.pow(src.z - obj.z, 2)
                );

                if (obj.props.linkedTo && obj.props.linkedTo.includes(sourceId)) {
                    if (signal === 'on' || signal === true) {
                        this.sm.transition(id, 'on') || this.sm.transition(id, 'extend') || this.sm.transition(id, 'open');
                    } else if (signal === 'off' || signal === false) {
                        this.sm.transition(id, 'off') || this.sm.transition(id, 'retract') || this.sm.transition(id, 'close');
                    }
                }

                if (obj.type === 'LAMP' && dist < 10) {
                    if (signal === 'on') this.sm.transition(id, 'toggle');
                }
                if (obj.type === 'DOOR' && dist < 10) {
                    if (signal === 'on') this.sm.transition(id, 'on');
                    if (signal === 'off') this.sm.transition(id, 'off');
                }
                if (obj.type === 'PISTON' && dist < 8) {
                    if (signal === 'on') this.sm.transition(id, 'extend');
                    if (signal === 'off') this.sm.transition(id, 'retract');
                }
            }
        }

        link(sourceId, targetId) {
            const src = this.objects.get(sourceId);
            if (!src) return false;
            if (!src.props.linkedTo) src.props.linkedTo = [];
            src.props.linkedTo.push(targetId);
            return true;
        }

        tick(dt) {
            this.sm.tick(dt);

            for (const [id, obj] of this.objects) {
                if (obj.type === 'CONVEYOR' && obj.state === 'CONVEYOR_ON') {
                    obj.pulseTime += dt;
                }
                if (obj.type === 'LAMP' && obj.state === 'LAMP_ON') {
                    obj.pulseTime += dt;
                }
                if (obj.type === 'FIRE' && obj.state === 'FIRE_BURNING') {
                    obj.pulseTime += dt;
                    if (Math.random() < 0.3) {
                        obj.particles.push({
                            x: obj.x + (Math.random() - 0.5) * 0.5,
                            y: obj.y + 0.5,
                            z: obj.z + (Math.random() - 0.5) * 0.5,
                            life: 1.0
                        });
                    }
                    obj.particles = obj.particles.filter(p => {
                        p.y += dt * 2;
                        p.life -= dt * 1.5;
                        return p.life > 0;
                    });
                }
                if (obj.type === 'TIMER' && obj.state === 'TIMER_RUNNING') {
                    const elapsed = (Date.now() - this.sm.get(obj.id).lastTick) / 1000;
                    obj.props.remaining = Math.max(0, (obj.props.duration || 5) - elapsed);
                }
            }
        }

        on(type, fn) { this.listeners.push(fn); }
        off(type, fn) { this.listeners = this.listeners.filter(f => f !== fn); }
        emit(type, data) {
            for (const fn of this.listeners) fn({ type, ...data });
        }

        serialize() {
            const objs = {};
            for (const [id, obj] of this.objects) {
                objs[id] = {
                    type: obj.type, x: obj.x, y: obj.y, z: obj.z,
                    props: obj.props, state: obj.state
                };
            }
            return { objects: objs, sm: this.sm.serialize() };
        }

        deserialize(data) {
            for (const [id, d] of Object.entries(data.objects)) {
                const obj = new InteractiveObject(id, d.type, d.x, d.y, d.z, d.props);
                obj.state = d.state;
                this.objects.set(id, obj);
                this.sm.register(id, d.type, d.state, d.props);
            }
            if (data.sm) this.sm.deserialize(data.sm);
        }
    }

    window.ObjectType = ObjectType;
    window.VisualDefs = VisualDefs;
    window.InteractiveObject = InteractiveObject;
    window.InteractiveObjectSystem = InteractiveObjectSystem;
})();
