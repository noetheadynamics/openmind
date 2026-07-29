class HostSystem {
    constructor(network) {
        this.network = network;
        this.engine = null;
        this.roomCode = null;
        this.worldVersion = 0;
        this.connectedClients = new Map(); // peerId -> { name, color, role, state, joinedAt }
        this.isHosting = false;

        this.permissions = new PermissionsSystem();
        this.blockUpdateBuffer = [];
        this.broadcastTimer = null;
        this.broadcastInterval = 50;

        this.onClientJoined = null;
        this.onClientLeft = null;
        this.onWorldSyncNeeded = null;

        this.tickRate = 20;
        this.tickTimer = null;
    }

    start(engine) {
        this.engine = engine;
        this.network.role = 'host';
        this.network.playerId = this.network.generatePlayerId();
        this.roomCode = this.network.generateRoomCode();
        this.network.roomCode = this.roomCode;

        this.network.onPeerConnected = (peerId, name, color) => {
            this.handleClientConnected(peerId, name, color);
        };

        this.network.onPeerDisconnected = (peerId, name) => {
            this.handleClientDisconnected(peerId, name);
        };

        this.network.onBlockUpdate = (msg) => {
            this.handleClientBlockUpdate(msg);
        };

        this.network.onPlayerState = (msg) => {
            this.handleClientPlayerState(msg);
        };

        this.network.onChatMessage = (msg) => {
            this.handleClientChat(msg);
        };

        this.isHosting = true;
        this.startBroadcastLoop();
        this.startTickLoop();
        this.network.startPing();

        return this.roomCode;
    }

    handleClientConnected(peerId, name, color) {
        if (this.connectedClients.size >= this.permissions.worldSettings.maxPlayers) {
            this.network.sendToPeer(peerId, JSON.stringify({
                type: 'kick', reason: 'Server full', sender: this.network.playerId
            }));
            return;
        }

        const clientInfo = {
            name: name || 'Guest',
            color: color || this.network.generateRandomColor(),
            role: 'builder',
            state: null,
            joinedAt: Date.now(),
            position: null
        };

        this.connectedClients.set(peerId, clientInfo);
        this.permissions.setRole(peerId, 'builder');

        this.sendWorldSync(peerId);
        this.broadcastPlayerList();

        if (this.onClientJoined) this.onClientJoined(peerId, clientInfo);
    }

    handleClientDisconnected(peerId, name) {
        this.connectedClients.delete(peerId);
        this.permissions.removePlayer(peerId);
        this.broadcastPlayerList();

        if (this.onClientLeft) this.onClientLeft(peerId, name);
    }

    sendWorldSync(peerId) {
        if (!this.engine) return;

        const worldData = this.engine.exportCSV();
        const syncMsg = {
            type: 'world-sync',
            version: this.worldVersion,
            blocks: worldData,
            timeOfDay: this.engine.getTimeOfDay(),
            weather: this.engine.getWeather(),
            sender: this.network.playerId
        };

        this.network.sendToPeer(peerId, JSON.stringify(syncMsg));
    }

    handleClientBlockUpdate(msg) {
        const peerId = msg.sender;
        const client = this.connectedClients.get(peerId);

        if (!client) return;

        if (!this.permissions.canBuild(peerId)) {
            this.network.sendToPeer(peerId, JSON.stringify({
                type: 'permission-change',
                reason: 'You do not have build permission',
                sender: this.network.playerId
            }));
            return;
        }

        if (this.isProtected(msg.x, msg.y, msg.z)) {
            this.network.sendToPeer(peerId, JSON.stringify({
                type: 'permission-change',
                reason: 'This area is protected',
                sender: this.network.playerId
            }));
            return;
        }

        if (!this.engine) return;

        if (msg.action === 'place') {
            this.engine.setBlock(msg.x, msg.y, msg.z, msg.blockType);
        } else if (msg.action === 'break') {
            this.engine.setBlock(msg.x, msg.y, msg.z, 0);
        } else if (msg.action === 'interact') {
            this.handleInteraction(msg);
        }

        this.worldVersion++;

        this.network.broadcast(JSON.stringify({
            type: 'block-update',
            x: msg.x,
            y: msg.y,
            z: msg.z,
            blockType: msg.blockType,
            action: msg.action,
            sender: peerId,
            version: this.worldVersion
        }));
    }

    handleClientPlayerState(msg) {
        const peerId = msg.sender;
        const client = this.connectedClients.get(peerId);
        if (client) {
            client.state = msg.state;
            client.position = msg.state?.position;
        }

        this.network.broadcast(JSON.stringify({
            ...msg,
            type: 'player-state'
        }));
    }

    handleClientChat(msg) {
        const peerId = msg.sender;
        const client = this.connectedClients.get(peerId);

        if (!client) return;

        if (msg.message.startsWith('/')) {
            this.handleCommand(peerId, client, msg.message);
            return;
        }

        this.network.broadcast(JSON.stringify({
            type: 'chat',
            sender: peerId,
            playerName: client.name,
            playerColor: client.color,
            message: msg.message,
            timestamp: Date.now()
        }));
    }

    handleCommand(peerId, client, command) {
        const parts = command.split(' ');
        const cmd = parts[0].toLowerCase();

        switch (cmd) {
            case '/kick':
                if (client.role === 'admin') {
                    if (!parts[1]) {
                        this.network.sendToPeer(peerId, JSON.stringify({
                            type: 'chat',
                            sender: 'system',
                            playerName: 'System',
                            playerColor: '#eab308',
                            message: 'Usage: /kick <playerName>'
                        }));
                        break;
                    }
                    const targetName = parts[1];
                    this.kickPlayer(targetName, 'Kicked by admin');
                }
                break;
            case '/promote':
                if (client.role === 'admin') {
                    const targetName = parts[1];
                    this.promotePlayer(targetName);
                }
                break;
            case '/demote':
                if (client.role === 'admin') {
                    const targetName = parts[1];
                    this.demotePlayer(targetName);
                }
                break;
            case '/protect':
                if (client.role === 'admin') {
                    this.protectArea(parts.slice(1).join(' '));
                }
                break;
            case '/players':
                this.sendPlayerListTo(peerId);
                break;
            default:
                this.network.sendToPeer(peerId, JSON.stringify({
                    type: 'chat',
                    sender: 'system',
                    playerName: 'System',
                    playerColor: '#eab308',
                    message: 'Unknown command: ' + cmd
                }));
        }
    }

    handleInteraction(msg) {
        this.network.broadcast(JSON.stringify({
            type: 'interaction',
            x: msg.x,
            y: msg.y,
            z: msg.z,
            interaction: msg.interaction,
            sender: msg.sender,
            version: this.worldVersion
        }));
    }

    isProtected(x, y, z) {
        return this.permissions.isProtected(x, y, z);
    }

    kickPlayer(name, reason) {
        const toKick = [];
        this.connectedClients.forEach((client, peerId) => {
            if (client.name === name) toKick.push(peerId);
        });
        toKick.forEach(peerId => {
            this.network.sendToPeer(peerId, JSON.stringify({
                type: 'kick',
                reason: reason || 'Kicked by host',
                sender: this.network.playerId
            }));
            this.connectedClients.delete(peerId);
            this.permissions.removePlayer(peerId);
        });
        this.broadcastPlayerList();
    }

    promotePlayer(name) {
        this.connectedClients.forEach((client, peerId) => {
            if (client.name === name) {
                client.role = 'admin';
                this.permissions.setRole(peerId, 'admin');
                this.network.broadcast(JSON.stringify({
                    type: 'permission-change',
                    peerId: peerId,
                    playerName: name,
                    role: 'admin',
                    sender: this.network.playerId
                }));
            }
        });
    }

    demotePlayer(name) {
        this.connectedClients.forEach((client, peerId) => {
            if (client.name === name) {
                client.role = 'builder';
                this.permissions.setRole(peerId, 'builder');
                this.network.broadcast(JSON.stringify({
                    type: 'permission-change',
                    peerId: peerId,
                    playerName: name,
                    role: 'builder',
                    sender: this.network.playerId
                }));
            }
        });
    }

    setGuestMode(name) {
        this.connectedClients.forEach((client, peerId) => {
            if (client.name === name) {
                client.role = 'guest';
                this.permissions.setRole(peerId, 'guest');
            }
        });
    }

    protectArea(coords) {
        const parts = coords.split(',');
        if (parts.length === 6) {
            const [x1,y1,z1,x2,y2,z2] = parts.map(Number);
            if ([x1,y1,z1,x2,y2,z2].some(isNaN)) {
                this.network.broadcast(JSON.stringify({
                    type: 'chat',
                    sender: 'system',
                    playerName: 'System',
                    playerColor: '#ef4444',
                    message: 'Invalid coordinates for /protect'
                }));
                return;
            }
            if (this.permissions.protectedAreas.length >= MAX_PROTECTED_AREAS) {
                this.network.broadcast(JSON.stringify({
                    type: 'chat', sender: 'system', playerName: 'System',
                    playerColor: '#ef4444',                     message: 'Max protected areas reached'
                }));
                return;
            }
            this.permissions.protectedAreas.push({ x1,y1,z1,x2,y2,z2 });
            if (this.permissions.protectedAreas.length > MAX_PROTECTED_AREAS) {
                this.permissions.protectedAreas = this.permissions.protectedAreas.slice(0, MAX_PROTECTED_AREAS);
            }
            this.network.broadcast(JSON.stringify({
                type: 'chat',
                sender: 'system',
                playerName: 'System',
                playerColor: '#22c55e',
                message: 'Area protected: (' + x1+','+y1+','+z1 + ') to (' + x2+','+y2+','+z2 + ')'
            }));
        }
    }

    broadcastPlayerList() {
        const players = [];
        players.push({
            id: this.network.playerId,
            name: this.network.playerName,
            color: this.network.playerColor,
            role: 'admin',
            isHost: true
        });
        this.connectedClients.forEach((client, peerId) => {
            players.push({
                id: peerId,
                name: client.name,
                color: client.color,
                role: client.role,
                isHost: false
            });
        });

        const msg = JSON.stringify({
            type: 'player-list',
            players: players,
            sender: this.network.playerId
        });

        this.network.broadcast(msg);
    }

    sendPlayerListTo(peerId) {
        const players = [];
        players.push({
            id: this.network.playerId,
            name: this.network.playerName,
            color: this.network.playerColor,
            role: 'admin',
            isHost: true
        });
        this.connectedClients.forEach((client, pid) => {
            players.push({
                id: pid,
                name: client.name,
                color: client.color,
                role: client.role,
                isHost: false
            });
        });

        this.network.sendToPeer(peerId, JSON.stringify({
            type: 'player-list',
            players: players,
            sender: this.network.playerId
        }));
    }

    startBroadcastLoop() {
        this.broadcastTimer = setInterval(() => {
            if (this.blockUpdateBuffer.length > 0) {
                const updates = this.blockUpdateBuffer.splice(0, 50);
                this.network.broadcast(JSON.stringify({
                    type: 'block-batch',
                    updates,
                    version: this.worldVersion
                }));
            }
            if (this.blockUpdateBuffer.length > 500) {
                this.blockUpdateBuffer = this.blockUpdateBuffer.slice(-200);
            }
        }, this.broadcastInterval);
    }

    startTickLoop() {
        this.tickTimer = setInterval(() => {
            this.network.broadcast(JSON.stringify({
                type: 'heartbeat',
                sender: this.network.playerId,
                timeOfDay: this.engine?.getTimeOfDay(),
                worldVersion: this.worldVersion
            }));
        }, 1000 / this.tickRate);
    }

    stop() {
        if (this.broadcastTimer) clearInterval(this.broadcastTimer);
        if (this.tickTimer) clearInterval(this.tickTimer);
        this.isHosting = false;
        this.connectedClients.clear();
        this.permissions.clear();
        this.engine = null;
        this.network.disconnect();
    }

    getClientCount() { return this.connectedClients.size; }
    getClients() { return this.connectedClients; }
    getRoomCode() { return this.roomCode; }
}

window.HostSystem = HostSystem;
