/**
 * OpenMind – Selection System
 * Box/Brush/Paint modes, visual highlight, dimension display
 */
(function() {
    'use strict';

    const SelMode = Object.freeze({ BOX: 'box', BRUSH: 'brush', PAINT: 'paint' });

    class Selection {
        constructor() {
            this.mode = SelMode.BOX;
            this.corner1 = null;
            this.corner2 = null;
            this.blocks = new Set();
            this.brushSize = 3;
            this.brushShape = 'square';
            this.paintBlocks = new Set();
            this.active = false;
            this.mesh = null;
            this.scene = null;
            this.previewMesh = null;
            this.listeners = [];
        }

        setScene(scene) { this.scene = scene; }

        setMode(mode) {
            this.mode = mode;
            this.clear();
            this.emit('mode', { mode });
        }

        startBox(x, y, z) {
            this.corner1 = { x, y, z };
            this.corner2 = null;
            this.blocks.clear();
            this.active = true;
            this.emit('start', { mode: 'box', x, y, z });
        }

        updateBox(x, y, z) {
            if (!this.corner1 || !this.active) return;
            this.corner2 = { x, y, z };
            this.blocks.clear();
            const minX = Math.min(this.corner1.x, x);
            const maxX = Math.max(this.corner1.x, x);
            const minY = Math.min(this.corner1.y, y);
            const maxY = Math.max(this.corner1.y, y);
            const minZ = Math.min(this.corner1.z, z);
            const maxZ = Math.max(this.corner1.z, z);
            for (let bx = minX; bx <= maxX; bx++)
                for (let by = minY; by <= maxY; by++)
                    for (let bz = minZ; bz <= maxZ; bz++)
                        this.blocks.add(`${bx},${by},${bz}`);
            this.updateMesh();
        }

        endBox() {
            this.active = false;
            this.emit('select', { mode: 'box', blocks: this.blocks, dimensions: this.getDimensions() });
        }

        brushAt(x, y, z, engine) {
            this.blocks.clear();
            const r = Math.floor(this.brushSize / 2);
            for (let dx = -r; dx <= r; dx++)
                for (let dy = -r; dy <= r; dy++)
                    for (let dz = -r; dz <= r; dz++) {
                        if (this.brushShape === 'circle' && dx*dx+dy*dy+dz*dz > r*r) continue;
                        this.blocks.add(`${x+dx},${y+dy},${z+dz}`);
                    }
            this.corner1 = { x: x-r, y: y-r, z: z-r };
            this.corner2 = { x: x+r, y: y+r, z: z+r };
            this.updateMesh();
            this.emit('select', { mode: 'brush', blocks: this.blocks, dimensions: this.getDimensions() });
        }

        paintAt(x, y, z) {
            this.paintBlocks.add(`${x},${y},${z}`);
            this.blocks.add(`${x},${y},${z}`);
            this.emit('paint', { x, y, z, total: this.paintBlocks.size });
        }

        selectAll(engine) {
            this.blocks.clear();
            if (engine && engine.getBlockData) {
                const data = engine.getBlockData(0, 0, 0, 64, 32, 64);
                if (data) {
                    for (let i = 0; i < data.length; i += 4) {
                        if (data[i + 3] > 0) {
                            this.blocks.add(`${data[i]},${data[i+1]},${data[i+2]}`);
                        }
                    }
                }
            }
            this.emit('select', { mode: 'all', blocks: this.blocks, dimensions: this.getDimensions() });
        }

        getDimensions() {
            if (this.blocks.size === 0) return { x: 0, y: 0, z: 0, count: 0 };
            let minX = Infinity, maxX = -Infinity, minY = Infinity, maxY = -Infinity, minZ = Infinity, maxZ = -Infinity;
            for (const key of this.blocks) {
                const [x, y, z] = key.split(',').map(Number);
                minX = Math.min(minX, x); maxX = Math.max(maxX, x);
                minY = Math.min(minY, y); maxY = Math.max(maxY, y);
                minZ = Math.min(minZ, z); maxZ = Math.max(maxZ, z);
            }
            return { x: maxX - minX + 1, y: maxY - minY + 1, z: maxZ - minZ + 1, count: this.blocks.size };
        }

        getBounds() {
            if (this.blocks.size === 0) return null;
            let minX = Infinity, maxX = -Infinity, minY = Infinity, maxY = -Infinity, minZ = Infinity, maxZ = -Infinity;
            for (const key of this.blocks) {
                const [x, y, z] = key.split(',').map(Number);
                minX = Math.min(minX, x); maxX = Math.max(maxX, x);
                minY = Math.min(minY, y); maxY = Math.max(maxY, y);
                minZ = Math.min(minZ, z); maxZ = Math.max(maxZ, z);
            }
            return { minX, minY, minZ, maxX, maxY, maxZ };
        }

        updateMesh() {
            if (!this.scene) return;
            if (this.mesh) {
                this.scene.remove(this.mesh);
                this.mesh.traverse((child) => {
                    if (child.geometry) child.geometry.dispose();
                    if (child.material) child.material.dispose();
                });
            }
            const bounds = this.getBounds();
            if (!bounds) return;
            const w = bounds.maxX - bounds.minX + 1;
            const h = bounds.maxY - bounds.minY + 1;
            const d = bounds.maxZ - bounds.minZ + 1;
            const cx = (bounds.minX + bounds.maxX) / 2 + 0.5;
            const cy = (bounds.minY + bounds.maxY) / 2 + 0.5;
            const cz = (bounds.minZ + bounds.maxZ) / 2 + 0.5;
            const geo = new THREE.BoxGeometry(w + 0.05, h + 0.05, d + 0.05);
            const mat = new THREE.MeshBasicMaterial({
                color: 0x8b5cf6, transparent: true, opacity: 0.15,
                depthWrite: false, side: THREE.DoubleSide
            });
            this.mesh = new THREE.Mesh(geo, mat);
            this.mesh.position.set(cx, cy, cz);
            const edges = new THREE.EdgesGeometry(geo);
            const lineMat = new THREE.LineBasicMaterial({ color: 0x8b5cf6, transparent: true, opacity: 0.8 });
            const wireframe = new THREE.LineSegments(edges, lineMat);
            this.mesh.add(wireframe);
            this.scene.add(this.mesh);
        }

        clear() {
            this.corner1 = null;
            this.corner2 = null;
            this.blocks.clear();
            this.paintBlocks.clear();
            this.active = false;
            if (this.mesh && this.scene) {
                this.scene.remove(this.mesh);
                this.mesh.traverse((child) => {
                    if (child.geometry) child.geometry.dispose();
                    if (child.material) child.material.dispose();
                });
                this.mesh = null;
            }
            this.emit('clear');
        }

        getBlockList() {
            return Array.from(this.blocks).map(k => {
                const [x, y, z] = k.split(',').map(Number);
                return { x, y, z };
            });
        }

        on(fn) { this.listeners.push(fn); }
        emit(type, data) { for (const fn of this.listeners) fn({ type, ...data }); }

        destroy() { this.clear(); this.listeners = []; }
    }

    window.SelMode = SelMode;
    window.Selection = Selection;
})();
