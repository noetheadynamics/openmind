class WorldEditor {
    constructor(renderer, engine) {
        this.renderer = renderer;
        this.engine = engine;
        this.selectedType = 1;
        this.ghostMesh = null;
        this.ghostFace = null;
        this.raycaster = new THREE.Raycaster();
        this.mouse = new THREE.Vector2();
        this.enabled = true;
        this.listeners = {};
        this.placeCount = 0;
        this.breakCount = 0;
    }

    on(event, cb) {
        if (!this.listeners[event]) this.listeners[event] = [];
        this.listeners[event].push(cb);
    }

    emit(event, data) {
        (this.listeners[event] || []).forEach(cb => cb(data));
    }

    setSelectedType(type) {
        this.selectedType = type;
        if (this.ghostMesh) this.updateGhostMaterial();
    }

    createGhostMesh() {
        if (this.ghostMesh) this.renderer.scene.remove(this.ghostMesh);
        const geo = new THREE.BoxGeometry(1.005, 1.005, 1.005);
        const color = this.renderer.getBlockColor(this.selectedType);
        const isTransp = this.renderer.isBlockTransparent(this.selectedType);
        const mat = new THREE.MeshBasicMaterial({
            color: color,
            transparent: true,
            opacity: isTransp ? 0.25 : 0.4,
            depthWrite: false,
            side: THREE.FrontSide
        });
        this.ghostMesh = new THREE.Mesh(geo, mat);
        this.ghostMesh.visible = false;
        this.ghostMesh.renderOrder = 998;
        this.renderer.scene.add(this.ghostMesh);

        const edgeGeo = new THREE.EdgesGeometry(geo);
        const edgeMat = new THREE.LineBasicMaterial({ color: 0xffffff, transparent: true, opacity: 0.6 });
        this.ghostFace = new THREE.LineSegments(edgeGeo, edgeMat);
        this.ghostMesh.add(this.ghostFace);
    }

    updateGhostMaterial() {
        if (!this.ghostMesh) return;
        const color = this.renderer.getBlockColor(this.selectedType);
        const isTransp = this.renderer.isBlockTransparent(this.selectedType);
        this.ghostMesh.material.color.set(color);
        this.ghostMesh.material.opacity = isTransp ? 0.25 : 0.4;
    }

    getTarget() {
        if (!this.renderer.camera || !this.renderer.raycaster) return null;
        this.renderer.raycaster.setFromCamera(this.renderer.mouse, this.renderer.camera);
        const meshes = [];
        this.renderer.scene.traverse((c) => {
            if (c.isInstancedMesh) meshes.push(c);
            if (c.isGroup) c.children.forEach((ch) => { if (ch.isInstancedMesh) meshes.push(ch); });
        });
        const intersects = this.renderer.raycaster.intersectObjects(meshes, false);
        if (intersects.length === 0) return null;
        const hit = intersects[0];
        const n = hit.face.normal;
        const p = hit.point;
        return {
            x: Math.floor(p.x - n.x * 0.5),
            y: Math.floor(p.y - n.y * 0.5),
            z: Math.floor(p.z - n.z * 0.5),
            nx: Math.round(n.x), ny: Math.round(n.y), nz: Math.round(n.z),
            blockX: Math.floor(p.x + n.x * 0.5),
            blockY: Math.floor(p.y + n.y * 0.5),
            blockZ: Math.floor(p.z + n.z * 0.5)
        };
    }

    updateGhost() {
        if (!this.enabled || !this.ghostMesh) return;
        const target = this.getTarget();
        if (target) {
            const px = target.x + target.nx + 0.5;
            const py = target.y + target.ny + 0.5;
            const pz = target.z + target.nz + 0.5;
            this.ghostMesh.position.set(px, py, pz);
            this.ghostMesh.visible = true;
        } else {
            this.ghostMesh.visible = false;
        }
    }

    placeBlock() {
        if (!this.enabled || !this.engine || !this.engine.wasmReady) return false;
        const target = this.getTarget();
        if (!target) return false;
        const x = target.x + target.nx;
        const y = target.y + target.ny;
        const z = target.z + target.nz;
        if (x < 0 || x > 255 || y < 0 || y > 255 || z < 0 || z > 255) return false;
        if (this.selectedType === 0) return false;

        const existing = this.renderer.getBlockAt(x, y, z);
        if (existing && existing.type !== 0) return false;

        const result = this.engine.setBlock(x, y, z, this.selectedType);
        if (result !== null) {
            this.renderer.blocks.set(`${x},${y},${z}`, { x, y, z, type: this.selectedType });
            this.renderer.dirty = true;
            this.placeCount++;
            this.emit('place', { x, y, z, type: this.selectedType });
            return true;
        }
        return false;
    }

    removeBlock() {
        if (!this.enabled || !this.engine || !this.engine.wasmReady) return false;
        const target = this.getTarget();
        if (!target) return false;
        const key = `${target.blockX},${target.blockY},${target.blockZ}`;
        const block = this.renderer.blocks.get(key);
        if (!block || block.type === 0) return false;

        const result = this.engine.setBlock(target.blockX, target.blockY, target.blockZ, 0);
        if (result !== null) {
            this.renderer.blocks.delete(key);
            this.renderer.interactives.delete(key);
            this.renderer.dirty = true;
            this.breakCount++;
            this.emit('break', { x: target.blockX, y: target.blockY, z: target.blockZ });
            return true;
        }
        return false;
    }

    handleMouseDown(e) {
        if (!this.enabled) return;
        this.renderer._editorMouseDown = { x: e.clientX, y: e.clientY, button: e.button, moved: false };
    }

    handleMouseMove(e) {
        if (!this.enabled) return;
        if (this.renderer._editorMouseDown) {
            const dx = e.clientX - this.renderer._editorMouseDown.x;
            const dy = e.clientY - this.renderer._editorMouseDown.y;
            if (Math.abs(dx) > 5 || Math.abs(dy) > 5) {
                this.renderer._editorMouseDown.moved = true;
            }
        }
    }

    handleMouseUp(e) {
        if (!this.enabled) return;
        const md = this.renderer._editorMouseDown;
        if (!md) return;
        this.renderer._editorMouseDown = null;

        if (md.moved) return;

        if (e.button === 0 && !e.shiftKey) {
            this.placeBlock();
        } else if (e.button === 2 || (e.button === 0 && e.shiftKey)) {
            this.removeBlock();
        }
    }

    destroy() {
        if (this.ghostMesh) {
            this.renderer.scene.remove(this.ghostMesh);
            if (this.ghostMesh.geometry) this.ghostMesh.geometry.dispose();
            if (this.ghostMesh.material) this.ghostMesh.material.dispose();
            this.ghostMesh = null;
        }
    }
}

window.WorldEditor = WorldEditor;
