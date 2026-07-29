/**
 * OpenMind – Symmetry Tools
 * X/Y/Z axis mirroring, symmetry plane visualization
 */
(function() {
    'use strict';

    const SymAxis = Object.freeze({ X: 'x', Y: 'y', Z: 'z' });

    class Symmetry {
        constructor() {
            this.enabled = false;
            this.axes = new Set();
            this.center = { x: 0, y: 0, z: 0 };
            this.planeMeshes = [];
            this.scene = null;
            this.listeners = [];
        }

        setScene(s) { this.scene = s; }

        toggle(axis) {
            if (this.axes.has(axis)) this.axes.delete(axis);
            else this.axes.add(axis);
            this.enabled = this.axes.size > 0;
            this.updatePlanes();
            this.emit('toggle', { axis, enabled: this.enabled, activeAxes: Array.from(this.axes) });
        }

        setCenter(x, y, z) {
            this.center = { x, y, z };
            this.updatePlanes();
        }

        setAxes(axes) {
            this.axes = new Set(axes);
            this.enabled = this.axes.size > 0;
            this.updatePlanes();
        }

        getMirrorPositions(x, y, z) {
            const positions = [];
            if (!this.enabled) return positions;
            const cx = this.center.x, cy = this.center.y, cz = this.center.z;
            const addMirror = (px, py, pz) => {
                const key = `${px},${py},${pz}`;
                if (!positions.find(p => p.x === px && p.y === py && p.z === pz)) {
                    positions.push({ x: px, y: py, z: pz });
                }
            };
            addMirror(x, y, z);
            if (this.axes.has('x')) addMirror(2 * cx - x, y, z);
            if (this.axes.has('y')) addMirror(x, 2 * cy - y, z);
            if (this.axes.has('z')) addMirror(x, y, 2 * cz - z);
            if (this.axes.has('x') && this.axes.has('y')) addMirror(2 * cx - x, 2 * cy - y, z);
            if (this.axes.has('x') && this.axes.has('z')) addMirror(2 * cx - x, y, 2 * cz - z);
            if (this.axes.has('y') && this.axes.has('z')) addMirror(x, 2 * cy - y, 2 * cz - z);
            if (this.axes.has('x') && this.axes.has('y') && this.axes.has('z')) addMirror(2 * cx - x, 2 * cy - y, 2 * cz - z);
            return positions;
        }

        mirrorBlock(x, y, z, blockType, engine) {
            if (!this.enabled || !engine) return;
            const positions = this.getMirrorPositions(x, y, z);
            for (const p of positions) {
                if (p.x !== x || p.y !== y || p.z !== z) {
                    engine.setBlock(p.x, p.y, p.z, blockType);
                }
            }
        }

        updatePlanes() {
            this.clearPlanes();
            if (!this.scene) return;
            const size = 64;
            const makePlane = (axis, color) => {
                const geo = new THREE.PlaneGeometry(size, size);
                const mat = new THREE.MeshBasicMaterial({
                    color, transparent: true, opacity: 0.08,
                    depthWrite: false, side: THREE.DoubleSide
                });
                const mesh = new THREE.Mesh(geo, mat);
                if (axis === 'x') { mesh.rotation.y = Math.PI / 2; mesh.position.set(this.center.x, this.center.y, this.center.z); }
                else if (axis === 'y') { mesh.rotation.x = Math.PI / 2; mesh.position.set(this.center.x, this.center.y, this.center.z); }
                else { mesh.position.set(this.center.x, this.center.y, this.center.z); }

                const edgeGeo = new THREE.EdgesGeometry(geo);
                const edgeMat = new THREE.LineBasicMaterial({ color, transparent: true, opacity: 0.3 });
                mesh.add(new THREE.LineSegments(edgeGeo, edgeMat));
                this.scene.add(mesh);
                this.planeMeshes.push(mesh);
            };
            if (this.axes.has('x')) makePlane('x', 0xff4444);
            if (this.axes.has('y')) makePlane('y', 0x44ff44);
            if (this.axes.has('z')) makePlane('z', 0x4444ff);
        }

        clearPlanes() {
            for (const m of this.planeMeshes) {
                if (this.scene) this.scene.remove(m);
                m.traverse((child) => {
                    if (child.geometry) child.geometry.dispose();
                    if (child.material) child.material.dispose();
                });
            }
            this.planeMeshes = [];
        }

        disable() {
            this.enabled = false;
            this.axes.clear();
            this.clearPlanes();
            this.emit('disable');
        }

        on(fn) { this.listeners.push(fn); }
        emit(type, data) { for (const fn of this.listeners) fn({ type, ...data }); }

        destroy() { this.disable(); this.listeners = []; this.scene = null; }
    }

    window.SymAxis = SymAxis;
    window.Symmetry = Symmetry;
})();
