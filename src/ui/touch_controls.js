class TouchControls {
    constructor() {
        this.enabled = false;
        this.engine = null;
        this.renderer = null;
        this.camera = null;
        this.scene = null;

        this.joystick = { x: 0, y: 0, active: false, id: null, baseX: 0, baseY: 0 };
        this.look = { active: false, id: null, lastX: 0, lastY: 0 };
        this.zoom = { active: false, id: null, startDist: 0, currentDist: 0 };

        this.tapTimer = null;
        this.tapCount = 0;
        this.lastTap = 0;

        this.selectedSlot = 0;
        this.hotbarSlots = 9;

        this.moveSpeed = 5;
        this.lookSensitivity = 0.003;
        this.jumpVelocity = 8;

        this.velocity = { x: 0, y: 0, z: 0 };
        this.grounded = true;
        this.playerHeight = 2;

        this.hintsShown = false;
        this.hintsEl = null;

        this.onBlockPlace = null;
        this.onBlockBreak = null;
        this.onSlotChange = null;
        this.onJump = null;

        this._onTouchStart = this._onTouchStart.bind(this);
        this._onTouchMove = this._onTouchMove.bind(this);
        this._onTouchEnd = this._onTouchEnd.bind(this);
        this._onContextMenu = this._onContextMenu.bind(this);

        this._els = {};
        this._playerPos = { x: 0, y: 10, z: 0 };
        this._playerYaw = 0;
        this._playerPitch = 0;
        this._lastFrameTime = 0;
        this._animFrame = null;
        this._crosshairEl = null;
    }

    init(engine, renderer) {
        this.engine = engine;
        this.renderer = renderer;
        if (renderer?.scene) this.scene = renderer.scene;
        if (renderer?.camera) this.camera = renderer.camera;
        this._injectStyles();
        this._createUI();
        this._bindEvents();
    }

    enable() {
        this.enabled = true;
        Object.values(this._els).forEach(el => { if (el) el.style.display = ''; });
        if (this._els.lookArea) this._els.lookArea.style.display = 'block';
        this._startLoop();
        if (!this.hintsShown) this._showHints();
    }

    disable() {
        this.enabled = false;
        Object.values(this._els).forEach(el => { if (el) el.style.display = 'none'; });
        if (this._crosshairEl) this._crosshairEl.style.display = 'none';
        this._stopLoop();
        this._hideHints();
        this.joystick.active = false;
        this.look.active = false;
    }

    _injectStyles() {
        if (document.getElementById('tc-styles')) return;
        const s = document.createElement('style');
        s.id = 'tc-styles';
        s.textContent = `
            @keyframes tc-pulse { 0%,100%{opacity:0.5;transform:scale(1)} 50%{opacity:0.8;transform:scale(1.05)} }
            @keyframes tc-ripple { 0%{transform:translate(-50%,-50%) scale(0);opacity:0.6} 100%{transform:translate(-50%,-50%) scale(2.5);opacity:0} }
            @keyframes tc-fadeIn { from{opacity:0;transform:scale(0.9)} to{opacity:1;transform:scale(1)} }
            @keyframes tc-slideUp { from{opacity:0;transform:translateY(20px)} to{opacity:1;transform:translateY(0)} }

            #tc-joystick-zone {
                position:fixed; bottom:30px; left:20px; width:140px; height:140px; z-index:9998;
                display:none; pointer-events:auto;
            }
            #tc-joystick-base {
                width:140px; height:140px; border-radius:50%;
                background: radial-gradient(circle, rgba(139,92,246,0.12) 0%, rgba(139,92,246,0.04) 70%, transparent 100%);
                border: 2px solid rgba(139,92,246,0.25);
                box-shadow: 0 0 20px rgba(139,92,246,0.1), inset 0 0 30px rgba(139,92,246,0.05);
                position:absolute; top:0; left:0;
                transition: border-color 0.2s, box-shadow 0.2s;
            }
            #tc-joystick-zone.active #tc-joystick-base {
                border-color: rgba(139,92,246,0.5);
                box-shadow: 0 0 30px rgba(139,92,246,0.25), inset 0 0 40px rgba(139,92,246,0.1);
            }
            #tc-joystick-knob {
                width:56px; height:56px; border-radius:50%;
                background: radial-gradient(circle at 35% 35%, rgba(196,181,253,0.5), rgba(139,92,246,0.35));
                border: 2px solid rgba(196,181,253,0.4);
                box-shadow: 0 4px 15px rgba(139,92,246,0.3), inset 0 1px 3px rgba(255,255,255,0.15);
                position:absolute; top:50%; left:50%; transform:translate(-50%,-50%);
                transition: none;
                backdrop-filter: blur(4px);
            }
            #tc-joystick-knob::after {
                content:''; position:absolute; top:50%; left:50%; transform:translate(-50%,-50%);
                width:12px; height:12px; border-radius:50%;
                background: rgba(255,255,255,0.25);
                box-shadow: 0 0 8px rgba(139,92,246,0.4);
            }

            #tc-look-zone {
                position:fixed; top:0; right:0; width:55%; height:100%; z-index:9997;
                display:none; background:transparent; pointer-events:none;
            }

            #tc-crosshair {
                position:fixed; top:50%; left:50%; transform:translate(-50%,-50%);
                width:24px; height:24px; z-index:9996; pointer-events:none; display:none;
            }
            #tc-crosshair::before, #tc-crosshair::after {
                content:''; position:absolute; background:rgba(255,255,255,0.7);
                box-shadow: 0 0 4px rgba(0,0,0,0.5);
            }
            #tc-crosshair::before { width:2px; height:24px; left:11px; top:0; border-radius:1px; }
            #tc-crosshair::after { width:24px; height:2px; top:11px; left:0; border-radius:1px; }

            #tc-actions {
                position:fixed; right:20px; bottom:120px; display:none;
                flex-direction:column; gap:12px; z-index:9998;
                animation: tc-slideUp 0.3s ease;
            }
            .tc-action-btn {
                width:64px; height:64px; border-radius:50%; border:none;
                display:flex; align-items:center; justify-content:center;
                font-size:22px; color:white; cursor:pointer;
                backdrop-filter: blur(12px); -webkit-backdrop-filter: blur(12px);
                transition: transform 0.15s, box-shadow 0.15s;
                position:relative; overflow:hidden;
            }
            .tc-action-btn:active { transform:scale(0.9); }
            .tc-action-btn .tc-ripple {
                position:absolute; border-radius:50%; width:60px; height:60px;
                background:rgba(255,255,255,0.3); transform:translate(-50%,-50%) scale(0);
                animation: tc-ripple 0.5s ease-out;
            }
            .tc-btn-jump {
                background: linear-gradient(135deg, rgba(59,130,246,0.45), rgba(37,99,235,0.35));
                border: 2px solid rgba(96,165,250,0.4);
                box-shadow: 0 4px 15px rgba(59,130,246,0.25);
            }
            .tc-btn-jump:active { box-shadow: 0 2px 8px rgba(59,130,246,0.4); }
            .tc-btn-place {
                background: linear-gradient(135deg, rgba(34,197,94,0.45), rgba(22,163,74,0.35));
                border: 2px solid rgba(74,222,128,0.4);
                box-shadow: 0 4px 15px rgba(34,197,94,0.25);
            }
            .tc-btn-place:active { box-shadow: 0 2px 8px rgba(34,197,94,0.4); }
            .tc-btn-break {
                background: linear-gradient(135deg, rgba(239,68,68,0.45), rgba(220,38,38,0.35));
                border: 2px solid rgba(248,113,113,0.4);
                box-shadow: 0 4px 15px rgba(239,68,68,0.25);
            }
            .tc-btn-break:active { box-shadow: 0 2px 8px rgba(239,68,68,0.4); }
            .tc-btn-inv {
                width:52px; height:52px; font-size:18px; margin-top:4px;
                background: linear-gradient(135deg, rgba(139,92,246,0.45), rgba(124,58,237,0.35));
                border: 2px solid rgba(167,139,250,0.4);
                box-shadow: 0 4px 15px rgba(139,92,246,0.25);
            }

            #tc-hotbar {
                position:fixed; bottom:12px; left:50%; transform:translateX(-50%);
                display:none; gap:5px; padding:8px 12px;
                background: rgba(10,10,30,0.75);
                border: 1px solid rgba(139,92,246,0.2);
                border-radius:14px;
                backdrop-filter: blur(16px); -webkit-backdrop-filter: blur(16px);
                box-shadow: 0 8px 32px rgba(0,0,0,0.4), 0 0 20px rgba(139,92,246,0.08);
                z-index:9998;
                animation: tc-slideUp 0.3s ease;
            }
            .tc-slot {
                width:46px; height:46px; border-radius:10px;
                border: 2px solid rgba(100,100,200,0.2);
                background: rgba(30,30,60,0.6);
                display:flex; align-items:center; justify-content:center;
                font-size:11px; color:rgba(255,255,255,0.5); cursor:pointer;
                position:relative; overflow:hidden;
                transition: border-color 0.2s, background 0.2s, box-shadow 0.2s, transform 0.15s;
            }
            .tc-slot:active { transform:scale(0.92); }
            .tc-slot.selected {
                border-color: rgba(139,92,246,0.7);
                background: linear-gradient(135deg, rgba(139,92,246,0.25), rgba(139,92,246,0.1));
                box-shadow: 0 0 12px rgba(139,92,246,0.25), inset 0 0 12px rgba(139,92,246,0.08);
                color: white;
            }
            .tc-slot-block {
                width:26px; height:26px; border-radius:4px;
                box-shadow: 0 2px 6px rgba(0,0,0,0.3), inset 0 1px 2px rgba(255,255,255,0.1);
            }
            .tc-slot-num {
                position:absolute; bottom:2px; right:4px; font-size:8px;
                color:rgba(255,255,255,0.35); font-weight:bold;
            }

            #tc-hints {
                position:fixed; top:50%; left:50%; transform:translate(-50%,-50%);
                z-index:10001;
                background: rgba(10,10,30,0.95);
                border: 1px solid rgba(139,92,246,0.3);
                border-radius:20px; padding:28px 24px;
                backdrop-filter: blur(20px); -webkit-backdrop-filter: blur(20px);
                max-width:320px; width:85%;
                box-shadow: 0 20px 60px rgba(0,0,0,0.5), 0 0 40px rgba(139,92,246,0.1);
                animation: tc-fadeIn 0.3s ease;
                color:#e0e0e0; font-size:13px;
            }
            #tc-hints h3 {
                color:#c4b5fd; margin-bottom:16px; font-size:16px; text-align:center;
                text-shadow: 0 0 12px rgba(139,92,246,0.3);
            }
            #tc-hints .tc-hint-row {
                display:flex; align-items:center; gap:12px; padding:8px 0;
                border-bottom:1px solid rgba(100,100,200,0.1);
            }
            #tc-hints .tc-hint-row:last-child { border-bottom:none; }
            #tc-hints .tc-hint-icon {
                width:36px; height:36px; border-radius:10px; flex-shrink:0;
                display:flex; align-items:center; justify-content:center; font-size:18px;
                background:rgba(139,92,246,0.15); border:1px solid rgba(139,92,246,0.2);
            }
            #tc-hints .tc-hint-text { flex:1; line-height:1.4; }
            #tc-hints .tc-hint-label { color:#c4b5fd; font-size:11px; font-weight:600; }
            #tc-hints .tc-hint-desc { color:#94a3b8; font-size:12px; }
            #tc-hints-close {
                margin-top:18px; width:100%; padding:12px;
                border:1px solid rgba(139,92,246,0.4); border-radius:12px;
                background:linear-gradient(135deg, rgba(139,92,246,0.25), rgba(124,58,237,0.2));
                color:#c4b5fd; cursor:pointer; font-size:14px; font-weight:600;
                transition: all 0.2s;
            }
            #tc-hints-close:hover { background:rgba(139,92,246,0.35); border-color:rgba(139,92,246,0.6); }

            @media (max-width:480px) {
                #tc-joystick-zone { width:120px; height:120px; bottom:20px; left:12px; }
                #tc-joystick-base { width:120px; height:120px; }
                #tc-joystick-knob { width:48px; height:48px; }
                .tc-action-btn { width:56px; height:56px; font-size:20px; }
                .tc-btn-inv { width:46px; height:46px; }
                #tc-actions { right:14px; bottom:100px; gap:10px; }
                #tc-hotbar { padding:6px 8px; gap:3px; }
                .tc-slot { width:40px; height:40px; }
                .tc-slot-block { width:22px; height:22px; }
            }
        `;
        document.head.appendChild(s);
    }

    _createUI() {
        this._createJoystick();
        this._createLookZone();
        this._createCrosshair();
        this._createActionButtons();
        this._createHotbar();
    }

    _createJoystick() {
        const zone = document.createElement('div');
        zone.id = 'tc-joystick-zone';
        zone.style.display = 'none';

        const base = document.createElement('div');
        base.id = 'tc-joystick-base';

        const knob = document.createElement('div');
        knob.id = 'tc-joystick-knob';

        zone.appendChild(base);
        zone.appendChild(knob);
        document.body.appendChild(zone);

        this._els.joystickZone = zone;
        this._els.joystickKnob = knob;
    }

    _createLookZone() {
        const zone = document.createElement('div');
        zone.id = 'tc-look-zone';
        zone.style.display = 'none';
        document.body.appendChild(zone);
        this._els.lookArea = zone;
    }

    _createCrosshair() {
        const ch = document.createElement('div');
        ch.id = 'tc-crosshair';
        ch.style.display = 'none';
        document.body.appendChild(ch);
        this._crosshairEl = ch;
    }

    _createActionButtons() {
        const container = document.createElement('div');
        container.id = 'tc-actions';
        container.style.display = 'none';

        const jumpBtn = this._makeActionBtn('tc-btn-jump', '▲', 'Jump');
        jumpBtn.addEventListener('touchstart', (e) => { e.preventDefault(); this._ripple(jumpBtn); this._jump(); });

        const placeBtn = this._makeActionBtn('tc-btn-place', '＋', 'Place');
        placeBtn.addEventListener('touchstart', (e) => { e.preventDefault(); this._ripple(placeBtn); this._placeBlock(); });

        const breakBtn = this._makeActionBtn('tc-btn-break', '✕', 'Break');
        breakBtn.addEventListener('touchstart', (e) => { e.preventDefault(); this._ripple(breakBtn); this._breakBlock(); });

        const invBtn = this._makeActionBtn('tc-btn-inv', '◻', 'Inventory');
        invBtn.addEventListener('touchstart', (e) => {
            e.preventDefault(); this._ripple(invBtn);
            const panel = document.getElementById('panel-inventory');
            if (panel) panel.style.display = panel.style.display === 'none' ? 'block' : 'none';
        });

        container.appendChild(jumpBtn);
        container.appendChild(placeBtn);
        container.appendChild(breakBtn);
        container.appendChild(invBtn);
        document.body.appendChild(container);
        this._els.actions = container;
    }

    _makeActionBtn(className, icon) {
        const btn = document.createElement('button');
        btn.className = 'tc-action-btn ' + className;
        btn.textContent = icon;
        return btn;
    }

    _ripple(btn) {
        const r = document.createElement('span');
        r.className = 'tc-ripple';
        btn.appendChild(r);
        setTimeout(() => r.remove(), 500);
    }

    _createHotbar() {
        const container = document.createElement('div');
        container.id = 'tc-hotbar';
        container.style.display = 'none';

        const blockColors = [
            { bg: 'linear-gradient(135deg, #6b7280, #4b5563)', label: '1' },
            { bg: 'linear-gradient(135deg, #92400e, #78350f)', label: '2' },
            { bg: 'linear-gradient(135deg, #16a34a, #15803d)', label: '3' },
            { bg: 'linear-gradient(135deg, #2563eb, #1d4ed8)', label: '4' },
            { bg: 'linear-gradient(135deg, #eab308, #ca8a04)', label: '5' },
            { bg: 'linear-gradient(135deg, #7dd3fc, #38bdf8)', label: '6' },
            { bg: 'linear-gradient(135deg, #c2410c, #9a3412)', label: '7' },
            { bg: 'linear-gradient(135deg, #22c55e, #16a34a)', label: '8' },
            { bg: 'linear-gradient(135deg, #94a3b8, #64748b)', label: '9' }
        ];

        for (let i = 0; i < this.hotbarSlots; i++) {
            const slot = document.createElement('div');
            slot.className = 'tc-slot' + (i === 0 ? ' selected' : '');
            slot.dataset.slot = i;

            const blockDiv = document.createElement('div');
            blockDiv.className = 'tc-slot-block';
            blockDiv.style.background = blockColors[i].bg;

            const num = document.createElement('div');
            num.className = 'tc-slot-num';
            num.textContent = blockColors[i].label;

            slot.appendChild(blockDiv);
            slot.appendChild(num);

            slot.addEventListener('touchstart', (e) => {
                e.preventDefault();
                this._selectSlot(i);
            });

            container.appendChild(slot);
        }

        document.body.appendChild(container);
        this._els.hotbar = container;
    }

    _bindEvents() {
        document.addEventListener('touchstart', this._onTouchStart, { passive: false });
        document.addEventListener('touchmove', this._onTouchMove, { passive: false });
        document.addEventListener('touchend', this._onTouchEnd, { passive: false });
        document.addEventListener('touchcancel', this._onTouchEnd, { passive: false });
        document.addEventListener('contextmenu', this._onContextMenu);
    }

    _onTouchStart(e) {
        if (!this.enabled) return;
        e.preventDefault();
        for (const touch of e.changedTouches) {
            const x = touch.clientX;
            const y = touch.clientY;
            const w = window.innerWidth;
            const h = window.innerHeight;

            if (x < w * 0.35 && y > h * 0.35) {
                this._startJoystick(touch);
            } else if (x > w * 0.45 && y < h * 0.85 && !e.target.closest('#tc-actions,#tc-hotbar')) {
                this._startLook(touch);
                if (this._crosshairEl) this._crosshairEl.style.display = 'block';
            } else if (!e.target.closest('#tc-actions,#tc-hotbar,#tc-joystick-zone')) {
                this._handleTap(touch);
            }
        }
        if (e.touches.length === 2) this._startZoom(e.touches);
    }

    _onTouchMove(e) {
        if (!this.enabled) return;
        e.preventDefault();
        for (const touch of e.changedTouches) {
            if (touch.identifier === this.joystick.id) this._moveJoystick(touch);
            else if (touch.identifier === this.look.id) this._moveLook(touch);
        }
        if (e.touches.length === 2 && this.zoom.active) this._updateZoom(e.touches);
    }

    _onTouchEnd(e) {
        if (!this.enabled) return;
        for (const touch of e.changedTouches) {
            if (touch.identifier === this.joystick.id) this._endJoystick();
            else if (touch.identifier === this.look.id) {
                this._endLook();
                if (this._crosshairEl) this._crosshairEl.style.display = 'none';
            }
        }
        if (e.touches.length < 2) this.zoom.active = false;
    }

    _onContextMenu(e) { if (this.enabled) e.preventDefault(); }

    _startJoystick(touch) {
        const zone = this._els.joystickZone;
        if (!zone) return;
        const rect = zone.getBoundingClientRect();
        this.joystick.active = true;
        this.joystick.id = touch.identifier;
        this.joystick.baseX = rect.left + rect.width / 2;
        this.joystick.baseY = rect.top + rect.height / 2;
        zone.classList.add('active');
        this._moveJoystick(touch);
    }

    _moveJoystick(touch) {
        if (!this.joystick.active) return;
        const dx = touch.clientX - this.joystick.baseX;
        const dy = touch.clientY - this.joystick.baseY;
        const dist = Math.sqrt(dx * dx + dy * dy);
        const maxDist = 55;
        const cx = dist > maxDist ? (dx / dist) * maxDist : dx;
        const cy = dist > maxDist ? (dy / dist) * maxDist : dy;

        if (this._els.joystickKnob) {
            this._els.joystickKnob.style.transform = `translate(calc(-50% + ${cx}px), calc(-50% + ${cy}px))`;
        }
        this.joystick.x = cx / maxDist;
        this.joystick.y = cy / maxDist;
    }

    _endJoystick() {
        this.joystick.active = false;
        this.joystick.id = null;
        this.joystick.x = 0;
        this.joystick.y = 0;
        if (this._els.joystickKnob) this._els.joystickKnob.style.transform = 'translate(-50%, -50%)';
        if (this._els.joystickZone) this._els.joystickZone.classList.remove('active');
    }

    _startLook(touch) {
        this.look.active = true;
        this.look.id = touch.identifier;
        this.look.lastX = touch.clientX;
        this.look.lastY = touch.clientY;
    }

    _moveLook(touch) {
        if (!this.look.active) return;
        const dx = touch.clientX - this.look.lastX;
        const dy = touch.clientY - this.look.lastY;
        this._playerYaw -= dx * this.lookSensitivity;
        this._playerPitch -= dy * this.lookSensitivity;
        this._playerPitch = Math.max(-Math.PI / 2.2, Math.min(Math.PI / 2.2, this._playerPitch));
        this.look.lastX = touch.clientX;
        this.look.lastY = touch.clientY;
        if (this.camera) {
            this.camera.rotation.order = 'YXZ';
            this.camera.rotation.y = this._playerYaw;
            this.camera.rotation.x = this._playerPitch;
        }
    }

    _endLook() {
        this.look.active = false;
        this.look.id = null;
    }

    _startZoom(touches) {
        const dx = touches[0].clientX - touches[1].clientX;
        const dy = touches[0].clientY - touches[1].clientY;
        this.zoom.active = true;
        this.zoom.startDist = Math.sqrt(dx * dx + dy * dy);
        this.zoom.currentDist = this.zoom.startDist;
    }

    _updateZoom(touches) {
        const dx = touches[0].clientX - touches[1].clientX;
        const dy = touches[0].clientY - touches[1].clientY;
        this.zoom.currentDist = Math.sqrt(dx * dx + dy * dy);
        const scale = this.zoom.startDist / this.zoom.currentDist;
        if (this.camera) {
            this.camera.fov = Math.max(30, Math.min(120, 75 * scale));
            this.camera.updateProjectionMatrix();
        }
    }

    _handleTap(touch) {
        const now = Date.now();
        if (now - this.lastTap < 300) {
            this.tapCount++;
            clearTimeout(this.tapTimer);
            if (this.tapCount >= 2) { this._breakBlock(); this.tapCount = 0; }
        } else {
            this.tapCount = 1;
            this.tapTimer = setTimeout(() => {
                if (this.tapCount === 1) this._placeBlock();
                this.tapCount = 0;
            }, 300);
        }
        this.lastTap = now;
    }

    _placeBlock() {
        if (this.onBlockPlace) { this.onBlockPlace(); return; }
        if (this.renderer?.hitBlock) {
            const h = this.renderer.hitBlock;
            if (h) {
                const types = [1, 2, 3, 5, 7, 6, 9, 10, 11];
                this.engine?.setBlock(h.x + h.normal.x, h.y + h.normal.y, h.z + h.normal.z, types[this.selectedSlot] || 1);
            }
        }
    }

    _breakBlock() {
        if (this.onBlockBreak) { this.onBlockBreak(); return; }
        if (this.renderer?.hitBlock) {
            const h = this.renderer.hitBlock;
            if (h) this.engine?.setBlock(h.x, h.y, h.z, 0);
        }
    }

    _jump() {
        if (this.grounded) { this.velocity.y = this.jumpVelocity; this.grounded = false; }
        if (this.onJump) this.onJump();
    }

    _selectSlot(index) {
        this.selectedSlot = index;
        const slots = this._els.hotbar?.children;
        if (slots) {
            for (let i = 0; i < slots.length; i++) {
                slots[i].classList.toggle('selected', i === index);
            }
        }
        if (this.onSlotChange) this.onSlotChange(index);
    }

    _startLoop() {
        this._lastFrameTime = performance.now();
        const loop = (time) => {
            const dt = Math.min((time - this._lastFrameTime) / 1000, 0.1);
            this._lastFrameTime = time;
            this._update(dt);
            this._animFrame = requestAnimationFrame(loop);
        };
        this._animFrame = requestAnimationFrame(loop);
    }

    _stopLoop() { if (this._animFrame) cancelAnimationFrame(this._animFrame); }

    _update(dt) {
        if (!this.enabled) return;
        const mx = this.joystick.x * this.moveSpeed * dt;
        const mz = this.joystick.y * this.moveSpeed * dt;
        const sin = Math.sin(this._playerYaw);
        const cos = Math.cos(this._playerYaw);
        this._playerPos.x += mx * cos + mz * sin;
        this._playerPos.z += -mx * sin + mz * cos;
        this.velocity.y -= 20 * dt;
        this._playerPos.y += this.velocity.y * dt;
        if (this._playerPos.y <= this.playerHeight) {
            this._playerPos.y = this.playerHeight;
            this.velocity.y = 0;
            this.grounded = true;
        }
        if (this.camera && this.joystick.active) this.camera.position.set(this._playerPos.x, this._playerPos.y, this._playerPos.z);
    }

    _showHints() {
        if (this.hintsEl) return;
        const hints = document.createElement('div');
        hints.id = 'tc-hints';
        hints.innerHTML = `
            <h3>Touch Controls</h3>
            <div class="tc-hint-row"><div class="tc-hint-icon">🕹️</div><div class="tc-hint-text"><div class="tc-hint-label">Move</div><div class="tc-hint-desc">Drag left side of screen</div></div></div>
            <div class="tc-hint-row"><div class="tc-hint-icon">👆</div><div class="tc-hint-text"><div class="tc-hint-label">Look</div><div class="tc-hint-desc">Drag right side of screen</div></div></div>
            <div class="tc-hint-row"><div class="tc-hint-icon">👆</div><div class="tc-hint-text"><div class="tc-hint-label">Place Block</div><div class="tc-hint-desc">Tap on screen or + button</div></div></div>
            <div class="tc-hint-row"><div class="tc-hint-icon">👆👆</div><div class="tc-hint-text"><div class="tc-hint-label">Break Block</div><div class="tc-hint-desc">Double-tap or × button</div></div></div>
            <div class="tc-hint-row"><div class="tc-hint-icon">🤏</div><div class="tc-hint-text"><div class="tc-hint-label">Zoom</div><div class="tc-hint-desc">Pinch with two fingers</div></div></div>
            <div class="tc-hint-row"><div class="tc-hint-icon">▲</div><div class="tc-hint-text"><div class="tc-hint-label">Jump</div><div class="tc-hint-desc">Tap the blue jump button</div></div></div>
            <button id="tc-hints-close">Got it!</button>
        `;
        document.body.appendChild(hints);
        this.hintsEl = hints;
        this.hintsShown = true;
        document.getElementById('tc-hints-close').addEventListener('click', () => this._hideHints());
        this._hintsTimer = setTimeout(() => this._hideHints(), 12000);
    }

    _hideHints() {
        if (this._hintsTimer) { clearTimeout(this._hintsTimer); this._hintsTimer = null; }
        if (this.hintsEl) { this.hintsEl.remove(); this.hintsEl = null; }
    }

    getPlayerPosition() { return { ...this._playerPos }; }
    getPlayerYaw() { return this._playerYaw; }
    getPlayerPitch() { return this._playerPitch; }
    isTouchDevice() {
        return /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent) || (window.matchMedia && window.matchMedia('(pointer: coarse)').matches);
    }

    static detect() {
        return /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent) || (window.matchMedia && window.matchMedia('(pointer: coarse)').matches);
    }

    destroy() {
        this.disable();
        document.removeEventListener('touchstart', this._onTouchStart);
        document.removeEventListener('touchmove', this._onTouchMove);
        document.removeEventListener('touchend', this._onTouchEnd);
        document.removeEventListener('touchcancel', this._onTouchEnd);
        document.removeEventListener('contextmenu', this._onContextMenu);
        const s = document.getElementById('tc-styles');
        if (s) s.remove();
        ['tc-joystick-zone','tc-look-zone','tc-crosshair','tc-actions','tc-hotbar'].forEach(id => {
            const el = document.getElementById(id);
            if (el) el.remove();
        });
    }
}

window.TouchControls = TouchControls;
