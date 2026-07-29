/**
 * OpenMind – Particle System
 * 1000+ particles at 60 FPS with pooling, fire/smoke/rain/snow/sparks/bubbles/leaves/dust
 */
(function() {
    'use strict';

    const ParticleType = Object.freeze({
        FIRE: 'fire', SMOKE: 'smoke', EXPLOSION: 'explosion',
        RAIN: 'rain', SNOW: 'snow', DUST: 'dust',
        SPARKS: 'sparks', BUBBLES: 'bubbles', LEAVES: 'leaves'
    });

    const Presets = {
        fire: {
            count: 8, speed: 1.5, speedVar: 0.5, size: 0.15, sizeVar: 0.08,
            lifetime: 1.2, lifetimeVar: 0.4,
            color: [1.0, 0.5, 0.0], colorEnd: [1.0, 0.1, 0.0],
            gravity: -2.0, wind: 0.3, spread: 0.3, opacity: 0.9, fadeOut: true
        },
        smoke: {
            count: 5, speed: 0.8, speedVar: 0.3, size: 0.25, sizeVar: 0.1,
            lifetime: 2.5, lifetimeVar: 0.8,
            color: [0.4, 0.4, 0.4], colorEnd: [0.2, 0.2, 0.2],
            gravity: -0.5, wind: 0.5, spread: 0.5, opacity: 0.5, fadeOut: true
        },
        explosion: {
            count: 60, speed: 8.0, speedVar: 3.0, size: 0.2, sizeVar: 0.15,
            lifetime: 0.8, lifetimeVar: 0.3,
            color: [1.0, 0.8, 0.0], colorEnd: [0.8, 0.2, 0.0],
            gravity: -4.0, wind: 0, spread: 3.14, opacity: 1.0, fadeOut: true
        },
        rain: {
            count: 200, speed: 12.0, speedVar: 2.0, size: 0.05, sizeVar: 0.02,
            lifetime: 0.8, lifetimeVar: 0.2,
            color: [0.4, 0.5, 0.8], colorEnd: [0.3, 0.4, 0.7],
            gravity: -15.0, wind: 1.0, spread: 0.1, opacity: 0.6, fadeOut: false, line: true, lineHeight: 0.4
        },
        snow: {
            count: 150, speed: 1.5, speedVar: 0.5, size: 0.08, sizeVar: 0.04,
            lifetime: 4.0, lifetimeVar: 1.5,
            color: [0.9, 0.95, 1.0], colorEnd: [0.8, 0.85, 0.95],
            gravity: -1.0, wind: 0.5, spread: 0.3, opacity: 0.8, fadeOut: false
        },
        dust: {
            count: 15, speed: 2.0, speedVar: 1.0, size: 0.1, sizeVar: 0.05,
            lifetime: 0.6, lifetimeVar: 0.2,
            color: [0.6, 0.5, 0.3], colorEnd: [0.4, 0.35, 0.2],
            gravity: -1.0, wind: 0.3, spread: 1.5, opacity: 0.6, fadeOut: true
        },
        sparks: {
            count: 20, speed: 6.0, speedVar: 2.0, size: 0.05, sizeVar: 0.03,
            lifetime: 0.4, lifetimeVar: 0.2,
            color: [1.0, 0.9, 0.3], colorEnd: [1.0, 0.5, 0.0],
            gravity: -8.0, wind: 0, spread: 3.14, opacity: 1.0, fadeOut: true
        },
        bubbles: {
            count: 10, speed: 0.8, speedVar: 0.3, size: 0.06, sizeVar: 0.03,
            lifetime: 3.0, lifetimeVar: 1.0,
            color: [0.5, 0.7, 1.0], colorEnd: [0.3, 0.5, 0.9],
            gravity: 2.0, wind: 0.2, spread: 0.5, opacity: 0.4, fadeOut: true
        },
        leaves: {
            count: 8, speed: 0.5, speedVar: 0.3, size: 0.12, sizeVar: 0.06,
            lifetime: 3.5, lifetimeVar: 1.5,
            color: [0.2, 0.7, 0.2], colorEnd: [0.6, 0.5, 0.1],
            gravity: -0.8, wind: 1.0, spread: 1.0, opacity: 0.85, fadeOut: false, tumble: true
        }
    };

    class Particle {
        constructor() { this.reset(); }
        reset() {
            this.x = 0; this.y = 0; this.z = 0;
            this.vx = 0; this.vy = 0; this.vz = 0;
            this.size = 0.1; this.life = 1; this.maxLife = 1;
            this.r = 1; this.g = 1; this.b = 1;
            this.er = 1; this.eg = 1; this.eb = 1;
            this.gravity = -2; this.wind = 0;
            this.opacity = 1; this.fadeOut = true;
            this.line = false; this.lineHeight = 0.3;
            this.tumble = false; this.angle = 0; this.spin = 0;
            this.active = false;
        }
        get progress() { return 1 - (this.life / this.maxLife); }
        get alive() { return this.active && this.life > 0; }
    }

    class ParticleSystem {
        constructor() {
            this.pool = [];
            this.active = [];
            this.maxParticles = 2000;
            this.enabled = true;
            this.emitters = [];
            this.camera = null;
            this.windTime = 0;
            this.windStrength = 0.5;
            this.weatherWind = 0;
            for (let i = 0; i < this.maxParticles; i++) this.pool.push(new Particle());
        }

        getParticle() {
            return this.pool.length > 0 ? this.pool.pop() : null;
        }

        returnParticle(p) {
            p.reset();
            this.pool.push(p);
        }

        emit(type, x, y, z, overrides) {
            if (!this.enabled) return;
            const preset = Presets[type] || Presets.dust;
            const count = overrides?.count || preset.count;
            for (let i = 0; i < count; i++) {
                const p = this.getParticle();
                if (!p) break;
                p.x = x + (Math.random() - 0.5) * (overrides?.spread || preset.spread) * 2;
                p.y = y + (Math.random() - 0.5) * (overrides?.spread || preset.spread);
                p.z = z + (Math.random() - 0.5) * (overrides?.spread || preset.spread) * 2;
                const speed = (overrides?.speed || preset.speed) + (Math.random() - 0.5) * (overrides?.speedVar || preset.speedVar);
                const theta = Math.random() * Math.PI * 2;
                const phi = Math.random() * Math.PI * (overrides?.spread || preset.spread) / Math.PI;
                p.vx = Math.sin(phi) * Math.cos(theta) * speed;
                p.vy = Math.cos(phi) * speed;
                p.vz = Math.sin(phi) * Math.sin(theta) * speed;
                p.size = (overrides?.size || preset.size) + (Math.random() - 0.5) * (overrides?.sizeVar || preset.sizeVar);
                p.maxLife = (overrides?.lifetime || preset.lifetime) + (Math.random() - 0.5) * (overrides?.lifetimeVar || preset.lifetimeVar);
                p.life = p.maxLife;
                p.r = preset.color[0]; p.g = preset.color[1]; p.b = preset.color[2];
                p.er = preset.colorEnd[0]; p.eg = preset.colorEnd[1]; p.eb = preset.colorEnd[2];
                p.gravity = overrides?.gravity ?? preset.gravity;
                p.wind = overrides?.wind ?? preset.wind;
                p.opacity = overrides?.opacity ?? preset.opacity;
                p.fadeOut = overrides?.fadeOut ?? preset.fadeOut;
                p.line = overrides?.line || preset.line || false;
                p.lineHeight = overrides?.lineHeight || preset.lineHeight || 0.3;
                p.tumble = overrides?.tumble || preset.tumble || false;
                p.angle = Math.random() * Math.PI * 2;
                p.spin = (Math.random() - 0.5) * 4;
                p.active = true;
                this.active.push(p);
            }
        }

        addEmitter(config) {
            const emitter = {
                type: config.type || 'fire',
                x: config.x || 0, y: config.y || 0, z: config.z || 0,
                rate: config.rate || 10,
                interval: 1000 / (config.rate || 10),
                lastEmit: 0,
                active: true,
                id: config.id || 'emitter_' + Math.random().toString(36).substr(2, 6),
                overrides: config.overrides || {}
            };
            this.emitters.push(emitter);
            return emitter.id;
        }

        removeEmitter(id) {
            this.emitters = this.emitters.filter(e => e.id !== id);
        }

        setEmitterPosition(id, x, y, z) {
            const e = this.emitters.find(em => em.id === id);
            if (e) { e.x = x; e.y = y; e.z = z; }
        }

        emitOnce(type, x, y, z, overrides) {
            this.emit(type, x, y, z, overrides);
        }

        emitExplosion(x, y, z, intensity) {
            intensity = intensity || 1;
            this.emit('explosion', x, y, z, { count: Math.floor(60 * intensity), spread: 3.14 });
            this.emit('sparks', x, y, z, { count: Math.floor(20 * intensity) });
            this.emit('smoke', x, y + 0.5, z, { count: Math.floor(15 * intensity) });
        }

        emitRain(areaMinX, areaMaxX, areaMinZ, areaMaxZ, y) {
            this.emit('rain', (areaMinX + areaMaxX) / 2, y || 30, (areaMinZ + areaMaxZ) / 2, {
                count: 200,
                spread: Math.max(areaMaxX - areaMinX, areaMaxZ - areaMinZ) / 2
            });
        }

        emitSnow(areaMinX, areaMaxX, areaMinZ, areaMaxZ, y) {
            this.emit('snow', (areaMinX + areaMaxX) / 2, y || 25, (areaMinZ + areaMaxZ) / 2, {
                count: 150,
                spread: Math.max(areaMaxX - areaMinX, areaMaxZ - areaMinZ) / 2
            });
        }

        update(dt, camera) {
            if (!this.enabled) return;
            this.camera = camera;
            this.windTime += dt;

            for (const em of this.emitters) {
                if (!em.active) continue;
                const now = performance.now();
                if (now - em.lastEmit >= em.interval) {
                    em.lastEmit = now;
                    this.emit(em.type, em.x, em.y, em.z, em.overrides);
                }
            }

            const wind = this.windStrength * Math.sin(this.windTime * 0.5) + this.weatherWind;
            for (let i = this.active.length - 1; i >= 0; i--) {
                const p = this.active[i];
                p.life -= dt;
                if (p.life <= 0) {
                    p.active = false;
                    this.active[i] = this.active[this.active.length - 1];
                    this.active.pop();
                    this.returnParticle(p);
                    continue;
                }
                p.vy += p.gravity * dt;
                p.vx += wind * dt;
                p.x += p.vx * dt;
                p.y += p.vy * dt;
                p.z += p.vz * dt;
                if (p.tumble) {
                    p.angle += p.spin * dt;
                }
            }
        }

        getMeshData() {
            const positions = [];
            const colors = [];
            const sizes = [];
            const normals = [];
            const uvs = [];
            const indices = [];

            for (const p of this.active) {
                const prog = p.progress;
                const t = p.life / p.maxLife;
                const cr = p.r + (p.er - p.r) * prog;
                const cg = p.g + (p.eg - p.g) * prog;
                const cb = p.b + (p.eb - p.b) * prog;
                const op = p.fadeOut ? t : p.opacity;
                const sz = p.size * (1 + prog * 0.5);

                if (p.line) {
                    const baseIdx = positions.length / 3;
                    const len = p.lineHeight;
                    positions.push(p.x, p.y, p.z);
                    positions.push(p.x, p.y + len, p.z);
                    colors.push(cr, cg, cb, op);
                    colors.push(cr, cg, cb, op * 0.3);
                    normals.push(0, 0, 1, 0, 0, 1);
                    normals.push(0, 0, 1, 0, 0, 1);
                    uvs.push(0, 0, 1, 0, 0, 1, 1, 1);
                    indices.push(baseIdx, baseIdx + 1);
                } else {
                    const baseIdx = positions.length / 3;
                    const cos = p.tumble ? Math.cos(p.angle) : 1;
                    const sin = p.tumble ? Math.sin(p.angle) : 0;
                    const hw = sz * 0.5;
                    const corners = [
                        [-hw, -hw], [hw, -hw], [hw, hw], [-hw, hw]
                    ];
                    for (const [cx, cy] of corners) {
                        const rx = cx * cos - cy * sin;
                        const ry = cx * sin + cy * cos;
                        positions.push(p.x + rx, p.y + ry, p.z);
                        colors.push(cr, cg, cb, op);
                        normals.push(0, 0, 1);
                    }
                    uvs.push(0, 0, 1, 0, 1, 1, 0, 1);
                    indices.push(baseIdx, baseIdx + 1, baseIdx + 2, baseIdx, baseIdx + 2, baseIdx + 3);
                }
            }

            return { positions, colors, sizes, normals, uvs, indices };
        }

        get count() { return this.active.length; }

        clear() {
            while (this.active.length > 0) {
                const p = this.active.pop();
                this.returnParticle(p);
            }
        }

        setWind(strength) { this.windStrength = strength; }
        setWeatherWind(w) { this.weatherWind = w; }
    }

    window.ParticleType = ParticleType;
    window.ParticlePresets = Presets;
    window.ParticleSystem = ParticleSystem;
})();
