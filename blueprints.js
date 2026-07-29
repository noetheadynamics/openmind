/**
 * OpenMind – Blueprint System
 * Save/load/export/import .BP files, IndexedDB library
 */
(function() {
    'use strict';

    class Blueprint {
        constructor(name, blocks, width, height, depth) {
            this.id = 'bp_' + Date.now().toString(36) + Math.random().toString(36).substr(2, 4);
            this.name = name || 'Untitled';
            this.blocks = blocks || [];
            this.width = width || 0;
            this.height = height || 0;
            this.depth = depth || 0;
            this.tags = [];
            this.created = Date.now();
            this.modified = Date.now();
        }
    }

    class BlueprintSystem {
        constructor() {
            this.library = [];
            this.dbName = 'openmind_blueprints';
            this.db = null;
            this.listeners = [];
        }

        async init() {
            return new Promise((resolve, reject) => {
                const req = indexedDB.open(this.dbName, 1);
                req.onupgradeneeded = (e) => {
                    const db = e.target.result;
                    if (!db.objectStoreNames.contains('blueprints')) {
                        db.createObjectStore('blueprints', { keyPath: 'id' });
                    }
                };
                req.onsuccess = (e) => {
                    this.db = e.target.result;
                    this.loadAll().then(() => resolve());
                };
                req.onerror = () => reject(new Error('Failed to open IndexedDB'));
            });
        }

        create(name, selection, engine) {
            if (!selection || selection.blocks.size === 0) return null;
            const bounds = selection.getBounds();
            const blocks = [];
            for (const key of selection.blocks) {
                const [x, y, z] = key.split(',').map(Number);
                const blockType = engine ? engine.getBlock(x, y, z)?.blockType || 0 : 1;
                blocks.push({
                    x: x - bounds.minX, y: y - bounds.minY, z: z - bounds.minZ,
                    type: blockType
                });
            }
            const bp = new Blueprint(name, blocks,
                bounds.maxX - bounds.minX + 1,
                bounds.maxY - bounds.minY + 1,
                bounds.maxZ - bounds.minZ + 1
            );
            this.library.push(bp);
            this.save(bp);
            this.emit('create', bp);
            return bp;
        }

        delete(id) {
            this.library = this.library.filter(b => b.id !== id);
            if (this.db) {
                const tx = this.db.transaction('blueprints', 'readwrite');
                tx.objectStore('blueprints').delete(id);
            }
            this.emit('delete', { id });
        }

        get(id) { return this.library.find(b => b.id === id); }
        getAll() { return [...this.library]; }
        search(query) {
            const q = query.toLowerCase();
            return this.library.filter(b => b.name.toLowerCase().includes(q) || b.tags.some(t => t.toLowerCase().includes(q)));
        }

        async save(bp) {
            if (!this.db) return;
            bp.modified = Date.now();
            const tx = this.db.transaction('blueprints', 'readwrite');
            tx.objectStore('blueprints').put(bp);
        }

        async loadAll() {
            if (!this.db) return;
            return new Promise((resolve) => {
                const tx = this.db.transaction('blueprints', 'readonly');
                const req = tx.objectStore('blueprints').getAll();
                req.onsuccess = () => { this.library = req.result || []; resolve(); };
                req.onerror = () => resolve();
            });
        }

        exportBP(bp) {
            return JSON.stringify({
                format: 'OpenMind-BP',
                version: 1,
                name: bp.name,
                width: bp.width, height: bp.height, depth: bp.depth,
                blocks: bp.blocks,
                tags: bp.tags
            });
        }

        importBP(json) {
            try {
                const data = JSON.parse(json);
                if (data.format !== 'OpenMind-BP') return null;
                const bp = new Blueprint(data.name, data.blocks, data.width, data.height, data.depth);
                bp.tags = data.tags || [];
                this.library.push(bp);
                this.save(bp);
                this.emit('import', bp);
                return bp;
            } catch (e) { return null; }
        }

        place(bp, x, y, z, engine, rotation, flipX, flipY, flipZ) {
            if (!engine || !bp) return 0;
            rotation = rotation || 0;
            let placed = 0;
            for (const block of bp.blocks) {
                let bx = block.x, by = block.y, bz = block.z;
                for (let r = 0; r < rotation; r++) {
                    const t = bx; bx = -bz; bz = t;
                    bx += bp.width - 1;
                }
                if (flipX) bx = (bp.width - 1) - bx;
                if (flipY) by = (bp.height - 1) - by;
                if (flipZ) bz = (bp.depth - 1) - bz;
                if (block.type > 0) {
                    engine.setBlock(x + bx, y + by, z + bz, block.type);
                    placed++;
                }
            }
            return placed;
        }

        addTag(id, tag) {
            const bp = this.get(id);
            if (bp && !bp.tags.includes(tag)) {
                bp.tags.push(tag);
                this.save(bp);
            }
        }

        removeTag(id, tag) {
            const bp = this.get(id);
            if (bp) {
                bp.tags = bp.tags.filter(t => t !== tag);
                this.save(bp);
            }
        }

        on(fn) { this.listeners.push(fn); }
        emit(type, data) { for (const fn of this.listeners) fn({ type, ...data }); }

        destroy() { if (this.db) this.db.close(); }
    }

    window.Blueprint = Blueprint;
    window.BlueprintSystem = BlueprintSystem;
})();
