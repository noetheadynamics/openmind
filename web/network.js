class NetworkManager {
    constructor(options) {
        this.role = 'single'; // single, host, client
        this.roomCode = null;
        this.playerId = null;
        this.playerName = 'Player';
        this.playerColor = '#8b5cf6';

        this.localPeer = null;
        this.peers = new Map(); // peerId -> { connection, playerName, playerColor }
        this.dataChannels = new Map();

        this.ws = null;
        this.wsReconnectTimer = null;
        this.wsReconnectDelay = 1000;

        this.signalingUrl = options?.signalingUrl || 'wss://signaling.openmind.dev';
        this.localSignaling = null;

        this.onPeerConnected = null;
        this.onPeerDisconnected = null;
        this.onWorldSync = null;
        this.onBlockUpdate = null;
        this.onPlayerState = null;
        this.onChatMessage = null;
        this.onPing = null;
        this.onError = null;

        this.latency = 0;
        this.pingInterval = null;
        this.lastPing = 0;

        this.stats = {
            bytesSent: 0,
            bytesReceived: 0,
            messagesSent: 0,
            messagesReceived: 0,
            peersConnected: 0
        };
    }

    generatePlayerId() {
        return 'P' + Date.now().toString(36) + Math.random().toString(36).substring(2, 6);
    }

    generateRoomCode() {
        const chars = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';
        let code = '';
        for (let i = 0; i < 6; i++) code += chars[Math.floor(Math.random() * chars.length)];
        return code;
    }

    generateRandomColor() {
        const colors = ['#8b5cf6','#ef4444','#22c55e','#3b82f6','#f59e0b','#ec4899','#06b6d4','#f97316'];
        return colors[Math.floor(Math.random() * colors.length)];
    }

    async connectSignaling() {
        try {
            if (this.ws && this.ws.readyState === WebSocket.OPEN) return;

            this.ws = new WebSocket(this.signalingUrl);
            this.ws.binaryType = 'arraybuffer';

            this.ws.onopen = () => {
                this.wsReconnectDelay = 1000;
                this.sendSignaling({ type: 'register', playerId: this.playerId });
            };

            this.ws.onmessage = (event) => {
                try {
                    const msg = JSON.parse(event.data);
                    this.handleSignalingMessage(msg);
                } catch (e) {
                    console.error('[NET] Failed to parse signaling message:', e);
                }
            };

            this.ws.onclose = () => {
                this.scheduleReconnect();
            };

            this.ws.onerror = (err) => {
                console.error('[NET] Signaling error:', err);
                if (this.onError) this.onError('Signaling server unavailable. Running in offline mode.');
            };
        } catch (e) {
            console.error('[NET] Failed to connect signaling:', e);
            if (this.onError) this.onError('Could not reach signaling server');
        }
    }

    scheduleReconnect() {
        if (this.wsReconnectTimer) clearTimeout(this.wsReconnectTimer);
        this.wsReconnectTimer = setTimeout(() => {
            if (this.role !== 'single') this.connectSignaling();
        }, this.wsReconnectDelay);
        this.wsReconnectDelay = Math.min(this.wsReconnectDelay * 2, 30000);
    }

    sendSignaling(msg) {
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify(msg));
        }
    }

    handleSignalingMessage(msg) {
        switch (msg.type) {
            case 'room-created':
                this.roomCode = msg.roomCode;
                break;
            case 'peer-joined':
                this.handlePeerJoin(msg);
                break;
            case 'peer-left':
                this.handlePeerLeave(msg);
                break;
            case 'offer':
                this.handleOffer(msg);
                break;
            case 'answer':
                this.handleAnswer(msg);
                break;
            case 'ice-candidate':
                this.handleIceCandidate(msg);
                break;
            case 'room-joined':
                this.roomCode = msg.roomCode;
                break;
            case 'error':
                console.error('[NET] Signaling error:', msg.message);
                if (this.onError) this.onError(msg.message);
                break;
        }
    }

    async handlePeerJoin(msg) {
        const peerId = msg.peerId;

        const pc = this.createPeerConnection(peerId);
        const channel = pc.createDataChannel('world', {
            ordered: true,
            maxRetransmits: 2
        });

        this.setupDataChannel(channel, peerId);

        const offer = await pc.createOffer();
        await pc.setLocalDescription(offer);

        this.sendSignaling({
            type: 'offer',
            targetPeer: peerId,
            senderPeer: this.playerId,
            offer: pc.localDescription,
            playerName: this.playerName,
            playerColor: this.playerColor
        });
    }

    async handleOffer(msg) {
        const peerId = msg.senderPeer;

        const pc = this.createPeerConnection(peerId);
        await pc.setRemoteDescription(new RTCSessionDescription(msg.offer));

        const answer = await pc.createAnswer();
        await pc.setLocalDescription(answer);

        this.sendSignaling({
            type: 'answer',
            targetPeer: peerId,
            senderPeer: this.playerId,
            answer: pc.localDescription
        });

        const peerInfo = this.peers.get(peerId);
        if (peerInfo) {
            peerInfo.playerName = msg.playerName;
            peerInfo.playerColor = msg.playerColor;
        }
    }

    async handleAnswer(msg) {
        const peerId = msg.senderPeer;
        const peer = this.peers.get(peerId);
        if (peer) {
            await peer.connection.setRemoteDescription(new RTCSessionDescription(msg.answer));
        }
    }

    async handleIceCandidate(msg) {
        const peerId = msg.senderPeer;
        const peer = this.peers.get(peerId);
        if (peer) {
            await peer.connection.addIceCandidate(new RTCIceCandidate(msg.candidate));
        }
    }

    handlePeerLeave(msg) {
        const peerId = msg.peerId;
        if (this.peers.has(peerId)) {
            const peer = this.peers.get(peerId);
            peer.connection.close();
            this.peers.delete(peerId);
            this.dataChannels.delete(peerId);
            this.stats.peersConnected = this.peers.size;
            if (this.onPeerDisconnected) this.onPeerDisconnected(peerId, peer.playerName);
        }
    }

    createPeerConnection(peerId) {
        const config = {
            iceServers: [
                { urls: 'stun:stun.l.google.com:19302' },
                { urls: 'stun:stun1.l.google.com:19302' }
            ]
        };

        const pc = new RTCPeerConnection(config);

        const peerInfo = {
            connection: pc,
            playerName: 'Unknown',
            playerColor: '#ffffff',
            playerState: null,
            lastStateTime: 0
        };

        this.peers.set(peerId, peerInfo);

        pc.onicecandidate = (event) => {
            if (event.candidate) {
                this.sendSignaling({
                    type: 'ice-candidate',
                    targetPeer: peerId,
                    senderPeer: this.playerId,
                    candidate: event.candidate
                });
            }
        };

        pc.onconnectionstatechange = () => {
            if (pc.connectionState === 'connected') {
                this.stats.peersConnected = this.peers.size;
                if (this.onPeerConnected) {
                    this.onPeerConnected(peerId, peerInfo.playerName, peerInfo.playerColor);
                }
            } else if (pc.connectionState === 'disconnected' || pc.connectionState === 'failed') {
                pc.close();
                this.peers.delete(peerId);
                this.dataChannels.delete(peerId);
                this.stats.peersConnected = this.peers.size;
                if (this.onPeerDisconnected) this.onPeerDisconnected(peerId, peerInfo.playerName);
            }
        };

        pc.ondatachannel = (event) => {
            this.setupDataChannel(event.channel, peerId);
        };

        return pc;
    }

    setupDataChannel(channel, peerId) {
        channel.binaryType = 'arraybuffer';

        channel.onopen = () => {
            this.dataChannels.set(peerId, channel);
        };

        channel.onclose = () => {
            this.dataChannels.delete(peerId);
        };

        channel.onmessage = (event) => {
            this.handlePeerMessage(peerId, event.data);
        };
    }

    handlePeerMessage(peerId, data) {
        this.stats.bytesReceived += data.byteLength || data.length;
        this.stats.messagesReceived++;

        try {
            let msg;
            if (data instanceof ArrayBuffer) {
                const decoder = new TextDecoder();
                msg = JSON.parse(decoder.decode(data));
            } else {
                msg = JSON.parse(data);
            }

            switch (msg.type) {
                case 'world-sync':
                    if (this.onWorldSync) this.onWorldSync(msg);
                    break;
                case 'block-update':
                    if (this.onBlockUpdate) this.onBlockUpdate(msg);
                    break;
                case 'player-state':
                    this.handlePlayerState(peerId, msg);
                    break;
                case 'chat':
                    if (this.onChatMessage) this.onChatMessage(msg);
                    break;
                case 'ping':
                    this.handlePing(msg);
                    break;
                case 'pong':
                    this.handlePong(msg);
                    break;
                case 'interaction':
                    if (this.onBlockUpdate) this.onBlockUpdate(msg);
                    break;
                case 'permission-change':
                    if (this.onPlayerState) this.onPlayerState(msg);
                    break;
                case 'kick':
                    if (this.onError) this.onError('Kicked by host: ' + (msg.reason || ''));
                    this.disconnect();
                    break;
            }
        } catch (e) {
            console.error('[NET] Failed to parse message:', e);
        }
    }

    handlePlayerState(peerId, msg) {
        const peer = this.peers.get(peerId);
        if (peer) {
            peer.playerState = msg;
            peer.lastStateTime = Date.now();
        }
        if (this.onPlayerState) this.onPlayerState(msg);
    }

    handlePing(msg) {
        this.sendMessage({ type: 'pong', timestamp: msg.timestamp, sender: this.playerId });
    }

    handlePong(msg) {
        this.latency = Date.now() - msg.timestamp;
        if (this.onPing) this.onPing(this.latency);
    }

    sendMessage(msg) {
        const data = JSON.stringify(msg);
        const bytes = new TextEncoder().encode(data).byteLength;
        this.stats.bytesSent += bytes;
        this.stats.messagesSent++;

        if (this.role === 'host') {
            this.broadcast(data);
        } else if (this.role === 'client') {
            this.sendToHost(data);
        }
    }

    sendBlockUpdate(x, y, z, blockType, action) {
        this.sendMessage({
            type: 'block-update',
            x, y, z, blockType, action,
            sender: this.playerId
        });
    }

    sendPlayerState(state) {
        this.sendMessage({
            type: 'player-state',
            sender: this.playerId,
            playerName: this.playerName,
            playerColor: this.playerColor,
            state
        });
    }

    sendChat(message) {
        this.sendMessage({
            type: 'chat',
            sender: this.playerId,
            playerName: this.playerName,
            playerColor: this.playerColor,
            message
        });
    }

    sendPing() {
        this.lastPing = Date.now();
        this.sendMessage({ type: 'ping', timestamp: this.lastPing, sender: this.playerId });
    }

    broadcast(data) {
        this.dataChannels.forEach((channel) => {
            if (channel.readyState === 'open') {
                try {
                    channel.send(data);
                } catch (e) {
                    console.error('[NET] Broadcast error:', e);
                }
            }
        });
    }

    sendToHost(data) {
        const hostChannel = this.dataChannels.get('host');
        if (hostChannel && hostChannel.readyState === 'open') {
            hostChannel.send(data);
        }
    }

    sendToPeer(peerId, data) {
        const channel = this.dataChannels.get(peerId);
        if (channel && channel.readyState === 'open') {
            channel.send(data);
        }
    }

    startPing() {
        this.pingInterval = setInterval(() => this.sendPing(), 3000);
    }

    stopPing() {
        if (this.pingInterval) clearInterval(this.pingInterval);
    }

    disconnect() {
        this.stopPing();

        if (this.wsReconnectTimer) {
            clearTimeout(this.wsReconnectTimer);
            this.wsReconnectTimer = null;
        }

        this.dataChannels.forEach((ch) => ch.close());
        this.dataChannels.clear();

        this.peers.forEach((p) => p.connection.close());
        this.peers.clear();

        if (this.ws) {
            this.ws.close();
            this.ws = null;
        }

        this.role = 'single';
        this.roomCode = null;
        this.stats.peersConnected = 0;
    }

    getStats() {
        return { ...this.stats, latency: this.latency };
    }
}

window.NetworkManager = NetworkManager;
