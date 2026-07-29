/**
 * OpenMind – Tutorial/Onboarding System
 * Welcome screen, guided steps, tooltips, skippable, persistent
 */
(function() {
    'use strict';

    const TutorialSteps = [
        {
            id: 'welcome',
            title: 'Welcome to OpenMind',
            text: 'A universal voxel sandbox powered by AI. Let\'s get you started!',
            position: 'center',
            action: 'click',
            button: 'Get Started'
        },
        {
            id: 'hotbar',
            title: 'Block Selection',
            text: 'Press 1-9 to select different block types from the hotbar at the bottom.',
            target: '#hotbar',
            position: 'top',
            action: 'keypress',
            keys: ['1','2','3','4','5','6','7','8','9']
        },
        {
            id: 'place',
            title: 'Place Blocks',
            text: 'Click anywhere in the 3D view to place a block.',
            target: '#viewport3d',
            position: 'center',
            action: 'click',
            targetEvent: 'click'
        },
        {
            id: 'break',
            title: 'Break Blocks',
            text: 'Hold Shift and click a block to break it.',
            target: '#viewport3d',
            position: 'center',
            action: 'shiftclick'
        },
        {
            id: 'camera',
            title: 'Camera Controls',
            text: 'Right-drag to orbit, scroll to zoom, WASD + QE to move freely.',
            position: 'center',
            action: 'click',
            button: 'Got it'
        },
        {
            id: 'prompt',
            title: 'AI Prompt',
            text: 'Type commands in the AI panel. Try "Create a 10x10 stone platform".',
            target: '#promptInput',
            position: 'bottom',
            action: 'type',
            placeholder: 'Try typing something...'
        },
        {
            id: 'weather',
            title: 'Change Weather',
            text: 'Type "weather rain" in the AI prompt to change the weather.',
            position: 'center',
            action: 'command',
            command: 'weather'
        },
        {
            id: 'finish',
            title: 'You\'re Ready!',
            text: 'Explore the panels, build amazing things, and use AI to bring your vision to life.',
            position: 'center',
            action: 'click',
            button: 'Start Building!'
        }
    ];

    class Tutorial {
        constructor() {
            this.steps = [...TutorialSteps];
            this.currentStep = 0;
            this.active = false;
            this.completed = false;
            this.overlay = null;
            this.tooltip = null;
            this.storageKey = 'openmind_tutorial';
            this.listeners = [];
            this._activeKeyHandler = null;
            this.loadProgress();
        }

        shouldShow() {
            try { return !localStorage.getItem(this.storageKey); } catch (e) { return true; }
        }

        markComplete() {
            try { localStorage.setItem(this.storageKey, 'done'); } catch (e) {}
            this.completed = true;
        }

        reset() {
            try { localStorage.removeItem(this.storageKey); } catch (e) {}
            this.completed = false;
            this.currentStep = 0;
        }

        start() {
            if (this.active) return;
            this.active = true;
            this.currentStep = 0;
            this.showStep();
        }

        skip() {
            this.active = false;
            this.removeUI();
            this.markComplete();
            this.emit('skip');
        }

        showStep() {
            if (!this.active || this.currentStep >= this.steps.length) {
                this.complete();
                return;
            }
            this.removeUI();
            const step = this.steps[this.currentStep];

            this.overlay = document.createElement('div');
            this.overlay.style.cssText = 'position:fixed;top:0;left:0;width:100%;height:100%;z-index:9998;background:rgba(0,0,0,0.5);transition:opacity 0.3s';
            this.overlay.addEventListener('click', (e) => {
                if (e.target === this.overlay && step.position === 'center') this.nextStep();
            });

            this.tooltip = document.createElement('div');
            this.tooltip.style.cssText = `
                position:fixed;z-index:9999;background:rgba(15,15,35,0.95);border:1px solid rgba(139,92,246,0.4);
                border-radius:12px;padding:16px;max-width:320px;width:90%;
                box-shadow:0 8px 32px rgba(0,0,0,0.5);backdrop-filter:blur(8px);
                animation:fadeIn 0.3s ease-out;
            `;

            const progress = ((this.currentStep + 1) / this.steps.length * 100).toFixed(0);
            this.tooltip.innerHTML = `
                <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:8px">
                    <span style="color:#8b5cf6;font-size:10px;text-transform:uppercase;letter-spacing:1px">Step ${this.currentStep + 1}/${this.steps.length}</span>
                    <button id="tutorialSkip" style="background:none;border:none;color:#94a3b8;cursor:pointer;font-size:11px">Skip</button>
                </div>
                <h3 style="color:#e0e0e0;font-size:15px;margin-bottom:6px">${step.title}</h3>
                <p style="color:#94a3b8;font-size:12px;margin-bottom:12px;line-height:1.5">${step.text}</p>
                <div style="background:rgba(10,10,30,0.8);border-radius:4px;height:3px;margin-bottom:12px">
                    <div style="height:100%;width:${progress}%;background:linear-gradient(90deg,#8b5cf6,#60a5fa);border-radius:4px;transition:width 0.3s"></div>
                </div>
                ${step.button ? `<button id="tutorialNext" style="width:100%;padding:8px;border:1px solid rgba(139,92,246,0.4);border-radius:8px;background:rgba(139,92,246,0.2);color:#e0e0e0;cursor:pointer;font-size:12px">${step.button}</button>` : ''}
                ${step.placeholder ? `<div style="text-align:center;color:#60a5fa;font-size:10px;font-style:italic">${step.placeholder}</div>` : ''}
            `;

            document.body.appendChild(this.overlay);
            document.body.appendChild(this.tooltip);

            this.positionTooltip(step);

            this.tooltip.querySelector('#tutorialSkip')?.addEventListener('click', () => this.skip());
            this.tooltip.querySelector('#tutorialNext')?.addEventListener('click', () => this.nextStep());

            this.setupAction(step);
        }

        positionTooltip(step) {
            if (step.position === 'center' || !step.target) {
                this.tooltip.style.top = '50%';
                this.tooltip.style.left = '50%';
                this.tooltip.style.transform = 'translate(-50%, -50%)';
            } else {
                const target = document.querySelector(step.target);
                if (target) {
                    const rect = target.getBoundingClientRect();
                    this.tooltip.style.position = 'fixed';
                    switch (step.position) {
                        case 'top':
                            this.tooltip.style.top = (rect.top - this.tooltip.offsetHeight - 10) + 'px';
                            this.tooltip.style.left = (rect.left + rect.width / 2 - this.tooltip.offsetWidth / 2) + 'px';
                            break;
                        case 'bottom':
                            this.tooltip.style.top = (rect.bottom + 10) + 'px';
                            this.tooltip.style.left = (rect.left + rect.width / 2 - this.tooltip.offsetWidth / 2) + 'px';
                            break;
                        case 'left':
                            this.tooltip.style.top = (rect.top + rect.height / 2 - this.tooltip.offsetHeight / 2) + 'px';
                            this.tooltip.style.left = (rect.right + 10) + 'px';
                            break;
                        case 'right':
                            this.tooltip.style.top = (rect.top + rect.height / 2 - this.tooltip.offsetHeight / 2) + 'px';
                            this.tooltip.style.left = (rect.left - this.tooltip.offsetWidth - 10) + 'px';
                            break;
                    }
                }
            }
        }

        setupAction(step) {
            if (!step.action) return;
            if (this._activeKeyHandler) {
                document.removeEventListener('keydown', this._activeKeyHandler);
                this._activeKeyHandler = null;
            }
            const handler = (e) => {
                let proceed = false;
                switch (step.action) {
                    case 'click': proceed = true; break;
                    case 'keypress':
                        if (step.keys && step.keys.includes(e.key)) proceed = true;
                        break;
                    case 'shiftclick':
                        if (e.shiftKey) proceed = true;
                        break;
                    case 'type':
                        if (document.activeElement?.id === 'promptInput') proceed = true;
                        break;
                    case 'command':
                        proceed = true;
                        break;
                }
                if (proceed) {
                    document.removeEventListener('keydown', handler);
                    this._activeKeyHandler = null;
                    setTimeout(() => this.nextStep(), 300);
                }
            };
            this._activeKeyHandler = handler;
            document.addEventListener('keydown', handler);
        }

        nextStep() {
            this.currentStep++;
            this.saveProgress();
            this.showStep();
        }

        complete() {
            this.active = false;
            this.removeUI();
            this.markComplete();
            this.emit('complete');
        }

        removeUI() {
            if (this.overlay) { this.overlay.remove(); this.overlay = null; }
            if (this.tooltip) { this.tooltip.remove(); this.tooltip = null; }
            if (this._activeKeyHandler) {
                document.removeEventListener('keydown', this._activeKeyHandler);
                this._activeKeyHandler = null;
            }
        }

        saveProgress() {
            try { localStorage.setItem(this.storageKey + '_step', this.currentStep); } catch (e) {}
        }

        loadProgress() {
            try {
                const step = localStorage.getItem(this.storageKey + '_step');
                if (step) this.currentStep = parseInt(step) || 0;
            } catch (e) {}
        }

        on(fn) { this.listeners.push(fn); }
        emit(type) {
            for (const fn of this.listeners) fn({ type });
        }
    }

    window.Tutorial = Tutorial;
})();
