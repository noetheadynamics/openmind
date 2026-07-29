const DAY_LENGTH = (typeof OMUtils !== 'undefined') ? OMUtils.DAY_LENGTH : 36000;

class OmniConsole {
    constructor() {
        this.engine = new EngineConnection();
        this.renderer = null;
        this.llm = new LLMClient();
        this.worldIO = new WorldIO();
        this.liveStats = null;
        this.interactive = new InteractiveObjectSystem();
        this.inventory = new InventorySystem();
        this.crafting = new CraftingSystem();
        this.particles = new ParticleSystem();
        this.water = null;
        this.skybox = null;
        this.postProcessing = null;
        this.sound = new SoundSystem();
        this.physicsVisuals = null;
        this.errorHandler = new ErrorHandler();
        this.notifications = null;
        this.undoRedo = new UndoRedo();
        this.shortcuts = new Shortcuts();
        this.tutorial = null;
        this.settingsPanel = new SettingsPanel();
        this.selection = new Selection();
        this.copyPaste = new CopyPaste();
        this.blueprints = new BlueprintSystem();
        this.symmetry = new Symmetry();
        this.patternTools = new PatternTools();
        this.importExport = new ImportExportBuilding();
        this.buildingHistory = new BuildingHistory();
        this.loadingScreen = null;
        this.activePanel = 'prompt';
        this.promptHistory = [];
        this.historyIndex = -1;
        this.bookmarks = [];
        this.forgeHistory = [];
        this.currentEnv = 'earth';
        this.selectedAgent = -1;
        this.lightMode = false;
        this.lastTickTime = 0;
        this.suggestions = [
            'Create a 10x10 stone platform',
            'weather rain',
            'time 12',
            'Place a diamond block at 5,5,5',
            'Generate a fireproof alien glass',
            'pause',
            'speed 5',
            'Show temperature heatmap',
            'Create a house with wood walls',
            'Set gravity to Moon levels'
        ];
        this.speedLabels = ['0.1x', '0.25x', '0.5x', '1x', '2x', '5x', '10x', '100x'];
        this._exportProgressInterval = null;
        this._simLoopTimeout = null;
    }

    destroy() {
        this.stopSimulation();
        if (this.agentInterval) { clearInterval(this.agentInterval); this.agentInterval = null; }
        if (this._exportProgressInterval) { clearInterval(this._exportProgressInterval); this._exportProgressInterval = null; }
        if (this._ecoGraphRaf) { cancelAnimationFrame(this._ecoGraphRaf); this._ecoGraphRaf = null; }
        if (this.shortcuts) this.shortcuts.destroy();
        if (this.liveStats) this.liveStats.stop();
        if (this.worldIO) this.worldIO.stopAutoSave();
        if (this.tutorial) this.tutorial.removeUI();
    }

    async init() {
        this.bindTopBar();
        this.bindPrompt();
        this.bindTime();
        this.bindOverlays();
        this.bindAgents();
        this.bindEnvironment();
        this.bindForge();
        this.bindExport();
        this.bindTeleport();
        this.bindBrain();
        this.bindMisc();
        this.bindInteractive();
        this.bindInventory();
        this.bindCrafting();
        this.bindVisual();
        this.bindSettings();
        this.bindBuildingTools();
        this.shortcuts.init();
        this.shortcuts.register('undo', () => this.undoRedo.undo());
        this.shortcuts.register('redo', () => this.undoRedo.redo());
        this.shortcuts.register('save', async () => {
            try { await this.worldIO.save('auto'); } catch (e) { this.addChatMessage('assistant', 'Save failed'); }
        });
        this.shortcuts.register('shortcuts', () => this.shortcuts.showReference());
        this.shortcuts.register('pause', () => {
            this.engine.paused = !this.engine.paused;
            const btn = document.getElementById('playPauseBtn');
            if (btn) btn.textContent = this.engine.paused ? '▶' : '⏸';
        });
        this.errorHandler.setNotificationFn((entry) => {
            if (this.notifications) this.notifications.error(entry.title + ': ' + entry.userMessage);
        });
        this.engine.on('log', d => this.log(d.msg, d.type));
        this.engine.on('connected', () => this.onConnected());
        await this.loadWASM();
        this.renderSuggestions();
    }

    /* ====== WASM LOADING ====== */
    async loadWASM() {
        const overlay = document.getElementById('loadingOverlay');
        const text = document.getElementById('loadingText');
        try {
            if (overlay) overlay.classList.add('visible');
            if (text) text.textContent = 'Loading WebAssembly engine...';
            const ok = await this.engine.load();
            if (ok) {
                this.engine.initWorld();
                this.engine.setTimeOfDay(6);
                this.renderer = new VoxelRenderer('viewport3d');
                this.renderer.start(this.engine);
                if (this.renderer.scene && this.renderer.camera) {
                    this.skybox = new OpenMindSkybox(this.renderer.scene);
                    this.water = new WaterRenderer(this.renderer.scene, this.renderer.camera);
                    this.physicsVisuals = new PhysicsVisuals(this.renderer.scene);
                    if (typeof THREE !== 'undefined' && THREE.EffectComposer) {
                        this.postProcessing = new PostProcessing(this.renderer.renderer, this.renderer.scene, this.renderer.camera);
                        this.postProcessing.setup();
                    }
                }
                try { this.sound.init(); } catch (e) { this.log('Sound init failed: ' + e.message, 'warn'); }
                this.notifications = new Notifications();
                this.tutorial = new Tutorial();
                this.tutorial.on((e) => {
                    if (e.type === 'complete') this.notifications?.success('Tutorial complete! You\'re ready to build.');
                });
                this.llm.engine = this.engine;
                this.llm.renderer = this.renderer;
                this.interactive.setEngine(this.engine);
                this.interactive.setRenderer(this.renderer);
                this.inventory.setEngine(this.engine);
                this.inventory.setRenderer(this.renderer);
                this.inventory.setInteractiveSystem(this.interactive);
                this.crafting.setInventory(this.inventory);
                this.interactive.setInventory(this.inventory);
                this.interactive.setCrafting(this.crafting);
                this.selection.setScene(this.renderer.scene);
                this.copyPaste.setScene(this.renderer.scene);
                this.copyPaste.setEngine(this.engine);
                this.patternTools.setEngine(this.engine);
                this.importExport.setEngine(this.engine);
                this.buildingHistory.setEngine(this.engine);
                this.symmetry.setScene(this.renderer.scene);
                await this.blueprints.init().catch(e => this.log('Blueprint DB unavailable: ' + e.message, 'warn'));
                await this.buildingHistory.init().catch(e => this.log('History DB unavailable: ' + e.message, 'warn'));
                await this.worldIO.init().catch(e => this.log('IndexedDB unavailable: ' + e.message, 'warn'));
                this.worldIO.setEngine(this.engine);
                this.worldIO.setRenderer(this.renderer);
                this.worldIO.startAutoSave(30000);
                if (typeof LiveStats !== 'undefined') {
                    this.liveStats = new LiveStats();
                    this.liveStats.setEngine(this.engine);
                    this.liveStats.setRenderer(this.renderer);
                    this.liveStats.start(1000);
                }
                this.startSimulation();
            }
            this.updateConnectionStatus(ok);
        } catch (e) {
            this.log('[loadWASM] Initialization error: ' + e.message, 'err');
            this.updateConnectionStatus(false);
        } finally {
            if (overlay) overlay.classList.remove('visible');
        }
    }

    onConnected() {
        this.updateConnectionStatus(true);
    }

    updateConnectionStatus(connected) {
        const dot = document.querySelector('#connectionStatus .status-dot');
        const text = document.querySelector('#connectionStatus .status-text');
        if (dot) { if (connected) dot.classList.add('connected'); else dot.classList.remove('connected'); }
        if (text) { text.textContent = connected ? 'Connected' : 'Offline'; }
    }

    startSimulation() {
        if (this.engine.simRunning) return;
        this.engine.simRunning = true;
        this.statsVisible = true;
        const statsOverlay = document.getElementById('statsOverlay');
        if (statsOverlay) statsOverlay.style.display = '';
        this.simLoop();
    }

    stopSimulation() {
        this.engine.simRunning = false;
        if (this._simLoopTimeout) { clearTimeout(this._simLoopTimeout); this._simLoopTimeout = null; }
    }

    simLoop() {
        if (!this.engine.simRunning) return;
        if (!this.engine.paused) {
            const now = performance.now();
            if (now - this.lastTickTime >= 500) {
                const dt = 1/60;
                try {
                    this.engine.tick(dt);
                    if (this.renderer) this.renderer.dirty = true;
                    this.interactive.tick(dt);
                    this.particles.update(dt);
                    if (this.physicsVisuals) this.physicsVisuals.update(dt);
                    this._skyUpdateCount = (this._skyUpdateCount || 0) + 1;
                    if (this.skybox && this._skyUpdateCount % 4 === 0) {
                        const tod = this.engine.getTimeOfDay();
                        const w = this.engine.getWeather();
                        const wn = this.engine.weatherNames[w.type] || 'clear';
                        this.skybox.update(dt, tod, wn);
                    }
                    this._waterUpdateCount = (this._waterUpdateCount || 0) + 1;
                    if (this.water && this._waterUpdateCount % 2 === 0) {
                        const tod = this.engine.getTimeOfDay();
                        const w = this.engine.getWeather();
                        const wn = this.engine.weatherNames[w.type] || 'clear';
                        this.water.update(dt, tod, wn);
                    }
                } catch (e) {
                    this.log('Tick error: ' + e.message, 'err');
                }
                this.lastTickTime = now;
            }
        }
        this._simLoopTimeout = setTimeout(() => this.simLoop(), 200);
    }

