const CACHE_NAME = 'openmind-v1';
const CORE_ASSETS = [
    '/',
    '/index.html',
    '/omni_console.css',
    '/ui_connection.js',
    '/llm_client.js',
    '/world_io.js',
    '/stats.js',
    '/voxel_renderer.js',
    '/omni_console.js',
    '/touch_controls.js',
    '/manifest.json',
    '/openmind.js',
    '/openmind.wasm',
    '/interactive_objects.js',
    '/state_machine.js',
    '/inventory_system.js',
    '/crafting_system.js',
    '/particle_system.js',
    '/water_renderer.js',
    '/skybox.js',
    '/post_processing.js',
    '/sound_system.js',
    '/physics_visuals.js',
    '/ui_animations.js',
    '/error_handler.js',
    '/loading_screen.js',
    '/tutorial.js',
    '/undo_redo.js',
    '/shortcuts.js',
    '/notifications.js',
    '/settings_panel.js',
    '/selection.js',
    '/copy_paste.js',
    '/blueprints.js',
    '/symmetry.js',
    '/pattern_tools.js',
    '/import_export_building.js',
    '/building_history.js',
    '/network.js',
    '/host.js',
    '/client.js',
    '/player_entities.js',
    '/chat.js',
    '/multiplayer_ui.js',
    '/permissions.js'
];

const CDN_ASSETS = [
    'https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js'
];

self.addEventListener('install', (event) => {
    event.waitUntil(
        caches.open(CACHE_NAME)
            .then(cache => {
                console.log('[SW] Caching core assets');
                return cache.addAll(CORE_ASSETS).catch(err => {
                    console.warn('[SW] Some core assets failed to cache:', err);
                    return Promise.resolve();
                });
            })
            .then(() => self.skipWaiting())
    );
});

self.addEventListener('activate', (event) => {
    event.waitUntil(
        caches.keys()
            .then(keys => Promise.all(
                keys.filter(key => key !== CACHE_NAME)
                    .map(key => caches.delete(key))
            ))
            .then(() => self.clients.claim())
    );
});

self.addEventListener('fetch', (event) => {
    const url = new URL(event.request.url);

    if (event.request.url.includes('signaling.openmind.dev')) {
        event.respondWith(fetch(event.request));
        return;
    }

    if (event.request.url.includes('api.groq.com') || event.request.url.includes('api.openai.com') ||
        event.request.url.includes('api.anthropic.com') || event.request.url.includes('generativelanguage.googleapis.com')) {
        event.respondWith(fetch(event.request));
        return;
    }

    event.respondWith(
        caches.match(event.request)
            .then(cached => {
                if (cached) return cached;

                return fetch(event.request)
                    .then(response => {
                        if (response && response.status === 200 && response.type === 'basic') {
                            const responseToCache = response.clone();
                            caches.open(CACHE_NAME)
                                .then(cache => cache.put(event.request, responseToCache));
                        }
                        return response;
                    })
                    .catch(() => {
                        if (event.request.destination === 'document') {
                            return caches.match('/index.html');
                        }
                        return new Response('Offline', { status: 503 });
                    });
            })
    );
});

self.addEventListener('message', (event) => {
    if (event.data && event.data.type === 'SKIP_WAITING') {
        self.skipWaiting();
    }
});
