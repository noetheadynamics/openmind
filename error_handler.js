/**
 * OpenMind – Error Handler
 * Catches all errors, user-friendly messages, recovery options
 */
(function() {
    'use strict';

    const ErrorMessages = {
        'NETWORK_ERROR': { title: 'Connection Lost', msg: 'Check your internet connection and try again.', recoverable: true },
        'WASM_LOAD': { title: 'Engine Failed to Load', msg: 'The WebAssembly engine could not be initialized.', recoverable: true },
        'WASM_COMPILE': { title: 'Engine Compilation Error', msg: 'The engine module is corrupted. Try reloading.', recoverable: true },
        'LLM_API': { title: 'AI Service Error', msg: 'Could not reach the AI provider. Check your API key.', recoverable: true },
        'LLM_KEY': { title: 'Invalid API Key', msg: 'The API key is invalid or expired.', recoverable: false },
        'SAVE_FAIL': { title: 'Save Failed', msg: 'World data could not be saved.', recoverable: true },
        'LOAD_FAIL': { title: 'Load Failed', msg: 'World data could not be loaded.', recoverable: true },
        'BLOCK_FULL': { title: 'World Full', msg: 'Maximum block limit reached.', recoverable: false },
        'MEMORY': { title: 'Out of Memory', msg: 'The browser ran out of memory. Try closing other tabs.', recoverable: true },
        'GENERIC': { title: 'Something Went Wrong', msg: 'An unexpected error occurred.', recoverable: true }
    };

    class ErrorHandler {
        constructor() {
            this.errors = [];
            this.listeners = [];
            this.overlay = null;
            this.notificationFn = null;
            this.maxErrors = 50;
            this.setupGlobalHandlers();
        }

        setupGlobalHandlers() {
            window.onerror = (msg, url, line, col, err) => {
                console.error(`[ErrorHandler] UNCAUGHT at ${url}:${line}:${col}`, err || msg);
                this.handle(err || new Error(msg), 'UNCAUGHT', { url, line, col });
                return false;
            };
            window.addEventListener('unhandledrejection', e => {
                console.error('[ErrorHandler] UNHANDLED_PROMISE', e.reason);
                this.handle(e.reason, 'UNHANDLED_PROMISE');
                e.preventDefault();
            });
        }

        handle(error, code, context) {
            code = code || error?.code || 'GENERIC';
            const info = ErrorMessages[code] || ErrorMessages.GENERIC;
            const entry = {
                id: Date.now().toString(36) + Math.random().toString(36).substr(2, 4),
                code,
                title: info.title,
                message: error?.message || info.msg,
                userMessage: info.msg,
                recoverable: info.recoverable,
                context: context || {},
                timestamp: Date.now(),
                stack: error?.stack
            };
            this.errors.push(entry);
            if (this.errors.length > this.maxErrors) this.errors.shift();
            console.error(`[ErrorHandler] ${code}:`, error);
            this.notify(entry);
            return entry;
        }

        handleAsync(promise, code) {
            return promise.catch(err => {
                const entry = this.handle(err, code);
                return { error: entry };
            });
        }

        notify(entry) {
            for (const fn of this.listeners) fn(entry);
            if (this.notificationFn) {
                this.notificationFn(entry);
            }
        }

        showOverlay(entry) {
            if (!this.overlay) {
                this.overlay = document.createElement('div');
                this.overlay.id = 'errorOverlay';
                this.overlay.style.cssText = 'position:fixed;top:0;left:0;width:100%;height:100%;z-index:10000;display:flex;align-items:center;justify-content:center;background:rgba(0,0,0,0.8);backdrop-filter:blur(8px)';
                if (document.body) document.body.appendChild(this.overlay);
            }
            this.overlay.innerHTML = `
                <div style="background:var(--bg-card,rgba(15,15,35,0.95));border:1px solid var(--border,rgba(100,100,200,0.3));border-radius:16px;padding:24px;max-width:420px;width:90%;text-align:center">
                    <div style="font-size:24px;margin-bottom:8px;color:var(--error,#ef4444)">⚠</div>
                    <h3 style="color:var(--text,#e0e0e0);margin-bottom:8px;font-size:16px">${entry.title}</h3>
                    <p style="color:var(--text-dim,#94a3b8);font-size:13px;margin-bottom:16px">${entry.userMessage}</p>
                    <div style="display:flex;gap:8px;justify-content:center">
                        ${entry.recoverable ? `<button id="errorRetry" style="padding:8px 16px;border:1px solid var(--border);border-radius:8px;background:var(--bg);color:var(--text);cursor:pointer;font-size:12px">Retry</button>` : ''}
                        <button id="errorReload" style="padding:8px 16px;border:1px solid var(--border);border-radius:8px;background:var(--bg);color:var(--text);cursor:pointer;font-size:12px">Reload</button>
                        <button id="errorDismiss" style="padding:8px 16px;border:1px solid var(--border);border-radius:8px;background:var(--bg);color:var(--text);cursor:pointer;font-size:12px">Dismiss</button>
                    </div>
                    <div style="margin-top:12px;font-size:10px;color:var(--text-dim)">Error ID: ${entry.id}</div>
                </div>
            `;
            this.overlay.style.display = 'flex';
            this.overlay.querySelector('#errorRetry')?.addEventListener('click', () => {
                this.hideOverlay();
                this.emit('retry', entry);
            });
            this.overlay.querySelector('#errorReload')?.addEventListener('click', () => location.reload());
            this.overlay.querySelector('#errorDismiss')?.addEventListener('click', () => this.hideOverlay());
        }

        hideOverlay() {
            if (this.overlay) this.overlay.style.display = 'none';
        }

        on(fn) { this.listeners.push(fn); }
        off(fn) { this.listeners = this.listeners.filter(f => f !== fn); }
        emit(type, data) {
            for (const fn of this.listeners) fn({ type, ...data });
        }

        setNotificationFn(fn) { this.notificationFn = fn; }
        getHistory() { return [...this.errors]; }
        clearHistory() { this.errors = []; }
    }

    window.ErrorHandler = ErrorHandler;
})();