    updateStats() {
        const statBlocks = document.getElementById('statBlocks');
        if (!statBlocks) return;
        const s = this.engine.getWorldStats();
        statBlocks.textContent = s.totalBlocks || 0;
        const statTick = document.getElementById('statTick');
        if (statTick) statTick.textContent = s.currentTick || 0;
        const statTemp = document.getElementById('statTemp');
        if (statTemp) statTemp.textContent = (s.averageTemperature || 293.15).toFixed(1) + ' K';
        const statFPS = document.getElementById('statFPS');
        if (statFPS) statFPS.textContent = this.engine.fps;
        const tod = this.engine.getTimeOfDay();
        const statTimeOfDay = document.getElementById('statTimeOfDay');
        if (statTimeOfDay) statTimeOfDay.textContent = this.formatTime(tod);
        const w = this.engine.getWeather();
        const statWeather = document.getElementById('statWeather');
        if (statWeather) statWeather.textContent = this.engine.weatherNames[w.type] || 'CLEAR';
        const statEntities = document.getElementById('statEntities');
        if (statEntities) statEntities.textContent = this.engine.getAgentCount();
        const timeDisplay = document.getElementById('timeDisplay');
        if (timeDisplay) timeDisplay.textContent = this.formatTimeFull(tod);
        const tickCount = document.getElementById('tickCount');
        if (tickCount) tickCount.textContent = 'Tick ' + (s.currentTick || 0);
        const ticks = s.currentTick || 0;
        const dayLength = DAY_LENGTH;
        const dayCount = document.getElementById('dayCount');
        if (dayCount) dayCount.textContent = 'Day ' + (Math.floor(ticks / dayLength) + 1);
    }

    formatTime(h) {
        const hh = Math.floor(h) % 24;
        const mm = Math.floor((h - Math.floor(h)) * 60);
        return String(hh).padStart(2, '0') + ':' + String(mm).padStart(2, '0');
    }

    formatTimeFull(h) {
        const hh = Math.floor(h) % 24;
        const mm = Math.floor((h - Math.floor(h)) * 60);
        const ss = Math.floor(((h * 60) % 1) * 60);
        return String(hh).padStart(2, '0') + ':' + String(mm).padStart(2, '0') + ':' + String(ss).padStart(2, '0');
    }

    /* ====== TOP BAR ====== */
    bindTopBar() {
        document.querySelectorAll('.topbar-btn[data-panel]').forEach(btn => {
            btn.addEventListener('click', () => {
                document.querySelectorAll('.topbar-btn[data-panel]').forEach(b => b.classList.remove('active'));
                btn.classList.add('active');
                const panel = btn.dataset.panel;
                document.querySelectorAll('.console-main > .panel').forEach(p => p.classList.remove('active'));
                const target = document.getElementById('panel-' + panel);
                if (target) target.classList.add('active');
                this.activePanel = panel;
            });
        });
        document.querySelectorAll('.panel-collapse-btn').forEach(btn => {
            btn.addEventListener('click', () => {
                const panel = btn.closest('.panel');
                if (!panel) return;
                panel.classList.toggle('collapsed');
                btn.textContent = panel.classList.contains('collapsed') ? '+' : '−';
            });
        });
    }

    /* ====== PROMPT INPUT ====== */
    bindPrompt() {
        const input = document.getElementById('promptInput');
        const sendBtn = document.getElementById('promptSendBtn');
        const historyBtn = document.getElementById('promptHistoryBtn');

        if (sendBtn) sendBtn.addEventListener('click', () => this.sendPrompt());
        if (input) input.addEventListener('keydown', e => {
            if (e.key === 'Enter' && !e.shiftKey) { e.preventDefault(); this.sendPrompt(); }
            if (e.key === 'ArrowUp') { e.preventDefault(); this.navigateHistory(-1); }
            if (e.key === 'ArrowDown') { e.preventDefault(); this.navigateHistory(1); }
            if (e.key === 'Tab') { e.preventDefault(); this.autocomplete(); }
        });
        if (historyBtn) historyBtn.addEventListener('click', () => this.showHistoryDropdown());
    }

    sendPrompt() {
        const input = document.getElementById('promptInput');
        if (!input) return;
        const text = input.value.trim();
        if (!text) return;
        this.promptHistory.unshift(text);
        if (this.promptHistory.length > 50) this.promptHistory.pop();
        this.historyIndex = -1;
        input.value = '';
        this.addChatMessage('user', text);
        this.processPrompt(text).catch(e => this.addChatMessage('assistant', 'Error: ' + e.message));
    }

    async processPrompt(text) {
        const lower = text.toLowerCase();
        if (lower.startsWith('weather ')) {
            const w = lower.replace('weather ', '').trim();
            const map = { clear: 0, rain: 1, snow: 2, storm: 3, fog: 4 };
            if (map[w] !== undefined) {
                this.engine.setWeather(map[w]);
                this.sound.setWeather(w, this.renderer?.camera?.position?.x || 0, this.renderer?.camera?.position?.y || 0, this.renderer?.camera?.position?.z || 0);
                if (w === 'rain' || w === 'storm') {
                    this.particles.emitRain(-30, 30, -30, 30, 30);
                } else if (w === 'snow') {
                    this.particles.emitSnow(-30, 30, -30, 30, 25);
                }
                this.addChatMessage('assistant', 'Weather set to ' + w.toUpperCase());
            }
        } else if (lower.startsWith('time ')) {
            const t = parseFloat(lower.replace('time ', ''));
            if (!isNaN(t)) { this.engine.setTimeOfDay(t); this.addChatMessage('assistant', 'Time set to ' + this.formatTime(t)); }
        } else if (lower === 'pause' || lower === 'stop') {
            this.engine.paused = true;
            const btn = document.getElementById('playPauseBtn');
            if (btn) btn.textContent = '▶';
            this.addChatMessage('assistant', 'Simulation paused.');
        } else if (lower === 'resume' || lower === 'start' || lower === 'play') {
            this.engine.paused = false;
            const btn = document.getElementById('playPauseBtn');
            if (btn) btn.textContent = '⏸';
            this.addChatMessage('assistant', 'Simulation resumed.');
        } else if (lower.startsWith('speed ')) {
            const s = parseFloat(lower.replace('speed ', ''));
            if (!isNaN(s)) { this.engine.setTimeScale(s); this.addChatMessage('assistant', 'Speed set to ' + s + 'x'); }
        } else if (lower === 'help') {
            this.addChatMessage('assistant', 'Commands: weather <type>, time <hours>, pause, resume, speed <n>, save <name>, load <name>, worlds, new, delete <name>, help. Or describe a world to generate.');
        } else if (lower.startsWith('save ')) {
            const name = text.substring(5).trim() || 'untitled';
            this.saveWorld(name);
        } else if (lower === 'save') {
            this.saveWorld('autosave');
        } else if (lower.startsWith('load ')) {
            const name = text.substring(5).trim();
            this.loadWorld(name);
        } else if (lower === 'worlds' || lower === 'list') {
            this.listWorlds();
        } else if (lower === 'new') {
            this.newWorld();
        } else if (lower.startsWith('delete ')) {
            const name = text.substring(7).trim();
            this.deleteWorld(name);
        } else if (lower === 'export' || lower.startsWith('export ')) {
            const name = text.substring(7).trim() || 'export';
            const result = await this.worldIO.exportOMW(name);
            if (result.success) {
                this.addChatMessage('assistant', 'Exported .omw file (' + result.blocks + ' blocks).');
            } else {
                this.addChatMessage('assistant', 'Export failed: ' + result.error);
            }
        } else if (lower.startsWith('import')) {
            const input = document.createElement('input');
            input.type = 'file';
            input.accept = '.omw';
            input.onchange = async (e) => {
                const file = e.target.files[0];
                if (!file) return;
                const result = await this.worldIO.importOMW(file);
                if (result.success) {
                    this.addChatMessage('assistant', 'Imported "' + result.name + '" (' + result.blocks + ' blocks).');
                } else {
                    this.addChatMessage('assistant', 'Import failed: ' + result.error);
                }
            };
            input.click();
        } else {
            this.addChatMessage('assistant', 'Generating: "' + text + '"...');
            if (this.engine.wasmReady) {
                if (this.llm.apiKey) {
                    const result = await this.llm.generate(text);
                    if (result.success) {
                        const placed = await this.llm.executeBlocks(result.blocks);
                        this.engine.tick(0.1);
                        this.addChatMessage('assistant', 'Generated ' + placed + ' blocks via ' + this.llm.provider + '.');
                        this.updateStats();
                    } else {
                        this.addChatMessage('assistant', 'LLM error: ' + result.error);
                    }
                } else {
                    const result = this.engine.generateFromPrompt(text);
                    this.engine.tick(0.1);
                    const stats = this.engine.getWorldStats();
                    this.addChatMessage('assistant', 'Generated ' + (stats.totalBlocks || 0) + ' blocks (local mode). Add API key in Brain panel for AI generation.');
                    this.updateStats();
                }
            } else {
                this.addChatMessage('assistant', 'WASM engine not connected.');
            }
        }
    }

    async saveWorld(name) {
        if (!this.engine.wasmReady) { this.addChatMessage('assistant', 'WASM not connected.'); return; }
        try {
            const result = await this.worldIO.save(name);
            if (result.success) {
                this.addChatMessage('assistant', 'World saved as "' + name + '" (' + result.blocks + ' blocks).');
            } else {
                this.addChatMessage('assistant', 'Save failed: ' + result.error);
            }
        } catch (e) { this.addChatMessage('assistant', 'Save error: ' + e.message); }
    }

    async loadWorld(name) {
        try {
            const result = await this.worldIO.load(name);
            if (result.success) {
                this.addChatMessage('assistant', 'Loaded "' + name + '" (' + result.blocks + ' blocks).');
                if (this.renderer) { this.renderer.updateFromWASM(); this.renderer.rebuildMesh(); }
            } else {
                this.addChatMessage('assistant', 'Load failed: ' + result.error);
            }
        } catch (e) { this.addChatMessage('assistant', 'Load error: ' + e.message); }
    }

    async listWorlds() {
        try {
            const worlds = await this.worldIO.list();
            if (worlds.length === 0) { this.addChatMessage('assistant', 'No saved worlds.'); return; }
            const list = worlds.map(w => w.name + ' (' + w.blockCount + ' blocks, ' + new Date(w.savedAt).toLocaleDateString() + ')').join('\n');
            this.addChatMessage('assistant', 'Saved worlds:\n' + list);
        } catch (e) { this.addChatMessage('assistant', 'List error: ' + e.message); }
    }

