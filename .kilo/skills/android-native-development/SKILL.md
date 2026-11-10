---
name: android-native-development
description: Android native development with Java/Kotlin, Gradle, NDK, JNI, C/C++
---

# Android Native Development Skill

## Purpose
Build, debug, and optimize Android applications with native C/C++ code via JNI and NDK.

## Activation Conditions
- Modifying Java/Kotlin Android code
- Working with NDK, JNI, C/C++ native code
- Gradle build configuration
- APK generation and signing
- Native library compilation

## Technical Reference

### Build System
- Gradle 9.2.1, AGP 8.13.2
- `compileSdk 36`, `minSdk 21`, `targetSdk 28`
- NDK 29.0.14206865
- `externalNativeBuild.ndkBuild` with `Android.mk`
- ABI filters: x86, x86_64, armeabi-v7a, arm64-v8a

### Native Code Structure
| Module | Android.mk | Purpose |
|--------|-----------|---------|
| `:app` | `src/main/cpp/Android.mk` | Bootstrap utilities |
| `:terminal-emulator` | `src/main/jni/Android.mk` | Terminal emulation core |
| `:termux-shared` | `src/main/cpp/Android.mk` | Local socket utilities |

### Compiler Flags
`-std=c11 -Wall -Wextra -Werror -Os -fno-stack-protector -Wl,--gc-sections`

### JNI Bridge
- `JNI.java` declares native methods
- `termux.c` implements native functions
- Package: `com.termux.terminal`

### Signing
- Debug: `testkey_untrusted.jks` (in repo, NOT for production)
- Release: user-owned keystore outside repository

## Required Tools
- JDK 17+ (JDK 21 on this host)
- Android SDK 36, NDK 29
- Gradle wrapper 9.2.1

## Reusable Workflow
1. Modify Java/Kotlin code in `src/main/java/`
2. Modify C code in `src/main/jni/` or `src/main/cpp/`
3. Update `Android.mk` if adding sources
4. Build: `./gradlew assembleDebug`
5. Test: `./gradlew testDebugUnitTest`

## Verification Procedure
```bash
export JAVA_HOME=/data/data/com.termux/files/usr/lib/jvm/java-21-openjdk
export ANDROID_HOME=/data/data/com.termux/files/usr/opt/android-sdk
export ANDROID_SDK_ROOT=/data/data/com.termux/files/usr/opt/android-sdk
TERMUX_PACKAGE_VARIANT=apt-android-7 ./gradlew :app:assembleDebug --no-daemon
./gradlew testDebugUnitTest
./gradlew lintDebug
```

## Common Failure Modes
- NDK host binary architecture mismatch (x86_64 on aarch64)
- JDK version mismatch (AGP 8.13.2 requires JDK 17+)
- Missing SDK platform (compileSdk not installed)
- JNI signature mismatch
- Native library not packaged (check `useLegacyPackaging`)
