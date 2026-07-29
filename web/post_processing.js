/**
 * OpenMind – Post-Processing Effects
 * Bloom, DOF, fog, color grading, vignette, anti-aliasing
 */
(function() {
    'use strict';

    const BloomShader = {
        uniforms: {
            tDiffuse: { value: null },
            uIntensity: { value: 0.4 },
            uThreshold: { value: 0.7 },
            uRadius: { value: 0.8 }
        },
        vertexShader: `varying vec2 vUv; void main() { vUv = uv; gl_Position = projectionMatrix * modelViewMatrix * vec4(position,1.0); }`,
        fragmentShader: `
            uniform sampler2D tDiffuse;
            uniform float uIntensity;
            uniform float uThreshold;
            uniform float uRadius;
            varying vec2 vUv;
            void main() {
                vec4 color = texture2D(tDiffuse, vUv);
                vec4 bright = vec4(0.0);
                float luminance = dot(color.rgb, vec3(0.299, 0.587, 0.114));
                if (luminance > uThreshold) bright = color * uIntensity;
                vec4 sum = vec4(0.0);
                float total = 0.0;
                for (float x = -4.0; x <= 4.0; x += 1.0) {
                    for (float y = -4.0; y <= 4.0; y += 1.0) {
                        vec2 off = vec2(x, y) * uRadius * 0.003;
                        vec4 s = texture2D(tDiffuse, vUv + off);
                        float l = dot(s.rgb, vec3(0.299, 0.587, 0.114));
                        float w = max(l - uThreshold, 0.0);
                        sum += s * w;
                        total += w;
                    }
                }
                if (total > 0.0) bright += sum / total * uIntensity;
                gl_FragColor = color + bright;
            }
        `
    };

    const ColorGradingShader = {
        uniforms: {
            tDiffuse: { value: null },
            uSaturation: { value: 1.0 },
            uContrast: { value: 1.0 },
            uBrightness: { value: 0.0 },
            uTemperature: { value: 0.0 }
        },
        vertexShader: `varying vec2 vUv; void main() { vUv = uv; gl_Position = projectionMatrix * modelViewMatrix * vec4(position,1.0); }`,
        fragmentShader: `
            uniform sampler2D tDiffuse;
            uniform float uSaturation;
            uniform float uContrast;
            uniform float uBrightness;
            uniform float uTemperature;
            varying vec2 vUv;
            void main() {
                vec4 color = texture2D(tDiffuse, vUv);
                color.rgb += uBrightness;
                color.rgb = (color.rgb - 0.5) * uContrast + 0.5;
                float grey = dot(color.rgb, vec3(0.299, 0.587, 0.114));
                color.rgb = mix(vec3(grey), color.rgb, uSaturation);
                color.r += uTemperature * 0.1;
                color.b -= uTemperature * 0.1;
                gl_FragColor = color;
            }
        `
    };

    const VignetteShader = {
        uniforms: {
            tDiffuse: { value: null },
            uIntensity: { value: 0.3 },
            uRadius: { value: 0.8 }
        },
        vertexShader: `varying vec2 vUv; void main() { vUv = uv; gl_Position = projectionMatrix * modelViewMatrix * vec4(position,1.0); }`,
        fragmentShader: `
            uniform sampler2D tDiffuse;
            uniform float uIntensity;
            uniform float uRadius;
            varying vec2 vUv;
            void main() {
                vec4 color = texture2D(tDiffuse, vUv);
                vec2 center = vUv - 0.5;
                float dist = length(center);
                float vig = 1.0 - smoothstep(uRadius * 0.5, uRadius, dist) * uIntensity;
                gl_FragColor = vec4(color.rgb * vig, color.a);
            }
        `
    };

    const FogShader = {
        uniforms: {
            tDiffuse: { value: null },
            uFogColor: { value: new THREE.Color(0x88aacc) },
            uFogNear: { value: 30 },
            uFogFar: { value: 120 },
            uFogDensity: { value: 0.015 }
        },
        vertexShader: `varying vec2 vUv; varying float vFogDepth; void main() { vUv = uv; vec4 mvPos = modelViewMatrix * vec4(position,1.0); vFogDepth = -mvPos.z; gl_Position = projectionMatrix * mvPos; }`,
        fragmentShader: `
            uniform sampler2D tDiffuse;
            uniform vec3 uFogColor;
            uniform float uFogNear;
            uniform float uFogFar;
            uniform float uFogDensity;
            varying float vFogDepth;
            void main() {
                vec4 color = texture2D(tDiffuse, vUv);
                float fogFactor = 1.0 - exp(-uFogDensity * uFogDensity * vFogDepth * vFogDepth);
                fogFactor = clamp(fogFactor, 0.0, 1.0);
                gl_FragColor = vec4(mix(color.rgb, uFogColor, fogFactor), color.a);
            }
        `
    };

    class PostProcessing {
        constructor(renderer, scene, camera) {
            this.renderer = renderer;
            this.scene = scene;
            this.camera = camera;
            this.enabled = true;
            this.bloomEnabled = true;
            this.fogEnabled = true;
            this.colorGradingEnabled = true;
            this.vignetteEnabled = false;
            this.dofEnabled = false;

            this.bloomIntensity = 0.4;
            this.fogColor = new THREE.Color(0x88aacc);
            this.fogNear = 30;
            this.fogFar = 120;
            this.saturation = 1.0;
            this.contrast = 1.0;
            this.brightness = 0.0;
            this.temperature = 0.0;
            this.vignetteIntensity = 0.3;
        }

        setup() {
            if (typeof THREE.EffectComposer === 'undefined') {
                this.enabled = false;
                return;
            }

            try {
                this.composer = new THREE.EffectComposer(this.renderer);
                const renderPass = new THREE.RenderPass(this.scene, this.camera);
                this.composer.addPass(renderPass);

                this.bloomPass = new THREE.ShaderPass(BloomShader);
                this.bloomPass.uniforms.uIntensity.value = this.bloomIntensity;
                this.composer.addPass(this.bloomPass);

                this.fogPass = new THREE.ShaderPass(FogShader);
                this.fogPass.uniforms.uFogColor.value = this.fogColor;
                this.fogPass.uniforms.uFogNear.value = this.fogNear;
                this.fogPass.uniforms.uFogFar.value = this.fogFar;
                this.composer.addPass(this.fogPass);

                this.colorGradingPass = new THREE.ShaderPass(ColorGradingShader);
                this.composer.addPass(this.colorGradingPass);

                this.vignettePass = new THREE.ShaderPass(VignetteShader);
                this.vignettePass.enabled = false;
                this.composer.addPass(this.vignettePass);

                this.enabled = true;
            } catch (e) {
                this.enabled = false;
            }
        }

        render() {
            if (!this.enabled || !this.composer) {
                if (this.renderer && this.scene && this.camera) {
                    this.renderer.render(this.scene, this.camera);
                }
                return;
            }
            this.bloomPass && (this.bloomPass.enabled = this.bloomEnabled);
            this.fogPass && (this.fogPass.enabled = this.fogEnabled);
            this.colorGradingPass && (this.colorGradingPass.enabled = this.colorGradingEnabled);
            this.vignettePass && (this.vignettePass.enabled = this.vignetteEnabled);

            this.bloomPass && (this.bloomPass.uniforms.uIntensity.value = this.bloomIntensity);
            this.fogPass && (this.fogPass.uniforms.uFogColor.value = this.fogColor);
            this.fogPass && (this.fogPass.uniforms.uFogNear.value = this.fogNear);
            this.fogPass && (this.fogPass.uniforms.uFogFar.value = this.fogFar);
            this.colorGradingPass && (this.colorGradingPass.uniforms.uSaturation.value = this.saturation);
            this.colorGradingPass && (this.colorGradingPass.uniforms.uContrast.value = this.contrast);
            this.colorGradingPass && (this.colorGradingPass.uniforms.uBrightness.value = this.brightness);
            this.colorGradingPass && (this.colorGradingPass.uniforms.uTemperature.value = this.temperature);
            this.vignettePass && (this.vignettePass.uniforms.uIntensity.value = this.vignetteIntensity);

            this.composer.render();
        }

        setBloom(v) { this.bloomEnabled = v; }
        setBloomIntensity(v) { this.bloomIntensity = v; }
        setFog(v) { this.fogEnabled = v; }
        setFogColor(hex) { this.fogColor.setHex(hex); }
        setFogRange(near, far) { this.fogNear = near; this.fogFar = far; }
        setColorGrading(v) { this.colorGradingEnabled = v; }
        setSaturation(v) { this.saturation = v; }
        setContrast(v) { this.contrast = v; }
        setBrightness(v) { this.brightness = v; }
        setTemperature(v) { this.temperature = v; }
        setVignette(v) { this.vignetteEnabled = v; }
        setVignetteIntensity(v) { this.vignetteIntensity = v; }
        setDOF(v) { this.dofEnabled = v; }

        dispose() {
            if (this.composer) {
                this.composer.passes.forEach(p => {
                    if (p.renderTarget) p.renderTarget.dispose();
                    if (p.material) p.material.dispose();
                });
                this.composer.dispose();
            }
            this.enabled = false;
        }
    }

    window.PostProcessing = PostProcessing;
    window.BloomShader = BloomShader;
    window.ColorGradingShader = ColorGradingShader;
    window.VignetteShader = VignetteShader;
    window.FogShader = FogShader;
})();
