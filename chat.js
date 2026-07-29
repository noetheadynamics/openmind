class ChatSystem {
    constructor() {
        this.messages = [];
        this.maxMessages = 100;
        this.isOpen = false;
        this.inputValue = '';
        this.containerEl = null;
        this.inputEl = null;
        this.onSendMessage = null;
        this.onCommand = null;
        this.systemMessages = [
            'Welcome to OpenMind Multiplayer!',
            'Type /help for available commands'
        ];

        this._onKeyDown = this._onKeyDown.bind(this);
    }

    init(containerEl, inputEl) {
        this.containerEl = containerEl;
        this.inputEl = inputEl;

        if (this.inputEl) {
            this.inputEl.addEventListener('keydown', this._onKeyDown);
        }

        this.systemMessages.forEach(msg => this.addSystemMessage(msg));
    }

    _onKeyDown(e) {
        if (e.key === 'Enter') {
            e.preventDefault();
            this.sendMessage();
        } else if (e.key === 'Escape') {
            this.close();
        }
    }

    toggle() {
        if (this.isOpen) this.close();
        else this.open();
    }

    open() {
        this.isOpen = true;
        if (this.containerEl) this.containerEl.style.display = 'flex';
        if (this.inputEl) this.inputEl.focus();
    }

    close() {
        this.isOpen = false;
        if (this.containerEl) this.containerEl.style.display = 'none';
        if (this.inputEl) this.inputEl.blur();
    }

    sendMessage() {
        if (!this.inputEl) return;
        const text = this.inputEl.value.trim();
        if (!text) return;

        if (text.startsWith('/')) {
            this.handleCommand(text);
        } else {
            this.addMessage('You', text, '#8b5cf6', Date.now());
            if (this.onSendMessage) this.onSendMessage(text);
        }

        this.inputEl.value = '';
    }

    handleCommand(text) {
        const parts = text.split(' ');
        const cmd = parts[0].toLowerCase();

        const commands = {
            '/help': 'Commands: /help, /players, /clear, /time, /weather, /say, /me',
            '/players': 'Requesting player list...',
            '/clear': 'clear',
            '/time': 'Time: ' + (this._timeOfDay || 'Day'),
            '/weather': 'Weather: ' + (this._weather || 'Clear'),
            '/say': parts.slice(1).join(' ') || 'Usage: /say <message>',
            '/me': '* ' + (parts[1] || 'does something')
        };

        if (cmd === '/clear') {
            this.messages = [];
            this.renderMessages();
            return;
        }

        const response = commands[cmd];
        if (response) {
            this.addSystemMessage(response);
        } else {
            this.addSystemMessage('Unknown command: ' + cmd + '. Type /help for commands.');
        }

        if (this.onCommand) this.onCommand(text);
    }

    addMessage(playerName, text, color, timestamp) {
        this.messages.push({
            type: 'message',
            playerName,
            text,
            color: color || '#ffffff',
            timestamp: timestamp || Date.now()
        });

        if (this.messages.length > this.maxMessages) {
            this.messages.shift();
        }

        this.renderMessages();
        this.showNotification(playerName, text, color);
    }

    addSystemMessage(text, color) {
        this.messages.push({
            type: 'system',
            text,
            color: color || '#eab308',
            timestamp: Date.now()
        });

        if (this.messages.length > this.maxMessages) {
            this.messages.shift();
        }

        this.renderMessages();
    }

    addJoinMessage(playerName, color) {
        this.addSystemMessage(playerName + ' joined the world', color || '#22c55e');
    }

    addLeaveMessage(playerName) {
        this.addSystemMessage(playerName + ' left the world', '#ef4444');
    }

    addKickedMessage(playerName, reason) {
        this.addSystemMessage(playerName + ' was kicked: ' + (reason || 'by host'), '#ef4444');
    }

    showNotification(playerName, text, color) {
        if (this.isOpen) return;

        let notif = document.getElementById('chat-notification');
        if (!notif) {
            notif = document.createElement('div');
            notif.id = 'chat-notification';
            notif.style.cssText = 'position:fixed;bottom:80px;right:20px;z-index:9999;background:rgba(10,10,30,0.9);border:1px solid rgba(100,100,200,0.3);border-radius:12px;padding:10px 14px;backdrop-filter:blur(12px);font-size:12px;max-width:300px;animation:slideInRight 0.3s ease';
            document.body.appendChild(notif);
        }

        notif.innerHTML = '<div style="color:' + (color || '#8b5cf6') + ';font-weight:bold;margin-bottom:2px">' + this._escapeHtml(playerName) + '</div>' +
            '<div style="color:#e0e0e0">' + this._escapeHtml(text) + '</div>';
        notif.style.display = 'block';

        clearTimeout(this._notifTimer);
        this._notifTimer = setTimeout(() => {
            notif.style.display = 'none';
        }, 3000);
    }

    renderMessages() {
        if (!this.containerEl) return;

        const html = this.messages.map(msg => {
            if (msg.type === 'system') {
                return '<div class="chat-msg chat-system" style="color:' + msg.color + '">' + this._escapeHtml(msg.text) + '</div>';
            }
            return '<div class="chat-msg">' +
                '<span class="chat-name" style="color:' + msg.color + '">' + this._escapeHtml(msg.playerName) + ':</span> ' +
                '<span class="chat-text">' + this._escapeHtml(msg.text) + '</span>' +
                '</div>';
        }).join('');

        this.containerEl.innerHTML = html;
        this.containerEl.scrollTop = this.containerEl.scrollHeight;
    }

    setTimeOfDay(tod) { this._timeOfDay = tod; }
    setWeather(w) { this._weather = w; }

    _escapeHtml(text) {
        if (!this._escDiv) this._escDiv = document.createElement('div');
        this._escDiv.textContent = text;
        return this._escDiv.innerHTML;
    }

    destroy() {
        if (this.inputEl) {
            this.inputEl.removeEventListener('keydown', this._onKeyDown);
        }
        const notif = document.getElementById('chat-notification');
        if (notif) notif.remove();
    }
}

window.ChatSystem = ChatSystem;
