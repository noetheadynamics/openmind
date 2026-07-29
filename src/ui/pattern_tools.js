/**
 * OpenMind – Pattern & Fill Tools
 * Fill, gradient, scatter, line, circle, sphere, checkerboard, stripes
 */
(function() {
    'use strict';

    const PatternType = Object.freeze({
        SOLID: 'solid', CHECKERBOARD: 'checkerboard', STRIPES: 'stripes',
        GRADIENT_HEIGHT: 'gradient_height', GRADIENT_DISTANCE: 'gradient_distance',
        RANDOM: 'random', LINE: 'line', CIRCLE: 'circle', SPHERE: 'sphere'
    });

    class PatternTools {
        constructor() {
            this.engine = null;
            this.listeners = [];
        }

        setEngine(e) { this.engine = e; }

        fillArea(bounds, blockType, pattern, options) {
            if (!this.engine || !bounds) return 0;
            options = options || {};
            let placed = 0;
            const { minX, minY, minZ, maxX, maxY, maxZ } = bounds;
            for (let x = minX; x <= maxX; x++)
                for (let y = minY; y <= maxY; y++)
                    for (let z = minZ; z <= maxZ; z++) {
                        const type = this.getPatternBlock(x, y, z, blockType, pattern, bounds, options);
                        if (type > 0) {
                            this.engine.setBlock(x, y, z, type);
                            placed++;
                        }
                    }
            return placed;
        }

        getPatternBlock(x, y, z, baseType, pattern, bounds, options) {
            switch (pattern) {
                case PatternType.SOLID: return baseType;
                case PatternType.CHECKERBOARD:
                    return ((x + y + z) % 2 === 0) ? baseType : (options.secondaryType || 0);
                case PatternType.STRIPES:
                    return (x % 2 === 0) ? baseType : (options.secondaryType || 0);
                case PatternType.GRADIENT_HEIGHT: {
                    const t = bounds.maxY === bounds.minY ? 0 : (y - bounds.minY) / (bounds.maxY - bounds.minY);
                    const types = options.gradientTypes || [1, 2, 3, 4];
                    return types[Math.floor(t * (types.length - 1))] || baseType;
                }
                case PatternType.GRADIENT_DISTANCE: {
                    const cx = (bounds.minX + bounds.maxX) / 2;
                    const cy = (bounds.minY + bounds.maxY) / 2;
                    const cz = (bounds.minZ + bounds.maxZ) / 2;
                    const maxDist = Math.sqrt(
                        Math.pow((bounds.maxX - bounds.minX) / 2, 2) +
                        Math.pow((bounds.maxY - bounds.minY) / 2, 2) +
                        Math.pow((bounds.maxZ - bounds.minZ) / 2, 2)
                    );
                    const dist = Math.sqrt(Math.pow(x - cx, 2) + Math.pow(y - cy, 2) + Math.pow(z - cz, 2));
                    const t = maxDist > 0 ? dist / maxDist : 0;
                    const types = options.gradientTypes || [1, 2, 3, 4];
                    return types[Math.floor(t * (types.length - 1))] || baseType;
                }
                case PatternType.RANDOM:
                    return Math.random() < (options.density || 0.5) ? baseType : 0;
                default: return baseType;
            }
        }

        line(x1, y1, z1, x2, y2, z2, blockType) {
            if (!this.engine) return 0;
            let placed = 0;
            const dx = Math.abs(x2 - x1), dy = Math.abs(y2 - y1), dz = Math.abs(z2 - z1);
            const steps = Math.max(dx, dy, dz) || 1;
            for (let i = 0; i <= steps; i++) {
                const t = i / steps;
                const x = Math.round(x1 + (x2 - x1) * t);
                const y = Math.round(y1 + (y2 - y1) * t);
                const z = Math.round(z1 + (z2 - z1) * t);
                this.engine.setBlock(x, y, z, blockType);
                placed++;
            }
            return placed;
        }

        circle(cx, cy, cz, radius, blockType, axis) {
            if (!this.engine) return 0;
            let placed = 0;
            const steps = Math.max(Math.ceil(radius * 6), 20);
            for (let i = 0; i < steps; i++) {
                const angle = (i / steps) * Math.PI * 2;
                const x = cx + Math.round(Math.cos(angle) * radius);
                const y = cy + Math.round(Math.sin(angle) * radius);
                if (axis === 'xz') this.engine.setBlock(x, cy, cz + Math.round(Math.sin(angle) * radius), blockType);
                else if (axis === 'yz') this.engine.setBlock(cx, y, cz + Math.round(Math.cos(angle) * radius), blockType);
                else this.engine.setBlock(x, y, cz, blockType);
                placed++;
            }
            return placed;
        }

        sphere(cx, cy, cz, radius, blockType, hollow) {
            if (!this.engine) return 0;
            let placed = 0;
            for (let x = -radius; x <= radius; x++)
                for (let y = -radius; y <= radius; y++)
                    for (let z = -radius; z <= radius; z++) {
                        const dist = Math.sqrt(x*x + y*y + z*z);
                        if (hollow) {
                            if (Math.abs(dist - radius) < 1) {
                                this.engine.setBlock(cx + x, cy + y, cz + z, blockType);
                                placed++;
                            }
                        } else if (dist <= radius) {
                            this.engine.setBlock(cx + x, cy + y, cz + z, blockType);
                            placed++;
                        }
                    }
            return placed;
        }

        rectangle(x1, y1, z1, x2, y2, z2, blockType) {
            if (!this.engine) return 0;
            let placed = 0;
            const minX = Math.min(x1, x2), maxX = Math.max(x1, x2);
            const minY = Math.min(y1, y2), maxY = Math.max(y1, y2);
            const minZ = Math.min(z1, z2), maxZ = Math.max(z1, z2);
            for (let x = minX; x <= maxX; x++)
                for (let y = minY; y <= maxY; y++)
                    for (let z = minZ; z <= maxZ; z++) {
                        this.engine.setBlock(x, y, z, blockType);
                        placed++;
                    }
            return placed;
        }

        on(fn) { this.listeners.push(fn); }
        emit(type, data) { for (const fn of this.listeners) fn({ type, ...data }); }
    }

    window.PatternType = PatternType;
    window.PatternTools = PatternTools;
})();
