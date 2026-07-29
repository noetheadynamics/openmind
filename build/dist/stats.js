class LiveStats {
    constructor() {
        this.engine = null;
        this.renderer = null;
        this.interval = null;
        this.rafId = null;
        this.fps = 0;
        this.frameCount = 0;
        this.lastFpsTime = performance.now();
        this.listeners = {};
    }

    setEngine(engine) { this.engine = engine; }
    setRenderer(renderer) { this.renderer = renderer; }

    on(event, cb) {
        if (!this.listeners[event]) this.listeners[event] = [];
        this.listeners[event].push(cb);
    }

    emit(event, data) {
        (this.listeners[event] || []).forEach(cb => cb(data));
    }

    start(intervalMs) {
        this.stop();
        this.interval = setInterval(() => this.poll(), intervalMs || 1000);
        this.trackFPS();
    }

    stop() {
        if (this.interval) { clearInterval(this.interval); this.interval = null; }
        if (this.rafId) { cancelAnimationFrame(this.rafId); this.rafId = null; }
    }

    trackFPS() {
        const loop = () => {
            this.frameCount++;
            const now = performance.now();
            if (now - this.lastFpsTime >= 1000) {
                this.fps = this.frameCount;
                this.frameCount = 0;
                this.lastFpsTime = now;
            }
            if (this.interval) this.rafId = requestAnimationFrame(loop);
        };
        this.rafId = requestAnimationFrame(loop);
    }

    poll() {
        if (!this.engine || !this.engine.wasmReady) return;

        const stats = this.engine.getWorldStats();
        const weather = this.engine.getWeather();
        const tod = this.engine.getTimeOfDay();
        const agentCount = this.engine.getAgentCount();

        const data = {
            totalBlocks: stats.totalBlocks || 0,
            currentTick: stats.currentTick || 0,
            averageTemperature: stats.averageTemperature || 293.15,
            timeScale: stats.timeScale || 1,
            timeOfDay: tod,
            weatherType: weather.type || 0,
            weatherName: ['CLEAR', 'RAIN', 'SNOW', 'STORM', 'FOG'][weather.type] || 'CLEAR',
            agentCount: agentCount,
            fps: this.fps,
            rendererBlocks: this.renderer ? this.renderer.blocks.size : 0
        };

        this.emit('stats', data);
        this.updateDOM(data);
    }

    updateDOM(data) {
        const el = (id) => document.getElementById(id);
        if (el('statBlocks')) el('statBlocks').textContent = data.totalBlocks;
        if (el('statTick')) el('statTick').textContent = data.currentTick;
        if (el('statTemp')) el('statTemp').textContent = data.averageTemperature.toFixed(1) + ' K';
        if (el('statFPS')) el('statFPS').textContent = data.fps;
        if (el('statTimeOfDay')) el('statTimeOfDay').textContent = this.formatTime(data.timeOfDay);
        if (el('statWeather')) el('statWeather').textContent = data.weatherName;
        if (el('statEntities')) el('statEntities').textContent = data.agentCount;
        if (el('timeDisplay')) el('timeDisplay').textContent = this.formatTimeFull(data.timeOfDay);
        if (el('tickCount')) el('tickCount').textContent = 'Tick ' + data.currentTick;
        const dayLen = DAY_LENGTH;
        if (el('dayCount')) el('dayCount').textContent = 'Day ' + (Math.floor(data.currentTick / dayLen) + 1);
    }

    formatTime(h) {
        if (typeof OMUtils !== 'undefined') return OMUtils.formatTime(h);
        const hh = Math.floor(h) % 24;
        const mm = Math.floor((h - Math.floor(h)) * 60);
        return String(hh).padStart(2, '0') + ':' + String(mm).padStart(2, '0');
    }

    formatTimeFull(h) {
        if (typeof OMUtils !== 'undefined') return OMUtils.formatTimeFull(h);
        const hh = Math.floor(h) % 24;
        const mm = Math.floor((h - Math.floor(h)) * 60);
        const ss = Math.floor(((h * 60) % 1) * 60);
        return String(hh).padStart(2, '0') + ':' + String(mm).padStart(2, '0') + ':' + String(ss).padStart(2, '0');
    }
}

window.LiveStats = LiveStats;
