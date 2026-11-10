# ARCHITECTURE — Termux Workstation Forge

**Branch:** `feature/termux-workstation`
**Date:** 2026-11-09
**Status:** Baseline architecture documented; custom extensions planned

---

## 1. High-Level Architecture

The application follows a modular Android architecture with four Gradle modules:

```
:app                          — Application shell, UI, settings, bootstrap
  └── :terminal-view          — Terminal rendering and input view
  └── :terminal-emulator      — VT100/VT220 emulation engine (Java + JNI + C)
  └── :termux-shared          — Shared activities, native local socket
```

### 1.1 Module Responsibilities

| Module | Language | Responsibility |
|--------|----------|----------------|
| `:app` | Java + XML | Activities, fragments, settings, file provider, bootstrap download, signing |
| `:terminal-view` | Java + XML | TerminalView, rendering, gestures, text selection, keyboard toolbar |
| `:terminal-emulator` | Java + C | Terminal emulation core, escape sequences, screen buffer, image protocols |
| `:termux-shared` | Java + C | Shared activities (ReportActivity, TextIOActivity), local socket |

### 1.2 Terminal Engine Architecture

The terminal emulator is the core of the application:

- **TerminalEmulator** — state machine processing input bytes, maintaining screen buffer
- **TerminalBuffer / TerminalRow** — mutable screen grid with line wrapping and scrollback
- **TerminalColors / TerminalColorScheme** — ANSI 16-color + 256-color + truecolor palette
- **KeyHandler** — hardware and soft keyboard input mapping to terminal escape sequences
- **TerminalSession** — process lifecycle (pty fork, shell exec, session client)
- **JNI / termux.c** — native bridge for low-level terminal operations
- **ITermImage / TerminalSixel / TerminalBitmap** — Sixel (DEC) and iTerm (OSC 1337) image rendering

### 1.3 Session Lifecycle

1. User creates a new session (TermuxActivity → TermuxSession)
2. TerminalSession forks a pty and execs the user's shell (`$SHELL`)
3. TerminalView renders output; KeyHandler routes input to the pty
4. Session can be suspended, resumed, renamed, or terminated
5. Multiple sessions coexist; session list view allows switching

---

## 2. Current UI Architecture

- **Theme:** `Theme.TermuxApp.DayNight.DarkActionBar` (AppCompat DayNight)
- **Main activity:** `TermuxActivity` (singleTask, launchMode, resizeable)
- **Settings:** PreferenceFragment-based, XML-defined preference screens
- **Navigation:** Implicit (session switching within terminal; no bottom nav)
- **Views:** activity_termux.xml, item_terminal_sessions_list.xml, extra-keys toolbar
- **No Material 3** — uses Material Components 1.12.0

---

## 3. Planned Architecture Extensions (Termux Workstation Forge)

### 3.1 Navigation Destinations

Seven top-level destinations:
1. **Terminal** — existing terminal session view
2. **Workspaces** — project/workspace directory management
3. **Projects** — recent/favorite projects with git detection
4. **Commands** — command snippets and history
5. **AI Tools** — coding assistant detection and launching
6. **Device** — device diagnostics (CPU, memory, storage, battery, thermal, network)
7. **Settings** — existing settings + new workstation preferences

### 3.2 AI Integration Layer

- **AI Tool Manager** — detects installed CLIs (KiloCode, OpenCode, Gemini CLI, Codex CLI, Qwen CLI)
- **Provider Routing** — supports OmniRoute and OpenAI-compatible endpoints via environment variables
- **No hardcoded keys** — authentication via env vars only; nothing stored in APK
- **Optional** — terminal works fully without AI or network

### 3.3 Device Integration Layer

- Read-only device info via Android APIs (Build, BatteryManager, StorageManager, ConnectivityManager)
- Optional Shizuku/root integration — never assumed, always consent-gated
- ADB status and wireless debugging instructions

### 3.4 Toolchain Detection

- Detects Java, Android SDK, NDK, Gradle, Git, Node.js, npm, pnpm, Python, Rust, Go, Flutter, ADB
- Displays version, executable path, environment config, diagnostic status
- No automatic installation without explicit user approval

---

## 4. Design Principles (Non-Negotiable)

1. Preserve upstream Termux terminal engine and architecture
2. No wholesale rewrite without documented justification
3. No React Native, Flutter, or WebView — native Android UI only
4. Prefer existing libraries and Android APIs
5. Preserve terminal functionality, session management, plugin compatibility
6. No unnecessary dependencies
7. No hardcoded filesystem paths — use TermuxConstants
8. Efficient terminal rendering and process execution
9. Complete features with error handling and tests
10. Working debug build after every milestone
11. Document major architectural decisions

---

## 5. Application Identity Policy

- **Default:** Keep `com.termux` applicationId unchanged
- **sharedUserId** remains `com.termux` — requires signature compatibility with other Termux packages
- **Bootstrap variant** remains `apt-android-7`
- Any identity change requires complete fork strategy, bootstrap rebuild, and plugin compatibility testing
- Never uninstall existing Termux or replace its data automatically

---

## 6. Build Architecture

- **AGP 8.13.2**, Gradle 9.2.1, compileSdk 36, minSdk 21, targetSdk 28
- NDK 29.0.14206865, ABI splits (x86, x86_64, armeabi-v7a, arm64-v8a)
- Bootstrap downloaded and SHA-256 verified from termux-packages releases
- Debug signing with `testkey_untrusted.jks` (in repo, not for production)
- Release signing must use user-owned keystore outside repository

