/**
 * OpenMind – Building History System
 * Visual timeline, IndexedDB persistence, jump to any point
 */
(function() {
    'use strict';

    class HistoryEntry {
        constructor(action, description, snapshot) {
            this.id = Date.now().toString(36) + Math.random().toString(36).substr(2, 4);
            this.action = action;
            this.description = description;
            this.snapshot = snapshot;
            this.timestamp = Date.now();
            this.blockCount = 0;
        }
    }

    class BuildingHistory {
        constructor() {
            this.entries = [];
            this.currentIndex = -1;
            this.maxEntries = 50;
            this.engine = null;
            this.dbName = 'openmind_history';
            this.db = null;
            this.listeners = [];
        }

        setEngine(e) { this.engine = e; }

        async init() {
            return new Promise((resolve) => {
                const req = indexedDB.open(this.dbName, 1);
                req.onupgradeneeded = (e) => {
                    const db = e.target.result;
                    if (!db.objectStoreNames.contains('history')) {
                        db.createObjectStore('history', { keyPath: 'id' });
                    }
                };
                req.onsuccess = (e) => { this.db = e.target.result; resolve(); };
                req.onerror = () => resolve();
            });
        }

        record(action, description) {
            const snapshot = this.captureSnapshot();
            const entry = new HistoryEntry(action, description, snapshot);
            entry.blockCount = this.countBlocks();
            if (this.currentIndex < this.entries.length - 1) {
                this.entries = this.entries.slice(0, this.currentIndex + 1);
            }
            this.entries.push(entry);
            if (this.entries.length > this.maxEntries) this.entries.shift();
            this.currentIndex = this.entries.length - 1;
            this.saveEntry(entry);
            this.emit('record', entry);
            return entry;
        }

        undo() {
            if (this.currentIndex <= 0) return false;
            this.currentIndex--;
            this.restoreSnapshot(this.entries[this.currentIndex].snapshot);
            this.emit('undo', this.entries[this.currentIndex]);
            return true;
        }

        redo() {
            if (this.currentIndex >= this.entries.length - 1) return false;
            this.currentIndex++;
            this.restoreSnapshot(this.entries[this.currentIndex].snapshot);
            this.emit('redo', this.entries[this.currentIndex]);
            return true;
        }

        jumpTo(index) {
            if (index < 0 || index >= this.entries.length) return false;
            this.currentIndex = index;
            this.restoreSnapshot(this.entries[index].snapshot);
            this.emit('jump', this.entries[index]);
            return true;
        }

        captureSnapshot() {
            if (!this.engine || !this.engine.getBlockData) return null;
            try {
                const stats = this.engine.getWorldStats ? this.engine.getWorldStats() : null;
                const sz = stats?.size || 64;
                const data = this.engine.getBlockData(0, 0, 0, sz, Math.min(32, sz), sz);
                return data ? Array.from(data) : null;
            } catch (e) { return null; }
        }

        restoreSnapshot(snapshot) {
            if (!this.engine || !snapshot) return;
            try {
                for (let i = 0; i + 3 < snapshot.length; i += 4) {
                    if (snapshot[i + 3] > 0) {
                        this.engine.setBlock(snapshot[i], snapshot[i + 1], snapshot[i + 2], snapshot[i + 3]);
                    }
                }
            } catch (e) {}
        }

        countBlocks() {
            if (!this.engine || !this.engine.getBlockData) return 0;
            try {
                const stats = this.engine.getWorldStats ? this.engine.getWorldStats() : null;
                const sz = stats?.size || 64;
                const data = this.engine.getBlockData(0, 0, 0, sz, Math.min(32, sz), sz);
                if (!data) return 0;
                let count = 0;
                for (let i = 3; i < data.length; i += 4) {
                    if (data[i] > 0) count++;
                }
                return count;
            } catch (e) { return 0; }
        }

        async saveEntry(entry) {
            if (!this.db) return;
            try {
                const tx = this.db.transaction('history', 'readwrite');
                tx.onerror = () => {};
                tx.objectStore('history').put(entry);
            } catch (e) {}
        }

        async loadAll() {
            if (!this.db) return;
            return new Promise((resolve) => {
                const tx = this.db.transaction('history', 'readonly');
                const req = tx.objectStore('history').getAll();
                req.onsuccess = () => {
                    this.entries = (req.result || []).sort((a, b) => a.timestamp - b.timestamp);
                    this.currentIndex = this.entries.length - 1;
                    resolve();
                };
                req.onerror = () => resolve();
            });
        }

        clear() {
            this.entries = [];
            this.currentIndex = -1;
            if (this.db) {
                try {
                    const tx = this.db.transaction('history', 'readwrite');
                    tx.onerror = () => {};
                    tx.objectStore('history').clear();
                } catch (e) {}
            }
            this.emit('clear');
        }

        getTimeline() {
            return this.entries.map((e, i) => ({
                id: e.id,
                index: i,
                action: e.action,
                description: e.description,
                timestamp: e.timestamp,
                blockCount: e.blockCount,
                isCurrent: i === this.currentIndex
            }));
        }

        canUndo() { return this.currentIndex > 0; }
        canRedo() { return this.currentIndex < this.entries.length - 1; }

        on(fn) { this.listeners.push(fn); }
        emit(type, data) { for (const fn of this.listeners) fn({ type, ...data }); }

        destroy() { if (this.db) this.db.close(); this.db = null; }
    }

    window.BuildingHistory = BuildingHistory;
})();
