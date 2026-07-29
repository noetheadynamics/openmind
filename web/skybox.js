/**
 * OpenMind – Skybox & Atmosphere
 * Dynamic sky, stars, sun/moon, clouds, weather integration
 */
(function() {
    'use strict';

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
            this.skyDome = null;
            this.sunLight = null;
            this.ambientLight = null;

            this.createSkyDome();
            this.createStars();
            this.createSunMoon();
            this.createClouds();
            this.createLights();
        }

        createSkyDome() {
            const geo = new THREE.SphereGeometry(500, 32, 32);
            const mat = new THREE.ShaderMaterial({
                side: THREE.BackSide,
                uniforms: {
                    uTopColor: { value: new THREE.Color(0x0077ff) },
                    uBottomColor: { value: new THREE.Color(0xffffff) },
                    uHorizonColor: { value: new THREE.Color(0xffaa44) },
                    uTimeOfDay: { value: 6.0 }
                },
                vertexShader: `
                    varying vec3 vWorldPosition;
                    void main() {
                        vec4 worldPos = modelMatrix * vec4(position, 1.0);
                        vWorldPosition = worldPos.xyz;
                        gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
                    }
                `,
                fragmentShader: `
                    uniform vec3 uTopColor;
                    uniform vec3 uBottomColor;
                    uniform vec3 uHorizonColor;
                    uniform float uTimeOfDay;
                    varying vec3 vWorldPosition;

                    void main() {
                        float h = normalize(vWorldPosition).y;
                        float t = uTimeOfDay;

                        vec3 dayTop = vec3(0.1, 0.4, 0.9);
                        vec3 dayBot = vec3(0.6, 0.8, 1.0);
                        vec3 dayHorizon = vec3(0.7, 0.85, 1.0);

                        vec3 sunsetTop = vec3(0.1, 0.1, 0.4);
                        vec3 sunsetBot = vec3(0.8, 0.3, 0.1);
                        vec3 sunsetHorizon = vec3(1.0, 0.5, 0.2);

                        vec3 nightTop = vec3(0.0, 0.0, 0.05);
                        vec3 nightBot = vec3(0.02, 0.02, 0.08);
                        vec3 nightHorizon = vec3(0.05, 0.05, 0.15);

                        vec3 top, bot, horizon;

                        if (t >= 6.0 && t < 8.0) {
                            float f = (t - 6.0) / 2.0;
                            top = mix(sunsetTop, dayTop, f);
                            bot = mix(sunsetBot, dayBot, f);
                            horizon = mix(sunsetHorizon, dayHorizon, f);
                        } else if (t >= 8.0 && t < 17.0) {
                            top = dayTop; bot = dayBot; horizon = dayHorizon;
                        } else if (t >= 17.0 && t < 20.0) {
                            float f = (t - 17.0) / 3.0;
                            top = mix(dayTop, sunsetTop, f);
                            bot = mix(dayBot, sunsetBot, f);
                            horizon = mix(dayHorizon, sunsetHorizon, f);
                        } else if (t >= 20.0 && t < 22.0) {
                            float f = (t - 20.0) / 2.0;
                            top = mix(sunsetTop, nightTop, f);
                            bot = mix(sunsetBot, nightBot, f);
                            horizon = mix(sunsetHorizon, nightHorizon, f);
                        } else {
                            top = nightTop; bot = nightBot; horizon = nightHorizon;
                        }

                        vec3 color;
                        if (h > 0.0) {
                            color = mix(horizon, top, pow(h, 0.6));
                        } else {
                            color = mix(horizon, bot, pow(-h, 0.4));
                        }

                        gl_FragColor = vec4(color, 1.0);
                    }
                `
            });
            this.skyDome = new THREE.Mesh(geo, mat);
            this.scene.add(this.skyDome);
        }

        createStars() {
            const count = 500;
            const positions = new Float32Array(count * 3);
            const sizes = new Float32Array(count);
            for (let i = 0; i < count; i++) {
                const theta = Math.random() * Math.PI * 2;
                const phi = Math.acos(2 * Math.random() - 1);
                const r = 480;
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
                transparent: true, opacity: 0.8, depthWrite: false
            });
            this.stars = new THREE.Points(geo, mat);
            this.scene.add(this.stars);
        }

        createSunMoon() {
            const sunGeo = new THREE.SphereGeometry(8, 16, 16);
            const sunMat = new THREE.MeshBasicMaterial({ color: 0xffff00 });
            this.sunMesh = new THREE.Mesh(sunGeo, sunMat);
            this.scene.add(this.sunMesh);

            const moonGeo = new THREE.SphereGeometry(5, 16, 16);
            const moonMat = new THREE.MeshBasicMaterial({ color: 0xccccdd });
            this.moonMesh = new THREE.Mesh(moonGeo, moonMat);
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
                        color: 0xffffff, transparent: true, opacity: 0.7, depthWrite: false
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
                this.cloudMeshes.push(group);
                this.scene.add(group);
            }
        }

        createLights() {
            this.sunLight = null;
            this.ambientLight = null;
        }

        update(dt, timeOfDay, weather) {
            if (!this.enabled) return;
            this.timeOfDay = timeOfDay;
            this.weather = weather || 'clear';

            const hour = timeOfDay;
            const sunAngle = ((hour - 6) / 12) * Math.PI;

            const sunR = 350;
            const sunX = Math.cos(sunAngle) * sunR;
            const sunY = Math.sin(sunAngle) * sunR;
            this.sunMesh.position.set(sunX, sunY, 50);
            this.moonMesh.position.set(-sunX, -sunY, -50);

            const isDay = hour >= 6 && hour < 20;
            this.stars.material.opacity = isDay ? 0 : 0.8;
            this.sunMesh.visible = isDay;
            this.moonMesh.visible = !isDay;

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

            const skyMat = this.skyDome.material;
            skyMat.uniforms.uTimeOfDay.value = hour;

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
            if (this.skyDome) this.skyDome.visible = v;
            if (this.stars) this.stars.visible = v;
            if (this.sunMesh) this.sunMesh.visible = v;
            if (this.moonMesh) this.moonMesh.visible = v;
            this.cloudMeshes.forEach(c => c.visible = v);
            if (this.sunLight) this.sunLight.visible = v;
            if (this.ambientLight) this.ambientLight.visible = v;
        }
        dispose() {
            [this.skyDome, this.stars, this.sunMesh, this.moonMesh].forEach(m => {
                if (m) { this.scene.remove(m); if (m.geometry) m.geometry.dispose(); if (m.material) m.material.dispose(); }
            });
            this.cloudMeshes.forEach(c => { this.scene.remove(c); c.children.forEach(ch => { if (ch.geometry) ch.geometry.dispose(); if (ch.material) ch.material.dispose(); }); });
        }
    };

    window.OpenMindSkybox = Skybox;
})();
