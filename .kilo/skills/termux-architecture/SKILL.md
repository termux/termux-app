---
name: termux-architecture
description: Termux app architecture, module structure, terminal engine, bootstrap, plugin integration
---

# Termux Architecture Skill

## Purpose
Understand and work with the Termux app's four-module Gradle architecture, terminal emulation engine, bootstrap mechanism, and plugin integration points.

## Activation Conditions
- Working with Termux app architecture
- Modifying module structure or dependencies
- Understanding terminal engine integration
- Working with bootstrap download/verification
- Plugin compatibility (Termux:API, Styling, Boot, Tasker, Widget, Float)

## Technical Reference

### Module Structure
| Module | Path | Responsibility |
|--------|------|----------------|
| `:app` | `app/` | Application shell, activities, settings, file provider, bootstrap |
| `:terminal-view` | `terminal-view/` | Terminal rendering, gestures, text selection, keyboard toolbar |
| `:terminal-emulator` | `terminal-emulator/` | VT100/VT220 emulation engine, ANSI, JNI + C core |
| `:termux-shared` | `termux-shared/` | Shared activities, native local socket |

### Terminal Engine Core
- `TerminalEmulator.java` — VT100/VT220 state machine
- `TerminalBuffer.java`, `TerminalRow.java` — screen buffer
- `TerminalColors.java`, `TerminalColorScheme.java` — ANSI colors
- `KeyHandler.java` — keyboard input
- `TerminalSession.java` — pty fork, shell exec
- `JNI.java` + `termux.c` — native bridge
- `ITermImage.java`, `TerminalSixel.java`, `TerminalBitmap.java` — Sixel/iTerm images

### Bootstrap
- Downloaded from `github.com/termux/termux-packages/releases`
- SHA-256 verified before embedding
- Variant: `apt-android-7` (set via `TERMUX_PACKAGE_VARIANT`)
- Architecture-specific: aarch64, arm, i686, x86_64

### Plugin Integration Points
- `sharedUserId=com.termux` — requires signature match
- `RUN_COMMAND` permission — dangerous, for plugin IPC
- Content providers: `${TERMUX_PACKAGE_NAME}.documents`, `${TERMUX_PACKAGE_NAME}.files`
- Exported services: `RunCommandService`

## Required Tools
- Gradle 9.2.1, AGP 8.13.2
- JDK 17+ (JDK 21 on this host)
- Android SDK 36, NDK 29.0.14206865

## Reusable Workflow
1. Identify which module(s) affected
2. Check terminal engine impact (never break shell/SSH/editors/TUI)
3. Verify bootstrap compatibility if changing variant
4. Test plugin integration points
5. Run module-specific tests

## Verification Procedure
```bash
./gradlew :app:assembleDebug
./gradlew :terminal-emulator:testDebugUnitTest
./gradlew lintDebug
```

## Common Failure Modes
- Breaking terminal emulation (regressions in escape sequences)
- Bootstrap checksum mismatch
- Plugin signature mismatch
- NDK host binary architecture mismatch (x86_64 on aarch64 host)
