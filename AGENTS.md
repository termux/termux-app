# AGENTS.md — InVxTermux (`com.involvex.termux_app` fork)

> Agent operating guide for this repo. Source of truth for versions/commands is
> `gradle.properties`, `app/build.gradle`, `settings.gradle`, `README.md`,
> `ROADMAP.md`. No `CLAUDE.md` / `GEMINI.md` / `.github/copilot-instructions.md`
> exist — this file consolidates `README.md`, `ROADMAP.md`, `SECURITY.md`,
> `app/build.gradle`, and GitHub workflows.

## 1. Project Overview

**InVxTermux** is an unofficial fork of [Termux](https://termux.dev): an Android
terminal app + Linux environment. This repo (`involvex/termux-app`) holds the
**app UI + terminal emulation**. Packages installable *inside* the app live in
[`termux-packages`](https://github.com/termux/termux-packages) — do not add
package logic here. Docs: https://involvex.github.io/termux-app/

### Modules (`settings.gradle`)

| Module | Purpose | Namespace |
|--------|---------|-----------|
| `app` | Core app, bootstrap + Bun install, activities, drawer, Preview | `com.invapp` / appId `com.involvex.termux_app` |
| `termux-shared` | Shared constants/utils for app + plugins. **All shared code goes here** | `com.invapp.shared` |
| `terminal-view` | Terminal `View` widget | — |
| `terminal-emulator` | Emulator core + JNI (`src/main/jni/Android.mk`) | `com.invapp.emulator` |
| `termux-api` | Termux:API plugin integration | — |
| `termux-widget` | Classic `~/.shortcuts` Termux:Widget | `com.involvex.termux_app.widget` |
| `termux-terminal-widget` | Home-screen command-output widget (gardockt fork) | `com.involvex.termux_app.terminalwidget` |

Key shared entry points:

- `termux-shared/.../com/termux/shared/termux/TermuxConstants.java` — package
  names, `$PREFIX`, forking guide (read its javadoc before renaming anything).
- `app/src/main/cpp/` — `termux-bootstrap.c` + `bun-bootstrap.c` with
  `.incbin` zips, built via `app/src/main/cpp/Android.mk` (`libinvapp-bun`,
  redirector `libinvapp-redirector.so`).
- `app/build.gradle` — version, variants, bootstrap/Bun download tasks.

### This fork — InVxTermux / Terminal Dev (`com.involvex.termux_app`)

Display name **InVxTermux**. Stay in terminal, develop on PC + phone against
same git remote, preview localhost servers on-device, optionally attach AI CLI.
See `ROADMAP.md`.

| Piece | Path / behavior |
|-------|-----------------|
| Package id | `com.involvex.termux_app` |
| Display name | `InVxTermux` |
| Path redirector | `LD_PRELOAD=$PREFIX/lib/libinvapp-redirector.so` maps hardcoded `/data/data/com.termux` → this prefix (apt/SSH/node); also rewrites `#!/usr/bin/env` shebangs (npm/npx) |
| Bun real binary | `$PREFIX/libexec/bun` — official `bun-linux-*-android.zip`, currently **1.4.2** |
| Bun wrapper | `$PREFIX/bin/bun` → drops path redirector, preloads `libinvapp-bun-seccomp.so` (SIGSYS→ENOSYS for `openat2`/`fchmodat2`), sets OPENSSL + `--os=android --cpu=…` on install/add/create |
| bunx | `$PREFIX/bin/bunx` → `bun x --bun` (avoids silent fail when `node` missing) |
| node shim | `$PREFIX/bin/node` → bun if `nodejs` package not installed |
| Default cwd | `~/repos` (exec-capable). `~/storage/shared` is browse/sync only (**noexec**) |
| Preview | Drawer **Preview** → Scan + chips; **Copy LAN** / long-press chip → `http://<wifi-ip>:<port>` when bound on `0.0.0.0` |
| AI helper | `opencode-setup` / `td-ai [port]` → OpenCode web on `:4096` (binds **`0.0.0.0`** by default for LAN; Preview still uses `127.0.0.1`). Setup downloads official `opencode-linux-*.tar.gz` from GitHub (no `bun install` / no postinstall), then `glibc` + ld-linux wrapper with **`LD_PRELOAD=` empty** and DNS shim via **`ld-linux --preload $PREFIX/lib/libinvapp-opencode-shim.so`** (arm64 shipped in APK assets). Optional `OPENCODE_VERSION=v1.18.31`; bind override `OPENCODE_HOST=127.0.0.1`. Drawer **AI** probes missing/installed/ready; long-press / **Stop AI** kills `:4096`. On-demand into `$PREFIX`, not baked into APK |
| Dev server | `td-dev [script]` → `bun run` with Preview/LAN hints |
| Scaffold | `td-scaffold [name] [template]` → Vite under `~/repos` (host `0.0.0.0`); `pwa` / `pwa-react` add `vite-plugin-pwa` |
| Clone | `td-clone <url> [name] [--bun-i]` → git clone into `~/repos`; drawer **Clone…** |
| File share → Edit | Seeds `~/bin/termux-file-editor` (nvim→vim→nano→less) for FileReceiver |
| Widget scripts | `~/.shortcuts` templates (defaults: `clipboard-speak`, `clipboard-to-file`, `git-pull-repos`, `screen-ocr`, `tasks/td-ai`; optional catalog: camera/wifi/battery/torch/share/settings/vibrate/volume/location/telephony/`stop-ai`); Settings / right drawer; matching Widget APK + API/`pkg install termux-api`. `screen-ocr` → `td-screen-ocr`. **2-finger swipe down** (or Ctrl+Alt+T) opens Tools end drawer. Widget tap always shows **Running: name** toast |
| Workflow | Drawer quick bar (customizable via **⋯**): defaults pull / bun i / bun run dev / Repos / Clone… / New… / AI; overflow has Run… / Stop AI / Customize bar / Customize keys (writes quoted `extra-keys` in `~/.termux/termux.properties`). Extra-keys: 2-row nav page + swipe L/R for workflow page; end drawer = Tools (Preview/AI/Clone/New/Widget scripts). Long-press / Ctrl+Alt+M → actions bottom sheet. AI tap when ready opens Preview and may offer **Copy LAN** if `:4096` is wildcard-bound |
| Completions | App installs `$PREFIX/etc/profile.d/invapp-completions.sh` + `bash_completion.d` for bun/pkg/npm/gh/git. Prefer `pkg install bash-completion` for richer git. **New session** after update. Never edit `~/.bashrc` |

**Do NOT "fix" these with more wrappers:**

- `Unknown signal 31 (SIGSYS)` — usually Android seccomp trapping Bun's
  `openat2`/`fchmodat2` during install (fixed via `libinvapp-bun-seccomp.so`),
  or a glibc/Linux Bun / `LD_PRELOAD`+path-redirector combo. Use bundled
  Android Bun; never `curl … bun.sh/install | bash`. If a Windows lockfile
  pinned `linux-*` natives, delete `node_modules` + lockfile and reinstall
  on the phone (`bun i` injects `--os=android`).
- `/usr/bin/env: bad interpreter` on `npm`/`npx` — redirector rewrites those
  shebangs; open a **new** session after update. Install Node with
  `pkg install nodejs` if `npm` is missing.
- Green-bar `ls` folders — hacker theme v1 ANSI blue→green clash. Fixed in v2;
  new session or replace `~/.termux/colors.properties`.
- `Permission denied` on `tsc`/bins — project under `/storage/emulated/0`
  (noexec). Keep runnable projects in `~/repos`.
- OpenSSL/node looking at `com.termux` — shell env + redirector cover it;
  don't hardcode new paths.
- `opencode-setup` / glibc `Permission denied` / `version LIBC not found` /
  postinstall / SIGSYS on setup — OpenCode is a linux-glibc binary. Setup
  **downloads the GitHub tarball** (never `bun install -g opencode-ai` or
  `curl … opencode.ai/install`). It installs `glibc` and runs under
  `ld-linux` with **`LD_PRELOAD=`** (empty). Do not `unset LD_PRELOAD` (the
  path redirector reinjects when the key is absent). Do **not** put the DNS
  shim in `LD_PRELOAD` either — termux-exec will append
  `libinvapp-redirector.so` and you get `version \`LIBC' not found`; use
  `ld-linux --preload …/libinvapp-opencode-shim.so` instead. Never
  `PATH=$PREFIX/glibc/bin:$PATH` (breaks the shell). Never use stock `grun`
  alone on this package id without path fixups.
- `opencode` prints “postinstall script was not run” — leftover bun JS stub.
  Re-run `opencode-setup` (removes the stub) or `rm -f $PREFIX/bin/opencode`
  then setup again.
- OpenCode “Cannot connect to API” / `Failed to fetch models.dev` /
  `models.opencode.ai` — if `curl` works but OpenCode fails, glibc DNS is
  broken: stock glibc hardcodes `/data/data/com.termux/.../glibc/etc/*`.
  Ensure `$PREFIX/lib/libinvapp-opencode-shim.so` exists (APK ships arm64),
  wrapper uses `--preload`, seed resolv/CA under `$PREFIX/glibc/etc`, export
  `SSL_CERT_FILE` / `NODE_EXTRA_CA_CERTS`. Ready check is
  `GET http://127.0.0.1:4096/global/health` (not `:5000/api`).
- `getifaddrs returned an error` — non-fatal for web bind on `0.0.0.0`;
  never use `--mdns`. Override with `OPENCODE_HOST=127.0.0.1 td-ai` if needed.
  Firewall/VPN may block LAN clients; Preview always uses loopback.
- `screen-ocr` / screenshot OCR failing — needs Termux:API APK +
  `pkg install termux-api tesseract`, and usually `termux-setup-storage` for
  `--latest`. Eng lang pack ships with `tesseract`; other langs need
  `<lang>.traineddata` under `$PREFIX/share/tessdata` (toast hints this).
  Capture uses MediaProjection consent (not key injection, not Termux:X11).
  Optional OEM help: `adb shell appops set com.involvex.termux_app.api PROJECT_MEDIA allow`
  (and/or the main app id). If capture is denied, use `td-screen-ocr --latest`
  after a system screenshot. X11 does not capture the Android screen.
- `termux-api-start` / `KeepAliveService` “Not found” — stock short component
  `…api/.KeepAliveService` is wrong when applicationId ≠ Java package. Use
  seeded `$PREFIX/bin/termux-api-start` (FQCN) or reopen the app so helpers
  refresh; redirector also rewrites KeepAlive short forms.
- `gh` / wget `api.github.com/search/repositories?per_page=100` → **422** —
  GitHub Search requires a `q=` query. Not caused by InVxTermux seeding.
  Check `gh extension list`; fix or remove the extension/script that calls
  search without `q`.
- `openjdk-17` stuck / `java` dlopen / **Bad system call** — use Termux
  `openjdk-17` only (`td-jdk-setup` pulls `libandroid-shmem` + recommends).
  Not bundled in the APK (~96 MiB deb/arch). `jdk-doctor` + `docs/openjdk.md`.

## 2. Useful Commands

### Git / GitHub CLI

```bash
git status
git log --oneline -10
git checkout -b feature/description
git commit -m "Fixed(terminal): Fix cursor positioning bug"
git push origin feature/description
gh pr create --title "Description" --body "Details"
git --no-pager diff
```

### Gradle build / test / lint (Windows: use `gradlew.bat`)

```bash
./gradlew assembleDebug
./gradlew assembleRelease
./gradlew :app:assembleDebug            # after Bun/bootstrap changes
./gradlew test                          # unit tests (JUnit + Robolectric)
./gradlew lint
./gradlew clean
./gradlew versionName                   # prints e.g. 0.200.0
./gradlew --refresh-dependencies
./gradlew assembleDebug -DTERMUX_PACKAGE_VARIANT=apt-android-7
```

Env vars honored by `app/build.gradle`:

| Var | Default | Effect |
|-----|---------|--------|
| `TERMUX_PACKAGE_VARIANT` | `apt-android-7` | `apt-android-7` (Android 7+) or `apt-android-5` (Android 5/6, deprecated) |
| `TERMUX_APP_VERSION_NAME` | `0.200.0` | Override `versionName` (must stay semver) |
| `TERMUX_APK_VERSION_TAG` | — | APK filename tag |
| `TERMUX_SPLIT_APKS_FOR_DEBUG_BUILDS` | `1` | Per-ABI APKs for debug |
| `TERMUX_SPLIT_APKS_FOR_RELEASE_BUILDS` | `0` | F-Droid needs single APK (#1904) |
| `JITPACK_NDK_VERSION` | `29.0.14206865` | Optional NDK override (legacy env name) |

Bootstrap/Bun tasks (run automatically via `preBuild`/`JavaCompile`/native
build deps; run manually to prefetch):

```bash
./gradlew :app:downloadBootstraps
./gradlew :app:downloadBunBootstraps
```

APK naming: `termux-app_<variant-or-tag>_<abi>.apk`
(`universal`, `arm64-v8a`, `armeabi-v7a`, `x86_64`, `x86`).

### Debugging / device

```bash
adb devices
adb logcat
adb install -r termux-app_apt-android-7-debug_arm64-v8a.apk
# Inside Termux app:
logcat                 # realtime (Ctrl+C to stop)
logcat -d > logcat.txt # dump
```

In-app: Settings → `<APP_NAME>` → Debugging → Log Level (`Off`/`Normal`/
`Debug`/`Verbose`). Set for **both** Termux and the plugin (plugins send
intents; Termux executes). Revert to `Normal` after — `Verbose` may leak
private data + slows execution. `More` → `Report Issue` → `YES` auto-attaches
`stat` + logcat dump.

### Phone day-to-day (new session starts in `~/repos`)

```bash
cd ~/repos
git clone <url> myapp && cd myapp
bun install
bun run dev          # or bun run android / build
# Drawer → Preview → Scan → tap port (e.g. 3000)
td-ai                # OpenCode on 0.0.0.0:4096 → Preview 127.0.0.1 / LAN Copy
```

PC: same remote, normal git + bun. Pull on phone to continue. No Termux
package sync needed.

## 3. Technologies

Pin versions from `gradle.properties` + module `build.gradle` — do not bump
without testing bootstrap + all ABIs.

| Layer | Version / tool |
|-------|----------------|
| `minSdkVersion` | `21` (Android 5; 5–6 deprecated, no package updates) |
| `targetSdkVersion` | `28` |
| `compileSdkVersion` | `36` |
| AGP | `9.4.0` (`com.android.tools.build:gradle`, root `build.gradle`) |
| NDK | `29.0.14206865`, `ndk-build` (`Android.mk` per module) |
| JDK (build) | `17` (Android Studio Flamingo+) |
| Java compat | `1.8` + `coreLibraryDesugaring` (`desugar_jdk_libs:1.1.5`) |
| App version | `versionCode 202`, `versionName 0.202.0` (semver-enforced at build) |
| Bootstrap | `2026.02.12-r1+apt.android-7` (aarch64/arm/i686/x86_64, SHA-256 verified); android-5: `2022.04.28-r6` |
| Bun | `1.4.2` official `bun-linux-{aarch64,x64}-android.zip` (SHA-256 verified, `.incbin` into `libinvapp-bun`, extracted by `TermuxBunInstaller`) |
| Editor | 4-space, LF, UTF-8, final newline (`.editorconfig`); 2-space for `*.yaml` |

### Dependencies (current)

**`app`:** `annotation:1.9.0`, `core:1.13.1`, `drawerlayout:1.2.0`,
`preference:1.2.1`, `viewpager:1.0.0`, `material:1.12.0`,
`guava:24.1-jre` (+ `listenablefuture:9999.0-empty…`), Markwon
`4.6.2` (`core`, `ext-strikethrough`, `linkify`, `recycler`).

**`termux-shared`:** `appcompat:1.6.1`, above core/material/guava/markwon, plus
`hiddenapibypass:6.1` (Android 10+ hidden API), `window:1.1.0`,
`commons-io:2.5` (**do not exceed 2.5** — `java.nio.file.Path` missing on
Android < 8), `termux-am-library:v2.0.0`, `terminal-view`.

**`terminal-emulator`:** `annotation:1.9.0` only + JNI, `abiFilters` all four.

**Tests:** `junit:4.13.2`, `robolectric:4.10` (`app`), `androidx.test.ext:junit:1.1.5`
(`termux-shared`); `terminal-emulator` sets `unitTests.returnDefaultValues=true`.

Native flags: `-std=c11 -Wall -Wextra -Werror -Os -fno-stack-protector
-Wl,--gc-sections`; `jniLibs.useLegacyPackaging true`; release uses
`proguard-android-optimize.txt` + `proguard-rules.pro` (`minifyEnabled true`,
`shrinkResources false` for reproducible builds); `lint { disable
'ProtectedPermissions' }`.

### CI/CD (`.github/workflows/`)

- `debug_build.yml` — per-commit debug APKs (Artifacts; login required).
- `run_tests.yml` — unit tests on PRs.
- `attach_debug_apks_to_release.yml` — attaches APKs to GitHub Releases.
- `docs.yml` — MkDocs → GitHub Pages (`https://involvex.github.io/termux-app/`).
- `gradle-wrapper-validation.yml`, `dependency-submission.yml`.
- Dependabot (`dependabot.yml`) for dependency bumps.
- **No JitPack publish** for this fork (libraries are not published).

### Project layout

```text
termux-app/
├── app/  # applicationId com.involvex.termux_app, Bootstrap/Bun, UI
│   └── src/main/{java/com/invapp,cpp/{Android.mk,*.c,*.S,bootstrap-*.zip,bun-*.zip}}
├── termux-shared/      # com.invapp.shared — constants/utils, no hardcodes elsewhere
├── terminal-view/      # View widget
├── terminal-emulator/  # core + src/main/jni/Android.mk
├── termux-api/         # API plugin
├── termux-widget/      # Classic ~/.shortcuts Termux:Widget
├── termux-terminal-widget/ # Command-output home widget (not classic Termux:Widget)
├── .github/workflows/  # CI above + ISSUE_TEMPLATE/
├── fastlane/ docs/en/ site/  # store metadata, docs, sponsors page
└── build.gradle settings.gradle gradle.properties README.md ROADMAP.md AGENTS.md
```

## 4. Best Practices and Guidelines

### Commits — Conventional Commits + Keep a Changelog (enforced)

```text
<Type>[optional scope]: <Description in present tense, capital first letter>

[optional body]

[optional footer(s)]
```

- Allowed `<Type>` exactly (matches changelog headings): `Added`, `Changed`,
  `Deprecated`, `Removed`, `Fixed`, `Security`. Examples: `Added: Add foo`,
  `Added|Fixed: Add foo and fix bar`, `Fixed(terminal): Fix cursor bug`,
  `Changed!: Breaking API change` (`!` = breaking, highlighted in changelog).
- Space after `:` required. `create-conventional-changelog` generates the
  changelog — wrong types break it.
- One logical change per commit; PRs squash-merge with a clean message.

### Versioning — SemVer 2.0.0 (build-validated)

- Format `major.minor.patch(-prerelease)(+buildmetadata)`, always with patch:
  `0.200.0`, `0.201.0-beta.1`, never `v0.1`. Tag as `v0.200.0`.
- `app/build.gradle:validateVersionName()` fails the build on bad versions.
  - Keep `termux-shared`/`terminal-emulator` versions (`0.202.0`) in
  sync when cutting releases.

### Code style / quality

- `.editorconfig`: LF, UTF-8, 4 spaces (2 for YAML), final newline. Use
  Android Studio formatter; no wildcard imports; Android naming conventions.
- Java 8 compat — no `java.nio.file`/new APIs without desugaring; test on
  `minSdk 21` path.
- `@Nullable`/`@NonNull` (AndroidX) on all public APIs; fail fast with
  `IllegalArgumentException` on bad input.
- No hardcoded paths/package names/APK names outside `termux-shared`.
  Reference `TermuxConstants` (`$PREFIX`, `$HOME`, package, intents). PRs with
  hardcoded `com.termux`/`com.invapp` values **will not be accepted**.
- `termux-shared` additions: put app+plugin-shared code under
  `com.invapp.shared.termux`, general utils outside; honor/update
  `termux-shared/LICENSE.md` + third-party licenses.
- Handle errors gracefully: never swallow; log with correct level, surface
  user-actionable messages, keep `Verbose`-only PII out of `Normal` logs.
- Performance: avoid allocations in terminal draw/input hot paths, reuse
  buffers, prefer `StringBuilder`, offload I/O from UI thread; native code
  must stay `-Werror`-clean.
- Maintainability: small focused classes, no God activities; keep Preview /
  Bun / bootstrap logic isolated so upstream merges stay easy.

### Security (see `README.md` + `SECURITY.md` + https://termux.dev/security)

- GitHub APKs are signed with public test key `app/testkey_untrusted.jks`
  (`alias` / `xrj45yWGLbsO7W0v`, SHA256 `B6:DA:…:E1`). **Anyone can forge
  updates over them** — only install from trusted sources; never use the test
  key to impersonate upstream.
- `sharedUserId com.invapp` + same signature required across app + all plugins
  (API/Boot/Float/Styling/Tasker/Widget). Mixing sources → install failures
  (`INSTALL_FAILED_SHARED_USER_INCOMPATIBLE`). Switching source = uninstall
  all Termux APKs first (offer backup via https://wiki.termux.com/wiki/Backing_up_Termux).
- Review-sensitive: hardcoded paths, native/JNI changes, intent extras +
  `RUN_COMMAND` handling, storage `$PREFIX` traversal, WebView Preview
  (loopback-only, cleartext localhost only — no public tunnels by default).
- Dependencies via Dependabot; check advisories before bumping major versions.
- Report vulnerabilities privately per https://termux.dev/security, not as
  public issues.

### Plugins / build variants

- All plugins share signature; test plugin interop when touching
  `termux-shared`, intents, or permissions.
- **Termux:API** (`com.involvex.termux_app.api`) — `:termux-api` module; users also
  need `pkg install termux-api` for shell CLIs.
- **Termux:Widget** (`com.involvex.termux_app.widget`) — `:termux-widget`
  (classic `~/.shortcuts` one-tap scripts; same `sharedUserId` + test key).
  Stock `com.termux.widget` will not install. Main app seeds templates via
  `WidgetScriptsInstaller` (Settings / right drawer).
- **InVx Terminal Widget** (`com.involvex.termux_app.terminalwidget`) —
  `:termux-terminal-widget` (gardockt fork): command-output home widget via
  `RUN_COMMAND` (not classic Termux:Widget).
- Variants: `apt-android-7` (primary) vs `apt-android-5` (deprecated). Never
  mix bootstrap zips across variants — app crashes at startup
  (`TermuxBootstrap.PackageVariant`).

### Testing

- New functionality requires unit tests under `src/test/java/`.
- Run `./gradlew test` + `./gradlew lint` before pushing; fix all warnings
  from changed files. CI (`run_tests.yml`) gates PRs.
- Prefer Robolectric for Android-dependent logic; keep emulator/parser tests
  hermetic and fast.

### Contribution workflow

1. Branch from `master`: `feature/…` / `fix/…`.
2. Implement + tests; `./gradlew test lint`; format.
3. Conventional commit; push; `gh pr create`.
4. Fill template, link issues, attach `logcat.txt` for runtime bugs.
5. Address review; keep PR focused (app vs `termux-packages` repo!).

## 5. Development Workflow

### Setup

1. Android Studio Flamingo+, JDK 17, SDK API 24+ (compile 36), NDK
   `29.0.14206865`.
2. `git clone <this-fork> && cd termux-app`.
3. Open in Studio, let Gradle sync (downloads bootstrap + Bun zips with
   checksum verification).
4. Verify: `./gradlew :app:assembleDebug` + `./gradlew test`.

### Release (maintainers)

1. Bump `versionName` (semver) + `versionCode`; sync library publish versions.
2. Tag `vX.Y.Z`; CI builds APKs; `attach_debug_apks_to_release.yml` attaches
   universal + per-ABI APKs.
3. Update changelog (Keep a Changelog) + `README.md` latest-version line.
4. Play Store is a separate repo (`termux-play-store`) — report its issues
   there, not here.

## 6. Troubleshooting

| Symptom | Fix |
|---------|-----|
| Wrong-version build fail | `versionName` must be full semver in every `build.gradle` |
| Stale deps | `./gradlew clean` + `./gradlew --refresh-dependencies` |
| NDK fail | Match `ndkVersion` in `gradle.properties`; check `Android.mk` paths |
| Bootstrap missing/checksum | Let `downloadBootstraps` fetch from `termux-packages` releases; don't hand-place `$PREFIX` |
| Bun signal 31 / not found | Use bundled `$PREFIX/libexec/bun`; `downloadBunBootstraps`; never `bun.sh/install` on device |
| `Permission denied` bins | Move project to `~/repos` (shared storage is noexec) |
| Phantom kill `[signal 9]` (Android 12+) | OS phantom/excessive-CPU killer — see issue #2366 / AG docs; disable trimming or upgrade to 12L/13 |
| Plugin install fail | Same-source APKs only (sharedUserId+signature); uninstall all, reinstall set |

## 7. Code Review Checklist

- [ ] Conventional commit type/scope correct, present tense, `!` if breaking
- [ ] Tests added/updated; `./gradlew test lint` clean
- [ ] No hardcoded paths/package names (uses `termux-shared`/`TermuxConstants`)
- [ ] Nullability annotations; errors handled, no PII in Normal logs
- [ ] No security holes (intents, traversal, WebView scope, test-key misuse)
- [ ] Perf: no hot-path allocations, I/O off UI thread, native `-Werror` clean
- [ ] Backward compat kept or breaking change explicit + docs updated
- [ ] Correct repo (app vs packages vs play-store); right issue template
- [ ] Semver version/tag consistent if release-related

## 8. Useful Links

- Upstream: https://github.com/termux/termux-app · Wiki: https://wiki.termux.com/wiki/
- This fork: https://github.com/involvex/termux-app · Docs: https://involvex.github.io/termux-app/
- Packages: https://github.com/termux/termux-packages · File layout: https://github.com/termux/termux-packages/wiki/Termux-file-system-layout
- `RUN_COMMAND` intent: https://github.com/termux/termux-app/wiki/RUN_COMMAND-Intent · Libraries: https://github.com/termux/termux-app/wiki/Termux-Libraries
- Community: https://reddit.com/r/termux · Matrix: `#termux_termux:gitter.im`, `#termux_dev:gitter.im` · https://twitter.com/termuxdevs · support@termux.dev
- Security (this fork): [SECURITY.md](SECURITY.md) · Upstream: https://termux.dev/security
- Terminal refs: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html · https://vt100.net/
- License: `LICENSE.md` (GPLv3 + exceptions) · `termux-shared/LICENSE.md`
