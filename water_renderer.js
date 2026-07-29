/**
 * OpenMind – Water Renderer
 * Reflective/refractive water with waves, transparency, and multiple water types
 */
(function() {
    'use strict';

    const WaterType = Object.freeze({
        OCEAN: 'ocean', RIVER: 'river', POND: 'pond'
    });

    const WaterShaders = {
        vertex: `
            uniform float uTime;
            uniform float uWaveHeight;
            uniform float uWaveSpeed;
            varying vec3 vWorldPos;
            varying vec3 vNormal;
            varying vec2 vUv;
            varying float vFogDepth;

            void main() {
                vUv = uv;
                vec3 pos = position;

                float wave1 = sin(pos.x * 0.05 + uTime * uWaveSpeed) * uWaveHeight;
                float wave2 = sin(pos.z * 0.07 + uTime * uWaveSpeed * 0.8) * uWaveHeight * 0.6;
                float wave3 = cos(pos.x * 0.03 + pos.z * 0.04 + uTime * uWaveSpeed * 1.2) * uWaveHeight * 0.3;
                pos.y += wave1 + wave2 + wave3;

                float dx = cos(pos.x * 0.05 + uTime * uWaveSpeed) * uWaveHeight * 0.05;
                float dz = cos(pos.z * 0.07 + uTime * uWaveSpeed * 0.8) * uWaveHeight * 0.042;
                vNormal = normalize(vec3(-dx, 1.0, -dz));

                vec4 worldPos = modelMatrix * vec4(pos, 1.0);
                vWorldPos = worldPos.xyz;
                vFogDepth = -(viewMatrix * worldPos).z;

                gl_Position = projectionMatrix * viewMatrix * worldPos;
            }
        `,
        fragment: `
            uniform vec3 uDeepColor;
            uniform vec3 uShallowColor;
            uniform vec3 uReflectionColor;
            uniform float uOpacity;
            uniform float uReflectionStrength;
            uniform float uFogDensity;
            uniform vec3 uFogColor;
            uniform vec3 uSunDirection;
            uniform vec3 uSunColor;
            uniform float uTime;
            uniform sampler2D uTexture;

            varying vec3 vWorldPos;
            varying vec3 vNormal;
            varying vec2 vUv;
            varying float vFogDepth;

            void main() {
                vec3 viewDir = normalize(cameraPosition - vWorldPos);
                vec3 normal = normalize(vNormal);

                float fresnel = pow(1.0 - max(dot(viewDir, normal), 0.0), 3.0);
                fresnel = clamp(fresnel, 0.0, 1.0);

                vec3 reflDir = reflect(-viewDir, normal);
                float skyReflect = max(reflDir.y, 0.0);
                vec3 reflection = mix(uShallowColor, uReflectionColor, skyReflect) * uReflectionStrength;

                vec3 sunReflect = pow(max(dot(reflDir, uSunDirection), 0.0), 128.0) * uSunColor * 2.0;

                float depth = 1.0;
                vec3 waterColor = mix(uShallowColor, uDeepColor, depth);

                vec3 color = mix(waterColor, reflection, fresnel);
                color += sunReflect * fresnel;

                float edgeFactor = 1.0 - max(dot(viewDir, vec3(0.0, 1.0, 0.0)), 0.0);
                edgeFactor = pow(edgeFactor, 2.0);
                color = mix(color, uReflectionColor * 0.5, edgeFactor * 0.3);

                float fogFactor = 1.0 - exp(-uFogDensity * uFogDensity * vFogDepth * vFogDepth);
                color = mix(color, uFogColor, clamp(fogFactor, 0.0, 1.0));

                float alpha = uOpacity + fresnel * (1.0 - uOpacity) * 0.5;

                gl_FragColor = vec4(color, alpha);
            }
        `
    };

    class WaterRenderer {
        constructor(scene, camera) {
            this.scene = scene;
            this.camera = camera;
            this.meshes = new Map();
            this.enabled = true;
            this.time = 0;

            this.uniforms = {
                uTime: { value: 0 },
                uWaveHeight: { value: 0.3 },
                uWaveSpeed: { value: 1.0 },
                uDeepColor: { value: new THREE.Color(0x001133) },
                uShallowColor: { value: new THREE.Color(0x0066aa) },
                uReflectionColor: { value: new THREE.Color(0x88ccff) },
                uOpacity: { value: 0.7 },
                uReflectionStrength: { value: 0.6 },
                uFogDensity: { value: 0.02 },
                uFogColor: { value: new THREE.Color(0x88aacc) },
                uSunDirection: { value: new THREE.Vector3(0.5, 0.8, 0.3).normalize() },
                uSunColor: { value: new THREE.Color(0xffffee) },
                uTexture: { value: null }
            };

            this.material = new THREE.ShaderMaterial({
                vertexShader: WaterShaders.vertex,
                fragmentShader: WaterShaders.fragment,
                uniforms: this.uniforms,
                transparent: true,
                side: THREE.DoubleSide,
                depthWrite: false
            });
        }

        addWaterChunk(cx, cz, waterBlocks, chunkSize) {
            const key = cx + ',' + cz;
            if (this.meshes.has(key)) {
                this.scene.remove(this.meshes.get(key));
            }

            const positions = [];
            const indices = [];
            const normals = [];
            const uvs = [];
            const cs = chunkSize || 16;
            const halfCS = cs / 2;

            for (const block of waterBlocks) {
                const bx = block.x - cx * cs + halfCS;
                const bz = block.z - cz * cs + halfCS;
                const by = block.y;

                const baseIdx = positions.length / 3;
                const s = 0.5;

                positions.push(bx - s, by + 0.4, bz - s);
                positions.push(bx + s, by + 0.4, bz - s);
                positions.push(bx + s, by + 0.4, bz + s);
                positions.push(bx - s, by + 0.4, bz + s);

                for (let i = 0; i < 4; i++) normals.push(0, 1, 0);

                uvs.push(0, 0, 1, 0, 1, 1, 0, 1);

                indices.push(baseIdx, baseIdx + 1, baseIdx + 2);
                indices.push(baseIdx, baseIdx + 2, baseIdx + 3);
            }

            if (positions.length === 0) return;

            const geometry = new THREE.BufferGeometry();
            geometry.setAttribute('position', new THREE.Float32BufferAttribute(positions, 3));
            geometry.setAttribute('normal', new THREE.Float32BufferAttribute(normals, 3));
            geometry.setAttribute('uv', new THREE.Float32BufferAttribute(uvs, 2));
            geometry.setIndex(indices);

            const mesh = new THREE.Mesh(geometry, this.material);
            mesh.renderOrder = 1;
            this.scene.add(mesh);
            this.meshes.set(key, mesh);
        }

        removeWaterChunk(cx, cz) {
            const key = cx + ',' + cz;
            const mesh = this.meshes.get(key);
            if (mesh) {
                this.scene.remove(mesh);
                mesh.geometry.dispose();
                this.meshes.delete(key);
            }
        }

        setType(type) {
            switch (type) {
                case WaterType.OCEAN:
                    this.uniforms.uDeepColor.value.setHex(0x001133);
                    this.uniforms.uShallowColor.value.setHex(0x0066aa);
                    this.uniforms.uWaveHeight.value = 0.5;
                    this.uniforms.uWaveSpeed.value = 1.2;
                    this.uniforms.uOpacity.value = 0.75;
                    break;
                case WaterType.RIVER:
                    this.uniforms.uDeepColor.value.setHex(0x002244);
                    this.uniforms.uShallowColor.value.setHex(0x0088bb);
                    this.uniforms.uWaveHeight.value = 0.2;
                    this.uniforms.uWaveSpeed.value = 2.0;
                    this.uniforms.uOpacity.value = 0.65;
                    break;
                case WaterType.POND:
                    this.uniforms.uDeepColor.value.setHex(0x003322);
                    this.uniforms.uShallowColor.value.setHex(0x00aa88);
                    this.uniforms.uWaveHeight.value = 0.1;
                    this.uniforms.uWaveSpeed.value = 0.8;
                    this.uniforms.uOpacity.value = 0.6;
                    break;
            }
        }

        update(dt, timeOfDay, weatherType) {
            if (!this.enabled) return;
            this.time += dt;
            this.uniforms.uTime.value = this.time;

            const hour = timeOfDay || 12;
            const sunAngle = (hour / 24) * Math.PI * 2 - Math.PI / 2;
            this.uniforms.uSunDirection.value.set(
                Math.cos(sunAngle), Math.sin(sunAngle), 0.3
            ).normalize();

            if (hour < 6 || hour > 20) {
                this.uniforms.uSunColor.value.setHex(0x334466);
                this.uniforms.uReflectionColor.value.setHex(0x223344);
            } else if (hour < 8 || hour > 18) {
                this.uniforms.uSunColor.value.setHex(0xffaa44);
                this.uniforms.uReflectionColor.value.setHex(0xddaa66);
            } else {
                this.uniforms.uSunColor.value.setHex(0xffffee);
                this.uniforms.uReflectionColor.value.setHex(0x88ccff);
            }

            if (weatherType === 'rain' || weatherType === 'storm') {
                this.uniforms.uFogDensity.value = 0.04;
                this.uniforms.uFogColor.value.setHex(0x667788);
            } else {
                this.uniforms.uFogDensity.value = 0.02;
                this.uniforms.uFogColor.value.setHex(0x88aacc);
            }
        }

        setVisible(v) { this.enabled = v; this.visible = v; for (const m of this.meshes.values()) m.visible = v; }
        dispose() { for (const [k, m] of this.meshes) { this.scene.remove(m); m.geometry.dispose(); } this.meshes.clear(); }
    }

    window.WaterType = WaterType;
    window.WaterRenderer = WaterRenderer;
})();
