/**
 * OpenMind – Keyboard Shortcuts
 * All shortcuts, reference panel, custom mapping
 */
(function() {
    'use strict';

    const DefaultShortcuts = {
        'ctrl+z': { action: 'undo', description: 'Undo', category: 'Edit' },
        'ctrl+y': { action: 'redo', description: 'Redo', category: 'Edit' },
        'ctrl+shift+z': { action: 'redo', description: 'Redo', category: 'Edit' },
        'ctrl+c': { action: 'copy', description: 'Copy Selection', category: 'Edit' },
        'ctrl+v': { action: 'paste', description: 'Paste Clipboard', category: 'Edit' },
        'ctrl+s': { action: 'save', description: 'Save World', category: 'File' },
        'ctrl+o': { action: 'load', description: 'Load World', category: 'File' },
        'ctrl+n': { action: 'new', description: 'New World', category: 'File' },
        'ctrl+e': { action: 'export', description: 'Export', category: 'File' },
        'ctrl+shift+e': { action: 'import', description: 'Import', category: 'File' },
        'f11': { action: 'fullscreen', description: 'Fullscreen', category: 'View' },
        'escape': { action: 'close', description: 'Close Panel', category: 'UI' },
        'space': { action: 'pause', description: 'Pause/Resume', category: 'Simulation' },
        '1': { action: 'hotbar_1', description: 'Hotbar 1', category: 'Hotbar' },
        '2': { action: 'hotbar_2', description: 'Hotbar 2', category: 'Hotbar' },
        '3': { action: 'hotbar_3', description: 'Hotbar 3', category: 'Hotbar' },
        '4': { action: 'hotbar_4', description: 'Hotbar 4', category: 'Hotbar' },
        '5': { action: 'hotbar_5', description: 'Hotbar 5', category: 'Hotbar' },
        '6': { action: 'hotbar_6', description: 'Hotbar 6', category: 'Hotbar' },
        '7': { action: 'hotbar_7', description: 'Hotbar 7', category: 'Hotbar' },
        '8': { action: 'hotbar_8', description: 'Hotbar 8', category: 'Hotbar' },
        '9': { action: 'hotbar_9', description: 'Hotbar 9', category: 'Hotbar' },
        'e': { action: 'inventory', description: 'Open Inventory', category: 'UI' },
        'c': { action: 'crafting', description: 'Open Crafting', category: 'UI' },
        't': { action: 'prompt', description: 'AI Prompt', category: 'UI' },
        '?': { action: 'shortcuts', description: 'Show Shortcuts', category: 'UI' }
    };

    class Shortcuts {
        constructor() {
            this.shortcuts = { ...DefaultShortcuts };
            this.handlers = {};
            this.enabled = true;
            this.referencePanel = null;
            this.storageKey = 'openmind_shortcuts';
            this._boundHandle = null;
            this.loadCustom();
        }

        register(action, fn) {
            if (!this.handlers[action]) this.handlers[action] = [];
            this.handlers[action].push(fn);
        }

        unregister(action) {
            if (this.handlers[action]) delete this.handlers[action];
        }

        handle(e) {
            if (!this.enabled) return;
            const tag = e.target.tagName;
            if (tag === 'INPUT' || tag === 'TEXTAREA' || tag === 'SELECT') return;

            const key = this.getKey(e);
            const shortcut = this.shortcuts[key];
            if (!shortcut) return;

            e.preventDefault();
            e.stopPropagation();

            const handlers = this.handlers[shortcut.action];
            if (handlers) {
                for (const fn of handlers) fn(e);
            }
        }

        getKey(e) {
            const parts = [];
            if (e.ctrlKey || e.metaKey) parts.push('ctrl');
            if (e.shiftKey) parts.push('shift');
            if (e.altKey) parts.push('alt');
            let key = e.key.toLowerCase();
            if (key === ' ') key = 'space';
            if (key === 'escape') key = 'escape';
            if (key === '/') key = '/';
            parts.push(key);
            return parts.join('+');
        }

        init() {
            this._boundHandle = (e) => this.handle(e);
            document.addEventListener('keydown', this._boundHandle);
        }

        destroy() {
            if (this._boundHandle) {
                document.removeEventListener('keydown', this._boundHandle);
                this._boundHandle = null;
            }
            this.handlers = {};
            if (this.referencePanel) { this.referencePanel.remove(); this.referencePanel = null; }
        }

        setShortcut(action, newKey) {
            let existing = null;
            for (const [key, s] of Object.entries(this.shortcuts)) {
                if (s.action === action) {
                    existing = { ...s };
                    delete this.shortcuts[key];
                    break;
                }
            }
            const def = DefaultShortcuts[Object.keys(DefaultShortcuts).find(k => DefaultShortcuts[k].action === action)];
            this.shortcuts[newKey] = { ...(def || {}), ...(existing || {}), action };
            this.saveCustom();
        }

        showReference() {
            if (this.referencePanel) { this.referencePanel.remove(); this.referencePanel = null; return; }
            this.referencePanel = document.createElement('div');
            this.referencePanel.style.cssText = `
                position:fixed;top:50%;left:50%;transform:translate(-50%,-50%);z-index:9999;
                background:rgba(15,15,35,0.95);border:1px solid rgba(100,100,200,0.3);
                border-radius:16px;padding:20px;width:500px;max-height:80vh;overflow-y:auto;
                backdrop-filter:blur(12px);box-shadow:0 8px 32px rgba(0,0,0,0.5);
            `;
            const categories = {};
            for (const [key, s] of Object.entries(this.shortcuts)) {
                if (!categories[s.category]) categories[s.category] = [];
                categories[s.category].push({ key, ...s });
            }
            let html = '<div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:12px"><h3 style="color:#e0e0e0;font-size:16px">Keyboard Shortcuts</h3><button id="closeShortcuts" style="background:none;border:none;color:#94a3b8;cursor:pointer;font-size:18px">✕</button></div>';
            const esc = (typeof OMUtils !== 'undefined') ? OMUtils.escapeHtml : (s => String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;'));
            for (const [cat, items] of Object.entries(categories)) {
                html += `<div style="margin-bottom:12px"><div style="color:#8b5cf6;font-size:11px;text-transform:uppercase;letter-spacing:1px;margin-bottom:6px">${esc(cat)}</div>`;
                for (const item of items) {
                    html += `<div style="display:flex;justify-content:space-between;padding:4px 0;border-bottom:1px solid rgba(100,100,200,0.1);font-size:12px"><span style="color:#94a3b8">${esc(item.description)}</span><kbd style="background:rgba(30,30,60,0.8);border:1px solid rgba(100,100,200,0.3);border-radius:4px;padding:2px 6px;color:#e0e0e0;font-family:monospace;font-size:11px">${esc(item.key)}</kbd></div>`;
                }
                html += '</div>';
            }
            this.referencePanel.innerHTML = html;
            document.body.appendChild(this.referencePanel);
            this.referencePanel.querySelector('#closeShortcuts').addEventListener('click', () => {
                this.referencePanel.remove();
                this.referencePanel = null;
            });
        }

        saveCustom() {
            try { localStorage.setItem(this.storageKey, JSON.stringify(this.shortcuts)); } catch (e) {}
        }

        loadCustom() {
            try {
                const data = localStorage.getItem(this.storageKey);
                if (data) this.shortcuts = { ...DefaultShortcuts, ...JSON.parse(data) };
            } catch (e) {}
        }

        reset() {
            this.shortcuts = { ...DefaultShortcuts };
            this.saveCustom();
        }

        getAll() { return { ...this.shortcuts }; }
        getByCategory(cat) {
            return Object.entries(this.shortcuts)
                .filter(([, s]) => s.category === cat)
                .map(([key, s]) => ({ key, ...s }));
        }
    }

    window.Shortcuts = Shortcuts;
})();
