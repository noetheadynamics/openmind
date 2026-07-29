/**
 * OpenMind – Shared Utilities
 * escapeHtml, constants, formatting, file download
 */
(function() {
    'use strict';

    const DAY_LENGTH = 36000;

    const OMUtils = {
        DAY_LENGTH,

        escapeHtml(str) {
            if (str === null || str === undefined) return '';
            const div = document.createElement('div');
            div.textContent = String(str);
            return div.innerHTML;
        },

        isValidHexColor(str) {
            return typeof str === 'string' && /^#([0-9a-fA-F]{3}|[0-9a-fA-F]{6})$/.test(str);
        },

        safeHexColor(str, fallback) {
            if (OMUtils.isValidHexColor(str)) return str;
            return fallback || '#ffffff';
        },

        formatTime(h) {
            const hh = Math.floor(h) % 24;
            const mm = Math.floor((h - Math.floor(h)) * 60);
            return String(hh).padStart(2, '0') + ':' + String(mm).padStart(2, '0');
        },

        formatTimeFull(h) {
            const hh = Math.floor(h) % 24;
            const mm = Math.floor((h - Math.floor(h)) * 60);
            const ss = Math.floor(((h * 60) % 1) * 60);
            return String(hh).padStart(2, '0') + ':' + String(mm).padStart(2, '0') + ':' + String(ss).padStart(2, '0');
        },

        downloadBlob(data, filename, mimeType) {
            const blob = new Blob([data], { type: mimeType || 'text/plain' });
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = filename;
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            URL.revokeObjectURL(url);
        }
    };

    window.OMUtils = OMUtils;
})();
