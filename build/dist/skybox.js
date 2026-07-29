/**
 * OpenMind – Skybox & Atmosphere
 * Dynamic sky via scene.background color + CSS gradient,
 * stars, sun/moon, clouds, weather integration
 */
(function() {
    'use strict';

    function lerpColor(a, b, t) {
        const ar = (a >> 16) & 0xff, ag = (a >> 8) & 0xff, ab = a & 0xff;
        const br = (b >> 16) & 0xff, bg = (b >> 8) & 0xff, bb = b & 0xff;
        const rr = Math.round(ar + (br - ar) * t);
        const rg = Math.round(ag + (bg - ag) * t);
        const rb = Math.round(ab + (bb - ab) * t);
        return (rr << 16) | (rg << 8) | rb;
    }

    function getSkyColor(hour) {
        if (hour >= 8 && hour < 17) return 0x3344aa;
        if (hour >= 6 && hour < 8) {
            const f = (hour - 6) / 2;
            return lerpColor(0x1a1040, 0x3344aa, f);
        }
        if (hour >= 17 && hour < 20) {
            const f = (hour - 17) / 3;
            return lerpColor(0x3344aa, 0x1a1040, f);
        }
        if (hour >= 20 && hour < 22) {
            const f = (hour - 20) / 2;
            return lerpColor(0x1a1040, 0x050510, f);
        }
        return 0x050510;
    }

    const Skybox = class {
        constructor(scene) {
            this.scene = scene;
            this.enabled = true;
            this.timeOfDay = 6;
            this.weather = 'clear';
            this.stars = null;
            this.sunMesh = null;
            this.moonMesh = null;
            this.cloudMeshes = [];
            this.sunLight = null;
            this.ambientLight = null;
            this.skyDome = null;

            this._createViewportGradient();
            this.createStars();
            this.createSunMoon();
            this.createClouds();
            this.createLights();
            this.update(0, this.timeOfDay, this.weather);
        }

        _createViewportGradient() {
            this.gradientEl = document.getElementById('viewport3d');
            if (this.gradientEl) {
                this.gradientEl.style.background = 'linear-gradient(180deg, #1a1040 0%, #0a0a1a 100%)';
            }
        }

        createStars() {
            const count = 500;
            const positions = new Float32Array(count * 3);
            const sizes = new Float32Array(count);
            for (let i = 0; i < count; i++) {
                const theta = Math.random() * Math.PI * 2;
                const phi = Math.acos(2 * Math.random() - 1);
                const r = 200;
                positions[i * 3] = r * Math.sin(phi) * Math.cos(theta);
                positions[i * 3 + 1] = Math.abs(r * Math.cos(phi));
                positions[i * 3 + 2] = r * Math.sin(phi) * Math.sin(theta);
                sizes[i] = 0.5 + Math.random() * 2;
            }
            const geo = new THREE.BufferGeometry();
            geo.setAttribute('position', new THREE.BufferAttribute(positions, 3));
            geo.setAttribute('size', new THREE.BufferAttribute(sizes, 1));
            const mat = new THREE.PointsMaterial({
                color: 0xffffff, size: 1.5, sizeAttenuation: false,
                transparent: true, opacity: 0.8, depthWrite: false, depthTest: false
            });
            this.stars = new THREE.Points(geo, mat);
            this.stars.renderOrder = 999;
            this.scene.add(this.stars);
        }

        createSunMoon() {
            const sunGeo = new THREE.SphereGeometry(8, 16, 16);
            const sunMat = new THREE.MeshBasicMaterial({ color: 0xffee44, depthTest: false });
            this.sunMesh = new THREE.Mesh(sunGeo, sunMat);
            this.sunMesh.renderOrder = 998;
            this.sunMesh.visible = false;
            this.scene.add(this.sunMesh);

            const moonGeo = new THREE.SphereGeometry(5, 16, 16);
            const moonMat = new THREE.MeshBasicMaterial({ color: 0xddddee, depthTest: false });
            this.moonMesh = new THREE.Mesh(moonGeo, moonMat);
            this.moonMesh.renderOrder = 998;
            this.moonMesh.visible = false;
            this.scene.add(this.moonMesh);
        }

        createClouds() {
            for (let i = 0; i < 20; i++) {
                const group = new THREE.Group();
                const puffCount = 3 + Math.floor(Math.random() * 4);
                for (let j = 0; j < puffCount; j++) {
                    const size = 5 + Math.random() * 10;
                    const geo = new THREE.SphereGeometry(size, 8, 8);
                    const mat = new THREE.MeshBasicMaterial({
                        color: 0xffffff, transparent: true, opacity: 0.7,
                        depthWrite: false, depthTest: false
                    });
                    const puff = new THREE.Mesh(geo, mat);
                    puff.position.set(
                        (Math.random() - 0.5) * 20,
                        (Math.random() - 0.5) * 3,
                        (Math.random() - 0.5) * 15
                    );
                    puff.scale.y = 0.4;
                    group.add(puff);
                }
                group.position.set(
                    (Math.random() - 0.5) * 400,
                    60 + Math.random() * 30,
                    (Math.random() - 0.5) * 400
                );
                group.userData.speed = 0.5 + Math.random() * 1.5;
                group.renderOrder = 997;
                this.cloudMeshes.push(group);
                this.scene.add(group);
            }
        }

        createLights() {
        }

        update(dt, timeOfDay, weather) {
            if (!this.enabled) return;
            this.timeOfDay = timeOfDay;
            this.weather = weather || 'clear';

            const hour = timeOfDay;
            const sunAngle = ((hour - 6) / 12) * Math.PI;

            const sunR = 200;
            const sunX = Math.cos(sunAngle) * sunR;
            const sunY = Math.sin(sunAngle) * sunR;
            this.sunMesh.position.set(sunX, sunY, 50);
            this.moonMesh.position.set(-sunX, -sunY, -50);

            const isDay = hour >= 6 && hour < 20;
            this.stars.material.opacity = isDay ? 0 : 0.8;
            this.sunMesh.visible = isDay;
            this.moonMesh.visible = !isDay;

            const skyHex = getSkyColor(hour);
            this.scene.background = new THREE.Color(skyHex);

            if (this.gradientEl) {
                const r = (skyHex >> 16) & 0xff;
                const g = (skyHex >> 8) & 0xff;
                const b = skyHex & 0xff;
                const r2 = Math.round(r * 0.4);
                const g2 = Math.round(g * 0.4);
                const b2 = Math.round(b * 0.4);
                this.gradientEl.style.background = `linear-gradient(180deg, rgb(${r},${g},${b}) 0%, rgb(${r2},${g2},${b2}) 100%)`;
            }

            if (this.sunLight) {
                if (isDay) {
                    const t = Math.sin(sunAngle);
                    this.sunLight.intensity = Math.max(t, 0.1) * 1.2;
                    const sunColor = hour < 8 || hour > 18 ? 0xffaa44 : 0xffffee;
                    this.sunLight.color.setHex(sunColor);
                } else {
                    this.sunLight.intensity = 0.05;
                    this.sunLight.color.setHex(0x334466);
                }
            }
            if (this.ambientLight) {
                this.ambientLight.intensity = isDay ? 0.3 + Math.sin(sunAngle) * 0.3 : 0.1;
            }

            const cloudOpacity = this.weather === 'clear' ? 0.5 : this.weather === 'rain' ? 0.85 : 0.9;
            const cloudColor = this.weather === 'storm' ? 0x444444 : this.weather === 'rain' ? 0x888888 : 0xffffff;
            for (const cloud of this.cloudMeshes) {
                cloud.position.x += cloud.userData.speed * dt * (this.weather === 'storm' ? 3 : 1);
                if (cloud.position.x > 250) cloud.position.x = -250;
                for (const child of cloud.children) {
                    child.material.opacity = cloudOpacity;
                    child.material.color.setHex(cloudColor);
                }
            }
        }

        setTimeOfDay(t) { this.timeOfDay = t; }
        setWeather(w) { this.weather = w; }
        setVisible(v) {
            this.enabled = v;
            if (this.stars) this.stars.visible = v;
            if (this.sunMesh) this.sunMesh.visible = v;
            if (this.moonMesh) this.moonMesh.visible = v;
            this.cloudMeshes.forEach(c => c.visible = v);
            if (this.sunLight) this.sunLight.visible = v;
            if (this.ambientLight) this.ambientLight.visible = v;
        }
        dispose() {
            [this.stars, this.sunMesh, this.moonMesh].forEach(m => {
                if (m) { this.scene.remove(m); if (m.geometry) m.geometry.dispose(); if (m.material) m.material.dispose(); }
            });
            this.cloudMeshes.forEach(c => { this.scene.remove(c); c.children.forEach(ch => { if (ch.geometry) ch.geometry.dispose(); if (ch.material) ch.material.dispose(); }); });
        }
    };

    window.OpenMindSkybox = Skybox;
})();
