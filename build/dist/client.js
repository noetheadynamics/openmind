class ClientSystem {
    constructor(network) {
        this.network = network;
        this.engine = null;
        this.isJoining = false;
        this.isConnected = false;
        this.hostPeerId = null;
        this.roomCode = null;
        this.playerList = [];
        this.worldVersion = 0;

        this.onWorldSync = null;
        this.onBlockUpdate = null;
        this.onPlayerState = null;
        this.onPlayerList = null;
        this.onChatMessage = null;
        this.onConnected = null;
        this.onDisconnected = null;
        this.onError = null;

        this.reconnectAttempts = 0;
        this.maxReconnectAttempts = 5;
        this.reconnectTimer = null;
    }

    join(roomCode, engine) {
        this.engine = engine;
        this.roomCode = roomCode;
        this.network.role = 'client';
        this.network.playerId = this.network.generatePlayerId();
        this.isJoining = true;

        this.network.onWorldSync = (msg) => this.handleWorldSync(msg);
        this.network.onBlockUpdate = (msg) => this.handleBlockUpdate(msg);
        this.network.onPlayerState = (msg) => this.handlePlayerState(msg);
        this.network.onChatMessage = (msg) => this.handleChatMessage(msg);
        this.network.onPeerConnected = (peerId, name, color) => this.handleHostConnected(peerId, name, color);
        this.network.onPeerDisconnected = () => this.handleHostDisconnected();
        this.network.onError = (err) => this.handleError(err);

        this.network.startPing();

        return true;
    }

    handleHostConnected(peerId, name, color) {
        if (!peerId) return;
        this.hostPeerId = peerId;
        this.isConnected = true;
        this.isJoining = false;
        this.reconnectAttempts = 0;

        this.network.sendToPeer(peerId, JSON.stringify({
            type: 'client-info',
            playerName: this.network.playerName,
            playerColor: this.network.playerColor,
            playerId: this.network.playerId
        }));

        if (this.onConnected) this.onConnected();
    }

    handleHostDisconnected() {
        this.isConnected = false;
        this.hostPeerId = null;
        if (this.onDisconnected) this.onDisconnected();

        if (this.reconnectAttempts < this.maxReconnectAttempts) {
            this.reconnectAttempts++;
            this.reconnectTimer = setTimeout(() => {
                if (!this.isConnected && this.roomCode) {
                    this.network.connectSignaling();
                    setTimeout(() => {
                        if (this.network.ws?.readyState === 1) {
                            this.network.ws.send(JSON.stringify({ type: 'join-room', roomCode: this.roomCode }));
                        }
                    }, 500);
                }
            }, 2000 * this.reconnectAttempts);
        }
    }

    handleWorldSync(msg) {
        this.worldVersion = msg.version;

        if (this.engine && msg.blocks) {
            this.loadWorldFromHost(msg.blocks);
        }

        if (msg.timeOfDay && this.network.setTimeOfDay) {
            this.network.setTimeOfDay(msg.timeOfDay);
        }

        if (this.onWorldSync) this.onWorldSync(msg);
    }

    loadWorldFromHost(csvData) {
        if (!this.engine || !csvData) return;

        try {
            const lines = csvData.trim().split('\n');
            let blockCount = 0;

            for (const line of lines) {
                const parts = line.split(',');
                if (parts.length === 4) {
                    const x = parseInt(parts[0]);
                    const y = parseInt(parts[1]);
                    const z = parseInt(parts[2]);
                    const blockType = parseInt(parts[3]);

                    if (!isNaN(x) && !isNaN(y) && !isNaN(z) && !isNaN(blockType)) {
                        this.engine.setBlock(x, y, z, blockType);
                        blockCount++;
                    }
                }
            }

        } catch (e) {
            // parse error handled silently
        }
    }

    handleBlockUpdate(msg) {
        if (msg.sender === this.network.playerId) return;

        if (this.engine) {
            if (msg.action === 'place' || msg.action === 'break') {
                this.engine.setBlock(msg.x, msg.y, msg.z, msg.blockType || 0);
            } else if (msg.action === 'interact') {
                this.worldVersion = msg.version || this.worldVersion + 1;
            }
        }

        this.worldVersion = msg.version || this.worldVersion + 1;

        if (this.onBlockUpdate) this.onBlockUpdate(msg);
    }

    handlePlayerState(msg) {
        if (msg.sender === this.network.playerId) return;

        if (this.onPlayerState) this.onPlayerState(msg);
    }

    handleChatMessage(msg) {
        if (this.onChatMessage) this.onChatMessage(msg);
    }

    handleError(err) {
        if (this.onError) this.onError(err);
    }

    sendBlockUpdate(x, y, z, blockType, action) {
        if (!this.isConnected) return;
        this.network.sendBlockUpdate(x, y, z, blockType, action);
    }

    sendPlayerState(state) {
        if (!this.isConnected) return;
        this.network.sendPlayerState(state);
    }

    sendChat(message) {
        if (!this.isConnected) return;
        this.network.sendChat(message);
    }

    requestPlayerList() {
        if (!this.isConnected) return;
        this.network.sendMessage({
            type: 'chat',
            sender: this.network.playerId,
            message: '/players'
        });
    }

    disconnect() {
        if (this.reconnectTimer) clearTimeout(this.reconnectTimer);
        this.isConnected = false;
        this.isJoining = false;
        this.reconnectAttempts = 0;
        this.network.disconnect();
    }

    getPlayerList() { return this.playerList; }
    getRoomCode() { return this.roomCode; }
    isHostConnected() { return this.isConnected; }
}

window.ClientSystem = ClientSystem;