    newWorld() {
        try {
            this.worldIO.newWorld();
            if (this.renderer) { this.renderer.updateFromWASM(); this.renderer.rebuildMesh(); }
            this.addChatMessage('assistant', 'New empty world created.');
        } catch (e) { this.addChatMessage('assistant', 'New world error: ' + e.message); }
    }

    async deleteWorld(name) {
        try {
            const result = await this.worldIO.remove(name);
            if (result.success) {
                this.addChatMessage('assistant', 'Deleted "' + name + '".');
            } else {
                this.addChatMessage('assistant', 'Delete failed.');
            }
        } catch (e) { this.addChatMessage('assistant', 'Delete error: ' + e.message); }
    }

    addChatMessage(role, text) {
        const chat = document.getElementById('promptChat');
        if (!chat) return;
        const div = document.createElement('div');
        div.className = 'chat-message ' + role;
        const avatar = role === 'user' ? 'You' : role === 'assistant' ? 'AI' : 'SYS';
        const bubble = document.createElement('div');
        bubble.className = 'chat-bubble';
        bubble.textContent = typeof text === 'string' ? text : String(text);
        const avatarDiv = document.createElement('div');
        avatarDiv.className = 'chat-avatar';
        avatarDiv.textContent = avatar;
        div.appendChild(avatarDiv);
        div.appendChild(bubble);
        chat.appendChild(div);
        chat.scrollTop = chat.scrollHeight;
        while (chat.children.length > 200) chat.removeChild(chat.firstChild);
    }

    _escapeHtml(str) {
        if (str === null || str === undefined) return '';
        if (typeof OMUtils !== 'undefined') return OMUtils.escapeHtml(str);
        const d = document.createElement('div');
        d.textContent = String(str);
        return d.innerHTML;
    }

