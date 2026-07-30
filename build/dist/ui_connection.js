class EngineConnection {
    constructor() {
        this.wasmReady = false;
        this.Module = null;
        this.simRunning = false;
        this.paused = false;
        this.timeScale = 1;
        this.speedValues = [0.1, 0.25, 0.5, 1, 2, 5, 10, 100];
        this.weatherNames = ['CLEAR', 'RAIN', 'SNOW', 'STORM', 'FOG'];
        this.envPresets = {
            earth: { gravity: 9.81, airDensity: 1.225, temperature: 293.15, humidity: 50 },
            moon: { gravity: 1.62, airDensity: 0, temperature: 100, humidity: 0 },
            mars: { gravity: 3.71, airDensity: 0.020, temperature: 210, humidity: 0 },
            ocean: { gravity: 9.81, airDensity: 1025, temperature: 277, humidity: 100 },
            space: { gravity: 0, airDensity: 0, temperature: 2.7, humidity: 0 },
            custom: { gravity: 9.81, airDensity: 1.225, temperature: 293.15, humidity: 50 }
        };
        this.listeners = {};
        this.frameCount = 0;
        this.lastFpsTime = 0;
        this.fps = 0;
    }

    on(event, cb) {
        if (!this.listeners[event]) this.listeners[event] = [];
        this.listeners[event].push(cb);
    }

    emit(event, data) {
        (this.listeners[event] || []).forEach(cb => cb(data));
    }

    async load() {
        const Factory = window.OpenMindModule || (typeof OpenMindModule !== 'undefined' ? OpenMindModule : null);
        if (!Factory) {
            this.emit('log', { msg: 'WASM Module factory not found. Running demo mode.', type: 'warn' });
            return false;
        }
        try {
            this.Module = await Factory();
            this.wasmReady = true;
            this.emit('log', { msg: 'WASM engine loaded.', type: 'ok' });
            this.emit('connected');
            return true;
        } catch (e) {
            this.emit('log', { msg: 'WASM load failed: ' + e.message, type: 'warn' });
            return false;
        }
    }

    call(fn, retType, args, types) {
        if (!this.wasmReady || !this.Module) return null;
        try {
            return this.Module.ccall(fn, retType, args, types);
        } catch (e) {
            this.emit('log', { msg: fn + ' error: ' + e.message, type: 'err' });
            return null;
        }
    }

    initWorld() { this.call('initWorld', null, [], []); }

    tick(delta) {
        if (this.wasmReady && this.Module && !this.paused) {
            this.frameCount++;
            const now = performance.now();
            if (now - this.lastFpsTime >= 1000) {
                this.fps = this.frameCount;
                this.frameCount = 0;
                this.lastFpsTime = now;
            }
            this.call('tickPhysicsDelta', 'number', ['number'], [delta !== undefined ? delta : 1/60]);
        }
    }

    setBlock(x, y, z, type, propsJson) {
        if (this.renderer) this.renderer.dirty = true;
        if (propsJson) {
            return this.call('setBlock', 'number', ['number','number','number','number','string'], [x, y, z, type, propsJson]);
        } else {
            return this.call('setBlockSimple', 'number', ['number','number','number','number'], [x, y, z, type]);
        }
    }

    getBlock(x, y, z) {
        const json = this.call('getBlockData', 'string', ['number','number','number'], [x, y, z]);
        try { return JSON.parse(json); } catch (e) { return null; }
    }

    removeBlock(x, y, z) {
        return this.call('removeBlock', 'number', ['number','number','number'], [x, y, z]);
    }

    generateFromPrompt(prompt) {
        const result = this.call('generateFromPrompt', 'number', ['string'], [prompt]);
        if (this.renderer) this.renderer.dirty = true;
        return result;
    }

    setTimeOfDay(hours) { this.call('setTimeOfDay', null, ['number'], [hours]); }
    getTimeOfDay() { return this.call('getTimeOfDay', 'number', [], []) || 6; }
    getSunlightIntensity() { return this.call('getSunlightIntensity', 'number', [], []) || 0; }
    setCycleDuration(secs) { this.call('setCycleDuration', null, ['number'], [secs]); }
    rewindTime(secs) { return this.call('rewindTime', 'number', ['number'], [secs]) || 0; }

    setWeather(type) { this.call('setWeather', null, ['number'], [type]); }
    getWeather() {
        const json = this.call('getWeather', 'string', [], []);
        try { return JSON.parse(json); } catch (e) { return { type: 0, visibility: 1 }; }
    }
    getWeatherType() { return this.call('getWeatherType', 'number', [], []) || 0; }

    setTimeScale(s) { this.timeScale = s; this.call('setTimeScale', null, ['number'], [s]); }

    getWorldStats() {
        const json = this.call('getWorldStats', 'string', [], []);
        try { return JSON.parse(json); } catch {
            return { totalBlocks: 0, currentTick: 0, timeScale: 1, averageTemperature: 293.15, livingEntities: 0 };
        }
    }

    getAgentCount() { return this.call('getAgentCount', 'number', [], []) || 0; }
    getAgentData(idx) {
        const json = this.call('getAgentData', 'string', ['number'], [idx]);
        try { return JSON.parse(json); } catch { return null; }
    }
    addAgent(x, y, z, isPredator) { return this.call('addAgent', 'number', ['number','number','number','number'], [x, y, z, isPredator || 0]); }
    setAgentPosition(idx, x, y, z) { return this.call('setAgentPosition', 'number', ['number','number','number','number'], [idx, x, y, z]); }

    teleportCamera(x, y, z) { return this.call('teleportCamera', 'number', ['number','number','number'], [x, y, z]); }

    setOverlay(type, enabled) { return this.call('setOverlay', 'number', ['string','number'], [type, enabled ? 1 : 0]); }

    setGravity(g) { this.call('setGravity', null, ['number'], [g]); }
    setAmbientTemperature(t) { this.call('setAmbientTemperature', null, ['number'], [t]); }
    setWind(x, y, z) { this.call('setWind', null, ['number','number','number'], [x, y, z]); }

    exportCSV(filename) { return this.call('exportCSV', 'string', ['string'], [filename]); }
    exportGLTF(filename) { return this.call('exportGLTF', 'string', ['string'], [filename]); }
    getLastExportCSV() { return this.call('getLastExportCSV', 'string', [], []); }
    getLastExportGLTF() { return this.call('getLastExportGLTF', 'string', [], []); }
    saveWorld() { return this.call('saveWorld', 'string', [], []); }
    saveSnapshot() { return this.call('saveSnapshot', 'number', [], []); }
    getPendingFragments() {
        const json = this.call('getPendingFragments', 'string', [], []);
        try { return JSON.parse(json); } catch { return []; }
    }
    getSunPosition() {
        const json = this.call('getSunPosition', 'string', [], []);
        try { return JSON.parse(json); } catch { return { angle: 0, x: 0, y: 0, intensity: 0, colorTemperature: 0, isAboveHorizon: false }; }
    }
    cleanup() { this.call('cleanup', null, [], []); }
}

window.EngineConnection = EngineConnection;
