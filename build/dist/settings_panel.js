/**
 * OpenMind – Settings Panel
 * Full settings with persistence, display/audio/controls/performance
 */
(function() {
    'use strict';

    const DefaultSettings = {
        display: { theme: 'dark', fullscreen: false, fov: 75 },
        controls: { sensitivity: 1.0, invertMouse: false },
        audio: { master: 0.5, sfx: 0.8, music: 0.3 },
        performance: { particles: true, shadows: true, renderDistance: 100 },
        accessibility: { reduceMotion: false, highContrast: false, fontSize: 'normal' },
        gameplay: { autoSaveInterval: 30, showTutorial: true },
        network: { provider: 'groq', apiKey: '', model: 'llama-3.1-8b-instant' }
    };

    class SettingsPanel {
        constructor() {
            this.settings = JSON.parse(JSON.stringify(DefaultSettings));
            this.storageKey = 'openmind_settings';
            this.listeners = [];
            this.load();
            this.applyAccessibility();
        }

        get(path) {
            const parts = path.split('.');
            let val = this.settings;
            for (const p of parts) { val = val?.[p]; }
            return val;
        }

        set(path, value) {
            const parts = path.split('.');
            let obj = this.settings;
            for (let i = 0; i < parts.length - 1; i++) {
                if (!obj[parts[i]]) obj[parts[i]] = {};
                obj = obj[parts[i]];
            }
            obj[parts[parts.length - 1]] = value;
            this.save();
            this.emit('change', { path, value });
            this.applySetting(path, value);
        }

        reset() {
            this.settings = JSON.parse(JSON.stringify(DefaultSettings));
            this.save();
            this.emit('reset');
        }

        save() {
            try { localStorage.setItem(this.storageKey, JSON.stringify(this.settings)); } catch (e) { /* settings save failed */ }
        }

        load() {
            try {
                const data = localStorage.getItem(this.storageKey);
                if (data) {
                    const parsed = JSON.parse(data);
                    this.settings = this._deepMerge(JSON.parse(JSON.stringify(DefaultSettings)), parsed);
                }
            } catch (e) { /* settings load failed */ }
        }

        _deepMerge(target, source) {
            for (const key of Object.keys(source)) {
                if (source[key] && typeof source[key] === 'object' && !Array.isArray(source[key])) {
                    if (!target[key] || typeof target[key] !== 'object') target[key] = {};
                    this._deepMerge(target[key], source[key]);
                } else {
                    target[key] = source[key];
                }
            }
            return target;
        }

        applySetting(path, value) {
            switch (path) {
                case 'display.theme':
                    document.body.classList.toggle('light', value === 'light');
                    window.dispatchEvent(new CustomEvent('themechange', { detail: { light: value === 'light' } }));
                    break;
                case 'display.fullscreen':
                    if (value && !document.fullscreenElement) document.documentElement.requestFullscreen?.();
                    else if (!value && document.fullscreenElement) document.exitFullscreen?.();
                    break;
                case 'audio.master':
                    window.dispatchEvent(new CustomEvent('audiomaster', { detail: { volume: value } }));
                    break;
                case 'performance.particles':
                    window.dispatchEvent(new CustomEvent('particlestoggle', { detail: { enabled: value } }));
                    break;
                case 'accessibility.reduceMotion':
                    document.body.classList.toggle('reduce-motion', value);
                    break;
                case 'accessibility.highContrast':
                    document.body.classList.toggle('high-contrast', value);
                    break;
                case 'accessibility.fontSize':
                    document.body.classList.remove('font-small', 'font-normal', 'font-large');
                    document.body.classList.add('font-' + value);
                    break;
            }
        }

        applyAccessibility() {
            if (this.settings.accessibility.reduceMotion) {
                document.body.classList.add('reduce-motion');
            }
            if (this.settings.accessibility.highContrast) {
                document.body.classList.add('high-contrast');
            }
            if (this.settings.accessibility.fontSize) {
                document.body.classList.add('font-' + this.settings.accessibility.fontSize);
            }
        }

        renderUI(container) {
            if (!container) return;
            container.innerHTML = `
                <div style="display:flex;flex-direction:column;gap:12px;padding:8px">
                    ${this.section('Display', [
                        this.toggle('Theme', 'display.theme', this.settings.display.theme === 'light' ? 'light' : 'dark', v => this.set('display.theme', v)),
                        this.slider('FOV', 'display.fov', 60, 120, 1, this.settings.display.fov, v => this.set('display.fov', v))
                    ])}
                    ${this.section('Audio', [
                        this.slider('Master Volume', 'audio.master', 0, 1, 0.05, this.settings.audio.master, v => this.set('audio.master', v)),
                        this.slider('SFX Volume', 'audio.sfx', 0, 1, 0.05, this.settings.audio.sfx, v => this.set('audio.sfx', v)),
                        this.slider('Music Volume', 'audio.music', 0, 1, 0.05, this.settings.audio.music, v => this.set('audio.music', v))
                    ])}
                    ${this.section('Controls', [
                        this.slider('Sensitivity', 'controls.sensitivity', 0.1, 3, 0.1, this.settings.controls.sensitivity, v => this.set('controls.sensitivity', v)),
                        this.toggle('Invert Mouse', 'controls.invertMouse', this.settings.controls.invertMouse, v => this.set('controls.invertMouse', v))
                    ])}
                    ${this.section('Performance', [
                        this.toggle('Particles', 'performance.particles', this.settings.performance.particles, v => this.set('performance.particles', v)),
                        this.toggle('Shadows', 'performance.shadows', this.settings.performance.shadows, v => this.set('performance.shadows', v)),
                        this.slider('Render Distance', 'performance.renderDistance', 10, 300, 10, this.settings.performance.renderDistance, v => this.set('performance.renderDistance', v))
                    ])}
                    ${this.section('Accessibility', [
                        this.toggle('Reduce Motion', 'accessibility.reduceMotion', this.settings.accessibility.reduceMotion, v => this.set('accessibility.reduceMotion', v)),
                        this.toggle('High Contrast', 'accessibility.highContrast', this.settings.accessibility.highContrast, v => this.set('accessibility.highContrast', v)),
                        this.select('Font Size', 'accessibility.fontSize', { small: 'Small', normal: 'Normal', large: 'Large' }, this.settings.accessibility.fontSize, v => this.set('accessibility.fontSize', v))
                    ])}
                    ${this.section('Gameplay', [
                        this.slider('Auto-Save (s)', 'gameplay.autoSaveInterval', 10, 120, 10, this.settings.gameplay.autoSaveInterval, v => this.set('gameplay.autoSaveInterval', v)),
                        this.toggle('Show Tutorial', 'gameplay.showTutorial', this.settings.gameplay.showTutorial, v => this.set('gameplay.showTutorial', v))
                    ])}
                    <button id="settingsResetBtn" style="width:100%;padding:8px;border:1px solid rgba(239,68,68,0.4);border-radius:8px;background:rgba(239,68,68,0.1);color:#ef4444;cursor:pointer;font-size:12px;margin-top:8px">Reset All Settings</button>
                </div>
            `;
            container.querySelector('#settingsResetBtn')?.addEventListener('click', () => {
                this.reset();
                this.renderUI(container);
            });
            this.bindUI(container);
        }

        section(title, items) {
            const esc = (typeof OMUtils !== 'undefined') ? OMUtils.escapeHtml : (s => String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;'));
            return `<div style="background:var(--bg-alt);border-radius:8px;padding:8px"><div style="color:#8b5cf6;font-size:11px;text-transform:uppercase;letter-spacing:1px;margin-bottom:8px">${esc(title)}</div>${items.join('')}</div>`;
        }

        toggle(label, key, value, onChange) {
            const id = 'set_' + key.replace(/\./g, '_');
            const esc = (typeof OMUtils !== 'undefined') ? OMUtils.escapeHtml : (s => String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;'));
            return `<div style="display:flex;justify-content:space-between;align-items:center;padding:4px 0;font-size:12px"><span style="color:var(--text)">${esc(label)}</span><button class="settings-toggle" id="${esc(id)}" style="padding:3px 10px;border:1px solid var(--border);border-radius:6px;background:${value ? 'rgba(139,92,246,0.3)' : 'var(--bg)'};color:${value ? '#e0e0e0' : 'var(--text-dim)'};cursor:pointer;font-size:11px;min-width:40px">${value ? 'ON' : 'OFF'}</button></div>`;
        }

        slider(label, key, min, max, step, value, onChange) {
            const id = 'set_' + key.replace(/\./g, '_');
            const esc = (typeof OMUtils !== 'undefined') ? OMUtils.escapeHtml : (s => String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;'));
            return `<div style="padding:4px 0;font-size:12px"><div style="display:flex;justify-content:space-between;margin-bottom:4px"><span style="color:var(--text)">${esc(label)}</span><span style="color:#60a5fa;font-size:11px" id="${esc(id)}_val">${value}</span></div><input type="range" class="settings-slider" id="${esc(id)}" min="${min}" max="${max}" step="${step}" value="${value}" style="width:100%"></div>`;
        }

        select(label, key, options, value, onChange) {
            const id = 'set_' + key.replace(/\./g, '_');
            const esc = (typeof OMUtils !== 'undefined') ? OMUtils.escapeHtml : (s => String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;'));
            const opts = Object.entries(options).map(([k, v]) => `<option value="${esc(k)}" ${k === value ? 'selected' : ''}>${esc(v)}</option>`).join('');
            return `<div style="padding:4px 0;font-size:12px"><div style="color:var(--text);margin-bottom:4px">${esc(label)}</div><select class="settings-select" id="${esc(id)}" style="width:100%;padding:4px 8px;border:1px solid var(--border);border-radius:6px;background:var(--bg);color:var(--text);font-size:11px">${opts}</select></div>`;
        }

        bindUI(container) {
            container.querySelectorAll('.settings-toggle').forEach(btn => {
                btn.addEventListener('click', () => {
                    const key = btn.id.replace('set_', '').replace(/_/g, '.');
                    const current = this.get(key);
                    this.set(key, !current);
                    btn.textContent = !current ? 'ON' : 'OFF';
                    btn.style.background = !current ? 'rgba(139,92,246,0.3)' : 'var(--bg)';
                    btn.style.color = !current ? '#e0e0e0' : 'var(--text-dim)';
                });
            });
            container.querySelectorAll('.settings-slider').forEach(slider => {
                slider.addEventListener('input', () => {
                    const key = slider.id.replace('set_', '').replace(/_/g, '.');
                    const val = parseFloat(slider.value);
                    this.set(key, val);
                    const label = document.getElementById(slider.id + '_val');
                    if (label) label.textContent = val;
                });
            });
            container.querySelectorAll('.settings-select').forEach(sel => {
                sel.addEventListener('change', () => {
                    const key = sel.id.replace('set_', '').replace(/_/g, '.');
                    this.set(key, sel.value);
                });
            });
        }

        on(fn) { this.listeners.push(fn); }
        emit(type, data) {
            for (const fn of this.listeners) fn({ type, ...data });
        }

        exportSettings() { return JSON.stringify(this.settings, null, 2); }
        importSettings(json) {
            try {
                this.settings = this._deepMerge(JSON.parse(JSON.stringify(DefaultSettings)), JSON.parse(json));
                this.save();
                return true;
            } catch (e) { return false; }
        }
    }

    window.SettingsPanel = SettingsPanel;
})();
