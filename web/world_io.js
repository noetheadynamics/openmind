class WorldIO {
    constructor() {
        this.dbName = 'OpenMindWorlds';
        this.dbVersion = 1;
        this.db = null;
        this.engine = null;
        this.renderer = null;
        this.autoSaveInterval = null;
        this.lastAutoSave = null;
    }

    async init() {
        return new Promise((resolve, reject) => {
            const req = indexedDB.open(this.dbName, this.dbVersion);
            req.onupgradeneeded = (e) => {
                const db = e.target.result;
                if (!db.objectStoreNames.contains('worlds')) {
                    const store = db.createObjectStore('worlds', { keyPath: 'name' });
                    store.createIndex('savedAt', 'savedAt', { unique: false });
                }
            };
            req.onsuccess = (e) => { this.db = e.target.result; resolve(); };
            req.onerror = (e) => reject(e.target.error);
        });
    }

    setEngine(engine) { this.engine = engine; }
    setRenderer(renderer) { this.renderer = renderer; }

    startAutoSave(intervalMs) {
        this.stopAutoSave();
        this.autoSaveInterval = setInterval(() => this.autoSave(), intervalMs || 30000);
    }

    stopAutoSave() {
        if (this.autoSaveInterval) { clearInterval(this.autoSaveInterval); this.autoSaveInterval = null; }
    }

    async autoSave() {
        if (!this.engine || !this.engine.wasmReady) return;
        await this.save('__autosave');
        this.lastAutoSave = new Date();
    }

    async save(name) {
        if (!this.engine || !this.engine.wasmReady) return { success: false, error: 'Engine not ready' };
        if (!this.db) {
            try { await this.init(); } catch (e) { return { success: false, error: 'DB init failed: ' + e.message }; }
        }

        const blocks = [];
        const step = 1;
        for (let x = 0; x < 256; x += step) {
            for (let y = 0; y < 256; y += step) {
                for (let z = 0; z < 256; z += step) {
                    const data = this.engine.getBlock(x, y, z);
                    if (data && data.exists && data.blockType !== 0) {
                        blocks.push({ x, y, z, type: data.blockType });
                    }
                }
            }
        }

        const worldData = {
            name,
            blocks,
            blockCount: blocks.length,
            stats: this.engine.getWorldStats(),
            savedAt: new Date().toISOString(),
            version: 1
        };

        return new Promise((resolve, reject) => {
            const tx = this.db.transaction('worlds', 'readwrite');
            tx.objectStore('worlds').put(worldData);
            tx.oncomplete = () => resolve({ success: true, blocks: blocks.length });
            tx.onerror = (e) => reject(e.target.error);
        });
    }

    async load(name) {
        if (!this.db) await this.init();

        return new Promise((resolve, reject) => {
            const tx = this.db.transaction('worlds', 'readonly');
            const req = tx.objectStore('worlds').get(name);
            req.onsuccess = () => {
                const data = req.result;
                if (!data) { resolve({ success: false, error: 'World not found' }); return; }

                if (this.engine && this.engine.wasmReady) {
                    this.engine.initWorld();
                    for (const b of data.blocks) {
                        this.engine.setBlock(b.x, b.y, b.z, b.type);
                    }
                    this.engine.tick(0.1);
                }
                if (this.renderer) {
                    this.renderer.updateFromWASM();
                    this.renderer.rebuildMesh();
                }
                resolve({ success: true, blocks: data.blocks.length, name: data.name, savedAt: data.savedAt });
            };
            req.onerror = (e) => reject(e.target.error);
        });
    }

    async list() {
        if (!this.db) await this.init();

        return new Promise((resolve, reject) => {
            const tx = this.db.transaction('worlds', 'readonly');
            const req = tx.objectStore('worlds').getAll();
            req.onsuccess = () => {
                const worlds = req.result.map(w => ({
                    name: w.name,
                    blockCount: w.blockCount || w.blocks?.length || 0,
                    savedAt: w.savedAt,
                    version: w.version || 1
                }));
                resolve(worlds);
            };
            req.onerror = (e) => reject(e.target.error);
        });
    }

    async remove(name) {
        if (!this.db) await this.init();

        return new Promise((resolve, reject) => {
            const tx = this.db.transaction('worlds', 'readwrite');
            tx.objectStore('worlds').delete(name);
            tx.oncomplete = () => resolve({ success: true });
            tx.onerror = (e) => reject(e.target.error);
        });
    }

    async exportOMW(name) {
        if (!this.engine || !this.engine.wasmReady) return { success: false, error: 'Engine not ready' };

        const blocks = [];
        for (let x = 0; x < 256; x++) {
            for (let y = 0; y < 256; y++) {
                for (let z = 0; z < 256; z++) {
                    const data = this.engine.getBlock(x, y, z);
                    if (data && data.exists && data.blockType !== 0) {
                        blocks.push([x, y, z, data.blockType]);
                    }
                }
            }
        }

        const omw = {
            format: 'OpenMindWorld',
            version: 1,
            name: name || 'export',
            blocks,
            exportedAt: new Date().toISOString()
        };

        const blob = new Blob([JSON.stringify(omw)], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = (name || 'world') + '.omw';
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);

        return { success: true, blocks: blocks.length };
    }

    async importOMW(file) {
        return new Promise((resolve, reject) => {
            const reader = new FileReader();
            reader.onload = (e) => {
                try {
                    const omw = JSON.parse(e.target.result);
                    if (omw.format !== 'OpenMindWorld') { resolve({ success: false, error: 'Invalid file format' }); return; }

                    if (this.engine && this.engine.wasmReady) {
                        this.engine.initWorld();
                        for (const [x, y, z, type] of omw.blocks) {
                            this.engine.setBlock(x, y, z, type);
                        }
                        this.engine.tick(0.1);
                    }
                    if (this.renderer) {
                        this.renderer.updateFromWASM();
                        this.renderer.rebuildMesh();
                    }
                    resolve({ success: true, blocks: omw.blocks.length, name: omw.name });
                } catch (err) {
                    resolve({ success: false, error: err.message });
                }
            };
            reader.onerror = () => resolve({ success: false, error: 'Failed to read file' });
            reader.readAsText(file);
        });
    }

    newWorld() {
        if (this.engine && this.engine.wasmReady) {
            this.engine.initWorld();
            this.engine.setTimeOfDay(6);
        }
        if (this.renderer) {
            this.renderer.blocks.clear();
            this.renderer.dirty = true;
            this.renderer.rebuildMesh();
        }
        return { success: true };
    }
}

window.WorldIO = WorldIO;
