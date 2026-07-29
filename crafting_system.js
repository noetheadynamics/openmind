/**
 * OpenMind – Crafting System
 * Recipes, crafting UI, AI generation support
 */
(function() {
    'use strict';

    const Recipes = [
        { id: 'door', name: 'Door', result: 'door', count: 1, ingredients: [{ item: 'wood', count: 4 }, { item: 'iron', count: 1 }] },
        { id: 'lever', name: 'Lever', result: 'lever', count: 1, ingredients: [{ item: 'wood', count: 1 }, { item: 'stone', count: 1 }] },
        { id: 'button', name: 'Button', result: 'button', count: 2, ingredients: [{ item: 'stone', count: 1 }, { item: 'iron', count: 1 }] },
        { id: 'lamp', name: 'Lamp', result: 'lamp', count: 1, ingredients: [{ item: 'iron', count: 2 }, { item: 'gold', count: 1 }, { item: 'coal', count: 1 }] },
        { id: 'chest_item', name: 'Chest', result: 'chest_item', count: 1, ingredients: [{ item: 'wood', count: 8 }] },
        { id: 'trapdoor', name: 'Trapdoor', result: 'trapdoor', count: 2, ingredients: [{ item: 'wood', count: 4 }, { item: 'iron', count: 1 }] },
        { id: 'piston', name: 'Piston', result: 'piston', count: 1, ingredients: [{ item: 'iron', count: 3 }, { item: 'wood', count: 2 }, { item: 'copper', count: 1 }] },
        { id: 'conveyor', name: 'Conveyor', result: 'conveyor', count: 4, ingredients: [{ item: 'iron', count: 2 }, { item: 'copper', count: 2 }] },
        { id: 'switch_item', name: 'Switch', result: 'switch_item', count: 1, ingredients: [{ item: 'iron', count: 1 }, { item: 'copper', count: 1 }] },
        { id: 'launcher', name: 'Launcher', result: 'launcher', count: 1, ingredients: [{ item: 'iron', count: 4 }, { item: 'gold', count: 2 }, { item: 'diamond', count: 1 }] },
        { id: 'sensor', name: 'Sensor', result: 'sensor', count: 1, ingredients: [{ item: 'copper', count: 3 }, { item: 'iron', count: 1 }] },
        { id: 'timer', name: 'Timer', result: 'timer', count: 1, ingredients: [{ item: 'iron', count: 2 }, { item: 'copper', count: 2 }, { item: 'gold', count: 1 }] },
        { id: 'computer', name: 'Computer', result: 'computer', count: 1, ingredients: [{ item: 'iron', count: 4 }, { item: 'copper', count: 4 }, { item: 'diamond', count: 2 }, { item: 'gold', count: 2 }] },
        { id: 'key', name: 'Key', result: 'key', count: 1, ingredients: [{ item: 'iron', count: 2 }, { item: 'gold', count: 1 }] },
        { id: 'torch', name: 'Torch', result: 'torch', count: 4, ingredients: [{ item: 'wood', count: 1 }, { item: 'coal', count: 1 }] },
        { id: 'pickaxe', name: 'Pickaxe', result: 'pickaxe', count: 1, ingredients: [{ item: 'iron', count: 3 }, { item: 'wood', count: 2 }] },
        { id: 'shovel', name: 'Shovel', result: 'shovel', count: 1, ingredients: [{ item: 'iron', count: 1 }, { item: 'wood', count: 2 }] }
    ];

    class CraftingSystem {
        constructor() {
            this.recipes = [...Recipes];
            this.inventory = null;
            this.listeners = [];
            this.lastCrafted = null;
        }

        setInventory(inv) { this.inventory = inv; }

        findRecipe(resultId) {
            return this.recipes.find(r => r.result === resultId);
        }

        canCraft(recipeId) {
            const recipe = this.recipes.find(r => r.id === recipeId);
            if (!recipe || !this.inventory) return false;
            for (const ing of recipe.ingredients) {
                if (!this.inventory.player.hasItem(ing.item, ing.count)) return false;
            }
            return true;
        }

        craft(recipeId) {
            const recipe = this.recipes.find(r => r.id === recipeId);
            if (!recipe || !this.inventory) return null;
            if (!this.canCraft(recipeId)) return null;

            for (const ing of recipe.ingredients) {
                this.inventory.player.removeItem(ing.item, ing.count);
            }
            const left = this.inventory.player.addItem(recipe.result, recipe.count);
            this.lastCrafted = { recipe, count: recipe.count - left };
            this.emit('craft', { recipe, success: left === 0 });
            return { recipe, count: recipe.count, success: left === 0 };
        }

        getAvailableRecipes() {
            return this.recipes.map(r => ({
                ...r,
                canCraft: this.canCraft(r.id),
                ingredients: r.ingredients.map(ing => ({
                    ...ing,
                    has: this.inventory ? this.inventory.player.countItem(ing.item) : 0
                }))
            }));
        }

        addRecipe(recipe) {
            if (!recipe.id || !recipe.result || !recipe.ingredients) return false;
            if (this.recipes.find(r => r.id === recipe.id)) return false;
            this.recipes.push(recipe);
            this.emit('recipe_added', recipe);
            return true;
        }

        removeRecipe(recipeId) {
            const idx = this.recipes.findIndex(r => r.id === recipeId);
            if (idx === -1) return false;
            this.recipes.splice(idx, 1);
            this.emit('recipe_removed', { id: recipeId });
            return true;
        }

        generateFromAI(prompt) {
            const hash = this.simpleHash(prompt);
            const items = Object.keys(ItemDefs).filter(k => ItemDefs[k].type === 'material');
            const resultItem = items[hash % items.length];
            const count = (hash % 4) + 1;
            const ingCount = (hash % 3) + 1;
            const ingredients = [];
            for (let i = 0; i < ingCount; i++) {
                const item = items[(hash + i * 7) % items.length];
                const c = ((hash + i * 3) % 5) + 1;
                const existing = ingredients.find(ig => ig.item === item);
                if (existing) {
                    existing.count += c;
                } else {
                    ingredients.push({ item, count: c });
                }
            }
            return {
                id: 'ai_' + hash,
                name: 'AI: ' + prompt.substring(0, 20),
                result: resultItem,
                count,
                ingredients,
                aiGenerated: true
            };
        }

        simpleHash(str) {
            let hash = 0;
            for (let i = 0; i < str.length; i++) {
                hash = ((hash << 5) - hash) + str.charCodeAt(i);
                hash |= 0;
            }
            return Math.abs(hash);
        }

        on(fn) { this.listeners.push(fn); }
        emit(type, data) {
            for (const fn of this.listeners) fn({ type, ...data });
        }
    }

    window.CraftingRecipes = Recipes;
    window.CraftingSystem = CraftingSystem;
})();
