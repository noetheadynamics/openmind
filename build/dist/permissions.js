const MAX_PROTECTED_AREAS = 100;

class PermissionsSystem {
    constructor() {
        this.roles = new Map(); // peerId -> role
        this.protectedAreas = [];
        this.protectedBlocks = new Map(); // "x,y,z" -> peerId
        this.worldSettings = {
            allowBuilding: true,
            allowInteractions: true,
            maxPlayers: 8,
            worldName: 'OpenMind World'
        };
    }

    setRole(peerId, role) {
        if (!['admin', 'builder', 'guest'].includes(role)) return;
        this.roles.set(peerId, role);
    }

    getRole(peerId) {
        return this.roles.get(peerId) || 'guest';
    }

    removePlayer(peerId) {
        this.roles.delete(peerId);
        const toDelete = [];
        this.protectedBlocks.forEach((owner, key) => {
            if (owner === peerId) toDelete.push(key);
        });
        toDelete.forEach(key => this.protectedBlocks.delete(key));
    }

    canBuild(peerId) {
        const role = this.getRole(peerId);
        return role === 'admin' || role === 'builder';
    }

    canInteract(peerId) {
        const role = this.getRole(peerId);
        if (role === 'admin') return true;
        if (role === 'builder') return true;
        if (this.worldSettings.allowInteractions && role === 'guest') return true;
        return false;
    }

    canKick(peerId) {
        return this.getRole(peerId) === 'admin';
    }

    canProtect(peerId) {
        return this.getRole(peerId) === 'admin';
    }

    canChangeSettings(peerId) {
        return this.getRole(peerId) === 'admin';
    }

    isProtected(x, y, z) {
        for (const area of this.protectedAreas) {
            if (x >= area.x1 && x <= area.x2 &&
                y >= area.y1 && y <= area.y2 &&
                z >= area.z1 && z <= area.z2) {
                return true;
            }
        }
        return false;
    }

    protectArea(x1, y1, z1, x2, y2, z2, peerId) {
        if (this.protectedAreas.length >= MAX_PROTECTED_AREAS) return false;
        this.protectedAreas.push({
            x1: Math.min(x1, x2), y1: Math.min(y1, y2), z1: Math.min(z1, z2),
            x2: Math.max(x1, x2), y2: Math.max(y1, y2), z2: Math.max(z1, z2),
            owner: peerId
        });
    }

    unprotectArea(index) {
        if (index >= 0 && index < this.protectedAreas.length) {
            this.protectedAreas.splice(index, 1);
            return true;
        }
        return false;
    }

    protectBlock(x, y, z, peerId) {
        this.protectedBlocks.set(`${x},${y},${z}`, peerId);
    }

    unprotectBlock(x, y, z) {
        this.protectedBlocks.delete(`${x},${y},${z}`);
    }

    isBlockProtected(x, y, z) {
        return this.protectedBlocks.has(`${x},${y},${z}`);
    }

    canModifyBlock(peerId, x, y, z) {
        if (!this.canBuild(peerId)) return false;
        if (this.isProtected(x, y, z)) {
            const role = this.getRole(peerId);
            return role === 'admin';
        }
        const owner = this.protectedBlocks.get(`${x},${y},${z}`);
        if (owner && owner !== peerId) {
            return this.getRole(peerId) === 'admin';
        }
        return true;
    }

    setWorldSetting(key, value) {
        if (key in this.worldSettings) {
            this.worldSettings[key] = value;
        }
    }

    getWorldSettings() {
        return { ...this.worldSettings };
    }

    getProtectedAreas() {
        return [...this.protectedAreas];
    }

    getRoleCounts() {
        const counts = { admin: 0, builder: 0, guest: 0 };
        this.roles.forEach(role => {
            if (counts[role] !== undefined) counts[role]++;
        });
        return counts;
    }

    serialize() {
        return {
            roles: Object.fromEntries(this.roles),
            protectedAreas: this.protectedAreas,
            protectedBlocks: Object.fromEntries(this.protectedBlocks),
            worldSettings: this.worldSettings
        };
    }

    deserialize(data) {
        if (data.roles && typeof data.roles === 'object') this.roles = new Map(Object.entries(data.roles));
        if (Array.isArray(data.protectedAreas)) this.protectedAreas = [...data.protectedAreas];
        if (data.protectedBlocks && typeof data.protectedBlocks === 'object') this.protectedBlocks = new Map(Object.entries(data.protectedBlocks));
        if (data.worldSettings && typeof data.worldSettings === 'object') this.worldSettings = { ...this.worldSettings, ...data.worldSettings };
    }

    clear() {
        this.roles.clear();
        this.protectedAreas = [];
        this.protectedBlocks.clear();
        this.worldSettings = {
            allowBuilding: true,
            allowInteractions: true,
            maxPlayers: 8,
            worldName: 'OpenMind World'
        };
    }
}

window.PermissionsSystem = PermissionsSystem;
window.MAX_PROTECTED_AREAS = MAX_PROTECTED_AREAS;
