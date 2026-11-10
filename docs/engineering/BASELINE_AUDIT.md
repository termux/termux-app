# BASELINE AUDIT — Termux Workstation Forge

**Repository:** `00AstroGit00/termux-app` (fork of `termux/termux-app`)
**Branch:** `feature/termux-workstation`
**Audit date:** 2026-11-09
**Auditor:** vibe-lead (Principal AI Systems Architect)

---

## 1. Repository Identity

| Field | Value |
|-------|-------|
| Working directory | `/data/data/com.termux/files/home/PROJECTS/GITHUB/termux-app` |
| Origin remote | `https://github.com/00AstroGit00/termux-app.git` |
| Upstream remote | `https://github.com/termux/termux-app.git` |
| Current branch | `feature/termux-workstation` (created from `master`) |
| Latest commit | `8629e632` Changed: Update `dependency-submission` to `v6` |
| Working tree | Clean (no uncommitted modifications) |
| Tags | v0.70 … v0.99 (latest v0.99) |
| Application ID | `com.termux` |
| Application name | `Termux` |
| Package variant | `apt-android-7` |
| Version code | 118 |
| Version name | 0.118.0 |

---

## 2. Gradle Modules

| Module | Path | Purpose |
|--------|------|---------|
| `:app` | `app/` | Main application, activities, settings, file provider, bootstrap |
| `:termux-shared` | `termux-shared/` | Shared activities, native local-socket, report activity |
| `:terminal-emulator` | `terminal-emulator/` | VT100/VT220 terminal engine, ANSI escape sequences, JNI bridge, Sixel/iTerm image |
| `:terminal-view` | `terminal-view/` | Terminal rendering, gesture/scale, text selection, UI view |

---

## 3. Build Configuration (baseline `gradle.properties`)

| Property | Value |
|----------|-------|
| `minSdkVersion` | 21 |
| `targetSdkVersion` | 28 |
| `compileSdkVersion` | 36 |
| `ndkVersion` | 29.0.14206865 |
| `markwonVersion` | 4.6.2 |
| `org.gradle.jvmargs` | `-Xmx2048M` |
| `android.useAndroidX` | true |

**AGP:** 8.13.2 (in `build.gradle` buildscript)
**Gradle Wrapper:** 9.2.1
**JDK:** 17 specified by AGP 8.13.2 requirement; only JDK 21 functional on this host (JDK 17 incomplete — `lib/` only, no `bin/java`)

---

## 4. Android Manifest (app/src/main/AndroidManifest.xml)

**Key declarations:**
- `android:sharedUserId="${TERMUX_PACKAGE_NAME}"` — shared user ID `com.termux`
- `android:installLocation="internalOnly"`
- Permission `${TERMUX_PACKAGE_NAME}.permission.RUN_COMMAND` (dangerous)
- Permissions: INTERNET, ACCESS_NETWORK_STATE, READ/WRITE_EXTERNAL_STORAGE, MANAGE_EXTERNAL_STORAGE, WAKE_LOCK, VIBRATE, FOREGROUND_SERVICE, REQUEST_IGNORE_BATTERY_OPTIMIZATIONS, SYSTEM_ALERT_WINDOW, READ_LOGS, DUMP, WRITE_SECURE_SETTINGS, REQUEST_INSTALL_PACKAGES, RECEIVE_BOOT_COMPLETED, PACKAGE_USAGE_STATS, SET_ALARM
- Exported components: `TermuxActivity` (MAIN/LAUNCHER), `HomeActivity` alias, `SettingsActivity`, `FileShareReceiverActivity`, `FileViewReceiverActivity`, `TermuxDocumentsProvider` (authority `${TERMUX_PACKAGE_NAME}.documents`), `TermuxOpenReceiver$ContentProvider` (authority `${TERMUX_PACKAGE_NAME}.files`, permission RUN_COMMAND), `RunCommandService` (exported, permission RUN_COMMAND)
- Theme: `Theme.TermuxApp.DayNight.DarkActionBar`

---

## 5. Native Code (C/C++)

| Module | Android.mk | Purpose |
|--------|-----------|---------|
| `:app` | `src/main/cpp/Android.mk` | Bootstrap utilities |
| `:terminal-emulator` | `src/main/jni/Android.mk` | Terminal emulation core (`termux.c`), JNI bridge |
| `:termux-shared` | `src/main/cpp/Android.mk` | Local socket utilities |

**Compiler flags:** `-std=c11 -Wall -Wextra -Werror -Os -fno-stack-protector -Wl,--gc-sections`
**ABIs:** x86, x86_64, armeabi-v7a, arm64-v8a
**Packaging:** `useLegacyPackaging true` (uncompressed JNI libs)

---

## 6. Terminal Emulator

**Source:** `terminal-emulator/src/main/java/com/termux/terminal/`
- `TerminalEmulator.java` — core VT100/VT220 emulation
- `TerminalBuffer.java`, `TerminalRow.java` — screen buffer
- `TerminalColors.java`, `TerminalColorScheme.java` — ANSI color handling
- `KeyHandler.java` — keyboard input
- `TerminalSession.java` — session management
- `ITermImage.java`, `TerminalSixel.java`, `TerminalBitmap.java` — Sixel and iTerm image protocol support
- `WcWidth.java` — Unicode width calculation
- `JNI.java` — native bridge

