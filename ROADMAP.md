# InVxTermux roadmap

Product direction for this Termux fork (`com.involvex.termux_app`, display name
**InVxTermux**): stay in the terminal, develop on PC and phone against the same
git remotes, preview local servers on-device, and optionally attach AI CLIs.

Docs: https://involvex.github.io/termux-app/ · Repo:
https://github.com/involvex/termux-app

> Unofficial fork of [termux/termux-app](https://github.com/termux/termux-app).
> Not affiliated with the Termux maintainers.

## Pillars

| Pillar | Intent |
|--------|--------|
| Terminal-first | git / bun / build stay in-session |
| PC ↔ phone | Same repo: push on PC, pull on phone under `~/repos` |
| Preview | In-app view of `http://127.0.0.1:<port>` |
| AI CLI | e.g. `td-ai` → OpenCode web → Preview |

## Phases

### 1. Runtime (done)

- Bundled Android Bun (`$PREFIX/libexec/bun` + `$PREFIX/bin/bun` shim)
- Shim preloads `libinvapp-bun-seccomp.so` (Android seccomp SIGSYS→ENOSYS) and
  injects `--os=android` on install/add/create
- Redirector rewrites `#!/usr/bin/env` so stock `npm`/`npx` shebangs work
- Default cwd `~/repos` (exec); shared storage for browse/sync only

### 2. Preview (done)

- Drawer **Preview** → `LocalhostPreviewActivity`
- Port field + **Scan** → listening TCP chips (one-tap open)
- Loopback-only navigation; cleartext only for localhost

### 3. AI tools (done — bootstrap)

- `opencode-setup` — download official `opencode-linux-*.tar.gz` + glibc wrapper
- `td-ai [port]` — install if needed, start `opencode web` (default **4096**), print Preview hint
- Not baked into the APK; installs into the Termux prefix on demand

### 4. Workflow UX (done)

- Drawer snippets: `git pull`, `bun i`, `bun run dev`, **Repos**, **Run…**
- Default extra-keys row: pull / bun i / dev / repos (if user has not customized `extra-keys`)
- Snackbar when a preferred localhost port newly appears → open Preview
- Repo picker (`~/repos`) and `package.json` script run sheet

### 5. LAN share (done — opt-in)

- Preview **Copy LAN** / long-press port chip → clipboard `http://<wifi-ip>:<port>/`
- Only when the server listens on `0.0.0.0` / `::` (loopback-only binds explain how to fix)
- No public tunnels by default

### 6. Dev CLI reliability (done)

- `node` shim → Bun when `nodejs` pkg is absent (package bins with
  `#!/usr/bin/env node`)
- Bun preload = seccomp + redirector (SIGSYS fix **and** shebang rewrite for
  bunx children)
- `bunx` passes `--bun`; `td-dev [script]` for Preview/LAN hints; hardened `td-ai`
- [bun-termux-loader](https://github.com/kaan-escober/bun-termux-loader) is for
  glibc `bun build --compile` bundles — not needed for our official Android Bun

### 7. Vite golden path + AI session UX (done)

- `td-scaffold [name] [template]` — Vite app under `~/repos` (default
  `react-ts`; also `react`, `vue`, `vanilla-ts`, `pwa`, …) with `dev` on
  `0.0.0.0:5173` for Preview / Copy LAN; seeds `.gitignore` + README;
  flags `--here` / `--no-install`
- Drawer **New…** → name + template → runs `td-scaffold`
- Drawer **AI** → `td-ai` in the current session + open Preview on `:4096`
### 8. Vite-PWA scaffold (done)

- `td-scaffold [name] pwa` / `pwa-react` — create-vite base + `vite-plugin-pwa`
  (autoUpdate, basic manifest, same `0.0.0.0:5173` Preview / Copy LAN path)
- Drawer **New…** lists `pwa` and `pwa-react` alongside vanilla/react/vue
- Dev remains the golden path; `bun run build` produces a installable PWA
  for home-screen use (no Capacitor / native packaging)

### 9. AI session status + control (done)

- Drawer **AI** probes OpenCode: missing / installed / ready (listening on `:4096`)
- Start `td-ai` only when needed; if ready, jump straight to Preview
- Drawer **Stop AI** — SIGTERM listeners on `:4096` (+ `pkill` OpenCode fallback)
- Optional Snackbar: recent terminal error → copy for paste into OpenCode

### 10. Clone into ~/repos (done)

- `td-clone <git-url> [name] [--bun-i]` — clone under `~/repos` (name from URL if omitted);
  preloads path redirector so stock git works with this package id
- Drawer **Clone…** → URL + name + optional **bun install**
- Completes PC ↔ phone: push on desktop, clone/pull on phone

### 11. OpenCode postinstall + customizable quick bar (done)

- `opencode-setup` downloads GitHub `opencode-linux-*.tar.gz` (avoids bun
  SIGSYS + postinstall stub; no `opencode-android-*`), then `glibc` +
  ld-linux wrapper (`LD_PRELOAD=` so the path redirector does not reinject)
- Drawer quick bar: fewer/taller defaults; **⋯** overflow; **Customize bar…**
  picks which actions appear; long-press **AI** stops OpenCode

### 12. Widget ~/.shortcuts templates (done — Phase 1)

- App seeds `clipboard-speak`, `clipboard-to-file`, `git-pull-repos`,
  `screen-ocr`, and `tasks/td-ai` under `~/.shortcuts` on first launch
- Settings + right-drawer picker: install / reset / status (Widget APK, API APK,
  `pkg termux-api`); **Run once** from the drawer

### 12b. Screenshot OCR + capture API (done)

- `$PREFIX/bin/td-screen-ocr` + widget `screen-ocr`: OCR → clipboard via
  `tesseract` + `termux-clipboard-set` (toasts hint missing eng/other lang packs)
- Prefers `$PREFIX/bin/termux-screenshot` (Termux:API `Screenshot` /
  MediaProjection consent → `~/repos/screen-ocr/latest.png`); falls back to
  newest file under Screenshots (`termux-setup-storage`)
- Upstream-ready CLI: `contrib/termux-api-package/scripts/termux-screenshot.in`
  (needs matching Java API before merge to termux-api-package)
- Termux:X11 is out of scope (does not capture the Android display)

### 12c. KeepAlive FQCN + Tools gesture + more API widgets (done)

- Seeded `termux-api-start` / `termux-api-stop` with KeepAlive FQCN; redirector
  rewrites short/broken KeepAlive components
- Two-finger swipe down / Ctrl+Alt+T opens Tools (end) drawer
- Optional widget catalog: camera-photo, wifi-info, battery-status,
  torch-toggle, share-clipboard, open-settings, vibrate, volume-info,
  location, telephony-info, stop-ai

### 12d. AI polish + release 0.202.0 (done)

- Drawer AI when already ready: snackbar **Copy LAN** if `:4096` is wildcard-bound
- `AiSessionHelper` ignores leftover bun JS `opencode` stubs; also finds
  `$PREFIX/libexec/opencode`
- `tasks/td-ai` exits early when `/global/health` is already healthy
- Optional `stop-ai` widget companion

### 12e. OpenJDK 17 helper (done — on-demand, not APK-bundled)

- `td-jdk-setup` — `pkg install` OpenJDK 17 with explicit Depends + Recommends
  (`libandroid-shmem`, `openjdk-17-x`, …) and `dpkg --configure -a` recovery
- `jdk-doctor` — PATH, JVM tree, pkg status
- Docs: [openjdk.md](docs/openjdk.md) — why embedding ~100 MiB/arch in the APK
  is deferred; mirrors `opencode-setup` on-demand model

### 13. InVx Terminal Widget module (done)

- `:termux-terminal-widget` — gardockt Termux Terminal Widget adapted for
  InVxTermux (`com.involvex.termux_app.terminalwidget`, local `termux-shared`,
  `RUN_COMMAND` for command-output home widgets)

### 14. Classic Termux:Widget module (done)

- `:termux-widget` — [involvex/termux-widget](https://github.com/involvex/termux-widget)
  adapted as `com.involvex.termux_app.widget` / `com.invapp.widget` with
  `sharedUserId` + same test key (one-tap `~/.shortcuts` scripts)

## Non-goals (for now)

- Full in-app IDE / multi-tab browser
- Reverse tunnels / public URLs by default
- Shipping every npm CLI as an app-managed wrapper

## Day-to-day usage

```bash
cd ~/repos
td-scaffold myapp react   # or: drawer → New… → pwa / pwa-react
td-dev                    # vite on :5173
# Drawer → Preview → Scan → 5173
```

```bash
# AI web UI in Preview
td-ai          # or: drawer → AI (skips start if :4096 already up)
# Preview opens on :4096
# drawer → Stop AI   # when done
```

```bash
# Clone from PC remote
td-clone https://github.com/org/app.git   # or: drawer → Clone…
# optional: --bun-i
td-dev
```

On desktop: same remote, normal git + bun. Pull on the phone to continue.
