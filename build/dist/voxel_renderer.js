class VoxelRenderer {
    constructor(containerId) {
        this.container = document.getElementById(containerId);
        this.scene = null;
        this.camera = null;
        this.renderer = null;
        this.blocks = new Map();
        this.interactives = new Map();
        this.customColors = new Map();
        this.dirty = true;
        this.engine = null;
        this.running = false;
        this.selectedBlockType = 1;
        this.isLightMode = false;
        this.animatingBlocks = new Map();

        this.blockTypes = [
            { id: 0, name: 'AIR', color: 0x000000, interactive: false },
            { id: 1, name: 'STONE', color: 0x808080 },
            { id: 2, name: 'DIRT', color: 0x8B4513 },
            { id: 3, name: 'GRASS', color: 0x228B22 },
            { id: 4, name: 'WATER', color: 0x1E90FF, transparent: true },
            { id: 5, name: 'SAND', color: 0xF4A460 },
            { id: 6, name: 'GLASS', color: 0x87CEEB, transparent: true },
            { id: 7, name: 'WOOD', color: 0xDEB887 },
            { id: 8, name: 'LEAVES', color: 0x006400 },
            { id: 9, name: 'IRON', color: 0xA9A9A9 },
            { id: 10, name: 'COPPER', color: 0xB87333 },
            { id: 11, name: 'GOLD', color: 0xFFD700 },
            { id: 12, name: 'STEEL', color: 0x708090 },
            { id: 13, name: 'DIAMOND', color: 0x00FFFF },
            { id: 14, name: 'COAL', color: 0x2F4F4F },
            { id: 15, name: 'BEDROCK', color: 0x1C1C1C },
            { id: 17, name: 'TNT', color: 0xFF4500 },
            { id: 18, name: 'SNOW', color: 0xFFFAFA },
            { id: 30, name: 'DOOR', color: 0x8B4513, interactive: true },
            { id: 31, name: 'BUTTON', color: 0xC0C0C0, interactive: true },
            { id: 32, name: 'LAUNCHER', color: 0x444444, interactive: true },
            { id: 33, name: 'LOCK', color: 0xB8860B, interactive: true },
            { id: 34, name: 'LAMP', color: 0xFFFF00, emissive: true },
            { id: 35, name: 'CHEST', color: 0xCD853F, interactive: true },
            { id: 36, name: 'SWITCH', color: 0x228B22, interactive: true },
            { id: 37, name: 'CONVEYOR', color: 0x555555, interactive: true },
            { id: 38, name: 'PISTON', color: 0x888888, interactive: true },
            { id: 39, name: 'TRAPDOOR', color: 0xA0522D, interactive: true },
            { id: 40, name: 'FIRE', color: 0xFF6600, emissive: true }
        ];

        this.raycaster = null;
        this.mouse = new THREE.Vector2();
        this.highlightMesh = null;
        this.cameraTarget = new THREE.Vector3(16, 4, 16);
        this.cameraSpherical = { theta: 0.8, phi: 1.0, radius: 45 };
        this.isDragging = false;
        this.isRightDrag = false;
        this.prevMouse = { x: 0, y: 0 };
        this.keys = {};
        this.hotbarIndex = 1;
        this._launchIntervals = new Map();
        this._boundOnResize = null;
        this._boundOnThemeChange = null;
        this._boundOnKeyDown = null;
        this._boundOnKeyUp = null;

        this.fov = 70;
        this.shadowSize = 60;
        this.shadowMapSize = 2048;
        this.zoomSpeed = 0.05;
        this.rocketOrbitDistance = 30;
        this.rocketAnimInterval = 50;
        this.cameraSpeed = 0.5;
        this.wasmPollInterval = 1000;

        this.init();
    }

    init() {
        if (!this.container || !window.THREE) return;

        this.scene = new THREE.Scene();
        this.updateTheme();

        this.camera = new THREE.PerspectiveCamera(this.fov, window.innerWidth / window.innerHeight, 0.1, 500);

        this.renderer = new THREE.WebGLRenderer({ antialias: true });
        this.renderer.setSize(window.innerWidth, window.innerHeight);
        this.renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
        this.renderer.shadowMap.enabled = true;
        this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;
        this.container.appendChild(this.renderer.domElement);

        this.ambientLight = new THREE.AmbientLight(0x404060, 0.5);
        this.scene.add(this.ambientLight);

        this.sunLight = new THREE.DirectionalLight(0xfff4e0, 1.0);
        this.sunLight.position.set(50, 80, 30);
        this.sunLight.castShadow = true;
        const s = this.shadowSize;
        this.sunLight.shadow.mapSize.set(this.shadowMapSize, this.shadowMapSize);
        this.sunLight.shadow.camera.left = -s;
        this.sunLight.shadow.camera.right = s;
        this.sunLight.shadow.camera.top = s;
        this.sunLight.shadow.camera.bottom = -s;
        this.sunLight.shadow.camera.near = 0.5;
        this.sunLight.shadow.camera.far = 200;
        this.scene.add(this.sunLight);

        this.hemiLight = new THREE.HemisphereLight(0x87CEEB, 0x362907, 0.3);
        this.scene.add(this.hemiLight);

        this.gridHelper = new THREE.GridHelper(64, 64, 0x222244, 0x111133);
        this.gridHelper.position.y = 0.01;
        this.scene.add(this.gridHelper);

        const hlGeo = new THREE.BoxGeometry(1.02, 1.02, 1.02);
        const hlMat = new THREE.MeshBasicMaterial({ color: 0xffffff, wireframe: true, transparent: true, opacity: 0.5 });
        this.highlightMesh = new THREE.Mesh(hlGeo, hlMat);
        this.highlightMesh.visible = false;
        this.scene.add(this.highlightMesh);

        this.raycaster = new THREE.Raycaster();
        this.initControls();
        this.initHotbar();
        this._boundOnResize = () => this.onResize();
        this._boundOnThemeChange = (e) => {
            this.isLightMode = e.detail.light;
            this.updateTheme();
        };
        window.addEventListener('resize', this._boundOnResize);
        window.addEventListener('themechange', this._boundOnThemeChange);
    }

    updateTheme() {
        if (!this.scene) return;
        if (this.isLightMode) {
            this.scene.background = new THREE.Color(0xe8eaf0);
            this.scene.fog = null;
            if (this.ambientLight) this.ambientLight.intensity = 0.7;
            if (this.hemiLight) this.hemiLight.intensity = 0.5;
            if (this.gridHelper && Array.isArray(this.gridHelper.material) && this.gridHelper.material.length >= 2) {
                this.gridHelper.material[0].color.set(0xccccdd);
                this.gridHelper.material[1].color.set(0xbbbbcc);
            }
        } else {
            this.scene.background = new THREE.Color(0x111118);
            this.scene.fog = null;
            if (this.ambientLight) this.ambientLight.intensity = 0.5;
            if (this.hemiLight) this.hemiLight.intensity = 0.3;
            if (this.gridHelper && Array.isArray(this.gridHelper.material) && this.gridHelper.material.length >= 2) {
                this.gridHelper.material[0].color.set(0x333340);
                this.gridHelper.material[1].color.set(0x222230);
            }
        }
    }

    initControls() {
        const el = this.renderer.domElement;

        el.addEventListener('mousedown', (e) => {
            if (e.button === 2) {
                this.isRightDrag = true;
                this.isDragging = true;
                this.prevMouse = { x: e.clientX, y: e.clientY };
                e.preventDefault();
                return;
            }
            if (e.button === 0 && e.shiftKey) {
                this.removeBlock();
                return;
            }
            if (e.button === 0) {
                const target = this.getTargetBlock();
                if (target) {
                    const bt = this.blockTypes.find(b => b.id === this.getBlockAt(target.x, target.y, target.z)?.type);
                    if (bt && bt.interactive) {
                        this.interact(target.x, target.y, target.z, bt);
                    } else {
                        this.placeBlock();
                    }
                } else {
                    this.placeBlock();
                }
            }
        });

        el.addEventListener('mousemove', (e) => {
            this.mouse.x = (e.clientX / window.innerWidth) * 2 - 1;
            this.mouse.y = -(e.clientY / window.innerHeight) * 2 + 1;
            if (this.isDragging && this.isRightDrag) {
                const dx = e.clientX - this.prevMouse.x;
                const dy = e.clientY - this.prevMouse.y;
                this.cameraSpherical.theta -= dx * 0.005;
                this.cameraSpherical.phi = Math.max(0.1, Math.min(Math.PI - 0.1, this.cameraSpherical.phi + dy * 0.005));
                this.prevMouse = { x: e.clientX, y: e.clientY };
                this.updateCamera();
            }
        });

        el.addEventListener('mouseup', () => { this.isDragging = false; this.isRightDrag = false; });
        el.addEventListener('mouseleave', () => { this.isDragging = false; this.isRightDrag = false; this.highlightMesh.visible = false; });
        el.addEventListener('contextmenu', (e) => e.preventDefault());

        el.addEventListener('wheel', (e) => {
            e.preventDefault();
            this.cameraSpherical.radius = Math.max(3, Math.min(150, this.cameraSpherical.radius + e.deltaY * this.zoomSpeed));
            this.updateCamera();
        }, { passive: false });

        this._boundOnKeyDown = (e) => {
            this.keys[e.key] = true;
            if (e.key >= '1' && e.key <= '9') {
                this.hotbarIndex = parseInt(e.key);
                this.updateHotbar();
            }
        };
        this._boundOnKeyUp = (e) => { this.keys[e.key] = false; };
        window.addEventListener('keydown', this._boundOnKeyDown);
        window.addEventListener('keyup', this._boundOnKeyUp);
        this.updateCamera();
    }

    updateCamera() {
        const t = this.cameraTarget;
        const s = this.cameraSpherical;
        this.camera.position.set(
            t.x + s.radius * Math.sin(s.phi) * Math.cos(s.theta),
            t.y + s.radius * Math.cos(s.phi),
            t.z + s.radius * Math.sin(s.phi) * Math.sin(s.theta)
        );
        this.camera.lookAt(t);
    }

    getTargetBlock() {
        this.raycaster.setFromCamera(this.mouse, this.camera);
        const meshes = [];
        this.scene.traverse((c) => {
            if (c.isInstancedMesh) meshes.push(c);
            if (c.isGroup) c.children.forEach((ch) => { if (ch.isInstancedMesh) meshes.push(ch); });
        });
        const intersects = this.raycaster.intersectObjects(meshes, false);
        if (intersects.length === 0) return null;
        const hit = intersects[0];
        const n = hit.face.normal;
        const p = hit.point;
        return {
            x: Math.floor(p.x - n.x * 0.5),
            y: Math.floor(p.y - n.y * 0.5),
            z: Math.floor(p.z - n.z * 0.5),
            nx: Math.round(n.x), ny: Math.round(n.y), nz: Math.round(n.z)
        };
    }

    getBlockAt(x, y, z) {
        return this.blocks.get(`${x},${y},${z}`) || null;
    }

    placeBlock() {
        const target = this.getTargetBlock();
        if (!target || !this.engine || !this.engine.wasmReady) return;
        const x = target.x + target.nx;
        const y = target.y + target.ny;
        const z = target.z + target.nz;
        if (x < 0 || x > 255 || y < 0 || y > 255 || z < 0 || z > 255 || this.selectedBlockType === 0) return;
        this.engine.setBlock(x, y, z, this.selectedBlockType);
        this.blocks.set(`${x},${y},${z}`, { x, y, z, type: this.selectedBlockType });
        this.dirty = true;
    }

    removeBlock() {
        const target = this.getTargetBlock();
        if (!target || !this.engine || !this.engine.wasmReady) return;
        const key = `${target.x},${target.y},${target.z}`;
        if (this.blocks.has(key)) {
            this.engine.setBlock(target.x, target.y, target.z, 0);
            this.blocks.delete(key);
            this.interactives.delete(key);
            this.dirty = true;
        }
    }

    interact(x, y, z, blockType) {
        const key = `${x},${y},${z}`;
        const state = this.interactives.get(key) || { open: false, on: false, launched: false, locked: false };

        switch (blockType.id) {
            case 30: // DOOR
                state.open = !state.open;
                this.animateBlock(x, y + (state.open ? 1 : 0), z, state.open ? 0 : 1);
                this.addChatMessage(blockType.name + (state.open ? ' opened' : ' closed'));
                break;
            case 31: // BUTTON
                state.on = !state.on;
                this.animatePulse(x, y, z);
                this.addChatMessage('Button ' + (state.on ? 'pressed' : 'released'));
                this.triggerNearby(x, y, z, state.on);
                break;
            case 32: // LAUNCHER
                if (!state.launched) {
                    state.launched = true;
                    this.animateLaunch(x, y, z);
                    this.addChatMessage('Rocket launched!');
                } else {
                    this.addChatMessage('Already launched.');
                }
                break;
            case 33: // LOCK
                state.locked = !state.locked;
                this.animatePulse(x, y, z);
                this.addChatMessage('Lock ' + (state.locked ? 'engaged' : 'disengaged'));
                break;
            case 35: // CHEST
                state.open = !state.open;
                this.addChatMessage('Chest ' + (state.open ? 'opened' : 'closed'));
                break;
            case 36: // SWITCH
                state.on = !state.on;
                this.animatePulse(x, y, z);
                this.addChatMessage('Switch ' + (state.on ? 'ON' : 'OFF'));
                break;
            case 37: // CONVEYOR
                state.on = !state.on;
                this.addChatMessage('Conveyor ' + (state.on ? 'running' : 'stopped'));
                break;
            case 38: // PISTON
                state.open = !state.open;
                this.animateBlock(x, y + (state.open ? 1 : 0), z, state.open ? 0 : 1);
                this.addChatMessage('Piston ' + (state.open ? 'extended' : 'retracted'));
                break;
            case 39: // TRAPDOOR
                state.open = !state.open;
                this.animateBlock(x, y, z, state.open ? 0 : 1);
                this.addChatMessage('Trapdoor ' + (state.open ? 'opened' : 'closed'));
                break;
            case 40: // FIRE
                state.on = !state.on;
                if (!state.on) {
                    this.engine.setBlock(x, y, z, 0);
                    this.blocks.delete(key);
                    this.dirty = true;
                }
                this.addChatMessage('Fire ' + (state.on ? 'lit' : 'extinguished'));
                break;
        }
        this.interactives.set(key, state);
    }

    triggerNearby(x, y, z, on) {
        const offsets = [[1,0,0],[-1,0,0],[0,1,0],[0,-1,0],[0,0,1],[0,0,-1]];
        for (const [dx, dy, dz] of offsets) {
            const neighbor = this.getBlockAt(x+dx, y+dy, z+dz);
            if (neighbor) {
                const bt = this.blockTypes.find(b => b.id === neighbor.type);
                if (bt && bt.id === 34) { // LAMP
                    if (on) {
                        const lampMat = new THREE.MeshBasicMaterial({ color: 0xFFFF88 });
                        this.animateMaterialChange(x+dx, y+dy, z+dz, lampMat);
                    } else {
                        this.dirty = true;
                    }
                }
            }
        }
    }

    animateBlock(x, y, z) {
        this.dirty = true;
    }

    animatePulse(x, y, z) {
        const key = `${x},${y},${z}`;
        const block = this.blocks.get(key);
        if (!block) return;
        const origType = block.type;
        block.type = 99;
        this.dirty = true;
        setTimeout(() => { block.type = origType; this.dirty = true; }, 200);
    }

    animateLaunch(x, y, z) {
        const key = `${x},${y},${z}`;
        if (this._launchIntervals.has(key)) clearInterval(this._launchIntervals.get(key));
        let yOffset = 0;
        const interval = setInterval(() => {
            yOffset += 0.5;
            this.blocks.delete(`${x},${y + Math.floor(yOffset) - 1},${z}`);
            this.blocks.set(`${x},${y + Math.floor(yOffset)},${z}`, { x, y: y + Math.floor(yOffset), z, type: 32 });
            this.dirty = true;
            if (yOffset > this.rocketOrbitDistance) {
                clearInterval(interval);
                this._launchIntervals.delete(key);
                this.blocks.delete(`${x},${y + Math.floor(yOffset)},${z}`);
                this.dirty = true;
                this.addChatMessage('Rocket reached orbit!');
            }
        }, this.rocketAnimInterval);
        this._launchIntervals.set(key, interval);
    }

    animateMaterialChange(x, y, z) {
        this.dirty = true;
    }

    addChatMessage(msg) {
        window.dispatchEvent(new CustomEvent('renderer-message', { detail: msg }));
    }

    setCustomBlockColor(typeId, color) {
        this.customColors.set(typeId, parseInt(color.replace('#', ''), 16));
        this.dirty = true;
    }

    getBlockColor(type) {
        if (this.customColors.has(type)) return this.customColors.get(type);
        const bt = this.blockTypes.find(b => b.id === type);
        return bt ? bt.color : 0xFF00FF;
    }

    isBlockTransparent(type) {
        const bt = this.blockTypes.find(b => b.id === type);
        return bt && bt.transparent;
    }

    isBlockEmissive(type) {
        const bt = this.blockTypes.find(b => b.id === type);
        return bt && bt.emissive;
    }

    initHotbar() {
        let bar = document.getElementById('hotbar');
        if (!bar) {
            bar = document.createElement('div');
            bar.id = 'hotbar';
            bar.style.cssText = 'position:fixed;bottom:16px;left:50%;transform:translateX(-50%);z-index:1000;display:flex;gap:3px;padding:6px;background:rgba(10,10,30,0.85);border:1px solid rgba(100,100,200,0.3);border-radius:8px;backdrop-filter:blur(8px);';
            this.container.appendChild(bar);
        }
        this.updateHotbar();
    }

    updateHotbar() {
        const bar = document.getElementById('hotbar');
        if (!bar) return;
        bar.innerHTML = '';
        const types = this.blockTypes.filter(b => b.id !== 0);
        types.forEach((bt, i) => {
            const slot = document.createElement('div');
            const isSel = (i + 1) === this.hotbarIndex;
            const color = this.getBlockColor(bt.id);
            slot.style.cssText = `width:36px;height:36px;border-radius:5px;border:2px solid ${isSel ? '#fff' : 'rgba(100,100,200,0.3)'};background:#${color.toString(16).padStart(6, '0')};cursor:pointer;display:flex;align-items:flex-end;justify-content:center;font-size:8px;color:#fff;text-shadow:0 0 3px #000;padding-bottom:2px;box-sizing:border-box;`;
            slot.textContent = bt.name.substring(0, 4);
            slot.title = bt.name + (bt.interactive ? ' [interactive]' : '') + ' [' + (i + 1) + ']';
            slot.addEventListener('click', () => {
                this.hotbarIndex = Math.min(i + 1, 9);
                this.selectedBlockType = bt.id;
                this.updateHotbar();
            });
            bar.appendChild(slot);
        });
        this.selectedBlockType = types[this.hotbarIndex - 1]?.id || 1;
    }

    setEngine(engine) { this.engine = engine; }

    updateFromWASM() {
        if (!this.engine || !this.engine.wasmReady) return;
        const newBlocks = new Map();
        const stats = this.engine.getWorldStats();
        const blockCount = stats.totalBlocks || 0;
        if (blockCount === 0) {
            if (this.blocks.size > 0) { this.blocks.clear(); this.dirty = true; }
            return;
        }
        const step = Math.max(1, Math.floor(blockCount / 20000));
        let scanned = 0;
        for (let x = 0; x < 256 && scanned < 50000; x += step) {
            for (let y = 0; y < 256 && scanned < 50000; y += step) {
                for (let z = 0; z < 256 && scanned < 50000; z += step) {
                    const data = this.engine.getBlock(x, y, z);
                    if (data && data.exists && data.blockType !== 0) {
                        newBlocks.set(`${x},${y},${z}`, { x, y, z, type: data.blockType });
                    }
                    scanned++;
                }
            }
        }
        let changed = newBlocks.size !== this.blocks.size;
        if (!changed) {
            for (const [k, v] of newBlocks) {
                const old = this.blocks.get(k);
                if (!old || old.type !== v.type) { changed = true; break; }
            }
        }
        if (changed) { this.blocks = newBlocks; this.dirty = true; }
    }

    rebuildMesh() {
        if (!this.dirty || !window.THREE || !this.scene) return;
        this.dirty = false;
        this.scene.traverse((c) => { if (c.isInstancedMesh || c.isGroup) { if (c.geometry && c.geometry !== this.boxGeometry) c.geometry.dispose(); if (c.material) c.material.dispose(); } });
        const toRemove = [];
        this.scene.traverse((c) => { if (c.isGroup || c.isInstancedMesh) toRemove.push(c); });
        toRemove.forEach(c => this.scene.remove(c));
        if (this.blocks.size === 0) return;

        if (!this.boxGeometry) this.boxGeometry = new THREE.BoxGeometry(1, 1, 1);
        const colorMap = new Map();
        this.blocks.forEach((block) => {
            if (!colorMap.has(block.type)) colorMap.set(block.type, []);
            colorMap.get(block.type).push(block);
        });

        const group = new THREE.Group();
        colorMap.forEach((blocks, type) => {
            const color = this.getBlockColor(type);
            const isTransp = this.isBlockTransparent(type);
            const isEmit = this.isBlockEmissive(type);
            const material = isEmit
                ? new THREE.MeshBasicMaterial({ color })
                : new THREE.MeshLambertMaterial({ color, transparent: isTransp, opacity: isTransp ? 0.5 : 1.0 });
            const mesh = new THREE.InstancedMesh(this.boxGeometry, material, blocks.length);
            mesh.castShadow = !isTransp;
            mesh.receiveShadow = true;
            const m = new THREE.Matrix4();
            blocks.forEach((block, i) => {
                const animKey = `${block.x},${block.y},${block.z}`;
                const anim = this.animatingBlocks.get(animKey);
                const sy = anim || 1;
                if (anim !== undefined) this.animatingBlocks.delete(animKey);
                m.identity();
                m.scale(new THREE.Vector3(1, sy, 1));
                m.setPosition(block.x + 0.5, block.y + 0.5, block.z + 0.5);
                mesh.setMatrixAt(i, m);
            });
            mesh.instanceMatrix.needsUpdate = true;
            group.add(mesh);
        });
        this.scene.add(group);
    }

    updateHighlight() {
        const target = this.getTargetBlock();
        if (target) {
            this.highlightMesh.position.set(target.x + 0.5, target.y + 0.5, target.z + 0.5);
            this.highlightMesh.visible = true;
            const bt = this.blockTypes.find(b => b.id === this.getBlockAt(target.x, target.y, target.z)?.type);
            this.highlightMesh.material.color.set(bt?.interactive ? 0x00ff88 : 0xffffff);
        } else {
            this.highlightMesh.visible = false;
        }
    }

    handleMovement() {
        const speed = this.cameraSpeed;
        const forward = new THREE.Vector3();
        this.camera.getWorldDirection(forward);
        forward.y = 0; forward.normalize();
        const right = new THREE.Vector3().crossVectors(forward, new THREE.Vector3(0, 1, 0)).normalize();
        let moved = false;
        if (this.keys['w']) { this.cameraTarget.add(forward.clone().multiplyScalar(speed)); moved = true; }
        if (this.keys['s']) { this.cameraTarget.add(forward.clone().multiplyScalar(-speed)); moved = true; }
        if (this.keys['a']) { this.cameraTarget.add(right.clone().multiplyScalar(-speed)); moved = true; }
        if (this.keys['d']) { this.cameraTarget.add(right.clone().multiplyScalar(speed)); moved = true; }
        if (this.keys['q']) { this.cameraTarget.y -= speed; moved = true; }
        if (this.keys['e']) { this.cameraTarget.y += speed; moved = true; }
        if (moved) this.updateCamera();
    }

    render() { if (this.renderer) this.renderer.render(this.scene, this.camera); }

    onResize() {
        if (!this.camera || !this.renderer) return;
        this.camera.aspect = window.innerWidth / window.innerHeight;
        this.camera.updateProjectionMatrix();
        this.renderer.setSize(window.innerWidth, window.innerHeight);
    }

    start(engine) {
        this.setEngine(engine);
        this.running = true;
        this.updateFromWASM();
        this.rebuildMesh();
        this.updateCamera();
        const loop = () => {
            if (!this.running) return;
            requestAnimationFrame(loop);
            this.handleMovement();
            this.updateHighlight();
            this.render();
        };
        loop();
        this._wasmPollTimer = setInterval(() => {
            if (!this.running) return;
            if (!this.dirty) return;
            this.updateFromWASM();
            this.rebuildMesh();
        }, this.wasmPollInterval);
    }

    stop() {
        this.running = false;
        if (this._wasmPollTimer) { clearInterval(this._wasmPollTimer); this._wasmPollTimer = null; }
        this._launchIntervals.forEach(iv => clearInterval(iv));
        this._launchIntervals.clear();
    }

    destroy() {
        this.stop();
        if (this._boundOnResize) window.removeEventListener('resize', this._boundOnResize);
        if (this._boundOnThemeChange) window.removeEventListener('themechange', this._boundOnThemeChange);
        if (this._boundOnKeyDown) window.removeEventListener('keydown', this._boundOnKeyDown);
        if (this._boundOnKeyUp) window.removeEventListener('keyup', this._boundOnKeyUp);
    }
}

window.VoxelRenderer = VoxelRenderer;
