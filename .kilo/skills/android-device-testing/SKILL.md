---
name: android-device-testing
description: Android device testing — ADB, installation, runtime verification, performance, battery, compatibility
---

# Android Device Testing Skill

## Purpose
Test Termux Workstation Forge on real Android devices — installation, runtime verification, performance, battery, compatibility across Android versions.

## Activation Conditions
- Testing on physical Android device
- ADB device connection and verification
- APK installation and signature verification
- Runtime testing (bootstrap, shell, sessions)
- Performance and battery testing
- Multi-window, rotation, background testing

## Technical Reference

### Target Device
- Samsung Galaxy S23 Ultra (SM-S918B)
- ARM64 (arm64-v8a)
- Android 17 / API 37
- One UI 6+

### ADB Commands
```bash
adb devices -l                    # List connected devices
adb install -r app.apk            # Install/replace APK
adb shell pm list packages com.termux  # Verify package
adb shell am start -n com.termux/.app.TermuxActivity  # Launch
adb logcat -s Termux:*            # Filter logs
adb shell dumpsys package com.termux  # Package info
```

### Test Matrix
| Test | Description |
|------|-------------|
| APK installation | Verify signature compatibility before install |
| First launch | Bootstrap initialization, shell startup |
| Package management | `pkg update`, `pkg install` |
| Interactive apps | vim, nano, htop, tmux, ssh |
| Session switching | Multiple terminals, rename, close |
| Keyboard input | Soft keyboard, hardware keys, external keyboard |
| Landscape | Rotation handling |
| Multi-window | Split screen, freeform |
| Storage permissions | Scoped storage, MANAGE_EXTERNAL_STORAGE |
| Clipboard | Copy/paste between apps |
| File sharing | Send/receive files |
| Long-running processes | Compile, download, server |
| Background execution | Foreground service, battery optimization |
| Battery behavior | Wake locks, CPU usage |
| Network changes | WiFi/cellular switching |
| Screen rotation | Portrait/landscape |
| Process recovery | App kill/restart |

### Signature Verification
```bash
# Before installing, verify signing certificate matches existing Termux
adb shell pm dump com.termux | grep -A5 "signatures"
# Compare with new APK
apksigner verify --print-certs app.apk
```

### Backup & Recovery
1. Identify installed package version
2. Inspect signing certificate compatibility
3. Verify package identity
4. Establish backup procedure (Termux data, $PREFIX)
5. If certificates don't match — STOP, do not uninstall

## Required Tools
- ADB (Android Debug Bridge)
- Target device (Galaxy S23 Ultra, Android 17)
- USB debugging enabled
- Wireless debugging (optional)

## Reusable Workflow
1. Connect device via ADB (`adb devices -l`)
2. Verify signature compatibility
3. Install APK (`adb install -r`)
4. Run test matrix systematically
5. Log defects with steps to reproduce
6. Fix actual defects (not hide them)
7. Retest after fixes

## Verification Procedure
```bash
adb devices -l
adb install -r app/build/outputs/apk/debug/termux-app_apt-android-7-debug_arm64-v8a.apk
adb shell am start -n com.termux/.app.TermuxActivity
# Manual testing per test matrix
```

## Common Failure Modes
- Signature mismatch (cannot upgrade existing Termux)
- Bootstrap download failure (network, checksum)
- Scoped storage permission denial
- Background service killed by battery optimization
- Multi-window rendering issues
- Keyboard input not reaching terminal
- Native library loading failure (wrong ABI)
- Battery drain from wake locks
