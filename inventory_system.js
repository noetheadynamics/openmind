/**
 * OpenMind – Inventory & Item System
 * Player/chest inventory, drag-drop, item use, crafting integration
 */
(function() {
    'use strict';

    const ItemDefs = {
        wood: { name: 'Wood', icon: '🪵', stackable: true, maxStack: 64, type: 'material' },
        stone: { name: 'Stone', icon: '🪨', stackable: true, maxStack: 64, type: 'material' },
        iron: { name: 'Iron', icon: '⚙️', stackable: true, maxStack: 64, type: 'material' },
        copper: { name: 'Copper', icon: '🔶', stackable: true, maxStack: 64, type: 'material' },
        gold: { name: 'Gold', icon: '🟡', stackable: true, maxStack: 64, type: 'material' },
        diamond: { name: 'Diamond', icon: '💎', stackable: true, maxStack: 32, type: 'material' },
        coal: { name: 'Coal', icon: '⚫', stackable: true, maxStack: 64, type: 'material' },
        key: { name: 'Key', icon: '🔑', stackable: false, maxStack: 1, type: 'tool' },
        key_DOOR: { name: 'Door Key', icon: '🔑', stackable: false, maxStack: 1, type: 'tool' },
        key_CHEST: { name: 'Chest Key', icon: '🗝️', stackable: false, maxStack: 1, type: 'tool' },
        key_LOCK: { name: 'Lock Key', icon: '🔐', stackable: false, maxStack: 1, type: 'tool' },
        torch: { name: 'Torch', icon: '🔥', stackable: true, maxStack: 32, type: 'placeable' },
        door: { name: 'Door', icon: '🚪', stackable: false, maxStack: 1, type: 'placeable' },
        lever: { name: 'Lever', icon: '🔧', stackable: false, maxStack: 1, type: 'placeable' },
        button: { name: 'Button', icon: '🔘', stackable: true, maxStack: 16, type: 'placeable' },
        lamp: { name: 'Lamp', icon: '💡', stackable: true, maxStack: 16, type: 'placeable' },
        chest_item: { name: 'Chest', icon: '📦', stackable: true, maxStack: 8, type: 'placeable' },
        trapdoor: { name: 'Trapdoor', icon: '🔲', stackable: true, maxStack: 16, type: 'placeable' },
        piston: { name: 'Piston', icon: '🔵', stackable: true, maxStack: 16, type: 'placeable' },
        conveyor: { name: 'Conveyor', icon: '➡️', stackable: true, maxStack: 32, type: 'placeable' },
        switch_item: { name: 'Switch', icon: '🎚️', stackable: true, maxStack: 16, type: 'placeable' },
        launcher: { name: 'Launcher', icon: '🚀', stackable: false, maxStack: 1, type: 'placeable' },
        sensor: { name: 'Sensor', icon: '📡', stackable: true, maxStack: 8, type: 'placeable' },
        timer: { name: 'Timer', icon: '⏱️', stackable: true, maxStack: 8, type: 'placeable' },
        computer: { name: 'Computer', icon: '🖥️', stackable: false, maxStack: 1, type: 'placeable' },
        rocket: { name: 'Rocket', icon: '🚀', stackable: false, maxStack: 1, type: 'tool' },
        pickaxe: { name: 'Pickaxe', icon: '⛏️', stackable: false, maxStack: 1, type: 'tool', durability: 100 },
        shovel: { name: 'Shovel', icon: '🪏', stackable: false, maxStack: 1, type: 'tool', durability: 80 }
    };

    const BLOCK_TO_ITEM = {
        1: 'stone', 2: 'stone', 3: 'wood', 5: 'stone', 9: 'iron',
        10: 'copper', 11: 'gold', 12: 'iron', 13: 'diamond', 14: 'coal',
        30: 'door', 31: 'button', 32: 'launcher', 33: 'key_DOOR',
        34: 'lamp', 35: 'chest_item', 36: 'switch_item', 37: 'conveyor',
        38: 'piston', 39: 'trapdoor'
    };

    class InventorySlot {
        constructor(itemId, count) {
            this.itemId = itemId || null;
            this.count = count || 0;
        }

        get isEmpty() { return !this.itemId || this.count <= 0; }

        get def() { return this.itemId ? ItemDefs[this.itemId] : null; }

        add(count) {
            if (!this.itemId) return 0;
            const def = this.def;
            if (!def) return 0;
            const space = def.maxStack - this.count;
            const added = Math.min(count, space);
            this.count += added;
            return count - added;
        }

        remove(count) {
            const removed = Math.min(count, this.count);
            this.count -= removed;
            if (this.count <= 0) { this.itemId = null; this.count = 0; }
            return removed;
        }

        set(itemId, count) { this.itemId = itemId; this.count = count; }
        clear() { this.itemId = null; this.count = 0; }
        clone() { return new InventorySlot(this.itemId, this.count); }
    }

    class Inventory {
        constructor(size) {
            this.size = size || 36;
            this.slots = [];
            for (let i = 0; i < this.size; i++) this.slots.push(new InventorySlot());
            this.selectedSlot = 0;
            this.listeners = [];
        }

        get selected() { return this.slots[this.selectedSlot]; }

        addItem(itemId, count) {
            count = count || 1;
            for (const slot of this.slots) {
                if (slot.itemId === itemId && slot.count < (ItemDefs[itemId]?.maxStack || 64)) {
                    const left = slot.add(count);
                    if (left <= 0) { this.emit('change'); return 0; }
                    count = left;
                }
            }
            for (const slot of this.slots) {
                if (slot.isEmpty) {
                    slot.set(itemId, count);
                    this.emit('change');
                    return 0;
                }
            }
            return count;
        }

        removeItem(itemId, count) {
            count = count || 1;
            for (const slot of this.slots) {
                if (slot.itemId === itemId && slot.count > 0) {
                    const removed = slot.remove(count);
                    count -= removed;
                    if (count <= 0) { this.emit('change'); return 0; }
                }
            }
            return count;
        }

        hasItem(itemId, count) {
            count = count || 1;
            let total = 0;
            for (const slot of this.slots) {
                if (slot.itemId === itemId) total += slot.count;
            }
            return total >= count;
        }

        countItem(itemId) {
            let total = 0;
            for (const slot of this.slots) {
                if (slot.itemId === itemId) total += slot.count;
            }
            return total;
        }

        getSlot(index) { return this.slots[index] || null; }

        swapSlots(a, b) {
            const temp = this.slots[a].clone();
            this.slots[a] = this.slots[b].clone();
            this.slots[b] = temp;
            this.emit('change');
        }

        selectSlot(index) {
            if (index >= 0 && index < this.size) {
                this.selectedSlot = index;
                this.emit('select', index);
            }
        }

        clear() {
            for (const slot of this.slots) slot.clear();
            this.emit('change');
        }

        on(fn) { this.listeners.push(fn); }
        off(fn) { this.listeners = this.listeners.filter(f => f !== fn); }
        emit(type, data) {
            for (const fn of this.listeners) fn({ type, data });
        }

        serialize() {
            return this.slots.map(s => ({ id: s.itemId, c: s.count }));
        }

        deserialize(data) {
            for (let i = 0; i < this.size; i++) {
                if (data[i] && data[i].id) {
                    this.slots[i].set(data[i].id, data[i].c);
                } else {
                    this.slots[i].clear();
                }
            }
            this.emit('change');
        }
    }

    class InventorySystem {
        constructor() {
            this.player = new Inventory(36);
            this.chests = new Map();
            this.hotbarSize = 9;
            this.selectedHotbar = 0;
            this.renderer = null;
            this.engine = null;
            this.interactiveSystem = null;
            this.listeners = [];
            this.dragItem = null;
            this.dragCount = 0;
        }

        setRenderer(r) { this.renderer = r; }
        setEngine(e) { this.engine = e; }
        setInteractiveSystem(s) { this.interactiveSystem = s; }

        getChestInventory(chestId) {
            if (!this.chests.has(chestId)) {
                this.chests.set(chestId, new Inventory(27));
            }
            return this.chests.get(chestId);
        }

        openChest(chestId) {
            const inv = this.getChestInventory(chestId);
            this.emit('chest_open', { chestId, inventory: inv });
            return inv;
        }

        closeChest(chestId) {
            this.emit('chest_close', { chestId });
        }

        transferToChest(chestId, slotIndex, count) {
            const srcSlot = this.player.getSlot(slotIndex);
            if (srcSlot.isEmpty) return false;
            const chest = this.getChestInventory(chestId);
            const left = chest.addItem(srcSlot.itemId, count || srcSlot.count);
            if (left < srcSlot.count) {
                srcSlot.remove(srcSlot.count - left);
                this.player.emit('change');
                return true;
            }
            return false;
        }

        transferFromChest(chestId, slotIndex, count) {
            const chest = this.getChestInventory(chestId);
            const srcSlot = chest.getSlot(slotIndex);
            if (srcSlot.isEmpty) return false;
            const left = this.player.addItem(srcSlot.itemId, count || srcSlot.count);
            if (left < srcSlot.count) {
                srcSlot.remove(srcSlot.count - left);
                chest.emit('change');
                return true;
            }
            return false;
        }

        pickUpBlock(blockType) {
            const itemId = BLOCK_TO_ITEM[blockType];
            if (itemId) {
                const left = this.player.addItem(itemId, 1);
                if (left === 0) {
                    this.emit('pickup', { blockType, itemId });
                    return true;
                }
            }
            return false;
        }

        useItem(itemId) {
            if (!this.player.hasItem(itemId)) return false;
            const def = ItemDefs[itemId];
            if (!def) return false;

            if (itemId.startsWith('key_')) {
                this.emit('use_key', { itemId, keyType: itemId });
                return true;
            }

            if (def.type === 'placeable') {
                this.emit('use_placeable', { itemId });
                return true;
            }

            if (def.type === 'tool') {
                this.emit('use_tool', { itemId });
                return true;
            }

            return false;
        }

        startDrag(slotIndex, inventory) {
            const slot = inventory.getSlot(slotIndex);
            if (slot.isEmpty) return false;
            this.dragItem = slot.itemId;
            this.dragCount = 1;
            slot.remove(1);
            this.emit('drag_start', { item: this.dragItem, count: this.dragCount });
            return true;
        }

        dropDrag(targetSlot) {
            if (!this.dragItem) return false;
            if (targetSlot.isEmpty) {
                targetSlot.set(this.dragItem, this.dragCount);
            } else if (targetSlot.itemId === this.dragItem) {
                const left = targetSlot.add(this.dragCount);
                if (left > 0) return false;
            } else {
                const tempId = targetSlot.itemId;
                const tempCount = targetSlot.count;
                targetSlot.set(this.dragItem, this.dragCount);
                this.dragItem = tempId;
                this.dragCount = tempCount;
                return true;
            }
            this.dragItem = null;
            this.dragCount = 0;
            this.emit('drag_end');
            return true;
        }

        cancelDrag() {
            if (this.dragItem) {
                this.player.addItem(this.dragItem, this.dragCount);
                this.dragItem = null;
                this.dragCount = 0;
                this.emit('drag_end');
            }
        }

        serialize() {
            return {
                player: this.player.serialize(),
                chests: Object.fromEntries(
                    Array.from(this.chests.entries()).map(([k, v]) => [k, v.serialize()])
                ),
                selectedHotbar: this.selectedHotbar
            };
        }

        deserialize(data) {
            this.player.deserialize(data.player || []);
            this.selectedHotbar = data.selectedHotbar || 0;
            for (const [k, v] of Object.entries(data.chests || {})) {
                const inv = this.getChestInventory(k);
                inv.deserialize(v);
            }
        }

        on(fn) { this.listeners.push(fn); }
        emit(type, data) {
            for (const fn of this.listeners) fn({ type, ...data });
        }
    }

    window.ItemDefs = ItemDefs;
    window.BLOCK_TO_ITEM = BLOCK_TO_ITEM;
    window.InventorySlot = InventorySlot;
    window.Inventory = Inventory;
    window.InventorySystem = InventorySystem;
})();
