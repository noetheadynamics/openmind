class MultiplayerUI {
    constructor() {
        this.container = null;
        this.isHost = false;
        this.roomCode = null;
        this.playerList = [];
        this.chatSystem = null;
        this.hostSystem = null;
        this.clientSystem = null;
        this.onHost = null;
        this.onJoin = null;
        this.onLeave = null;
        this.onKick = null;

        this._setupStyles();
    }

    _setupStyles() {
        if (document.getElementById('mp-styles')) return;
        const style = document.createElement('style');
        style.id = 'mp-styles';
        style.textContent = `
            .mp-container {
                display: flex; flex-direction: column; height: 100%; gap: 8px;
            }
            .mp-lobby {
                display: flex; flex-direction: column; gap: 8px; padding: 8px 0;
            }
            .mp-lobby-btn {
                width: 100%; padding: 10px 16px; border: 1px solid rgba(100,100,200,0.4);
                border-radius: 8px; background: rgba(30,30,60,0.8); color: #e0e0e0;
                cursor: pointer; font-size: 13px; transition: all 0.2s;
            }
            .mp-lobby-btn:hover { background: rgba(60,60,120,0.8); border-color: #8b5cf6; }
            .mp-lobby-btn.primary { border-color: #22c55e; background: rgba(34,197,94,0.2); }
            .mp-lobby-btn.primary:hover { background: rgba(34,197,94,0.3); }
            .mp-lobby-btn.danger { border-color: #ef4444; background: rgba(239,68,68,0.2); }
            .mp-lobby-btn.danger:hover { background: rgba(239,68,68,0.3); }
            .mp-input {
                width: 100%; padding: 8px 12px; border: 1px solid rgba(100,100,200,0.4);
                border-radius: 6px; background: rgba(20,20,40,0.9); color: #e0e0e0;
                font-size: 13px; font-family: 'Courier New', monospace;
            }
            .mp-input::placeholder { color: #64748b; }
            .mp-room-code {
                font-size: 24px; font-weight: bold; text-align: center;
                padding: 12px; border: 2px solid rgba(139,92,246,0.5);
                border-radius: 10px; background: rgba(139,92,246,0.1);
                color: #c4b5fd; letter-spacing: 4px; user-select: all;
            }
            .mp-player-list {
                flex: 1; overflow-y: auto; border: 1px solid rgba(100,100,200,0.2);
                border-radius: 8px; padding: 4px; max-height: 200px;
            }
            .mp-player {
                display: flex; align-items: center; gap: 8px; padding: 6px 8px;
                border-radius: 6px; font-size: 12px;
            }
            .mp-player:hover { background: rgba(60,60,120,0.3); }
            .mp-player-dot {
                width: 8px; height: 8px; border-radius: 50%; flex-shrink: 0;
            }
            .mp-player-name { flex: 1; overflow: hidden; text-overflow: ellipsis; }
            .mp-player-role {
                font-size: 10px; padding: 2px 6px; border-radius: 4px;
                background: rgba(100,100,200,0.2); color: #94a3b8;
            }
            .mp-player-role.admin { background: rgba(234,179,8,0.2); color: #eab308; }
            .mp-player-kick {
                padding: 2px 6px; border: 1px solid rgba(239,68,68,0.4);
                border-radius: 4px; background: none; color: #ef4444; cursor: pointer;
                font-size: 10px;
            }
            .mp-chat {
                display: flex; flex-direction: column; flex: 1; min-height: 150px;
                border: 1px solid rgba(100,100,200,0.2); border-radius: 8px;
                overflow: hidden;
            }
            .mp-chat-messages {
                flex: 1; overflow-y: auto; padding: 8px; font-size: 11px;
            }
            .mp-chat-msg { padding: 2px 0; }
            .mp-chat-msg.chat-system { font-style: italic; font-size: 10px; }
            .mp-chat-name { font-weight: bold; }
            .mp-chat-input {
                width: 100%; padding: 6px 10px; border: none; border-top: 1px solid rgba(100,100,200,0.2);
                background: rgba(20,20,40,0.9); color: #e0e0e0; font-size: 11px;
                outline: none;
            }
            .mp-stats {
                font-size: 10px; color: #64748b; padding: 4px 0;
                display: flex; justify-content: space-between;
            }
            .mp-name-input {
                display: flex; gap: 6px;
            }
            .mp-name-input input { flex: 1; }
        `;
        document.head.appendChild(style);
    }

    init(containerEl) {
        this.container = containerEl;
        this.renderLobby();
    }

    renderLobby() {
        if (!this.container) return;
        this.container.innerHTML = `
            <div class="mp-container">
                <div class="mp-lobby">
                    <div class="mp-name-input">
                        <input id="mp-name" class="mp-input" placeholder="Your name" maxlength="20" value="Player">
                    </div>
                    <button id="mp-host-btn" class="mp-lobby-btn primary">Host New World</button>
                    <div style="display:flex;gap:6px">
                        <input id="mp-join-code" class="mp-input" placeholder="Room code" maxlength="6" style="text-transform:uppercase">
                        <button id="mp-join-btn" class="mp-lobby-btn" style="width:auto;padding:8px 16px">Join</button>
                    </div>
                </div>
            </div>
        `;

        document.getElementById('mp-host-btn').onclick = () => this._handleHost();
        document.getElementById('mp-join-btn').onclick = () => this._handleJoin();
        document.getElementById('mp-join-code').onkeydown = (e) => {
            if (e.key === 'Enter') this._handleJoin();
        };
    }

    renderHosting() {
        if (!this.container) return;
        const code = this.roomCode || '------';
        this.container.innerHTML = `
            <div class="mp-container">
                <div class="mp-room-code">${code}</div>
                <div class="mp-stats" id="mp-stats">
                    <span>Host: You</span>
                    <span id="mp-latency">0ms</span>
                </div>
                <div class="mp-player-list" id="mp-player-list"></div>
                <button id="mp-leave-btn" class="mp-lobby-btn danger">Leave World</button>
            </div>
            ${this._getChatHtml()}
        `;
        document.getElementById('mp-leave-btn').onclick = () => this._handleLeave();
        this._initChat();
    }

    renderJoining() {
        if (!this.container) return;
        this.container.innerHTML = `
            <div class="mp-container">
                <div style="text-align:center;padding:20px;color:#eab308">Connecting...</div>
                <div class="mp-stats" id="mp-stats">
                    <span>Room: ${this.roomCode}</span>
                    <span id="mp-latency">0ms</span>
                </div>
                <div class="mp-player-list" id="mp-player-list"></div>
                <button id="mp-leave-btn" class="mp-lobby-btn danger">Disconnect</button>
            </div>
            ${this._getChatHtml()}
        `;
        document.getElementById('mp-leave-btn').onclick = () => this._handleLeave();
        this._initChat();
    }

    renderConnected() {
        if (!this.container) return;
        const code = this.roomCode || '------';
        this.container.innerHTML = `
            <div class="mp-container">
                <div class="mp-room-code">${code}</div>
                <div class="mp-stats" id="mp-stats">
                    <span>${this.isHost ? 'Host: You' : 'Connected'}</span>
                    <span id="mp-latency">0ms</span>
                </div>
                <div class="mp-player-list" id="mp-player-list"></div>
                <button id="mp-leave-btn" class="mp-lobby-btn danger">${this.isHost ? 'Stop Hosting' : 'Disconnect'}</button>
            </div>
            ${this._getChatHtml()}
        `;
        document.getElementById('mp-leave-btn').onclick = () => this._handleLeave();
        this._initChat();
        this.updatePlayerList();
    }

    _getChatHtml() {
        return `
            <div class="mp-chat">
                <div class="mp-chat-messages" id="mp-chat-messages"></div>
                <input id="mp-chat-input" class="mp-chat-input" placeholder="Type a message... (press Enter)" maxlength="200">
            </div>
        `;
    }

    _initChat() {
        if (this.chatSystem) this.chatSystem.destroy();
        this.chatSystem = new ChatSystem();
        const chatContainer = document.getElementById('mp-chat-messages');
        const chatInput = document.getElementById('mp-chat-input');
        this.chatSystem.init(chatContainer, chatInput);
    }

    updatePlayerList() {
        const listEl = document.getElementById('mp-player-list');
        if (!listEl) return;

        listEl.innerHTML = this.playerList.map(p => `
            <div class="mp-player">
                <div class="mp-player-dot" style="background:${p.color}"></div>
                <div class="mp-player-name">${this._escapeHtml(p.name)}${p.isHost ? ' (Host)' : ''}</div>
                <div class="mp-player-role ${p.role === 'admin' ? 'admin' : ''}">${p.role}</div>
                ${!p.isHost && this.isHost ? `<button class="mp-player-kick" onclick="window._mpUI.kickPlayer('${p.id}')">Kick</button>` : ''}
            </div>
        `).join('');
    }

    updateLatency(ms) {
        const el = document.getElementById('mp-latency');
        if (el) el.textContent = ms + 'ms';
    }

    showNotification(text, color) {
        let notif = document.getElementById('mp-notification');
        if (!notif) {
            notif = document.createElement('div');
            notif.id = 'mp-notification';
            notif.style.cssText = 'position:fixed;top:60px;right:20px;z-index:10001;background:rgba(10,10,30,0.92);border:1px solid rgba(100,100,200,0.3);border-radius:10px;padding:10px 16px;backdrop-filter:blur(12px);font-size:12px;max-width:250px';
            document.body.appendChild(notif);
        }
        notif.textContent = text;
        notif.style.borderColor = color || 'rgba(100,100,200,0.3)';
        notif.style.display = 'block';
        clearTimeout(this._notifTimer);
        this._notifTimer = setTimeout(() => { notif.style.display = 'none'; }, 3000);
    }

    _handleHost() {
        const name = (document.getElementById('mp-name')?.value || 'Player').trim();
        this.isHost = true;
        if (this.onHost) this.onHost(name);
    }

    _handleJoin() {
        const name = (document.getElementById('mp-name')?.value || 'Player').trim();
        const code = (document.getElementById('mp-join-code')?.value || '').trim().toUpperCase();
        if (!code || code.length !== 6) {
            this.showNotification('Enter a 6-character room code', '#ef4444');
            return;
        }
        this.isHost = false;
        this.roomCode = code;
        if (this.onJoin) this.onJoin(name, code);
    }

    _handleLeave() {
        if (this.onLeave) this.onLeave();
    }

    kickPlayer(peerId) {
        if (this.onKick) this.onKick(peerId);
    }

    _escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    destroy() {
        if (this.chatSystem) this.chatSystem.destroy();
        const notif = document.getElementById('mp-notification');
        if (notif) notif.remove();
        clearTimeout(this._notifTimer);
        const styles = document.getElementById('mp-styles');
        if (styles) styles.remove();
    }
}

window.MultiplayerUI = MultiplayerUI;
