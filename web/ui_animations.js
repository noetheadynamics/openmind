/**
 * OpenMind – UI Animations
 * Panel transitions, hover effects, toasts, loading states, reduce motion
 */
(function() {
    'use strict';

    const UIAnimations = {
        reduceMotion: window.matchMedia?.('(prefers-reduced-motion: reduce)').matches || false,

        init() {
            window.matchMedia?.('(prefers-reduced-motion: reduce)').addEventListener('change', e => {
                this.reduceMotion = e.matches;
            });
        },

        slideIn(el, dir, duration) {
            if (this.reduceMotion) { el.style.opacity = '1'; return Promise.resolve(); }
            dir = dir || 'right';
            duration = duration || 300;
            return new Promise(resolve => {
                el.style.transition = 'none';
                el.style.opacity = '0';
                const axis = dir === 'right' || dir === 'left' ? 'X' : 'Y';
                const sign = dir === 'right' || dir === 'down' ? '1' : '-1';
                el.style.transform = `translate${axis}(${sign * 30}px)`;
                el.offsetHeight;
                el.style.transition = `opacity ${duration}ms cubic-bezier(0.4,0,0.2,1), transform ${duration}ms cubic-bezier(0.4,0,0.2,1)`;
                el.style.opacity = '1';
                el.style.transform = 'translate(0)';
                el.addEventListener('transitionend', () => resolve(), { once: true });
            });
        },

        slideOut(el, dir, duration) {
            if (this.reduceMotion) { el.style.opacity = '0'; return Promise.resolve(); }
            dir = dir || 'right';
            duration = duration || 250;
            return new Promise(resolve => {
                const axis = dir === 'right' || dir === 'left' ? 'X' : 'Y';
                const sign = dir === 'right' || dir === 'down' ? '1' : '-1';
                el.style.transition = `opacity ${duration}ms cubic-bezier(0.4,0,0.2,1), transform ${duration}ms cubic-bezier(0.4,0,0.2,1)`;
                el.style.opacity = '0';
                el.style.transform = `translate${axis}(${sign * 30}px)`;
                el.addEventListener('transitionend', () => {
                    el.style.transform = '';
                    resolve();
                }, { once: true });
            });
        },

        fadeIn(el, duration) {
            if (this.reduceMotion) { el.style.opacity = '1'; return Promise.resolve(); }
            duration = duration || 200;
            return new Promise(resolve => {
                el.style.transition = 'none';
                el.style.opacity = '0';
                el.offsetHeight;
                el.style.transition = `opacity ${duration}ms ease-out`;
                el.style.opacity = '1';
                el.addEventListener('transitionend', () => resolve(), { once: true });
            });
        },

        fadeOut(el, duration) {
            if (this.reduceMotion) { el.style.opacity = '0'; return Promise.resolve(); }
            duration = duration || 200;
            return new Promise(resolve => {
                el.style.transition = `opacity ${duration}ms ease-in`;
                el.style.opacity = '0';
                el.addEventListener('transitionend', () => resolve(), { once: true });
            });
        },

        pulse(el, color, duration) {
            if (this.reduceMotion) return;
            duration = duration || 400;
            const orig = el.style.boxShadow;
            el.style.transition = `box-shadow ${duration}ms ease-out`;
            el.style.boxShadow = `0 0 20px ${color || '#8b5cf6'}`;
            setTimeout(() => {
                el.style.boxShadow = orig || '';
            }, duration);
        },

        shake(el, intensity, duration) {
            if (this.reduceMotion) return;
            intensity = intensity || 5;
            duration = duration || 300;
            const keyframes = [
                { transform: 'translateX(0)' },
                { transform: `translateX(-${intensity}px)` },
                { transform: `translateX(${intensity}px)` },
                { transform: `translateX(-${intensity / 2}px)` },
                { transform: 'translateX(0)' }
            ];
            el.animate(keyframes, { duration, easing: 'ease-out' });
        },

        bounce(el) {
            if (this.reduceMotion) return;
            el.animate([
                { transform: 'scale(1)' },
                { transform: 'scale(1.1)' },
                { transform: 'scale(0.95)' },
                { transform: 'scale(1)' }
            ], { duration: 300, easing: 'ease-out' });
        },

        flash(el, color, duration) {
            if (this.reduceMotion) return;
            duration = duration || 200;
            const orig = el.style.backgroundColor;
            el.style.transition = `background-color ${duration}ms ease-out`;
            el.style.backgroundColor = color || 'rgba(139,92,246,0.3)';
            setTimeout(() => {
                el.style.backgroundColor = orig || '';
            }, duration);
        },

        progressFill(el, from, to, duration) {
            if (this.reduceMotion) { el.style.width = to + '%'; return Promise.resolve(); }
            duration = duration || 500;
            return new Promise(resolve => {
                el.style.transition = `width ${duration}ms cubic-bezier(0.4,0,0.2,1)`;
                el.style.width = from + '%';
                el.offsetHeight;
                el.style.width = to + '%';
                el.addEventListener('transitionend', () => resolve(), { once: true });
            });
        },

        animateHotbar(slots, fromIdx, toIdx) {
            if (this.reduceMotion) return;
            if (fromIdx >= 0 && fromIdx < slots.length) {
                slots[fromIdx].animate([
                    { transform: 'scale(1.15)', borderColor: '#fff' },
                    { transform: 'scale(1)', borderColor: 'rgba(100,100,200,0.3)' }
                ], { duration: 200, easing: 'ease-out' });
            }
            if (toIdx >= 0 && toIdx < slots.length) {
                slots[toIdx].animate([
                    { transform: 'scale(0.9)', borderColor: '#8b5cf6' },
                    { transform: 'scale(1.15)', borderColor: '#fff' }
                ], { duration: 250, easing: 'ease-out', fill: 'forwards' });
            }
        },

        blockPlaceFeedback(el) {
            if (this.reduceMotion) return;
            el.animate([
                { boxShadow: '0 0 0 0 rgba(139,92,246,0.8)' },
                { boxShadow: '0 0 20px 10px rgba(139,92,246,0)' }
            ], { duration: 400, easing: 'ease-out' });
        }
    };

    window.UIAnimations = UIAnimations;
})();
