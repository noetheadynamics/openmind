/**
 * OpenMind – Automatic Updater
 * Detects GitHub releases, prompts the user, downloads + installs + relaunches.
 */
(function() {
    'use strict';

    class AutoUpdater {
        constructor() {
            this.isTauri = !!(window.__TAURI_INTERNALS__);
            this.checkIntervalMs = 60 * 60 * 1000; // hourly
            this._timer = null;
            this._installing = false;
            this.lastCheck = 0;
            this.enabled = true;
        }

        start() {
            if (!this.isTauri || !this.enabled) return;
            this._check();
            this._timer = setInterval(() => this._check(), this.checkIntervalMs);
        }

        stop() {
            if (this._timer) { clearInterval(this._timer); this._timer = null; }
        }

        async _invoke(cmd, args) {
            try {
                return await window.__TAURI_INTERNALS__.invoke(cmd, args || {});
            } catch (e) {
                console.warn('[Updater] invoke failed:', cmd, e);
                return null;
            }
        }

        async _check() {
            const info = await this._invoke('check_for_updates');
            if (!info) return;
            this.lastCheck = Date.now();
            if (info.has_update && info.download_url && !this._installing) {
                this._prompt(info);
            }
        }

        _prompt(info) {
            const notify = (window.Notifications && new window.Notifications()) || null;
            const msg = `OpenMind v${info.latest_version} is available. Install and restart now?`;
            if (notify) {
                notify.show(msg, window.NotifType.INFO, {
                    title: 'Update Available',
                    duration: 0,
                    action: () => this._install(info)
                });
            } else if (window.confirm) {
                if (window.confirm(msg)) this._install(info);
            }
        }

        async _install(info) {
            if (this._installing) return;
            this._installing = true;
            const result = await this._invoke('download_and_install', { url: info.download_url });
            if (result !== 'installing') {
                this._installing = false;
                console.error('[Updater] install failed:', result);
            }
        }

        checkNow() {
            return this._check();
        }
    }

    window.AutoUpdater = AutoUpdater;
    if (!window.openmindUpdater) window.openmindUpdater = new AutoUpdater();
})();
