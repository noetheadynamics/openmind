/**
 * OpenMind – Notifications System
 * Toast notifications, auto-dismiss, types, clickable
 */
(function() {
    'use strict';

    const NotifType = Object.freeze({
        SUCCESS: 'success', INFO: 'info', WARNING: 'warning', ERROR: 'error'
    });

    class Notifications {
        constructor() {
            this.notifications = [];
            this.container = null;
            this.maxVisible = 5;
            this.duration = 3000;
            this.count = 0;
            this.listeners = [];
            this._timeouts = {};
            this.createContainer();
        }

        createContainer() {
            this.container = document.createElement('div');
            this.container.id = 'notifContainer';
            this.container.style.cssText = `
                position:fixed;top:60px;right:16px;z-index:9997;
                display:flex;flex-direction:column;gap:6px;pointer-events:none;
                max-width:340px;width:100%;
            `;
            if (document.body) document.body.appendChild(this.container);
        }

        show(message, type, options) {
            type = type || NotifType.INFO;
            options = options || {};
            const id = ++this.count;
            const notif = {
                id, message, type,
                title: options.title || '',
                duration: options.duration ?? this.duration,
                action: options.action || null,
                dismissible: options.dismissible !== false,
                timestamp: Date.now()
            };
            this.notifications.push(notif);
            this.render(notif);
            if (notif.duration > 0) {
                this._timeouts[id] = setTimeout(() => {
                    delete this._timeouts[id];
                    this.dismiss(id);
                }, notif.duration);
            }
            this.emit('show', notif);
            return id;
        }

        success(msg, opts) { return this.show(msg, NotifType.SUCCESS, opts); }
        info(msg, opts) { return this.show(msg, NotifType.INFO, opts); }
        warning(msg, opts) { return this.show(msg, NotifType.WARNING, opts); }
        error(msg, opts) { return this.show(msg, NotifType.ERROR, { duration: 5000, ...opts }); }

        render(notif) {
            const colors = {
                success: { bg: 'rgba(34,197,94,0.15)', border: 'rgba(34,197,94,0.4)', icon: '✓', text: '#22c55e' },
                info: { bg: 'rgba(96,165,250,0.15)', border: 'rgba(96,165,250,0.4)', icon: 'ℹ', text: '#60a5fa' },
                warning: { bg: 'rgba(234,179,8,0.15)', border: 'rgba(234,179,8,0.4)', icon: '⚠', text: '#eab308' },
                error: { bg: 'rgba(239,68,68,0.15)', border: 'rgba(239,68,68,0.4)', icon: '✕', text: '#ef4444' }
            };
            const c = colors[notif.type] || colors.info;
            const el = document.createElement('div');
            el.dataset.notifId = notif.id;
            el.style.cssText = `
                pointer-events:auto;background:${c.bg};border:1px solid ${c.border};
                border-radius:10px;padding:10px 14px;display:flex;align-items:flex-start;gap:8px;
                backdrop-filter:blur(8px);cursor:${notif.action ? 'pointer' : 'default'};
                transition:opacity 0.2s,transform 0.2s;opacity:0;transform:translateX(20px);
                box-shadow:0 4px 12px rgba(0,0,0,0.3);
            `;
            el.innerHTML = `
                <span style="color:${c.text};font-size:14px;margin-top:1px">${c.icon}</span>
                <div style="flex:1;min-width:0">
                    ${notif.title ? `<div style="color:#e0e0e0;font-size:12px;font-weight:600;margin-bottom:2px">${this._escapeHtml(notif.title)}</div>` : ''}
                    <div style="color:#94a3b8;font-size:11px;line-height:1.4">${this._escapeHtml(notif.message)}</div>
                </div>
                ${notif.dismissible ? `<button class="notif-dismiss" style="background:none;border:none;color:#94a3b8;cursor:pointer;font-size:14px;padding:0">✕</button>` : ''}
            `;
            el.querySelector('.notif-dismiss')?.addEventListener('click', (e) => {
                e.stopPropagation();
                this.dismiss(notif.id);
            });
            if (notif.action) {
                el.addEventListener('click', () => {
                    notif.action();
                    this.dismiss(notif.id);
                });
            }
            this.container.appendChild(el);
            requestAnimationFrame(() => {
                el.style.opacity = '1';
                el.style.transform = 'translateX(0)';
            });
            const visible = this.container.children;
            if (visible.length > this.maxVisible) {
                const oldest = visible[0];
                this.fadeOut(oldest);
            }
        }

        dismiss(id) {
            const idx = this.notifications.findIndex(n => n.id === id);
            if (idx === -1) return;
            this.notifications.splice(idx, 1);
            const el = this.container.querySelector(`[data-notif-id="${id}"]`);
            if (el) this.fadeOut(el);
            this.emit('dismiss', { id });
        }

        fadeOut(el) {
            el.style.opacity = '0';
            el.style.transform = 'translateX(20px)';
            setTimeout(() => el.remove(), 200);
        }

        dismissAll() {
            for (const id of Object.keys(this._timeouts)) {
                clearTimeout(this._timeouts[id]);
                delete this._timeouts[id];
            }
            this.notifications = [];
            this.container.innerHTML = '';
        }

        on(fn) { this.listeners.push(fn); }
        off(fn) { this.listeners = this.listeners.filter(f => f !== fn); }
        emit(type, data) {
            for (const fn of this.listeners) fn({ type, ...data });
        }
        _escapeHtml(str) {
            if (!str) return '';
            const div = document.createElement('div');
            div.textContent = str;
            return div.innerHTML;
        }
    }

    window.NotifType = NotifType;
    window.Notifications = Notifications;
})();
