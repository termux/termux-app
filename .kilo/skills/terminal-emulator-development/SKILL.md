---
name: terminal-emulator-development
description: VT100/VT220 terminal emulation, ANSI escape sequences, rendering, Unicode, performance
---

# Terminal Emulator Development Skill

## Purpose
Develop and maintain the Termux terminal emulation engine — VT100/VT220 state machine, ANSI escape sequences, screen buffer, rendering, Unicode handling, and performance optimization.

## Activation Conditions
- Modifying terminal emulation core
- Working with ANSI escape sequences
- Screen buffer and rendering changes
- Unicode/emoji/combining character handling
- Keyboard input processing
- Image protocol support (Sixel, iTerm)
- Performance optimization

## Technical Reference

### Core Classes (terminal-emulator/src/main/java/com/termux/terminal/)
| Class | Responsibility |
|-------|----------------|
| `TerminalEmulator` | VT100/VT220 state machine, input processing |
| `TerminalBuffer` | Screen buffer management, scrollback |
| `TerminalRow` | Row data, line wrapping, dirty tracking |
| `TerminalColors` | ANSI 16/256/truecolor palette |
| `TerminalColorScheme` | Color scheme definitions |
| `KeyHandler` | Keyboard input → escape sequences |
| `TerminalSession` | Pty fork, shell exec, session lifecycle |
| `JNI` | Native bridge to `termux.c` |
| `ITermImage` | Image protocol interface |
| `TerminalSixel` | DEC Sixel protocol (DCS q) |
| `TerminalBitmap` | iTerm image protocol (OSC 1337) |
| `WcWidth` | Unicode character width calculation |

### Native Code
- `terminal-emulator/src/main/jni/termux.c` — low-level terminal ops
- `terminal-emulator/src/main/jni/Android.mk` — NDK build config

### Tests (18 test classes)
- `ApcTest`, `ByteQueueTest`, `ControlSequenceIntroducerTest`
- `CursorAndScreenTest`, `DecSetTest`, `DeviceControlStringTest`
- `HistoryTest`, `KeyHandlerTest`, `OperatingSystemControlTest`
- `RectangularAreasTest`, `ResizeTest`, `ScreenBufferTest`
- `ScrollRegionTest`, `TerminalRowTest`, `TerminalTest`
- `TextStyleTest`, `UnicodeInputTest`, `WcWidthTest`

### Terminal View (terminal-view/)
- `TerminalView.java` — main view
- `TerminalRenderer.java` — rendering
- `GestureAndScaleRecognizer.java` — pinch/zoom
- Text selection: `CursorController`, `TextSelectionHandleView`

## Required Tools
- JDK 17+, Android SDK 36, NDK 29
- Gradle 9.2.1, AGP 8.13.2

## Reusable Workflow
1. Identify affected component (emulator, buffer, colors, input, rendering, images)
2. Write/update unit tests first
3. Implement changes preserving VT100/VT220 compliance
4. Test with shell, SSH, editors (vim/nano), TUI programs (htop, tmux)
5. Run all 18 terminal-emulator tests

## Verification Procedure
```bash
./gradlew :terminal-emulator:testDebugUnitTest
./gradlew :terminal-emulator:assembleDebug
./gradlew :terminal-view:assembleDebug
```

## Common Failure Modes
- Breaking standard shell applications (bash, zsh, fish)
- Breaking SSH sessions
- Breaking editors (vim, nano, emacs)
- Breaking TUI programs (htop, tmux, less)
- Unicode rendering issues (emoji, combining chars)
- Image protocol memory leaks
- Performance regression on large scrollback
