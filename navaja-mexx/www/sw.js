// NAVAJA MEXX Service Worker v3.0 — jul 2026
// Motor IA: claude-sonnet-4-6 · Anthropic
const CACHE = 'navaja-mexx-v3-2026';
const ASSETS = ['./index.html', './manifest.json'];

self.addEventListener('install', e => {
  e.waitUntil(
    caches.open(CACHE).then(c => c.addAll(ASSETS))
  );
  self.skipWaiting();
});

self.addEventListener('activate', e => {
  e.waitUntil(
    caches.keys().then(keys =>
      Promise.all(
        keys.filter(k => k !== CACHE).map(k => caches.delete(k))
      )
    )
  );
  self.clients.claim();
});

self.addEventListener('fetch', e => {
  // APIs de Anthropic siempre network-first (sin caché)
  if (e.request.url.includes('anthropic.com') ||
      e.request.url.includes('ipapi.co') ||
      e.request.url.includes('cloudflare-dns.com')) {
    return; // network directo
  }
  // Assets locales: cache-first con fallback a network
  e.respondWith(
    caches.match(e.request).then(cached => cached || fetch(e.request))
  );
});
