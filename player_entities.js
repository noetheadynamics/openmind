class PlayerEntitySystem {
    constructor(scene) {
        this.scene = scene;
        this.entities = new Map(); // peerId -> { mesh, nameTag, color, position, targetPosition, nameText }
        this.localPlayer = null;
        this.interpolationSpeed = 10;
        this.nameLabelStyle = { font: '14px monospace', fill: '#ffffff', align: 'center' };

        this.onEntityUpdated = null;

        this._createLocalPlayer();
    }

    _createLocalPlayer() {
        const geo = new THREE.BoxGeometry(0.8, 1.6, 0.8);
        const mat = new THREE.MeshLambertMaterial({ color: 0x8b5cf6, transparent: true, opacity: 0.6 });
        const mesh = new THREE.Mesh(geo, mat);
        mesh.visible = false;
        this.scene.add(mesh);
        this.localPlayer = mesh;
    }

    setLocalPlayerVisible(visible) {
        if (this.localPlayer) this.localPlayer.visible = visible;
    }

    setLocalPlayerColor(color) {
        if (this.localPlayer) this.localPlayer.material.color.set(color);
    }

    addEntity(peerId, name, color, position) {
        if (this.entities.has(peerId)) {
            this.updateEntityPosition(peerId, position);
            return;
        }

        const group = new THREE.Group();

        const bodyGeo = new THREE.BoxGeometry(0.8, 1.6, 0.8);
        const bodyMat = new THREE.MeshLambertMaterial({ color: color });
        const body = new THREE.Mesh(bodyGeo, bodyMat);
        body.position.y = 0.8;
        body.castShadow = true;
        group.add(body);

        const headGeo = new THREE.BoxGeometry(0.6, 0.6, 0.6);
        const headMat = new THREE.MeshLambertMaterial({ color: 0xffcc99 });
        const head = new THREE.Mesh(headGeo, headMat);
        head.position.y = 1.9;
        head.castShadow = true;
        group.add(head);

        const eyeGeo = new THREE.SphereGeometry(0.05, 8, 8);
        const eyeMat = new THREE.MeshBasicMaterial({ color: 0x000000 });
        const leftEye = new THREE.Mesh(eyeGeo, eyeMat);
        leftEye.position.set(-0.15, 1.95, 0.28);
        group.add(leftEye);
        const rightEye = new THREE.Mesh(eyeGeo, eyeMat);
        rightEye.position.set(0.15, 1.95, 0.28);
        group.add(rightEye);

        const nameCanvas = document.createElement('canvas');
        nameCanvas.width = 256;
        nameCanvas.height = 64;
        const ctx = nameCanvas.getContext('2d');
        ctx.fillStyle = 'rgba(0,0,0,0.5)';
        ctx.roundRect(0, 0, 256, 64, 8);
        ctx.fill();
        ctx.font = 'bold 28px monospace';
        ctx.fillStyle = color;
        ctx.textAlign = 'center';
        ctx.fillText(name, 128, 42);

        const nameTexture = new THREE.CanvasTexture(nameCanvas);
        const nameMat = new THREE.SpriteMaterial({ map: nameTexture, transparent: true });
        const nameSprite = new THREE.Sprite(nameMat);
        nameSprite.position.y = 2.7;
        nameSprite.scale.set(2, 0.5, 1);
        group.add(nameSprite);

        const handGeo = new THREE.BoxGeometry(0.3, 0.3, 0.3);
        const handMat = new THREE.MeshLambertMaterial({ color: 0x94a3b8 });
        const hand = new THREE.Mesh(handGeo, handMat);
        hand.position.set(0.55, 0.8, 0);
        group.add(hand);

        if (position) {
            group.position.set(position.x, position.y, position.z);
        }

        this.scene.add(group);

        this.entities.set(peerId, {
            mesh: group,
            body,
            head,
            nameSprite,
            hand,
            color: color,
            name: name,
            position: position || { x: 0, y: 0, z: 0 },
            targetPosition: position || { x: 0, y: 0, z: 0 },
            targetRotation: 0,
            currentRotation: 0,
            handBlockType: 0,
            lastUpdate: Date.now()
        });

        console.log('[PLAYER] Added entity:', name, '(' + peerId + ')');
    }

    removeEntity(peerId) {
        const entity = this.entities.get(peerId);
        if (entity) {
            this.scene.remove(entity.mesh);
            entity.mesh.traverse((child) => {
                if (child.geometry) child.geometry.dispose();
                if (child.material) {
                    if (child.material.map) child.material.map.dispose();
                    child.material.dispose();
                }
            });
            this.entities.delete(peerId);
            console.log('[PLAYER] Removed entity:', entity.name);
        }
    }

    updateEntityPosition(peerId, position, rotation, handBlockType) {
        const entity = this.entities.get(peerId);
        if (!entity) return;

        entity.targetPosition = { x: position.x, y: position.y, z: position.z };
        if (rotation !== undefined) entity.targetRotation = rotation;
        if (handBlockType !== undefined) entity.handBlockType = handBlockType;
        entity.lastUpdate = Date.now();
    }

    setEntityColor(peerId, color) {
        const entity = this.entities.get(peerId);
        if (entity) {
            entity.color = color;
            entity.body.material.color.set(color);
            this._updateNameSprite(entity);
        }
    }

    setEntityName(peerId, name) {
        const entity = this.entities.get(peerId);
        if (entity) {
            entity.name = name;
            this._updateNameSprite(entity);
        }
    }

    _updateNameSprite(entity) {
        const canvas = document.createElement('canvas');
        canvas.width = 256;
        canvas.height = 64;
        const ctx = canvas.getContext('2d');
        ctx.fillStyle = 'rgba(0,0,0,0.5)';
        ctx.roundRect(0, 0, 256, 64, 8);
        ctx.fill();
        ctx.font = 'bold 28px monospace';
        ctx.fillStyle = entity.color;
        ctx.textAlign = 'center';
        ctx.fillText(entity.name, 128, 42);
        if (entity.nameSprite.material.map) entity.nameSprite.material.map.dispose();
        entity.nameSprite.material.map = new THREE.CanvasTexture(canvas);
        entity.nameSprite.material.needsUpdate = true;
    }

    updateHandBlock(peerId, blockType) {
        const entity = this.entities.get(peerId);
        if (!entity) return;

        const colors = {
            0: 0x94a3b8, 1: 0x808080, 2: 0x8b5e3c, 3: 0x22c55e,
            4: 0x3b82f6, 5: 0xeab308, 6: 0x88ccff, 7: 0xc4823c,
            8: 0x16a34a, 9: 0x94a3b8, 10: 0xf97316, 11: 0xeab308,
            12: 0x64748b, 13: 0x06b6d4, 14: 0x333333, 15: 0x1a1a2e
        };

        entity.hand.material.color.set(colors[blockType] || 0x94a3b8);
        entity.hand.visible = blockType !== 0;
    }

    showSpeechBubble(peerId, message) {
        const entity = this.entities.get(peerId);
        if (!entity) return;

        const bubbleCanvas = document.createElement('canvas');
        bubbleCanvas.width = 512;
        bubbleCanvas.height = 128;
        const ctx = bubbleCanvas.getContext('2d');

        ctx.fillStyle = 'rgba(255,255,255,0.9)';
        ctx.strokeStyle = 'rgba(100,100,100,0.5)';
        ctx.lineWidth = 2;
        ctx.roundRect(10, 10, 492, 90, 12);
        ctx.fill();
        ctx.stroke();

        ctx.fillStyle = '#1a1a2e';
        ctx.font = '24px monospace';
        ctx.textAlign = 'center';
        const lines = this._wrapText(message, 45);
        lines.forEach((line, i) => {
            ctx.fillText(line, 256, 40 + i * 28);
        });

        const bubbleTexture = new THREE.CanvasTexture(bubbleCanvas);
        const bubbleMat = new THREE.SpriteMaterial({ map: bubbleTexture, transparent: true });
        const bubble = new THREE.Sprite(bubbleMat);
        bubble.position.copy(entity.mesh.position);
        bubble.position.y += 3.5;
        bubble.scale.set(4, 1, 1);
        this.scene.add(bubble);

        setTimeout(() => {
            this.scene.remove(bubble);
            bubble.material.map.dispose();
            bubble.material.dispose();
        }, 5000);
    }

    _wrapText(text, maxChars) {
        const words = text.split(' ');
        const lines = [];
        let line = '';
        for (const word of words) {
            if ((line + ' ' + word).length > maxChars) {
                lines.push(line);
                line = word;
            } else {
                line = line ? line + ' ' + word : word;
            }
        }
        if (line) lines.push(line);
        return lines;
    }

    update(deltaTime) {
        const dt = deltaTime || 0.016;

        this.entities.forEach((entity, peerId) => {
            const dx = entity.targetPosition.x - entity.mesh.position.x;
            const dy = entity.targetPosition.y - entity.mesh.position.y;
            const dz = entity.targetPosition.z - entity.mesh.position.z;

            const dist = Math.sqrt(dx*dx + dy*dy + dz*dz);

            if (dist > 0.01) {
                const speed = Math.min(this.interpolationSpeed * dt, 1);
                entity.mesh.position.x += dx * speed;
                entity.mesh.position.y += dy * speed;
                entity.mesh.position.z += dz * speed;
            } else {
                entity.mesh.position.copy(entity.mesh.position);
            }

            const targetRot = entity.targetRotation || 0;
            const rotDiff = targetRot - entity.currentRotation;
            if (Math.abs(rotDiff) > 0.01) {
                entity.currentRotation += rotDiff * Math.min(this.interpolationSpeed * dt, 1);
                entity.mesh.rotation.y = entity.currentRotation;
            }

            const idle = Date.now() - entity.lastUpdate > 5000;
            if (idle) {
                entity.head.position.y = 1.9 + Math.sin(Date.now() * 0.003) * 0.03;
            }
        });
    }

    getEntityCount() { return this.entities.size; }
    getEntities() { return this.entities; }

    clear() {
        this.entities.forEach((_, peerId) => this.removeEntity(peerId));
        this.entities.clear();
        if (this.localPlayer) {
            this.scene.remove(this.localPlayer);
            if (this.localPlayer.geometry) this.localPlayer.geometry.dispose();
            if (this.localPlayer.material) {
                if (this.localPlayer.material.map) this.localPlayer.material.map.dispose();
                this.localPlayer.material.dispose();
            }
            this.localPlayer = null;
        }
    }
}

window.PlayerEntitySystem = PlayerEntitySystem;
