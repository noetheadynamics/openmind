/**
 * OpenMind – Sound System
 * Web Audio API, 3D positional audio, all sound types
 */
(function() {
    'use strict';

    const SoundType = Object.freeze({
        FOOTSTEP_DIRT: 'footstep_dirt',
        FOOTSTEP_STONE: 'footstep_stone',
        FOOTSTEP_WOOD: 'footstep_wood',
        FOOTSTEP_METAL: 'footstep_metal',
        BLOCK_PLACE: 'block_place',
        BLOCK_BREAK: 'block_break',
        DOOR_OPEN: 'door_open',
        DOOR_CLOSE: 'door_close',
        BUTTON_PRESS: 'button_press',
        EXPLOSION: 'explosion',
        FIRE_AMBIENT: 'fire_ambient',
        WATER_SPLASH: 'water_splash',
        RAIN_AMBIENT: 'rain_ambient',
        WIND_AMBIENT: 'wind_ambient',
        AGENT_SPEECH: 'agent_speech',
        PICKUP: 'pickup',
        CRAFT: 'craft',
        INTERACT: 'interact'
    });

    const BlockSoundMap = {
        1: 'stone', 2: 'wood', 3: 'wood', 5: 'stone', 9: 'metal',
        10: 'metal', 11: 'metal', 12: 'metal', 13: 'metal', 14: 'stone'
    };

    class SoundSynth {
        constructor(ctx) { this.ctx = ctx; }

        noise(duration) {
            const sr = this.ctx.sampleRate;
            const len = sr * duration;
            const buf = this.ctx.createBuffer(1, len, sr);
            const data = buf.getChannelData(0);
            for (let i = 0; i < len; i++) data[i] = Math.random() * 2 - 1;
            return buf;
        }

        sine(freq, duration) {
            const sr = this.ctx.sampleRate;
            const len = sr * duration;
            const buf = this.ctx.createBuffer(1, len, sr);
            const data = buf.getChannelData(0);
            for (let i = 0; i < len; i++) data[i] = Math.sin(2 * Math.PI * freq * i / sr);
            return buf;
        }

        footstep(type) {
            const dur = 0.08;
            const freq = type === 'metal' ? 800 : type === 'stone' ? 400 : type === 'wood' ? 250 : 150;
            const buf = this.sine(freq, dur);
            const data = buf.getChannelData(0);
            for (let i = 0; i < data.length; i++) {
                data[i] *= Math.exp(-i / (data.length * 0.2));
                data[i] += (Math.random() * 2 - 1) * 0.3 * Math.exp(-i / (data.length * 0.1));
            }
            return buf;
        }

        blockPlace() {
            const dur = 0.12;
            const buf = this.noise(dur);
            const data = buf.getChannelData(0);
            for (let i = 0; i < data.length; i++) data[i] *= Math.exp(-i / (data.length * 0.15));
            return buf;
        }

        blockBreak() {
            const dur = 0.15;
            const buf = this.noise(dur);
            const data = buf.getChannelData(0);
            for (let i = 0; i < data.length; i++) data[i] *= Math.exp(-i / (data.length * 0.1)) * (1 + Math.sin(i * 0.1) * 0.3);
            return buf;
        }

        door(closing) {
            const dur = 0.3;
            const f = closing ? 200 : 150;
            const buf = this.sine(f, dur);
            const data = buf.getChannelData(0);
            for (let i = 0; i < data.length; i++) {
                const t = i / data.length;
                data[i] *= Math.sin(t * Math.PI) * 0.5;
                data[i] += (Math.random() * 2 - 1) * 0.1 * Math.exp(-t * 3);
            }
            return buf;
        }

        buttonPress() {
            const dur = 0.05;
            const buf = this.sine(600, dur);
            const data = buf.getChannelData(0);
            for (let i = 0; i < data.length; i++) data[i] *= Math.exp(-i / (data.length * 0.1));
            return buf;
        }

        explosion() {
            const dur = 0.5;
            const buf = this.noise(dur);
            const data = buf.getChannelData(0);
            for (let i = 0; i < data.length; i++) data[i] *= Math.exp(-i / (data.length * 0.3)) * 2;
            return buf;
        }

        fire() {
            const dur = 0.4;
            const buf = this.noise(dur);
            const data = buf.getChannelData(0);
            for (let i = 0; i < data.length; i++) {
                const t = i / data.length;
                data[i] *= (0.5 + Math.sin(t * 20) * 0.5) * Math.exp(-t * 2);
            }
            return buf;
        }

        splash() {
            const dur = 0.2;
            const buf = this.noise(dur);
            const data = buf.getChannelData(0);
            for (let i = 0; i < data.length; i++) data[i] *= Math.exp(-i / (data.length * 0.08));
            return buf;
        }

        rain() {
            const dur = 1.0;
            const buf = this.noise(dur);
            const data = buf.getChannelData(0);
            for (let i = 0; i < data.length; i++) data[i] *= 0.15 + Math.random() * 0.05;
            return buf;
        }

        wind() {
            const dur = 2.0;
            const buf = this.noise(dur);
            const data = buf.getChannelData(0);
            for (let i = 0; i < data.length; i++) {
                const t = i / data.length;
                data[i] *= Math.sin(t * Math.PI) * 0.2 * (1 + Math.sin(t * 10) * 0.3);
            }
            return buf;
        }

        craft() {
            const dur = 0.2;
            const buf = this.sine(500, dur);
            const data = buf.getChannelData(0);
            for (let i = 0; i < data.length; i++) {
                const t = i / data.length;
                data[i] *= Math.sin(t * Math.PI * 3) * 0.5 * (1 - t);
            }
            return buf;
        }

        pickup() {
            const dur = 0.15;
            const buf = this.sine(800, dur);
            const data = buf.getChannelData(0);
            for (let i = 0; i < data.length; i++) data[i] *= Math.exp(-i / (data.length * 0.3)) * 0.5;
            return buf;
        }

        agentSpeech() {
            const dur = 0.3;
            const buf = this.sine(300 + Math.random() * 200, dur);
            const data = buf.getChannelData(0);
            for (let i = 0; i < data.length; i++) {
                data[i] *= Math.sin(i / data.length * Math.PI * 8) * 0.3;
            }
            return buf;
        }
    }

    class SoundSystem {
        constructor() {
            this.ctx = null;
            this.masterGain = null;
            this.listener = null;
            this.sources = new Map();
            this.ambientSources = new Map();
            this.bufferCache = new Map();
            this.synth = null;
            this.enabled = true;
            this.volume = 0.5;
            this.maxSources = 32;
            this.sourceCount = 0;
            this.positional = true;
            this.listenerX = 0;
            this.listenerY = 0;
            this.listenerZ = 0;
        }

        async init() {
            try {
                this.ctx = new (window.AudioContext || window.webkitAudioContext)();
                this.masterGain = this.ctx.createGain();
                this.masterGain.gain.value = this.volume;
                this.masterGain.connect(this.ctx.destination);

                this.listener = this.ctx.listener;
                this.synth = new SoundSynth(this.ctx);

                this.generateBuffers();
                return true;
            } catch (e) {
                return false;
            }
        }

        generateBuffers() {
            const types = ['dirt', 'stone', 'wood', 'metal'];
            for (const t of types) {
                this.bufferCache.set('footstep_' + t, this.synth.footstep(t));
            }
            this.bufferCache.set('block_place', this.synth.blockPlace());
            this.bufferCache.set('block_break', this.synth.blockBreak());
            this.bufferCache.set('door_open', this.synth.door(false));
            this.bufferCache.set('door_close', this.synth.door(true));
            this.bufferCache.set('button_press', this.synth.buttonPress());
            this.bufferCache.set('explosion', this.synth.explosion());
            this.bufferCache.set('fire_ambient', this.synth.fire());
            this.bufferCache.set('water_splash', this.synth.splash());
            this.bufferCache.set('rain_ambient', this.synth.rain());
            this.bufferCache.set('wind_ambient', this.synth.wind());
            this.bufferCache.set('craft', this.synth.craft());
            this.bufferCache.set('pickup', this.synth.pickup());
            this.bufferCache.set('agent_speech', this.synth.agentSpeech());
        }

        play(type, x, y, z, options) {
            if (!this.enabled || !this.ctx) return null;
            if (this.ctx.state === 'suspended') this.ctx.resume();
            if (this.sourceCount >= this.maxSources) return null;

            const key = type;
            const buf = this.bufferCache.get(key);
            if (!buf) return null;

            const source = this.ctx.createBufferSource();
            source.buffer = buf;

            const gain = this.ctx.createGain();
            gain.gain.value = options?.volume ?? 0.5;

            if (this.positional && x !== undefined) {
                const panner = this.ctx.createPanner();
                panner.panningModel = 'HRTF';
                panner.distanceModel = 'inverse';
                panner.refDistance = 1;
                panner.maxDistance = 100;
                panner.rolloffFactor = 1.5;
                panner.setPosition(x, y, z);

                source.connect(panner);
                panner.connect(gain);
            } else {
                source.connect(gain);
            }

            gain.connect(this.masterGain);

            if (options?.loop) {
                source.loop = true;
                const id = options.id || type + '_' + Math.random().toString(36).substr(2, 6);
                this.ambientSources.set(id, { source, gain, panner: null });
                source.start();
                this.sourceCount++;
                return id;
            }

            source.start();
            source.onended = () => { this.sourceCount--; };
            this.sourceCount++;
            return null;
        }

        playAtBlock(blockType, x, y, z) {
            const soundType = BlockSoundMap[blockType] || 'dirt';
            this.play('footstep_' + soundType, x, y, z, { volume: 0.3 });
        }

        playBlockPlace(x, y, z, blockType) {
            this.play('block_place', x, y, z, { volume: 0.4 });
        }

        playBlockBreak(x, y, z, blockType) {
            this.play('block_break', x, y, z, { volume: 0.5 });
        }

        playDoorOpen(x, y, z) { this.play('door_open', x, y, z, { volume: 0.5 }); }
        playDoorClose(x, y, z) { this.play('door_close', x, y, z, { volume: 0.5 }); }
        playButtonPress(x, y, z) { this.play('button_press', x, y, z, { volume: 0.4 }); }
        playExplosion(x, y, z) { this.play('explosion', x, y, z, { volume: 0.8 }); }
        playSplash(x, y, z) { this.play('water_splash', x, y, z, { volume: 0.5 }); }
        playCraft(x, y, z) { this.play('craft', x, y, z, { volume: 0.4 }); }
        playPickup(x, y, z) { this.play('pickup', x, y, z, { volume: 0.3 }); }
        playAgentSpeech(x, y, z) { this.play('agent_speech', x, y, z, { volume: 0.3 }); }

        startAmbient(type, x, y, z) {
            if (this.ambientSources.has(type)) return;
            this.play(type, x || 0, y || 0, z || 0, { volume: 0.2, loop: true, id: type });
        }

        stopAmbient(type) {
            const a = this.ambientSources.get(type);
            if (a) {
                try { a.source.stop(); } catch (e) {}
                this.ambientSources.delete(type);
                this.sourceCount--;
            }
        }

        updateListener(x, y, z) {
            this.listenerX = x;
            this.listenerY = y;
            this.listenerZ = z;
            if (this.listener && this.listener.positionX) {
                this.listener.positionX.value = x;
                this.listener.positionY.value = y;
                this.listener.positionZ.value = z;
            } else if (this.listener) {
                this.listener.setPosition(x, y, z);
            }
        }

        setVolume(v) {
            this.volume = v;
            if (this.masterGain) this.masterGain.gain.value = v;
        }

        setEnabled(v) { this.enabled = v; if (!v) this.stopAll(); }

        stopAll() {
            for (const [id, a] of this.ambientSources) {
                try { a.source.stop(); } catch (e) {}
            }
            this.ambientSources.clear();
        }

        setWeather(weather, x, y, z) {
            if (weather === 'rain' || weather === 'storm') {
                this.startAmbient('rain_ambient', x, y, z);
                this.startAmbient('wind_ambient', x, y, z);
            } else {
                this.stopAmbient('rain_ambient');
                this.stopAmbient('wind_ambient');
            }
        }
    }

    window.SoundType = SoundType;
    window.SoundSystem = SoundSystem;
})();