    _safeHexColor(str, fallback) {
        if (typeof OMUtils !== 'undefined') return OMUtils.safeHexColor(str, fallback);
        return (typeof str === 'string' && /^#([0-9a-fA-F]{3}|[0-9a-fA-F]{6})$/.test(str)) ? str : (fallback || '#ffffff');
    }

    navigateHistory(dir) {
        if (this.promptHistory.length === 0) return;
        this.historyIndex = Math.max(-1, Math.min(this.historyIndex + dir, this.promptHistory.length - 1));
        const input = document.getElementById('promptInput');
        if (input) input.value = this.historyIndex >= 0 ? this.promptHistory[this.historyIndex] : '';
    }

    autocomplete() {
        const input = document.getElementById('promptInput');
        if (!input) return;
        const val = input.value.toLowerCase();
        const match = this.suggestions.find(s => s.toLowerCase().startsWith(val) && s.toLowerCase() !== val);
        if (match) input.value = match;
    }

    renderSuggestions() {
        const container = document.getElementById('promptSuggestions');
        if (!container) return;
        container.innerHTML = '';
        this.suggestions.slice(0, 4).forEach(s => {
            const chip = document.createElement('button');
            chip.className = 'suggestion-chip';
            chip.textContent = s;
            chip.addEventListener('click', () => {
                const inp = document.getElementById('promptInput');
                if (inp) inp.value = s;
                this.sendPrompt();
            });
            container.appendChild(chip);
        });
    }

    showHistoryDropdown() {
        if (this.promptHistory.length === 0) return;
        const input = document.getElementById('promptInput');
        if (input) input.value = this.promptHistory[0];
    }

    /* ====== TIME CONTROLS ====== */
    bindTime() {
        const playPauseBtn = document.getElementById('playPauseBtn');
        if (playPauseBtn) playPauseBtn.addEventListener('click', () => {
            this.engine.paused = !this.engine.paused;
            if (playPauseBtn) playPauseBtn.textContent = this.engine.paused ? '▶' : '⏸';
            this.addChatMessage('assistant', this.engine.paused ? 'Simulation paused.' : 'Simulation resumed.');
        });
        const rewind = (secs) => { this.engine.rewindTime(secs); this.addChatMessage('assistant', 'Rewound ' + secs + 's'); };
        const ff = (secs) => { const t = this.engine.getTimeOfDay() + (secs/3600); this.engine.setTimeOfDay(t % 24); this.addChatMessage('assistant', 'Fast-forwarded +' + secs + 's'); };
        const rewind10s = document.getElementById('rewind10s');
        if (rewind10s) rewind10s.addEventListener('click', () => rewind(10));
        const rewind1m = document.getElementById('rewind1m');
        if (rewind1m) rewind1m.addEventListener('click', () => rewind(60));
        const rewind5m = document.getElementById('rewind5m');
        if (rewind5m) rewind5m.addEventListener('click', () => rewind(300));
        const ff10s = document.getElementById('ff10s');
        if (ff10s) ff10s.addEventListener('click', () => ff(10));
        const ff1m = document.getElementById('ff1m');
        if (ff1m) ff1m.addEventListener('click', () => ff(60));
        const ff5m = document.getElementById('ff5m');
        if (ff5m) ff5m.addEventListener('click', () => ff(300));

        const slider = document.getElementById('speedSlider');
        if (slider) slider.addEventListener('input', () => {
            const idx = parseInt(slider.value);
            const s = this.engine.speedValues[idx];
            this.engine.setTimeScale(s);
            const speedLabel = document.getElementById('speedLabel');
            if (speedLabel) speedLabel.textContent = this.speedLabels[idx];
        });

        document.querySelectorAll('.time-presets .preset-btn').forEach(btn => {
            btn.addEventListener('click', () => {
                const t = parseFloat(btn.dataset.time);
                this.engine.setTimeOfDay(t);
                this.addChatMessage('assistant', 'Time: ' + btn.textContent);
            });
        });

        const cycleSlider = document.getElementById('cycleDurationSlider');
        if (cycleSlider) cycleSlider.addEventListener('input', () => {
            const v = parseInt(cycleSlider.value);
            this.engine.setCycleDuration(v);
            const label = document.getElementById('cycleDurationLabel');
            if (label) label.textContent = v >= 60 ? (v/60).toFixed(0) + ' min' : v + 's';
        });
    }

    /* ====== VISUAL OVERLAYS ====== */
    bindOverlays() {
        const legendLabels = {
            stress: { min: 'Low', max: 'High', gradient: 'linear-gradient(90deg, #22c55e, #eab308, #ef4444)' },
            temperature: { min: 'Cold', max: 'Hot', gradient: 'linear-gradient(90deg, #3b82f6, #f97316)' },
            radiation: { min: 'Safe', max: 'Danger', gradient: 'linear-gradient(90deg, #a855f7, #ec4899)' },
            density: { min: 'Low', max: 'High', gradient: 'linear-gradient(90deg, #64748b, #e2e8f0)' },
            thoughts: { min: '', max: '', gradient: 'linear-gradient(90deg, #06b6d4, #6366f1)' }
        };
        const toggle = (id, type) => {
            const el = document.getElementById(id);
            if (el) el.addEventListener('change', e => {
                this.engine.setOverlay(type, e.target.checked);
                const legend = document.getElementById('overlayLegend');
                if (legend) {
                    if (e.target.checked) {
                        legend.style.display = '';
                        const l = legendLabels[type] || legendLabels.stress;
                        const bar = document.getElementById('legendBar');
                        const minEl = document.getElementById('legendMin');
                        const maxEl = document.getElementById('legendMax');
                        if (bar) bar.style.background = l.gradient;
                        if (minEl) minEl.textContent = l.min;
                        if (maxEl) maxEl.textContent = l.max;
                    } else {
                        legend.style.display = 'none';
                    }
                }
            });
        };
        toggle('overlayStress', 'stress');
        toggle('overlayTemp', 'temperature');
        toggle('overlayRadiation', 'radiation');
        toggle('overlayThoughts', 'thoughts');

        const overlayEcosystem = document.getElementById('overlayEcosystem');
        if (overlayEcosystem) overlayEcosystem.addEventListener('change', e => {
            const g = document.getElementById('ecoGraphOverlay');
            if (g) g.style.display = e.target.checked ? '' : 'none';
            if (e.target.checked) this.startEcoGraph();
        });
        toggle('overlayDensity', 'density');

        const ecoGraphClose = document.getElementById('ecoGraphClose');
        if (ecoGraphClose) ecoGraphClose.addEventListener('click', () => {
            if (this._ecoGraphRaf) cancelAnimationFrame(this._ecoGraphRaf);
            this._ecoGraphRunning = false;
            const g = document.getElementById('ecoGraphOverlay');
            if (g) g.style.display = 'none';
            const overlay = document.getElementById('overlayEcosystem');
            if (overlay) overlay.checked = false;
        });
    }

    startEcoGraph() {
        if (this._ecoGraphRaf) cancelAnimationFrame(this._ecoGraphRaf);
        this._ecoGraphRunning = true;
        const canvas = document.getElementById('ecoGraphCanvas');
        if (!canvas) return;
        const ctx = canvas.getContext('2d');
        const data = { predators: [], prey: [] };
        const draw = () => {
            const ecoOverlay = document.getElementById('ecoGraphOverlay');
            if (!this._ecoGraphRunning || !ecoOverlay || ecoOverlay.style.display === 'none') return;
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            let predCount = 0, preyCount = 0;
            if (this.engine.wasmReady) {
                const count = this.engine.getAgentCount();
                for (let i = 0; i < count; i++) {
                    const a = this.engine.getAgentData(i);
                    if (!a || !a.isAlive) continue;
                    if (a.isPredator) predCount++; else preyCount++;
                }
            }
            data.predators.push(predCount * 15 + 10);
            data.prey.push(preyCount * 15 + 10);
            if (data.predators.length > 150) { data.predators.shift(); data.prey.shift(); }
            ctx.beginPath();
            data.predators.forEach((v, i) => { i === 0 ? ctx.moveTo(i, v) : ctx.lineTo(i, v); });
            ctx.strokeStyle = '#ef4444'; ctx.lineWidth = 1.5; ctx.stroke();
            ctx.beginPath();
            data.prey.forEach((v, i) => { i === 0 ? ctx.moveTo(i, v) : ctx.lineTo(i, v); });
            ctx.strokeStyle = '#4ade80'; ctx.lineWidth = 1.5; ctx.stroke();
            ctx.fillStyle = '#ef4444'; ctx.fillRect(4, 4, 8, 8);
            ctx.fillStyle = '#94a3b8'; ctx.font = '9px Inter';
            ctx.fillText('Predators: ' + predCount, 16, 12);
            ctx.fillStyle = '#4ade80'; ctx.fillRect(4, 16, 8, 8);
            ctx.fillStyle = '#94a3b8'; ctx.fillText('Prey: ' + preyCount, 16, 24);
            this._ecoGraphRaf = requestAnimationFrame(draw);
        };
        draw();
    }

    /* ====== AGENT DASHBOARD ====== */
    bindAgents() {
        this.refreshAgents();
        const agentCloseBtn = document.getElementById('agentCloseBtn');
        if (agentCloseBtn) agentCloseBtn.addEventListener('click', () => {
            const inspector = document.getElementById('agentInspector');
            if (inspector) inspector.style.display = 'none';
            this.selectedAgent = -1;
            document.querySelectorAll('.agent-card').forEach(c => c.classList.remove('selected'));
        });
        if (this.agentInterval) clearInterval(this.agentInterval);
        this.agentInterval = setInterval(() => this.refreshAgents(), 5000);
    }

    refreshAgents() {
        const list = document.getElementById('agentList');
        if (!list) return;
        const count = this.engine.getAgentCount();
        if (count === 0) { list.innerHTML = '<div class="agent-empty">No agents in world</div>'; return; }
        list.innerHTML = '';
        for (let i = 0; i < count; i++) {
            const a = this.engine.getAgentData(i);
            if (!a || !a.exists) continue;
            const role = a.isPredator ? 'predator' : (a.isDiseased ? 'diseased' : 'prey');
            const card = document.createElement('div');
            card.className = 'agent-card' + (this.selectedAgent === i ? ' selected' : '');
            card.innerHTML = `<div class="agent-avatar-sm">${String.fromCharCode(65 + (a.id || i) % 26)}</div><div class="agent-info"><div class="agent-card-name">Agent #${this._escapeHtml(String(a.id !== undefined ? a.id : i))}</div><div class="agent-card-role">${this._escapeHtml(role)}</div></div><div class="agent-card-hp">HP:${Math.round(a.health || 0)}</div>`;
            card.addEventListener('click', () => this.inspectAgent(i));
            list.appendChild(card);
        }
    }

    inspectAgent(idx) {
        this.selectedAgent = idx;
        const a = this.engine.getAgentData(idx);
        if (!a || !a.exists) return;
        const inspector = document.getElementById('agentInspector');
        if (inspector) inspector.style.display = '';
        const name = 'Agent #' + (a.id !== undefined ? a.id : idx);
        const role = a.isPredator ? 'predator' : (a.isDiseased ? 'diseased' : 'prey');
        const agentAvatar = document.getElementById('agentAvatar');
        if (agentAvatar) agentAvatar.textContent = name[0].toUpperCase();
        const agentName = document.getElementById('agentName');
        if (agentName) agentName.textContent = name;
        const agentRole = document.getElementById('agentRole');
        if (agentRole) agentRole.textContent = role + (a.isAlive ? '' : ' (dead)');
        const agentHealth = document.getElementById('agentHealth');
        if (agentHealth) agentHealth.style.width = (a.health || 0) + '%';
        const agentHealthVal = document.getElementById('agentHealthVal');
        if (agentHealthVal) agentHealthVal.textContent = Math.round(a.health || 0);
        const agentHunger = document.getElementById('agentHunger');
        if (agentHunger) agentHunger.style.width = (a.hunger || 0) + '%';
        const agentHungerVal = document.getElementById('agentHungerVal');
        if (agentHungerVal) agentHungerVal.textContent = Math.round(a.hunger || 0);
        const agentEnergy = document.getElementById('agentEnergy');
        if (agentEnergy) agentEnergy.style.width = (a.energy || 0) + '%';
        const agentEnergyVal = document.getElementById('agentEnergyVal');
        if (agentEnergyVal) agentEnergyVal.textContent = Math.round(a.energy || 0);
        const agentPosition = document.getElementById('agentPosition');
        if (agentPosition) agentPosition.textContent = Math.round(a.x||0) + ', ' + Math.round(a.y||0) + ', ' + Math.round(a.z||0);
        const agentGoal = document.getElementById('agentGoal');
        if (agentGoal) agentGoal.textContent = a.isAlive ? (a.isPredator ? 'Hunt prey' : 'Find food') : 'Dead';

        const memDiv = document.getElementById('agentMemories');
        if (memDiv) {
            memDiv.innerHTML = '';
            const mems = ['Position: ' + Math.round(a.x||0) + ',' + Math.round(a.y||0) + ',' + Math.round(a.z||0)];
            if (a.vx || a.vz) mems.push('Velocity: ' + (a.vx||0).toFixed(1) + ',' + (a.vy||0).toFixed(1) + ',' + (a.vz||0).toFixed(1));
            if (a.isDiseased) mems.push('Status: Diseased');
            if (!a.isAlive) mems.push('Status: Dead');
            mems.forEach(m => { const d = document.createElement('div'); d.className = 'memory-entry'; d.textContent = m; memDiv.appendChild(d); });
        }

        const agentThoughts = document.getElementById('agentThoughts');
        if (agentThoughts) agentThoughts.textContent = a.isAlive ? (a.isPredator ? 'Looking for prey nearby...' : 'Scanning for food and safety.') : 'No longer active.';

        const relDiv = document.getElementById('agentRelationships');
        if (relDiv) {
            relDiv.innerHTML = '';
            const relType = a.isPredator ? 'enemy' : 'friend';
            const chip = document.createElement('span');
            chip.className = 'rel-chip ' + relType;
            chip.textContent = (a.isPredator ? 'Prey' : 'Allies') + ' (' + relType + ')';
            relDiv.appendChild(chip);
        }

        const invDiv = document.getElementById('agentInventory');
        if (invDiv) {
            invDiv.innerHTML = '';
            const invChip = document.createElement('span');
            invChip.className = 'inv-chip';
            invChip.textContent = a.isPredator ? 'Claws' : 'Food stores';
            invDiv.appendChild(invChip);
        }

        document.querySelectorAll('.agent-card').forEach((c, i) => {
            c.classList.toggle('selected', i === idx);
        });
    }

    /* ====== ENVIRONMENT PRESETS ====== */
    bindEnvironment() {
        document.querySelectorAll('.env-card').forEach(card => {
            card.addEventListener('click', () => {
                document.querySelectorAll('.env-card').forEach(c => c.classList.remove('active'));
                card.classList.add('active');
                this.currentEnv = card.dataset.preset;
                const p = this.engine.envPresets[this.currentEnv];
                const envCustomSliders = document.getElementById('envCustomSliders');
                if (envCustomSliders) envCustomSliders.style.display = this.currentEnv === 'custom' ? '' : 'none';
                const envInfoGravity = document.getElementById('envInfoGravity');
                if (envInfoGravity) envInfoGravity.textContent = p.gravity + ' m/s²';
                const envInfoAir = document.getElementById('envInfoAir');
                if (envInfoAir) envInfoAir.textContent = p.airDensity + ' kg/m³';
                const envInfoTemp = document.getElementById('envInfoTemp');
                if (envInfoTemp) envInfoTemp.textContent = p.temperature + ' K';
                if (this.currentEnv === 'custom') {
                    const envGravity = document.getElementById('envGravity');
                    if (envGravity) envGravity.value = p.gravity;
                    const envGravityVal = document.getElementById('envGravityVal');
                    if (envGravityVal) envGravityVal.textContent = p.gravity;
                    const envAirDensity = document.getElementById('envAirDensity');
                    if (envAirDensity) envAirDensity.value = p.airDensity;
                    const envAirDensityVal = document.getElementById('envAirDensityVal');
                    if (envAirDensityVal) envAirDensityVal.textContent = p.airDensity;
                    const envTemp = document.getElementById('envTemp');
                    if (envTemp) envTemp.value = p.temperature;
                    const envTempVal = document.getElementById('envTempVal');
                    if (envTempVal) envTempVal.textContent = p.temperature.toFixed(1);
                }
            });
        });

        ['envGravity', 'envAirDensity', 'envTemp', 'envHumidity'].forEach(id => {
            const el = document.getElementById(id);
            const valEl = document.getElementById(id + 'Val');
            if (el) el.addEventListener('input', e => {
                if (valEl) valEl.textContent = parseFloat(e.target.value).toFixed(id === 'envHumidity' ? 0 : id === 'envTemp' ? 1 : 2);
            });
        });

        const envApplyBtn = document.getElementById('envApplyBtn');
        if (envApplyBtn) envApplyBtn.addEventListener('click', () => {
            const p = this.engine.envPresets[this.currentEnv];
            if (this.engine.wasmReady) {
                this.engine.setGravity(p.gravity);
                this.engine.setAmbientTemperature(p.temperature);
                this.addChatMessage('assistant', 'Applied ' + this.currentEnv + ': gravity=' + p.gravity + ', temp=' + p.temperature + 'K');
            } else {
                this.addChatMessage('assistant', 'WASM not connected.');
            }
        });
    }

    /* ====== MATERIAL FORGE ====== */
    bindForge() {
        const forgeGenerateBtn = document.getElementById('forgeGenerateBtn');
        if (forgeGenerateBtn) forgeGenerateBtn.addEventListener('click', () => this.generateMaterial());
        const forgeInput = document.getElementById('forgeInput');
        if (forgeInput) forgeInput.addEventListener('keydown', e => {
            if (e.key === 'Enter' && (e.ctrlKey || e.metaKey)) { e.preventDefault(); this.generateMaterial(); }
        });
    }

    async generateMaterial() {
        const forgeInput = document.getElementById('forgeInput');
        const input = forgeInput ? forgeInput.value.trim() : '';
        if (!input) return;
        const forgeLoading = document.getElementById('forgeLoading');
        const forgeResult = document.getElementById('forgeResult');
        if (forgeLoading) forgeLoading.style.display = '';
        if (forgeResult) forgeResult.style.display = 'none';
        this.addChatMessage('assistant', 'Generating material: ' + input.substring(0, 50) + '...');

        if (this.llm.apiKey) {
            try {
                const prompt = 'Generate a single custom material block for: "' + input + '". Return ONLY a JSON object with: name, color (hex), mass (kg), hardness (1-10), meltingPoint (K), density (kg/m3), tensileStrength (MPa), description. No explanation.';
                const result = await this.llm.generate(prompt);
                if (result.success && result.blocks && result.blocks[0]) {
                    const mat = result.blocks[0];
                    this.showMaterialResult({
                        name: mat.name || input.substring(0, 25),
                        color: mat.color || '#' + Math.floor(Math.random()*16777215).toString(16).padStart(6,'0'),
                        mass: mat.mass || '1.0',
                        hardness: mat.hardness || 5,
                        meltingPoint: mat.meltingPoint || 1000,
                        density: mat.density || 2000,
                        tensileStrength: mat.tensileStrength || 100,
                        description: mat.description || input
                    });
                } else {
                    this.addChatMessage('assistant', 'LLM material generation failed: ' + (result.error || 'invalid response'));
                    this.showMaterialResult(this.generateMaterialFromPrompt(input));
                }
            } catch (e) {
                this.addChatMessage('assistant', 'LLM error: ' + e.message);
                this.showMaterialResult(this.generateMaterialFromPrompt(input));
            }
        } else {
            setTimeout(() => {
                this.showMaterialResult(this.generateMaterialFromPrompt(input));
            }, 300);
        }
    }

    generateMaterialFromPrompt(desc) {
        let hash = 0;
        for (let i = 0; i < desc.length; i++) { hash = ((hash << 5) - hash + desc.charCodeAt(i)) | 0; }
        const seed = Math.abs(hash);
        const colors = ['#6366f1', '#ef4444', '#22c55e', '#f97316', '#06b6d4', '#a855f7', '#ec4899', '#14b8a6', '#f59e0b', '#8b5cf6'];
        const lower = desc.toLowerCase();
        let density = 1000 + (seed % 9000);
        let hardness = 1 + (seed % 9);
        let meltingPoint = 200 + (seed % 3000);
        let tensileStrength = 10 + (seed % 500);
        let mass = (density / 1000).toFixed(2);
        if (lower.includes('light')) { density = Math.floor(density * 0.3); mass = (density / 1000).toFixed(2); }
        if (lower.includes('heavy')) { density = Math.floor(density * 3); mass = (density / 1000).toFixed(2); }
        if (lower.includes('strong') || lower.includes('tough')) { tensileStrength = 400 + (seed % 600); hardness = 7 + (seed % 3); }
        if (lower.includes('soft')) { hardness = 1 + (seed % 3); }
        if (lower.includes('fire') || lower.includes('heat')) { meltingPoint = 2000 + (seed % 2000); }
        if (lower.includes('cold') || lower.includes('ice')) { meltingPoint = 200 + (seed % 300); }
        const name = desc.substring(0, 25).replace(/[^a-zA-Z0-9 ]/g, '').trim() || 'Custom Material';
        return {
            name, color: colors[seed % colors.length],
            mass, hardness, meltingPoint, density, tensileStrength,
            description: desc
        };
    }

    showMaterialResult(mat) {
        const forgeLoading = document.getElementById('forgeLoading');
        const forgeResult = document.getElementById('forgeResult');
        const materialSwatch = document.getElementById('materialSwatch');
        const materialProps = document.getElementById('materialProps');
        const forgeAddBtn = document.getElementById('forgeAddBtn');
        if (forgeLoading) forgeLoading.style.display = 'none';
        if (forgeResult) forgeResult.style.display = '';
        if (materialSwatch) materialSwatch.style.background = this._safeHexColor(mat.color, '#888888');
        if (materialProps) materialProps.innerHTML = `
            <div class="material-prop"><span>Name</span><span>${this._escapeHtml(mat.name)}</span></div>
            <div class="material-prop"><span>Mass</span><span>${this._escapeHtml(mat.mass)} kg</span></div>
            <div class="material-prop"><span>Hardness</span><span>${this._escapeHtml(String(mat.hardness))}</span></div>
            <div class="material-prop"><span>Melting Pt</span><span>${this._escapeHtml(String(mat.meltingPoint))} K</span></div>
            <div class="material-prop"><span>Density</span><span>${this._escapeHtml(String(mat.density))} kg/m³</span></div>
            <div class="material-prop"><span>Tensile</span><span>${this._escapeHtml(String(mat.tensileStrength))} MPa</span></div>
        `;
        if (forgeAddBtn) forgeAddBtn.onclick = () => {
            if (!this.engine.wasmReady) { this.addChatMessage('assistant', 'WASM not connected.'); return; }
            const props = JSON.stringify({
                mass: parseFloat(mat.mass), density: mat.density, hardness: parseFloat(mat.hardness),
                tensileStrength: mat.tensileStrength, meltingPoint: mat.meltingPoint, baseColor: mat.color
            });
            this.engine.setBlock(8, 4, 8, 255, props);
            this.engine.tick(0.1);
            this.addChatMessage('assistant', 'Material "' + mat.name + '" placed at (8,4,8).');
            this.updateStats();
        };
    }

    renderForgeHistory() {
        const list = document.getElementById('forgeHistory');
        if (!list) return;
        list.innerHTML = '';
        this.forgeHistory.forEach(m => {
            const item = document.createElement('div');
            item.className = 'forge-history-item';
            item.innerHTML = `<div class="forge-history-swatch" style="background:${this._safeHexColor(m.color, '#888888')}"></div><div class="forge-history-name">${this._escapeHtml(m.name)}</div>`;
            item.addEventListener('click', () => this.showMaterialResult(m));
            list.appendChild(item);
        });
    }

    /* ====== EXPORT HUB ====== */
    bindExport() {
        document.querySelectorAll('.export-btn').forEach(btn => {
            btn.addEventListener('click', () => {
                const fmt = btn.dataset.format;
                if (!this.engine.wasmReady) { this.addChatMessage('assistant', 'WASM not connected.'); return; }
                this.addChatMessage('assistant', 'Exporting as .' + fmt.toUpperCase() + '...');
                const progress = document.getElementById('exportProgress');
                if (progress) progress.style.display = '';
                if (this._exportProgressInterval) clearInterval(this._exportProgressInterval);
                let pct = 0;
                this._exportProgressInterval = setInterval(() => {
                    pct += 20;
                    if (pct >= 100) {
                        pct = 100; clearInterval(this._exportProgressInterval); this._exportProgressInterval = null;
                        try {
                            if (fmt === 'csv') {
                                const meta = this.engine.exportCSV('world.csv');
                                const csv = this.engine.getLastExportCSV();
                                if (csv) {
                                    const blob = new Blob([csv], { type: 'text/csv' });
                                    const url = URL.createObjectURL(blob);
                                    const a = document.createElement('a'); a.href = url; a.download = 'world.csv';
                                    document.body.appendChild(a); a.click(); document.body.removeChild(a);
                                    URL.revokeObjectURL(url);
                                    this.addChatMessage('assistant', 'Exported CSV (' + csv.split('\n').length + ' rows).');
                                } else {
                                    this.addChatMessage('assistant', 'CSV export failed.');
                                }
                            } else if (fmt === 'gltf') {
                                const meta = this.engine.exportGLTF('world.gltf');
                                const gltf = this.engine.getLastExportGLTF();
                                if (gltf) {
                                    const blob = new Blob([gltf], { type: 'model/gltf+json' });
                                    const url = URL.createObjectURL(blob);
                                    const a = document.createElement('a'); a.href = url; a.download = 'world.gltf';
                                    document.body.appendChild(a); a.click(); document.body.removeChild(a);
                                    URL.revokeObjectURL(url);
                                    this.addChatMessage('assistant', 'Exported glTF.');
                                } else {
                                    this.addChatMessage('assistant', 'glTF export failed.');
                                }
                            } else if (fmt === 'omw') {
                                const json = this.engine.saveWorld();
                                if (json) {
                                    const blob = new Blob([json], { type: 'application/json' });
                                    const url = URL.createObjectURL(blob);
                                    const a = document.createElement('a'); a.href = url; a.download = 'world.omw';
                                    document.body.appendChild(a); a.click(); document.body.removeChild(a);
                                    URL.revokeObjectURL(url);
                                    this.addChatMessage('assistant', 'Exported OpenMind World (.omw).');
                                } else {
                                    this.addChatMessage('assistant', 'World save failed.');
                                }
                            } else {
                                this.addChatMessage('assistant', '.' + fmt.toUpperCase() + ' export not yet supported. Use CSV, glTF, or OMW.');
                            }
                        } catch (e) {
                            this.addChatMessage('assistant', 'Export error: ' + e.message);
                        }
                        setTimeout(() => { if (progress) progress.style.display = 'none'; }, 500);
                    }
                    const exportProgressFill = document.getElementById('exportProgressFill');
                    const exportProgressText = document.getElementById('exportProgressText');
                    if (exportProgressFill) exportProgressFill.style.width = pct + '%';
                    if (exportProgressText) exportProgressText.textContent = Math.round(pct) + '% — Exporting...';
                }, 100);
            });
        });
    }

    /* ====== TELEPORT ====== */
    bindTeleport() {
        const tpGoBtn = document.getElementById('tpGoBtn');
        if (tpGoBtn) tpGoBtn.addEventListener('click', () => {
            const x = parseFloat(document.getElementById('tpX')?.value) || 0;
            const y = parseFloat(document.getElementById('tpY')?.value) || 0;
            const z = parseFloat(document.getElementById('tpZ')?.value) || 0;
            this.engine.teleportCamera(x, y, z);
            this.addChatMessage('assistant', 'Camera teleported to ' + x + ',' + y + ',' + z);
        });
        const saveBookmarkBtn = document.getElementById('saveBookmarkBtn');
        if (saveBookmarkBtn) saveBookmarkBtn.addEventListener('click', () => {
            const x = parseFloat(document.getElementById('tpX')?.value) || 0;
            const y = parseFloat(document.getElementById('tpY')?.value) || 0;
            const z = parseFloat(document.getElementById('tpZ')?.value) || 0;
            const name = 'Bookmark ' + (this.bookmarks.length + 1);
            this.bookmarks.push({ name, x, y, z });
            this.renderBookmarks();
            this.addChatMessage('assistant', 'Saved bookmark: ' + name + ' at ' + x + ',' + y + ',' + z);
        });
        const freeflyToggle = document.getElementById('freeflyToggle');
        if (freeflyToggle) freeflyToggle.addEventListener('change', (e) => {
            this.freefly = e.target.checked;
            this.addChatMessage('assistant', e.target.checked ? 'Freefly camera enabled.' : 'Freefly camera disabled.');
        });
    }

    renderBookmarks() {
        const list = document.getElementById('bookmarkList');
        if (!list) return;
        if (this.bookmarks.length === 0) { list.innerHTML = '<div class="bookmark-empty">No bookmarks saved</div>'; return; }
        list.innerHTML = '';
        this.bookmarks.forEach((b, i) => {
            const item = document.createElement('div');
            item.className = 'bookmark-item';
            item.innerHTML = `<span class="bookmark-name">${this._escapeHtml(b.name)}</span><span class="bookmark-coords">${b.x},${b.y},${b.z}</span><span class="bookmark-del" data-idx="${i}">✕</span>`;
            item.querySelector('.bookmark-name').addEventListener('click', () => {
                const tpX = document.getElementById('tpX');
                const tpY = document.getElementById('tpY');
                const tpZ = document.getElementById('tpZ');
                if (tpX) tpX.value = b.x;
                if (tpY) tpY.value = b.y;
                if (tpZ) tpZ.value = b.z;
                this.engine.teleportCamera(b.x, b.y, b.z);
            });
            item.querySelector('.bookmark-del').addEventListener('click', (e) => {
                this.bookmarks.splice(parseInt(e.target.dataset.idx), 1);
                this.renderBookmarks();
            });
            list.appendChild(item);
        });
    }

    /* ====== AI BRAIN SWITCHER ====== */
    bindBrain() {
        const brainCloudBtn = document.getElementById('brainCloudBtn');
        if (brainCloudBtn) brainCloudBtn.addEventListener('click', () => {
            if (brainCloudBtn) brainCloudBtn.classList.add('active');
            const brainLocalBtn = document.getElementById('brainLocalBtn');
            if (brainLocalBtn) brainLocalBtn.classList.remove('active');
            const cloudSettings = document.getElementById('brainCloudSettings');
            if (cloudSettings) cloudSettings.style.display = '';
            const localSettings = document.getElementById('brainLocalSettings');
            if (localSettings) localSettings.style.display = 'none';
        });
        const brainLocalBtn = document.getElementById('brainLocalBtn');
        if (brainLocalBtn) brainLocalBtn.addEventListener('click', () => {
            if (brainLocalBtn) brainLocalBtn.classList.add('active');
            if (brainCloudBtn) brainCloudBtn.classList.remove('active');
            const localSettings = document.getElementById('brainLocalSettings');
            if (localSettings) localSettings.style.display = '';
            const cloudSettings = document.getElementById('brainCloudSettings');
            if (cloudSettings) cloudSettings.style.display = 'none';
        });
        const llmProvider = document.getElementById('llmProvider');
        if (llmProvider) llmProvider.addEventListener('change', e => {
            const models = {
                groq: ['llama-3.3-70b-versatile', 'llama-3.1-8b-instant', 'mixtral-8x7b-32768'],
                openai: ['gpt-4o', 'gpt-4o-mini', 'gpt-4-turbo'],
                anthropic: ['claude-3-opus', 'claude-3-sonnet', 'claude-3-haiku'],
                google: ['gemini-2.0-flash', 'gemini-1.5-pro', 'gemini-1.5-flash'],
                custom: []
            };
            const isCustom = e.target.value === 'custom';
            const modelRow = document.getElementById('cloudModelRow');
            const customModelRow = document.getElementById('cloudCustomModelRow');
            if (modelRow) { if (isCustom) modelRow.classList.add('hidden'); else modelRow.classList.remove('hidden'); }
            if (customModelRow) { if (isCustom) customModelRow.classList.remove('hidden'); else customModelRow.classList.add('hidden'); }
            const sel = document.getElementById('llmModel');
            if (sel) {
                sel.innerHTML = '';
                (models[e.target.value] || []).forEach(m => {
                    const opt = document.createElement('option');
                    opt.value = m; opt.textContent = m;
                    sel.appendChild(opt);
                });
            }
        });
        const brainTestBtn = document.getElementById('brainTestBtn');
        if (brainTestBtn) brainTestBtn.addEventListener('click', () => {
            const dot = document.getElementById('brainStatusDot');
            const text = document.getElementById('brainStatusText');
            if (dot) dot.style.background = 'var(--warning)';
            if (text) text.textContent = 'Testing...';
            const isLocal = document.getElementById('brainLocalBtn')?.classList.contains('active');
            const provider = isLocal ? document.getElementById('localProvider')?.value : document.getElementById('llmProvider')?.value;
            const endpoint = isLocal ? document.getElementById('localEndpoint')?.value : document.getElementById('cloudEndpoint')?.value || '';
            const apiKey = document.getElementById('llmApiKey')?.value || '';
            const model = isLocal ? document.getElementById('localModel')?.value : (provider === 'custom' ? document.getElementById('cloudCustomModel')?.value : document.getElementById('llmModel')?.value);

            if (isLocal) {
                fetch(endpoint + '/api/tags', { method: 'GET' }).then(r => {
                    if (r.ok) {
                        this.llm.setConfig('openai', '', endpoint + '/v1/chat/completions', model || 'local');
                        if (dot) dot.style.background = 'var(--success)';
                        if (text) text.textContent = 'Connected to ' + provider;
                    } else {
                        if (dot) dot.style.background = 'var(--error)';
                        if (text) text.textContent = 'Failed: ' + provider;
                    }
                }).catch(() => {
                    if (dot) dot.style.background = 'var(--error)';
                    if (text) text.textContent = provider + ' not reachable';
                });
            } else if (apiKey && apiKey.length > 5) {
                this.llm.setConfig(provider, apiKey, endpoint || '', model || '');
                if (dot) dot.style.background = 'var(--success)';
                if (text) text.textContent = 'Configured: ' + provider + (model ? ' / ' + model : '');
            } else {
                if (dot) dot.style.background = 'var(--error)';
                if (text) text.textContent = 'No API key provided';
            }
        });
    }

    /* ====== MISC ====== */
    bindMisc() {
        const darkModeToggle = document.getElementById('darkModeToggle');
        if (darkModeToggle) darkModeToggle.addEventListener('click', () => {
            this.lightMode = !this.lightMode;
            document.body.classList.toggle('light', this.lightMode);
            window.dispatchEvent(new CustomEvent('themechange', { detail: { light: this.lightMode } }));
        });
        const clearLogBtn = document.getElementById('clearLogBtn');
        if (clearLogBtn) clearLogBtn.addEventListener('click', () => {
            const body = document.getElementById('consoleLogBody');
            if (body) body.innerHTML = '';
        });
        window.addEventListener('renderer-message', (e) => {
            this.addChatMessage('assistant', e.detail);
        });
    }

    log(msg, type) {
        const body = document.getElementById('consoleLogBody');
        if (!body) return;
        const line = document.createElement('div');
        line.className = 'console-line';
        const now = new Date().toLocaleTimeString('en-US', { hour12: false, hour: '2-digit', minute: '2-digit', second: '2-digit' });
        const timeSpan = document.createElement('span');
        timeSpan.className = 'time';
        timeSpan.textContent = '[' + now + ']';
        const msgSpan = document.createElement('span');
        msgSpan.className = 'msg ' + (type || '');
        msgSpan.textContent = msg;
        line.appendChild(timeSpan);
        line.appendChild(msgSpan);
        body.appendChild(line);
        body.scrollTop = body.scrollHeight;
        if (body.children.length > 200) body.removeChild(body.firstChild);
    }

    /* ====== INTERACTIVE OBJECTS ====== */
    bindInteractive() {
        document.getElementById('intCreateBtn')?.addEventListener('click', () => {
            const type = document.getElementById('intCreateType')?.value || 'door';
            const x = parseInt(document.getElementById('intCreateX')?.value || '0');
            const y = parseInt(document.getElementById('intCreateY')?.value || '2');
            const z = parseInt(document.getElementById('intCreateZ')?.value || '0');
            const obj = this.interactive.create(type, x, y, z);
            if (this.engine?.wasmReady) this.engine.setBlock(x, y, z, this.getBlockForType(type));
            if (this.renderer) {
                this.renderer.updateFromWASM();
                this.renderer.rebuildMesh();
            }
            this.refreshInteractiveList();
            this.addChatMessage('assistant', `Created ${type} at ${x},${y},${z} (id: ${obj.id})`);
        });

        document.getElementById('intListRefresh')?.addEventListener('click', () => this.refreshInteractiveList());
    }

    refreshInteractiveList() {
        const list = document.getElementById('intObjectList');
        if (!list) return;
        list.innerHTML = '';
        const objs = this.interactive.getAll();
        if (objs.length === 0) {
            list.innerHTML = '<div style="color:var(--text-dim);font-size:12px">No interactive objects</div>';
            return;
        }
        for (const obj of objs) {
            const div = document.createElement('div');
            div.className = 'int-obj-row';
            div.style.cssText = 'display:flex;align-items:center;gap:6px;padding:4px 8px;background:var(--bg-alt);border-radius:6px;margin:3px 0;font-size:11px';
            const v = VisualDefs[obj.type];
            div.innerHTML = `
                <span style="color:${this._safeHexColor(this.colorToCSS(obj.getColor()), '#94a3b8')}">${this._escapeHtml(obj.type)}</span>
                <span style="color:var(--text-dim)">(${obj.x},${obj.y},${obj.z})</span>
                <span style="color:var(--success)">${this._escapeHtml(obj.state)}</span>
                <button class="int-interact-btn" data-id="${this._escapeHtml(String(obj.id))}" style="margin-left:auto;padding:2px 8px;border:1px solid var(--border);border-radius:4px;background:var(--bg);color:var(--text);cursor:pointer;font-size:10px">Interact</button>
                <button class="int-remove-btn" data-id="${this._escapeHtml(String(obj.id))}" style="padding:2px 6px;border:1px solid var(--error);border-radius:4px;background:var(--bg);color:var(--error);cursor:pointer;font-size:10px">X</button>
            `;
            list.appendChild(div);
        }
        list.querySelectorAll('.int-interact-btn').forEach(btn => {
            btn.addEventListener('click', () => {
                const result = this.interactive.interact(btn.dataset.id);
                const obj = this.interactive.get(btn.dataset.id);
                this.addChatMessage('assistant', obj ? `${obj.type}: ${obj.state}` : 'Object not found');
                if (this.renderer) {
                    this.renderer.updateFromWASM();
                    this.renderer.rebuildMesh();
                }
                this.refreshInteractiveList();
            });
        });
        list.querySelectorAll('.int-remove-btn').forEach(btn => {
            btn.addEventListener('click', () => {
                this.interactive.remove(btn.dataset.id);
                this.refreshInteractiveList();
            });
        });
    }

    getBlockForType(type) {
        const map = {
            DOOR: 30, BUTTON: 31, LEVER: 36, SWITCH: 36, CHEST: 35,
            LAMP: 34, PISTON: 38, CONVEYOR: 37, TRAPDOOR: 39, FIRE: 40,
            LOCK: 33, LAUNCHER: 32, SENSOR: 9, TIMER: 10, COMPUTER: 11
        };
        return map[type] || 1;
    }

    colorToCSS(hex) {
        if (typeof hex === 'string') return hex.startsWith('#') ? hex : '#' + hex;
        return '#' + hex.toString(16).padStart(6, '0');
    }

    /* ====== INVENTORY ====== */
    bindInventory() {
        this.renderInventory();
        if (this.inventory?.player?.on) this.inventory.player.on(() => this.renderInventory());
    }

    renderInventory() {
        const grid = document.getElementById('invGrid');
        if (!grid || !this.inventory?.player) return;
        grid.innerHTML = '';
        for (let i = 0; i < this.inventory.player.size; i++) {
            const slot = this.inventory.player.getSlot(i);
            const div = document.createElement('div');
            div.className = 'inv-slot';
            div.style.cssText = 'width:36px;height:36px;border:1px solid var(--border);border-radius:4px;background:var(--bg-alt);display:flex;align-items:flex-end;justify-content:center;font-size:9px;color:var(--text);cursor:pointer;position:relative';
            if (!slot.isEmpty) {
                const def = ItemDefs[slot.itemId];
                div.innerHTML = `<span style="font-size:16px">${def?.icon || '?'}</span><span style="position:absolute;bottom:1px;right:2px;font-size:8px;color:var(--text-dim)">${slot.count}</span>`;
            }
            if (i === this.inventory.player.selectedSlot) {
                div.style.borderColor = '#fff';
            }
            div.addEventListener('click', () => {
                this.inventory.player.selectSlot(i);
                this.renderInventory();
            });
            grid.appendChild(div);
        }
    }

    /* ====== CRAFTING ====== */
    bindCrafting() {
        document.getElementById('craftRefreshBtn')?.addEventListener('click', () => this.refreshCraftingList());
        document.getElementById('craftAiBtn')?.addEventListener('click', () => {
            const prompt = document.getElementById('craftAiPrompt')?.value || '';
            if (prompt.length < 3) return;
            try {
                const recipe = this.crafting.generateFromAI(prompt);
                if (!recipe) return;
                this.crafting.addRecipe(recipe);
                this.refreshCraftingList();
                this.addChatMessage('assistant', `Generated recipe: ${recipe.name} (${recipe.ingredients.map(i => i.item + ' x' + i.count).join(' + ')} → ${recipe.result} x${recipe.count})`);
            } catch (e) { this.log('Craft AI failed: ' + e.message, 'warn'); }
        });
    }

    refreshCraftingList() {
        const list = document.getElementById('craftRecipeList');
        if (!list) return;
        list.innerHTML = '';
        const recipes = this.crafting.getAvailableRecipes();
        for (const r of recipes) {
            const div = document.createElement('div');
            div.style.cssText = 'padding:6px 8px;background:var(--bg-alt);border-radius:6px;margin:3px 0;font-size:11px';
            const ings = r.ingredients.map(i => `${i.has >= i.count ? '✓' : '✗'} ${i.item} ${i.has}/${i.count}`).join(', ');
            div.innerHTML = `
                <div style="display:flex;justify-content:space-between;align-items:center">
                    <span style="color:var(--text)">${this._escapeHtml(r.name)} x${r.count}</span>
                    <button class="craft-btn" data-recipe="${this._escapeHtml(String(r.id))}" ${r.canCraft ? '' : 'disabled'} style="padding:2px 8px;border:1px solid ${r.canCraft ? 'var(--success)' : 'var(--border)'};border-radius:4px;background:${r.canCraft ? 'rgba(34,197,94,0.2)' : 'var(--bg)'};color:${r.canCraft ? 'var(--success)' : 'var(--text-dim)'};cursor:${r.canCraft ? 'pointer' : 'not-allowed'};font-size:10px">Craft</button>
                </div>
                <div style="color:var(--text-dim);font-size:10px;margin-top:2px">${this._escapeHtml(ings)}</div>
            `;
            list.appendChild(div);
        }
        list.querySelectorAll('.craft-btn:not([disabled])').forEach(btn => {
            btn.addEventListener('click', () => {
                const result = this.crafting.craft(btn.dataset.recipe);
                if (result) {
                    this.addChatMessage('assistant', `Crafted ${result.recipe.name} x${result.count}`);
                    this.refreshCraftingList();
                }
            });
        });
    }

    /* ====== VISUAL SYSTEMS ====== */
    bindVisual() {
        document.getElementById('toggleParticles')?.addEventListener('click', (e) => {
            this.particles.enabled = !this.particles.enabled;
            e.target.textContent = this.particles.enabled ? 'ON' : 'OFF';
            e.target.classList.toggle('active', this.particles.enabled);
        });
        document.getElementById('toggleSkybox')?.addEventListener('click', (e) => {
            if (this.skybox) { this.skybox.enabled = !this.skybox.enabled; this.skybox.setVisible(this.skybox.enabled); }
            e.target.textContent = this.skybox?.enabled ? 'ON' : 'OFF';
            e.target.classList.toggle('active', this.skybox?.enabled);
        });
        document.getElementById('toggleWater')?.addEventListener('click', (e) => {
            if (this.water) { this.water.enabled = !this.water.enabled; this.water.setVisible(this.water.enabled); }
            e.target.textContent = this.water?.enabled ? 'ON' : 'OFF';
            e.target.classList.toggle('active', this.water?.enabled);
        });
        document.getElementById('toggleBloom')?.addEventListener('click', (e) => {
            if (this.postProcessing) { this.postProcessing.setBloom(!this.postProcessing.bloomEnabled); }
            e.target.textContent = this.postProcessing?.bloomEnabled ? 'ON' : 'OFF';
            e.target.classList.toggle('active', this.postProcessing?.bloomEnabled);
        });
        document.getElementById('toggleFog')?.addEventListener('click', (e) => {
            if (this.postProcessing) { this.postProcessing.setFog(!this.postProcessing.fogEnabled); }
            e.target.textContent = this.postProcessing?.fogEnabled ? 'ON' : 'OFF';
            e.target.classList.toggle('active', this.postProcessing?.fogEnabled);
        });
        document.getElementById('toggleVignette')?.addEventListener('click', (e) => {
            if (this.postProcessing) { this.postProcessing.setVignette(!this.postProcessing.vignetteEnabled); }
            e.target.textContent = this.postProcessing?.vignetteEnabled ? 'ON' : 'OFF';
            e.target.classList.toggle('active', this.postProcessing?.vignetteEnabled);
        });
        document.getElementById('soundVolume')?.addEventListener('input', (e) => {
            if (this.sound) this.sound.setVolume(parseFloat(e.target.value));
            const label = document.getElementById('soundVolumeLabel');
            if (label) label.textContent = Math.round(parseFloat(e.target.value) * 100) + '%';
        });
        document.getElementById('toggleSound')?.addEventListener('click', (e) => {
            if (!this.sound) return;
            this.sound.enabled = !this.sound.enabled;
            e.target.textContent = this.sound.enabled ? 'ON' : 'OFF';
            e.target.classList.toggle('active', this.sound.enabled);
        });
        document.getElementById('bloomIntensity')?.addEventListener('input', (e) => {
            if (this.postProcessing) this.postProcessing.setBloomIntensity(parseFloat(e.target.value));
        });
        document.getElementById('fogDensity')?.addEventListener('input', (e) => {
            if (this.postProcessing) this.postProcessing.setFogRange(30, 200 - parseFloat(e.target.value) * 170);
        });
        document.getElementById('saturation')?.addEventListener('input', (e) => {
            if (this.postProcessing) this.postProcessing.setSaturation(parseFloat(e.target.value));
        });
        document.getElementById('contrast')?.addEventListener('input', (e) => {
            if (this.postProcessing) this.postProcessing.setContrast(parseFloat(e.target.value));
        });
        document.getElementById('temperature')?.addEventListener('input', (e) => {
            if (this.postProcessing) this.postProcessing.setTemperature(parseFloat(e.target.value));
        });
    }

    /* ====== SETTINGS ====== */
    bindSettings() {
        const panel = document.getElementById('panelSettings');
        if (panel) {
            this.settingsPanel.renderUI(panel);
            this.settingsPanel.bindUI(panel);
        }
        this.settingsPanel.on((e) => {
            if (e.type === 'change' && e.path === 'display.theme') {
                this.lightMode = e.value === 'light';
            }
        });

        document.getElementById('tutorialBtn')?.addEventListener('click', () => {
            if (this.tutorial) {
                this.tutorial.reset();
                this.tutorial.start();
            } else {
                this.addChatMessage('assistant', 'Tutorial not available (engine not loaded).');
            }
        });
        document.getElementById('settingsBtn')?.addEventListener('click', () => {
            document.querySelectorAll('.topbar-btn[data-panel]').forEach(b => b.classList.remove('active'));
            document.querySelector('[data-panel="settings"]')?.classList.add('active');
            document.querySelectorAll('.console-main > .panel').forEach(p => p.classList.remove('active'));
            document.getElementById('panel-settings')?.classList.add('active');
        });
    }

    /* ====== BUILDING TOOLS ====== */
    bindBuildingTools() {
        document.getElementById('selBoxBtn')?.addEventListener('click', () => {
            this.selection.setMode(SelMode.BOX);
            this.addChatMessage('assistant', 'Selection mode: BOX');
        });
        document.getElementById('selBrushBtn')?.addEventListener('click', () => {
            this.selection.setMode(SelMode.BRUSH);
            this.addChatMessage('assistant', 'Selection mode: BRUSH');
        });
        document.getElementById('selPaintBtn')?.addEventListener('click', () => {
            this.selection.setMode(SelMode.PAINT);
            this.addChatMessage('assistant', 'Selection mode: PAINT');
        });
        document.getElementById('selClearBtn')?.addEventListener('click', () => {
            this.selection.clear();
            this.addChatMessage('assistant', 'Selection cleared');
        });
        document.getElementById('copyBtn')?.addEventListener('click', () => {
            if (this.copyPaste.copy(this.selection)) {
                const info = this.copyPaste.getClipboardInfo();
                this.addChatMessage('assistant', `Copied ${info.size} blocks (${info.width}x${info.height}x${info.depth})`);
            }
        });
        document.getElementById('pasteBtn')?.addEventListener('click', () => {
            if (this.copyPaste.hasClipboard()) {
                this.buildingHistory.record('paste', 'Paste blocks');
                this.copyPaste.paste(0, 2, 0, this.selection);
                if (this.renderer) {
                    this.renderer.updateFromWASM();
                    this.renderer.rebuildMesh();
                }
                this.addChatMessage('assistant', 'Pasted blocks');
            }
        });
        document.getElementById('rotateBtn')?.addEventListener('click', () => {
            this.copyPaste.rotate(90);
            this.addChatMessage('assistant', 'Rotated clipboard 90°');
        });
        document.getElementById('flipXBtn')?.addEventListener('click', () => { this.copyPaste.toggleFlip('x'); });
        document.getElementById('flipYBtn')?.addEventListener('click', () => { this.copyPaste.toggleFlip('y'); });
        document.getElementById('flipZBtn')?.addEventListener('click', () => { this.copyPaste.toggleFlip('z'); });
        document.getElementById('symXBtn')?.addEventListener('click', () => {
            this.symmetry.toggle('x');
            this.addChatMessage('assistant', 'Symmetry X: ' + (this.symmetry.axes.has('x') ? 'ON' : 'OFF'));
        });
        document.getElementById('symYBtn')?.addEventListener('click', () => {
            this.symmetry.toggle('y');
            this.addChatMessage('assistant', 'Symmetry Y: ' + (this.symmetry.axes.has('y') ? 'ON' : 'OFF'));
        });
        document.getElementById('symZBtn')?.addEventListener('click', () => {
            this.symmetry.toggle('z');
            this.addChatMessage('assistant', 'Symmetry Z: ' + (this.symmetry.axes.has('z') ? 'ON' : 'OFF'));
        });
        document.getElementById('fillBtn')?.addEventListener('click', () => {
            const bounds = this.selection.getBounds();
            if (!bounds) { this.addChatMessage('assistant', 'Select an area first'); return; }
            const type = parseInt(document.getElementById('fillType')?.value || '1');
            const pattern = document.getElementById('fillPattern')?.value || 'solid';
            this.buildingHistory.record('fill', 'Fill area');
            const count = this.patternTools.fillArea(bounds, type, pattern);
            if (this.renderer) {
                this.renderer.updateFromWASM();
                this.renderer.rebuildMesh();
            }
            this.addChatMessage('assistant', `Filled ${count} blocks with pattern: ${pattern}`);
        });
        document.getElementById('lineBtn')?.addEventListener('click', () => {
            this.addChatMessage('assistant', 'Line tool: Click two points in the viewport');
        });
        document.getElementById('circleBtn')?.addEventListener('click', () => {
            this.addChatMessage('assistant', 'Circle tool: Click center in viewport');
        });
        document.getElementById('sphereBtn')?.addEventListener('click', () => {
            this.addChatMessage('assistant', 'Sphere tool: Click center in viewport');
        });
        document.getElementById('bpSaveBtn')?.addEventListener('click', async () => {
            const name = document.getElementById('bpName')?.value || 'Blueprint';
            const bp = this.blueprints.create(name, this.selection, this.engine);
            if (bp) {
                this.addChatMessage('assistant', `Blueprint saved: ${name} (${bp.blocks.length} blocks)`);
                this.refreshBlueprintList();
            }
        });
        document.getElementById('bpExportBtn')?.addEventListener('click', () => {
            const sel = this.blueprints.getAll()[0];
            if (sel) {
                const data = this.blueprints.exportBP(sel);
                this.importExport.exportFile(data, sel.name + '.bp', 'application/json');
            }
        });
        document.getElementById('bpImportBtn')?.addEventListener('click', () => {
            const input = document.createElement('input');
            input.type = 'file';
            input.accept = '.bp,.json';
            input.onchange = (e) => {
                const file = e.target.files[0];
                if (!file) return;
                const reader = new FileReader();
                reader.onload = () => {
                    const bp = this.blueprints.importBP(reader.result);
                    if (bp) {
                        this.addChatMessage('assistant', `Imported blueprint: ${bp.name}`);
                        this.refreshBlueprintList();
                    }
                };
                reader.readAsText(file);
            };
            input.click();
        });
        document.getElementById('exportJSONBtn')?.addEventListener('click', () => {
            const data = this.importExport.exportJSON(this.selection, 'selection');
            if (data) this.importExport.exportFile(data, 'selection.json', 'application/json');
        });
        document.getElementById('exportOBJBtn')?.addEventListener('click', () => {
            const data = this.importExport.exportOBJ(this.selection, 'selection');
            if (data) this.importExport.exportFile(data, 'selection.obj', 'text/plain');
        });
        document.getElementById('exportGLTFBtn')?.addEventListener('click', () => {
            if (!this.engine.wasmReady) return;
            this.engine.exportGLTF();
            const data = this.engine.getLastExportGLTF();
            if (data) this.importExport.exportFile(data, 'selection.gltf', 'application/json');
        });
        document.getElementById('historyRefreshBtn')?.addEventListener('click', () => this.refreshHistory());
        document.getElementById('historyUndoBtn')?.addEventListener('click', () => {
            if (this.buildingHistory.undo()) {
                if (this.renderer) {
                    this.renderer.updateFromWASM();
                    this.renderer.rebuildMesh();
                }
                this.addChatMessage('assistant', 'Undone');
            }
        });
        document.getElementById('historyRedoBtn')?.addEventListener('click', () => {
            if (this.buildingHistory.redo()) {
                if (this.renderer) {
                    this.renderer.updateFromWASM();
                    this.renderer.rebuildMesh();
                }
                this.addChatMessage('assistant', 'Redone');
            }
        });
    }

    refreshBlueprintList() {
        const list = document.getElementById('bpList');
        if (!list) return;
        list.innerHTML = '';
        for (const bp of this.blueprints.getAll()) {
            const div = document.createElement('div');
            div.style.cssText = 'padding:6px 8px;background:var(--bg-alt);border-radius:6px;margin:3px 0;font-size:11px;display:flex;justify-content:space-between;align-items:center';
            div.innerHTML = `<span style="color:var(--text)">${this._escapeHtml(bp.name)} (${bp.width}x${bp.height}x${bp.depth})</span><div style="display:flex;gap:4px"><button class="bp-place-btn" data-id="${this._escapeHtml(String(bp.id))}" style="padding:2px 6px;border:1px solid var(--border);border-radius:4px;background:var(--bg);color:var(--text);cursor:pointer;font-size:10px">Place</button><button class="bp-del-btn" data-id="${this._escapeHtml(String(bp.id))}" style="padding:2px 6px;border:1px solid var(--error);border-radius:4px;background:var(--bg);color:var(--error);cursor:pointer;font-size:10px">X</button></div>`;
            list.appendChild(div);
        }
        list.querySelectorAll('.bp-place-btn').forEach(btn => {
            btn.addEventListener('click', () => {
                const bp = this.blueprints.get(btn.dataset.id);
                if (bp && this.engine?.wasmReady) {
                    this.buildingHistory.record('place_bp', 'Place blueprint: ' + bp.name);
                    this.blueprints.place(bp, 0, 2, 0, this.engine);
                    if (this.renderer) {
                        this.renderer.updateFromWASM();
                        this.renderer.rebuildMesh();
                    }
                    this.addChatMessage('assistant', `Placed blueprint: ${bp.name}`);
                }
            });
        });
        list.querySelectorAll('.bp-del-btn').forEach(btn => {
            btn.addEventListener('click', () => {
                this.blueprints.delete(btn.dataset.id);
                this.refreshBlueprintList();
            });
        });
    }

    refreshHistory() {
        const list = document.getElementById('historyList');
        if (!list) return;
        list.innerHTML = '';
        const timeline = this.buildingHistory.getTimeline();
        for (const entry of timeline) {
            const div = document.createElement('div');
            div.style.cssText = `padding:4px 8px;background:${entry.isCurrent ? 'rgba(139,92,246,0.2)' : 'var(--bg-alt)'};border-radius:4px;margin:2px 0;font-size:10px;cursor:pointer;border-left:2px solid ${entry.isCurrent ? '#8b5cf6' : 'var(--border)'}`;
            div.innerHTML = `<span style="color:var(--text)">${this._escapeHtml(entry.description)}</span><span style="color:var(--text-dim);margin-left:4px">${entry.blockCount} blocks</span>`;
            div.addEventListener('click', () => {
                if (!this.engine?.wasmReady) return;
                this.buildingHistory.jumpTo(entry.index);
                if (this.renderer) {
                    this.renderer.updateFromWASM();
                    this.renderer.rebuildMesh();
                }
                this.refreshHistory();
            });
            list.appendChild(div);
        }
    }
}

async function startApp() {
    const app = new OmniConsole();
    await app.init();
    window.__omni = app;
}

document.addEventListener('DOMContentLoaded', startApp);
