/**
 * OpenMind – Loading Screen
 * Boot progress, tips, smooth transitions, failure handling
 */
(function() {
    'use strict';

    const Tips = [
        'Use WASD + QE to move the camera in 3D view',
        'Scroll to zoom, right-drag to orbit',
        'Press 1-9 to select block types from the hotbar',
        'Click to place blocks, Shift+Click to break',
        'Type "weather rain" to change the weather',
        'Type "time 18" for sunset lighting',
        'Use the AI prompt to generate entire worlds',
        'Right-click objects in the 3D view to interact',
        'Press Ctrl+Z to undo, Ctrl+Y to redo',
        'Press ? to see all keyboard shortcuts',
        'Material Forge can generate custom block types',
        'The LLM can create blocks from natural language',
        'Agents in the world think and act autonomously',
        'Export your world as .OMW to share with others',
        'Auto-save keeps your world safe every 30 seconds'
    ];

    class LoadingScreen {
        constructor() {
            this.overlay = null;
            this.progressBar = null;
            this.progressText = null;
            this.tipText = null;
            this.spinner = null;
            this.tipIndex = Math.floor(Math.random() * Tips.length);
            this.tipInterval = null;
            this.steps = [];
            this.currentStep = 0;
            this.complete = false;
        }

        show() {
            if (this.overlay) return;
            this.overlay = document.createElement('div');
            this.overlay.id = 'loadingScreen';
            this.overlay.style.cssText = `
                position:fixed;top:0;left:0;width:100%;height:100%;z-index:9999;
                display:flex;align-items:center;justify-content:center;
                background:linear-gradient(135deg,#0a0a1a 0%,#1a1035 50%,#0a0a1a 100%);
                transition:opacity 0.5s ease-out;
            `;
            this.overlay.innerHTML = `
                <div style="text-align:center;max-width:400px;width:90%">
                    <div style="font-size:36px;margin-bottom:12px">
                        <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="#8b5cf6" stroke-width="2">
                            <polygon points="12 2 2 7 12 12 22 7 12 2"/><polyline points="2 17 12 22 22 17"/><polyline points="2 12 12 17 22 12"/>
                        </svg>
                    </div>
                    <h1 style="color:#e0e0e0;font-size:28px;font-weight:300;margin-bottom:4px;letter-spacing:2px">OpenMind</h1>
                    <p style="color:#8b5cf6;font-size:11px;margin-bottom:24px;letter-spacing:4px;text-transform:uppercase">Universal Voxel Sandbox</p>

                    <div style="background:rgba(20,20,40,0.8);border:1px solid rgba(100,100,200,0.2);border-radius:12px;padding:20px;margin-bottom:16px">
                        <div id="loadStep" style="color:#94a3b8;font-size:12px;margin-bottom:12px">Initializing...</div>
                        <div style="background:rgba(10,10,30,0.8);border-radius:8px;height:6px;overflow:hidden;margin-bottom:8px">
                            <div id="loadBar" style="height:100%;width:0%;background:linear-gradient(90deg,#8b5cf6,#60a5fa);border-radius:8px;transition:width 0.3s ease-out"></div>
                        </div>
                        <div style="display:flex;justify-content:space-between">
                            <span id="loadPercent" style="color:#60a5fa;font-size:11px">0%</span>
                            <span id="loadDetail" style="color:#94a3b8;font-size:10px"></span>
                        </div>
                    </div>

                    <div style="min-height:40px">
                        <p id="loadTip" style="color:#94a3b8;font-size:11px;font-style:italic"></p>
                    </div>

                    <div id="loadSpinner" style="margin-top:12px;display:none">
                        <div style="width:24px;height:24px;border:2px solid rgba(100,100,200,0.3);border-top-color:#8b5cf6;border-radius:50%;margin:0 auto;animation:spin 0.8s linear infinite"></div>
                    </div>
                </div>
                <style>@keyframes spin { to { transform: rotate(360deg); } }</style>
            `;
            document.body.appendChild(this.overlay);

            this.progressBar = this.overlay.querySelector('#loadBar');
            this.progressText = this.overlay.querySelector('#loadPercent');
            this.tipText = this.overlay.querySelector('#loadTip');
            this.stepText = this.overlay.querySelector('#loadStep');
            this.detailText = this.overlay.querySelector('#loadDetail');
            this.spinner = this.overlay.querySelector('#loadSpinner');

            this.showTip();
            this.tipInterval = setInterval(() => this.showTip(), 5000);
        }

        showTip() {
            if (!this.tipText) return;
            this.tipText.style.opacity = '0';
            setTimeout(() => {
                this.tipText.textContent = Tips[this.tipIndex % Tips.length];
                this.tipText.style.opacity = '1';
                this.tipIndex++;
            }, 200);
        }

        setStep(name, progress, detail) {
            if (this.stepText) this.stepText.textContent = name;
            if (this.progressBar) this.progressBar.style.width = progress + '%';
            if (this.progressText) this.progressText.textContent = Math.round(progress) + '%';
            if (this.detailText && detail) this.detailText.textContent = detail;
        }

        showSpinner() { if (this.spinner) this.spinner.style.display = ''; }
        hideSpinner() { if (this.spinner) this.spinner.style.display = 'none'; }

        async runSteps(steps) {
            this.steps = steps;
            this.currentStep = 0;
            for (const step of steps) {
                this.currentStep++;
                const progress = (this.currentStep / steps.length) * 100;
                this.setStep(step.name, progress, step.detail || '');
                if (step.fn) {
                    try {
                        await step.fn();
                    } catch (e) {
                        console.error('[Loading] Step failed:', step.name, e);
                        this.setStep('Error: ' + step.name, progress, e.message);
                        return false;
                    }
                }
            }
            return true;
        }

        async hide() {
            if (this.complete) return;
            this.complete = true;
            if (this.tipInterval) clearInterval(this.tipInterval);
            this.setStep('Ready', 100, '');
            if (this.overlay) {
                this.overlay.style.opacity = '0';
                await new Promise(r => setTimeout(r, 500));
                this.overlay.remove();
                this.overlay = null;
            }
        }

        showError(msg) {
            if (this.stepText) {
                this.stepText.textContent = 'Error: ' + msg;
                this.stepText.style.color = '#ef4444';
            }
            this.hideSpinner();
        }
    }

    window.LoadingScreen = LoadingScreen;
})();
