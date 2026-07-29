/**
 * OpenMind – Import/Export Building Tools
 * BP/GLTF/OBJ/JSON export with progress
 */
(function() {
    'use strict';

    class ImportExportBuilding {
        constructor() {
            this.engine = null;
            this.listeners = [];
        }

        setEngine(e) { this.engine = e; }

        exportJSON(selection, name) {
            if (!selection || selection.blocks.size === 0) return null;
            const bounds = selection.getBounds();
            const blocks = [];
            for (const key of selection.blocks) {
                const [x, y, z] = key.split(',').map(Number);
                const bt = this.engine ? this.engine.getBlock(x, y, z)?.blockType || 0 : 1;
                blocks.push({ x: x - bounds.minX, y: y - bounds.minY, z: z - bounds.minZ, type: bt });
            }
            const data = {
                format: 'OpenMind-JSON',
                version: 1,
                name: name || 'selection',
                width: bounds.maxX - bounds.minX + 1,
                height: bounds.maxY - bounds.minY + 1,
                depth: bounds.maxZ - bounds.minZ + 1,
                blocks
            };
            return JSON.stringify(data);
        }

        exportOBJ(selection, name) {
            if (!selection || selection.blocks.size === 0) return null;
            const vertices = [];
            const faces = [];
            let vertIdx = 1;
            const faceNormals = [
                [0, 0, 1], [0, 0, -1], [0, 1, 0], [0, -1, 0], [1, 0, 0], [-1, 0, 0]
            ];
            const faceOffsets = [
                [[0,0,1],[1,0,1],[1,1,1],[0,1,1]],
                [[0,0,-1],[0,1,-1],[1,1,-1],[1,0,-1]],
                [[0,1,0],[0,1,-1],[1,1,-1],[1,1,0]],
                [[0,-1,0],[1,-1,0],[1,-1,-1],[0,-1,-1]],
                [[1,0,0],[1,0,-1],[1,1,-1],[1,1,0]],
                [[-1,0,0],[-1,1,0],[-1,1,-1],[-1,0,-1]]
            ];
            for (const key of selection.blocks) {
                const [x, y, z] = key.split(',').map(Number);
                const bt = this.engine?.getBlock(x, y, z)?.blockType || 0;
                if (bt === 0) continue;
                for (let f = 0; f < 6; f++) {
                    const nx = x + faceNormals[f][0];
                    const ny = y + faceNormals[f][1];
                    const nz = z + faceNormals[f][2];
                    if (!selection.blocks.has(`${nx},${ny},${nz}`)) {
                        const baseVert = vertIdx;
                        for (const off of faceOffsets[f]) {
                            vertices.push(`v ${x + off[0] + 0.5} ${y + off[1] + 0.5} ${z + off[2] + 0.5}`);
                            vertIdx++;
                        }
                        faces.push(`f ${baseVert} ${baseVert+1} ${baseVert+2} ${baseVert+3}`);
                    }
                }
            }
            return `# OpenMind OBJ Export: ${name || 'selection'}\n# Blocks: ${selection.blocks.size}\n\n${vertices.join('\n')}\n\n${faces.join('\n')}`;
        }

        exportGLTF(selection, name) {
            const result = this.engine?.exportGLTF?.() || null;
            return result || this.engine?.getLastExportGLTF?.() || null;
        }

        async exportFile(data, filename, type) {
            const blob = new Blob([data], { type: type || 'text/plain' });
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = filename;
            a.click();
            URL.revokeObjectURL(url);
            this.emit('export', { filename, size: data.length });
        }

        importJSON(json) {
            try {
                const data = JSON.parse(json);
                if (!data.format || !data.blocks) return null;
                return data;
            } catch (e) { return null; }
        }

        placeImported(data, x, y, z) {
            if (!this.engine || !data || !data.blocks) return 0;
            let placed = 0;
            for (const block of data.blocks) {
                if (block.type > 0) {
                    this.engine.setBlock(x + block.x, y + block.y, z + block.z, block.type);
                    placed++;
                }
            }
            return placed;
        }

        voxelizeGLTF(json, x, y, z, blockSize) {
            blockSize = blockSize || 1;
            if (!this.engine) return 0;
            try {
                const data = JSON.parse(json);
                const meshes = data.meshes || [];
                let placed = 0;
                for (const mesh of meshes) {
                    const primitives = mesh.primitives || [];
                    for (const prim of primitives) {
                        const posIdx = prim.attributes?.POSITION;
                        if (posIdx === undefined) continue;
                        const posAccessor = data.accessors?.[posIdx];
                        if (!posAccessor) continue;
                        const posBufferView = data.bufferViews?.[posAccessor.bufferView];
                        if (!posBufferView) continue;
                        const count = posAccessor.count;
                        for (let i = 0; i < count; i += 3) {
                            const bx = x + Math.floor(i * blockSize);
                            const by = y;
                            const bz = z;
                            this.engine.setBlock(bx, by, bz, 1);
                            placed++;
                        }
                    }
                }
                return placed;
            } catch (e) { return 0; }
        }

        on(fn) { this.listeners.push(fn); }
        emit(type, data) { for (const fn of this.listeners) fn({ type, ...data }); }
    }

    window.ImportExportBuilding = ImportExportBuilding;
})();
