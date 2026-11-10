# AGENTS.md — Termux Workstation Forge

This file configures AI coding assistants for the Termux Workstation Forge project.
It is portable across Kilo Code and other AGENTS.md-compatible agents.

---

## Project Identity

- **Codename:** Termux Workstation Forge
- **Repository:** `00AstroGit00/termux-app` (fork of `termux/termux-app`)
- **Branch:** `feature/termux-workstation`
- **Application ID:** `com.termux` (do not change without audit and approval)
- **Package variant:** `apt-android-7`
- **Target device:** Samsung Galaxy S23 Ultra (SM-S918B), ARM64, Android 17 / API 37
- **Target ABI:** arm64-v8a

---

## Architecture

Four Gradle modules:

| Module | Path | Responsibility |
|--------|------|----------------|
| `:app` | `app/` | Application shell, activities, settings, file provider, bootstrap |
| `:terminal-view` | `terminal-view/` | Terminal rendering, gestures, text selection, keyboard toolbar |
| `:terminal-emulator` | `terminal-emulator/` | VT100/VT220 emulation engine, ANSI escape sequences, JNI + C core |
| `:termux-shared` | `termux-shared/` | Shared activities, native local socket |

Preserve the upstream Termux terminal engine. No wholesale rewrite without documented justification. No React Native, Flutter, or WebView.

---

## Android Coding Conventions

- Language: Java (existing codebase), Kotlin permitted for new modules
- Build: Gradle 9.2.1, AGP 8.13.2
- compileSdk: 36, minSdk: 21, targetSdk: 28
- NDK: 29.0.14206865
- Package variant: `apt-android-7` (set via `TERMUX_PACKAGE_VARIANT` env var)
- No hardcoded filesystem paths — use `TermuxConstants` and existing shared abstractions
- Follow existing repository licensing (see `LICENSE.md`)
- Commit convention: `Added:`, `Changed:`, `Fixed:`, `Security:`

---

## Gradle Build Procedures

### Baseline debug build (ARM64)

```bash
export JAVA_HOME=/data/data/com.termux/files/usr/lib/jvm/java-21-openjdk
export ANDROID_HOME=/data/data/com.termux/files/usr/opt/android-sdk
export ANDROID_SDK_ROOT=/data/data/com.termux/files/usr/opt/android-sdk
TERMUX_PACKAGE_VARIANT=apt-android-7 TERMUX_SPLIT_APKS_FOR_DEBUG_BUILDS=1 ./gradlew :app:assembleDebug --no-daemon
```

### Test matrix

```bash
./gradlew testDebugUnitTest
./gradlew lintDebug
./gradlew :app:assembleDebug
```

### Environment notes (this host)

- JDK 17 specified by AGP but incomplete on this host; JDK 21 used as substitute
- NDK 29 host binaries replaced with aarch64 build from `lzhiyong/termux-ndk` (x86_64 NDK cannot execute on aarch64 Termux)
- Build output: `app/build/outputs/apk/debug/termux-app_apt-android-7-debug_arm64-v8a.apk`
- APK SHA-256: `75cecdda861e44adfb9c35e55454286c94cd25d54eccc2d58c900bc483af6104`

---

## Terminal Emulator Architecture

- Core: `TerminalEmulator.java` (VT100/VT220 state machine)
- Screen buffer: `TerminalBuffer.java`, `TerminalRow.java`
- Colors: `TerminalColors.java`, `TerminalColorScheme.java`
- Input: `KeyHandler.java`
- Session: `TerminalSession.java` (pty fork, shell exec)
- Native bridge: `JNI.java` + `termux.c`
- Image protocols: `ITermImage.java`, `TerminalSixel.java`, `TerminalBitmap.java` (Sixel + iTerm)
- Tests: 18 test classes in `terminal-emulator/src/test/`

Never introduce regressions in standard shell applications, SSH, editors, or TUI programs.

---

## Security Expectations

- Minimize permissions; do not add privileged permissions without documented need
- `sharedUserId=com.termux` — requires signature compatibility with other Termux packages
- `RUN_COMMAND` permission is dangerous; review exported components
- `MANAGE_EXTERNAL_STORAGE`, `READ_LOGS`, `DUMP`, `WRITE_SECURE_SETTINGS` — high concern
- No API keys, OAuth tokens, passwords, or signing credentials in source or APK
- Root/Shizuku: never assumed, always consent-gated, graceful degradation
- Debug signing key (`testkey_untrusted.jks`) is NOT for production

---

## Testing Procedures

- Unit tests: `./gradlew testDebugUnitTest`
- Terminal emulator tests: `./gradlew :terminal-emulator:testDebugUnitTest`
- Lint: `./gradlew lintDebug`
- Native compilation: included in `assembleDebug` (NDK build)
- Every important feature must have tests and error handling
- Maintain a working debug build after every milestone

---

## Git Conventions

- Branch: `feature/termux-workstation`
- No `git reset --hard` or destructive cleanup
- Never overwrite origin or existing git configuration
- Upstream remote: `https://github.com/termux/termux-app.git` (already configured as `upstream`)
- Commit messages: `Added:`, `Changed:`, `Fixed:`, `Security:`
- Do not push commits or publish releases without explicit approval

---

## Feature Development Workflow

1. USER REQUIREMENT → PRINCIPAL ARCHITECT → REQUIREMENT ANALYSIS
2. SUBAGENT DELEGATION (Android, Terminal, UI/UX, Security, QA)
3. INTEGRATION REVIEW → TESTING → APK BUILD → FINAL REVIEW
4. Principal architect reconciles conflicts and owns final integration

---

## Code Review Standards

- DRY/SOLID compliance
- Minimal diff hunks for surgical fixes
- No placeholders; zero-placeholder rule strictly enforced
- Zero-trust verification: never accept completion claims without empirical test execution (exit code 0)
- Preserve existing terminal functionality, session management, plugin compatibility

---

## Kilo Agent Ecosystem

Project agents in `.kilo/agent/`:
- `termux-architect` — overall engineering decisions, architecture, integration
- `android-engineer` — Java/Kotlin, Android lifecycle, services, permissions, native UI
- `terminal-engineer` — VT100/VT220, ANSI, escape sequences, rendering, Unicode, performance
- `ui-ux-engineer` — Material Design, mobile UI, dark/light themes, adaptive layouts
- `ai-integration-engineer` — coding assistant CLI integration, MCP, model-provider interop
- `security-engineer` — permissions, exported components, storage isolation, credential protection
- `qa-engineer` — unit tests, integration tests, regression testing, performance validation
- `build-release-engineer` — Gradle, NDK, SDK, APK generation, signing, reproducibility

Project skills in `.kilo/skills/`:
- `termux-architecture`, `android-native-development`, `terminal-emulator-development`
- `android-ui-design`, `android-security-audit`, `android-build-release`
- `ai-cli-integration`, `android-device-testing`

---

## Milestones

A — Baseline (complete: build verified, APK produced)
B — Agent ecosystem (this file + .kilo/ agents + skills)
C — Product architecture (backlog, UI/UX direction)
D — First implementation (highest-value, lowest-risk features)
E — Integration (no terminal regressions)
F — Quality (tests, lint, native compile, security)
G — APK (verified ARM64 debug APK + SHA-256)
H — Device testing (Galaxy S23 Ultra, Android 17)
I — Release readiness (documentation, engineering report)