**Tests:** 18 test classes covering APC, control sequences, cursor, history, key handling, OSC, resize, scroll, Unicode, etc.

---

## 7. Existing Capabilities

- Professional terminal emulation (VT100/VT220, ANSI escape sequences)
- Sixel and iTerm image protocol support
- Multiple terminal sessions with session list
- Termux:API, Termux:Boot, Termux:Styling, Termux:Tasker, Termux:Widget plugin integration points
- File provider and document provider for file sharing
- RunCommandService for remote command execution
- Settings fragments (terminal I/O, view, debugging, API, float, tasker, widget)
- Bootstrap download and verification (SHA-256) from termux-packages releases
- Native C core for terminal emulation
- Dark/light theme support (DayNight)

---

## 8. Architectural Limitations

- **targetSdkVersion 28** — does not target modern Android (scoped storage, background restrictions, permission model changes)
- **sharedUserId `com.termux`** — prevents independent distribution; requires signature match with other Termux packages
- **MANAGE_EXTERNAL_STORAGE** — broad storage permission, deprecated on modern Android
- **READ_LOGS, DUMP, WRITE_SECURE_SETTINGS** — privileged permissions requiring special handling
- **No Material 3** — uses Material Design 1 (`com.google.android.material:material:1.12.0`)
- **No split-pane terminal** — single session per view
- **No AI integration** — no coding assistant detection or launching
- **No device diagnostics** — no CPU/memory/storage/battery/thermal info UI
- **No toolchain detection** — no Java/SDK/NDK/Gradle/Git/Node/Python/Rust/Go detection
- **NDK host binaries are x86_64 only** — cannot compile natively on aarch64 Termux host without aarch64 NDK replacement

---

## 9. Known Bugs / Technical Debt

- JDK 17 specified but incomplete on this host; JDK 21 used as substitute
- NDK r29 host tools are x86_64 ELF; non-executable on aarch64 Termux (resolved by replacing with aarch64 NDK from `lzhiyong/termux-ndk`)
- Build-tools 35.0.0 aapt2 is x86_64; may need aarch64 replacement for full native build on this host
- `android.enableR8` deprecated flag removed in AGP 8.0+ (not present in gradle.properties, good)
- SDK package ID inconsistencies reported by sdkmanager (build-tools/ndk path mismatches) — cosmetic, does not block build

---

## 10. Security Concerns

- `sharedUserId` allows inter-process communication with other `com.termux` packages
- `RUN_COMMAND` permission is `dangerous` level
- `MANAGE_EXTERNAL_STORAGE` grants broad file system access
- `SYSTEM_ALERT_WINDOW` allows overlay drawing
- `READ_LOGS` / `DUMP` allow log and system info access
- `WRITE_SECURE_SETTINGS` allows modification of system settings
- Exported `RunCommandService` and `FileShareReceiverActivity`/`FileViewReceiverActivity`
- `allowBackup="false"` (good)
- Debug signing uses `testkey_untrusted.jks` (included in repo — not for production)

---

## 11. Dependencies on External Termux Projects

- **termux-packages** — bootstrap filesystem (`apt-android-7` variant), downloaded from `github.com/termux/termux-packages/releases`
- **Termux:API** — optional plugin for device APIs
- **Termux:Boot** — optional plugin for boot execution
- **Termux:Styling** — optional plugin for terminal themes
- **Termux:Tasker** — optional plugin for Tasker integration
- **Termux:Widget** — optional plugin for home screen widgets
- **Termux:Float** — optional plugin for floating terminal

---

## 12. Build Compatibility Summary

| Component | Status |
|-----------|--------|
| Gradle 9.2.1 | Functional |
| AGP 8.13.2 | Functional |
| JDK 21 (substitute for 17) | Functional |
| compileSdk 36 | Platform installed |
| build-tools 35.0.0 | Installed (aapt2 x86_64 — may need replacement on aarch64 host) |
| NDK 29.0.14206865 | Installed (replaced with aarch64 host build from `lzhiyong/termux-ndk`) |
| Platform android-36 | Installed |
| Platform android-37 | Installed |
| SDK licenses | Accepted |
| Baseline `assembleDebug` | **BUILD SUCCESSFUL** (1m 52s) |
| ARM64 APK | `termux-app_apt-android-7-debug_arm64-v8a.apk` (38.5 MB) |
| APK SHA-256 | `75cecdda861e44adfb9c35e55454286c94cd25d54eccc2d58c900bc483af6104` |

---

## 13. Feature Opportunities (Phases Two–Nine)

1. Material 3 UI redesign with dark/light themes
2. Workspace and project manager
3. AI coding assistant integration (KiloCode, OpenCode, Gemini CLI, Codex CLI)
4. Split-pane terminal support
5. Device diagnostics (CPU, memory, storage, battery, thermal, network)
6. ADB / wireless debugging / Shizuku integration
7. Toolchain detection (Java, SDK, NDK, Gradle, Git, Node, Python, Rust, Go)
8. Enhanced terminal: session naming, colors, keyboard shortcuts, URL detection, command snippets
9. Security hardening (permission reduction, exported component review)
10. CI/CD workflow improvements
