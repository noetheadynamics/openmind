/**
 * OpenMind – Copy & Paste System
 * Copy/paste with rotation, flip, ghost preview
 */
(function() {
    'use strict';

    class CopyPaste {
        constructor() {
            this.clipboard = null;
            this.ghostMeshes = [];
            this.scene = null;
            this.engine = null;
            this.rotation = 0;
            this.flipX = false;
            this.flipY = false;
            this.flipZ = false;
            this.pasteOffset = { x: 0, y: 0, z: 0 };
            this.listeners = [];
        }

        setScene(s) { this.scene = s; }
        setEngine(e) { this.engine = e; }

        copy(selection) {
            if (!selection || selection.blocks.size === 0) return false;
            const bounds = selection.getBounds();
            if (!bounds) return false;
            const blocks = [];
            for (const key of selection.blocks) {
                const [x, y, z] = key.split(',').map(Number);
                const blockType = this.engine ? this.engine.getBlock(x, y, z)?.blockType || 0 : 1;
                blocks.push({
                    x: x - bounds.minX,
                    y: y - bounds.minY,
                    z: z - bounds.minZ,
                    type: blockType
                });
            }
            this.clipboard = {
                blocks,
                width: bounds.maxX - bounds.minX + 1,
                height: bounds.maxY - bounds.minY + 1,
                depth: bounds.maxZ - bounds.minZ + 1,
                size: blocks.length
            };
            this.rotation = 0;
            this.flipX = false;
            this.flipY = false;
            this.flipZ = false;
            this.emit('copy', { size: this.clipboard.size });
            return true;
        }

        paste(x, y, z, selection) {
            if (!this.clipboard || !this.engine) return false;
            const placed = [];
            for (const block of this.clipboard.blocks) {
                let bx = block.x, by = block.y, bz = block.z;
                for (let r = 0; r < this.rotation; r++) {
                    const temp = bx;
                    bx = -bz;
                    bz = temp;
                    bx += this.clipboard.width - 1;
                }
                if (this.flipX) bx = (this.clipboard.width - 1) - bx;
                if (this.flipY) by = (this.clipboard.height - 1) - by;
                if (this.flipZ) bz = (this.clipboard.depth - 1) - bz;
                const wx = x + bx, wy = y + by, wz = z + bz;
                if (block.type > 0) {
                    this.engine.setBlock(wx, wy, wz, block.type);
                    placed.push({ x: wx, y: wy, z: wz, type: block.type });
                }
            }
            this.clearGhost();
            this.emit('paste', { count: placed.length, x, y, z });
            return true;
        }

        showGhost(x, y, z) {
            this.clearGhost();
            if (!this.clipboard || !this.scene) return;
            for (const block of this.clipboard.blocks) {
                let bx = block.x, by = block.y, bz = block.z;
                for (let r = 0; r < this.rotation; r++) {
                    const temp = bx; bx = -bz; bz = temp;
                    bx += this.clipboard.width - 1;
                }
                if (this.flipX) bx = (this.clipboard.width - 1) - bx;
                if (this.flipY) by = (this.clipboard.height - 1) - by;
                if (this.flipZ) bz = (this.clipboard.depth - 1) - bz;
                if (block.type > 0) {
                    const geo = new THREE.BoxGeometry(0.9, 0.9, 0.9);
                    const mat = new THREE.MeshBasicMaterial({
                        color: 0x8b5cf6, transparent: true, opacity: 0.3, depthWrite: false
                    });
                    const mesh = new THREE.Mesh(geo, mat);
                    mesh.position.set(x + bx + 0.5, y + by + 0.5, z + bz + 0.5);
                    const edgeGeo = new THREE.EdgesGeometry(geo);
                    const wireMat = new THREE.LineBasicMaterial({ color: 0x8b5cf6, transparent: true, opacity: 0.6 });
                    const wire = new THREE.LineSegments(edgeGeo, wireMat);
                    mesh.add(wire);
                    this.scene.add(mesh);
                    this.ghostMeshes.push(mesh);
                }
            }
        }

        clearGhost() {
            for (const m of this.ghostMeshes) {
                if (this.scene) this.scene.remove(m);
                if (m.geometry) m.geometry.dispose();
                if (m.material) m.material.dispose();
            }
            this.ghostMeshes = [];
        }

        rotate(degrees) {
            this.rotation = ((this.rotation || 0) + Math.round(degrees / 90)) % 4;
            this.emit('rotate', { rotation: this.rotation * 90 });
        }

        toggleFlip(axis) {
            if (axis === 'x') this.flipX = !this.flipX;
            else if (axis === 'y') this.flipY = !this.flipY;
            else if (axis === 'z') this.flipZ = !this.flipZ;
            this.emit('flip', { axis, value: axis === 'x' ? this.flipX : axis === 'y' ? this.flipY : this.flipZ });
        }

        hasClipboard() { return !!this.clipboard; }
        getClipboardInfo() { return this.clipboard ? { width: this.clipboard.width, height: this.clipboard.height, depth: this.clipboard.depth, size: this.clipboard.size } : null; }

        on(fn) { this.listeners.push(fn); }
        emit(type, data) { for (const fn of this.listeners) fn({ type, ...data }); }
    }

    window.CopyPaste = CopyPaste;
})();
