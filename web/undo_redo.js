/**
 * OpenMind – Undo/Redo System
 * 100-action history, Ctrl+Z/Y, batch undo/redo
 */
(function() {
    'use strict';

    class UndoRedo {
        constructor(maxHistory) {
            this.maxHistory = maxHistory || 100;
            this.undoStack = [];
            this.redoStack = [];
            this.listeners = [];
        }

        execute(action) {
            if (!action || !action.do) return false;
            action.do();
            this.undoStack.push(action);
            if (this.undoStack.length > this.maxHistory) this.undoStack.shift();
            this.redoStack = [];
            this.emit('execute', action);
            this.emit('change');
            return true;
        }

        undo() {
            if (this.undoStack.length === 0) return false;
            const action = this.undoStack.pop();
            if (action.undo) action.undo();
            this.redoStack.push(action);
            this.emit('undo', action);
            this.emit('change');
            return true;
        }

        redo() {
            if (this.redoStack.length === 0) return false;
            const action = this.redoStack.pop();
            if (action.do) action.do();
            this.undoStack.push(action);
            this.emit('redo', action);
            this.emit('change');
            return true;
        }

        batch(actions) {
            if (!Array.isArray(actions) || actions.length === 0) return false;
            const batchAction = {
                do: () => { for (const a of actions) a.do(); },
                undo: () => { for (let i = actions.length - 1; i >= 0; i--) { if (actions[i].undo) actions[i].undo(); } },
                description: actions.map(a => a.description).join(', ')
            };
            return this.execute(batchAction);
        }

        canUndo() { return this.undoStack.length > 0; }
        canRedo() { return this.redoStack.length > 0; }

        getHistory() {
            return this.undoStack.map((a, i) => ({
                index: i,
                description: a.description || 'Action ' + (i + 1)
            }));
        }

        getRedoHistory() {
            return this.redoStack.map((a, i) => ({
                index: i,
                description: a.description || 'Redo ' + (i + 1)
            }));
        }

        clear() {
            this.undoStack = [];
            this.redoStack = [];
            this.emit('change');
        }

        on(fn) { this.listeners.push(fn); }
        emit(type, data) {
            for (const fn of this.listeners) fn({ type, ...data });
        }
    }

    window.UndoRedo = UndoRedo;
})();
