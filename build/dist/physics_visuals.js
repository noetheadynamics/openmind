/**
 * OpenMind – Physics Visual Feedback
 * Cracks, dust, sparks, smoke trails, glow, frost, wetness
 */
(function() {
    'use strict';

    const VisualEffect = Object.freeze({
        CRACK: 'crack', DUST: 'dust', SPARKS: 'sparks',
        SMOKE_TRAIL: 'smoke_trail', GLOW: 'glow',
        FROST: 'frost', WETNESS: 'wetness'
    });

    class BlockEffect {
        constructor(x, y, z, type) {
            this.x = x; this.y = y; this.z = z;
            this.type = type;
            this.intensity = 1.0;
            this.life = 3.0;
            this.maxLife = 3.0;
            this.mesh = null;
            this.active = true;
        }

        get progress() { return 1 - this.life / this.maxLife; }
        get alive() { return this.active && this.life > 0; }
    }

    class PhysicsVisuals {
        constructor(scene) {
            this.scene = scene;
            this.effects = new Map();
            this.enabled = true;
            this.maxEffects = 200;
            this.effectCount = 0;
        }

        addCrack(x, y, z, stressLevel) {
            const key = `crack_${x}_${y}_${z}`;
            if (this.effects.has(key)) return;
            if (this.effectCount >= this.maxEffects) return;

            const e = new BlockEffect(x, y, z, 'crack');
            e.intensity = Math.min(stressLevel / 10, 1);
            e.life = 5.0;
            e.maxLife = 5.0;

            const geo = new THREE.PlaneGeometry(0.9, 0.9);
            const mat = new THREE.MeshBasicMaterial({
                color: 0x333333, transparent: true, opacity: 0.7,
                depthWrite: false, side: THREE.DoubleSide
            });
            e.mesh = new THREE.Mesh(geo, mat);
            e.mesh.position.set(x + 0.5, y + 0.5, z + 0.51);
            this.scene.add(e.mesh);

            const canvas = document.createElement('canvas');
            canvas.width = 64; canvas.height = 64;
            const ctx = canvas.getContext('2d');
            ctx.strokeStyle = '#111';
            ctx.lineWidth = 2;
            const cracks = Math.floor(stressLevel / 2) + 1;
            for (let i = 0; i < cracks; i++) {
                ctx.beginPath();
                let cx = 32, cy = 32;
                ctx.moveTo(cx, cy);
                for (let j = 0; j < 5; j++) {
                    cx += (Math.random() - 0.5) * 30;
                    cy += (Math.random() - 0.5) * 30;
                    ctx.lineTo(cx, cy);
                }
                ctx.stroke();
            }
            const tex = new THREE.CanvasTexture(canvas);
            mat.map = tex;

            this.effects.set(key, e);
            this.effectCount++;
        }

        addDust(x, y, z, particleSystem) {
            if (!this.enabled || !particleSystem) return;
            particleSystem.emit('dust', x + 0.5, y + 0.5, z + 0.5, { count: 15 });
        }

        addSparks(x, y, z, particleSystem) {
            if (!this.enabled || !particleSystem) return;
            particleSystem.emit('sparks', x + 0.5, y + 0.5, z + 0.5, { count: 20 });
        }

        addSmokeTrail(x, y, z, particleSystem) {
            if (!this.enabled || !particleSystem) return;
            particleSystem.emit('smoke', x + 0.5, y + 0.5, z + 0.5, { count: 3, spread: 0.2 });
        }

        addGlow(x, y, z, color, intensity) {
            const key = `glow_${x}_${y}_${z}`;
            if (this.effects.has(key)) return;
            if (this.effectCount >= this.maxEffects) return;

            const e = new BlockEffect(x, y, z, 'glow');
            e.intensity = intensity || 1;
            e.life = 999999;

            const geo = new THREE.SphereGeometry(0.6, 8, 8);
            const mat = new THREE.MeshBasicMaterial({
                color: color || 0xffaa00, transparent: true, opacity: 0.3 * (intensity || 1),
                depthWrite: false
            });
            e.mesh = new THREE.Mesh(geo, mat);
            e.mesh.position.set(x + 0.5, y + 0.5, z + 0.5);
            this.scene.add(e.mesh);

            this.effects.set(key, e);
            this.effectCount++;
        }

        addFrost(x, y, z) {
            const key = `frost_${x}_${y}_${z}`;
            if (this.effects.has(key)) return;
            if (this.effectCount >= this.maxEffects) return;

            const e = new BlockEffect(x, y, z, 'frost');
            e.life = 999999;

            const geo = new THREE.PlaneGeometry(0.95, 0.95);
            const mat = new THREE.MeshBasicMaterial({
                color: 0xaaddff, transparent: true, opacity: 0.4,
                depthWrite: false, side: THREE.DoubleSide
            });
            e.mesh = new THREE.Mesh(geo, mat);
            e.mesh.position.set(x + 0.5, y + 0.5, z + 0.51);
            this.scene.add(e.mesh);

            this.effects.set(key, e);
            this.effectCount++;
        }

        addWetness(x, y, z) {
            const key = `wet_${x}_${y}_${z}`;
            if (this.effects.has(key)) return;
            if (this.effectCount >= this.maxEffects) return;

            const e = new BlockEffect(x, y, z, 'wetness');
            e.life = 999999;

            const geo = new THREE.PlaneGeometry(0.95, 0.95);
            const mat = new THREE.MeshBasicMaterial({
                color: 0x4488cc, transparent: true, opacity: 0.15,
                depthWrite: false, side: THREE.DoubleSide
            });
            e.mesh = new THREE.Mesh(geo, mat);
            e.mesh.position.set(x + 0.5, y + 0.51, z + 0.51);
            this.scene.add(e.mesh);

            this.effects.set(key, e);
            this.effectCount++;
        }

        removeEffect(x, y, z, type) {
            const key = `${type}_${x}_${y}_${z}`;
            const e = this.effects.get(key);
            if (e) {
                if (e.mesh) {
                    if (this.scene) this.scene.remove(e.mesh);
                    if (e.mesh.geometry) e.mesh.geometry.dispose();
                    if (e.mesh.material) {
                        if (e.mesh.material.map) e.mesh.material.map.dispose();
                        e.mesh.material.dispose();
                    }
                }
                this.effects.delete(key);
                this.effectCount--;
            }
        }

        update(dt) {
            if (!this.enabled) return;
            for (const [key, e] of this.effects) {
                if (!e.alive) continue;
                if (e.mesh?.material) {
                    if (e.type === 'glow') {
                        e.mesh.material.opacity = 0.2 + Math.sin(Date.now() * 0.003) * 0.1 * e.intensity;
                    }
                    if (e.type === 'crack') {
                        e.mesh.material.opacity = 0.7 * (1 - e.progress * 0.5);
                    }
                }
                e.life -= dt;
                if (e.life <= 0 && e.type !== 'glow' && e.type !== 'frost' && e.type !== 'wetness') {
                    e.active = false;
                    if (e.mesh) {
                        if (this.scene) this.scene.remove(e.mesh);
                        if (e.mesh.geometry) e.mesh.geometry.dispose();
                        if (e.mesh.material) {
                            if (e.mesh.material.map) e.mesh.material.map.dispose();
                            e.mesh.material.dispose();
                        }
                    }
                    this.effects.delete(key);
                    this.effectCount--;
                }
            }
        }

        clearAll() {
            for (const [key, e] of this.effects) {
                if (e.mesh) {
                    if (this.scene) this.scene.remove(e.mesh);
                    if (e.mesh.geometry) e.mesh.geometry.dispose();
                    if (e.mesh.material) {
                        if (e.mesh.material.map) e.mesh.material.map.dispose();
                        e.mesh.material.dispose();
                    }
                }
            }
            this.effects.clear();
            this.effectCount = 0;
        }

        setEnabled(v) { this.enabled = v; }
    }

    window.VisualEffect = VisualEffect;
    window.PhysicsVisuals = PhysicsVisuals;
})();
