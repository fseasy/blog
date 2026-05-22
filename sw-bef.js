---
---
/**
 * ============================================================
 * DESIGN: Fallback Strategy with Per-Service Timeout
 * ============================================================
 *
 * Priority order defined in _config.yml: production_service_try_order
 * Each service has its own connect_timeout_ms.
 *
 * For each request:
 *   1. Try services in priority order
 *   2. If success → return (with long cache header)
 *   3. If 404 → try next
 *   4. If network error / timeout → try next, record failure;
 *      if 3 consecutive failures → disable that service (for current page)
 *
 * Service state is per-page (resets on page reload / SW update).
 *
 * ============================================================
 */

(function () {
  'use strict';

  // ---- Config (injected by Jekyll Liquid) ----
  const BEF_PROD_HOST = '{{ site.bef_process.production_gen_link }}';
  const BEF_PROD_HOSTNAME = new URL(BEF_PROD_HOST).hostname;
  console.log('[SW-bef] Script evaluated, BEF_PROD_HOSTNAME:', BEF_PROD_HOSTNAME);

  const SERVICES_ALT = {{ site.bef_process.production_service_alternative | jsonify }};
  const TRY_ORDER = {{ site.bef_process.production_service_try_order | jsonify }};
  const FAILURE_THRESHOLD = 3;

  // ---- Build service list from config ----
  function buildServices(order, alts) {
    const services = [];
    for (const name of order) {
      const cfg = alts[name];
      if (!cfg || !cfg.host) continue;

      let baseUrl = cfg.port && cfg.port !== 443
        ? `${cfg.host}:${cfg.port}`
        : cfg.host;

      services.push({
        name,
        baseUrl,
        timeoutMs: cfg.connect_timeout_ms || 5000
      });
    }
    return services.length > 0 ? services : [{ name: 'prod', baseUrl: BEF_PROD_HOST, timeoutMs: 5000 }];
  }

  const SERVICES = buildServices(TRY_ORDER, SERVICES_ALT);
  console.log('[SW-bef] Services:', SERVICES.map(s => `${s.name}(${s.timeoutMs}ms)`).join(' > '));

  // ---- State: per-service failure tracking ----
  const serviceState = {};
  for (const svc of SERVICES) {
    serviceState[svc.name] = { failures: 0, disabled: false };
  }

  // ---- Build target URL ----
  function buildUrl(baseUrl, originalUrl) {
    const url = new URL(originalUrl);
    return new URL(url.pathname, baseUrl).toString();
  }

  // ---- Wrap response with long cache (1 year) ----
  function withLongCache(response) {
    const headers = new Headers(response.headers);
    headers.set('Cache-Control', 'max-age=31536000, immutable');
    const expires = new Date(Date.now() + 31536000 * 1000).toUTCString();
    headers.set('Expires', expires);
    return new Response(response.body, {
      status: response.status,
      statusText: response.statusText,
      headers
    });
  }

  // ---- Record failure / success ----
  function recordFailure(svcName) {
    const state = serviceState[svcName];
    state.failures++;
    console.warn(`[SW-bef] ${svcName} failed (${state.failures}/${FAILURE_THRESHOLD})`);
    if (state.failures >= FAILURE_THRESHOLD) {
      state.disabled = true;
      console.warn(`[SW-bef] ${svcName} disabled for this page`);
    }
  }

  function recordSuccess(svcName) {
    const state = serviceState[svcName];
    if (state.failures > 0) {
      state.failures = 0;
      console.log(`[SW-bef] ${svcName} recovered`);
    }
  }

  // ---- Fetch from one service with its own timeout ----
  function fetchFrom(url, baseUrl, svcName, timeoutMs) {
    const targetUrl = buildUrl(baseUrl, url);
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), timeoutMs);

    return fetch(targetUrl, { signal: controller.signal })
      .finally(() => clearTimeout(timeout))
      .then(response => {
        if (response.status === 404) {
          recordFailure(svcName);
          return { status: 404, ok: false, response };
        }
        recordSuccess(svcName);
        return { status: response.status, ok: response.ok, response };
      })
      .catch(err => {
        if (err.name === 'AbortError') {
          console.warn(`[SW-bef] ${svcName} timeout (>${timeoutMs}ms)`);
        } else {
          console.warn(`[SW-bef] ${svcName} network error:`, err.message);
        }
        recordFailure(svcName);
        return { status: 0, ok: false, error: err };
      });
  }

  // ---- Try services in priority order ----
  async function fetchWithFallback(originalUrl) {
    for (const svc of SERVICES) {
      if (serviceState[svc.name].disabled) {
        console.log(`[SW-bef] Skipping disabled ${svc.name}`);
        continue;
      }

      const result = await fetchFrom(originalUrl, svc.baseUrl, svc.name, svc.timeoutMs);

      if (result.ok) {
        console.log(`[SW-bef] ${svc.name} succeeded for ${new URL(originalUrl).pathname}`);
        return withLongCache(result.response);
      }

      if (result.status === 404) {
        console.log(`[SW-bef] ${svc.name} returned 404, trying next`);
        continue;
      }

      // network error or timeout — already recorded failure, try next
      continue;
    }

    throw new Error('All BEF services failed');
  }

  // ---- Event listeners ----
  self.addEventListener('install', event => {
    console.log('[SW-bef] Installing, skipping wait');
    self.skipWaiting();
  });

  self.addEventListener('activate', event => {
    console.log('[SW-bef] Activating, claiming clients');
    event.waitUntil(self.clients.claim());
  });

  self.addEventListener('fetch', event => {
    const url = new URL(event.request.url);
    if (url.hostname !== BEF_PROD_HOSTNAME) return;

    // console.log('[SW-bef] Intercepted:', url.href);
    event.respondWith(fetchWithFallback(event.request.url));
  });

})();
