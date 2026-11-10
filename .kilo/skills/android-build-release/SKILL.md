---
name: android-build-release
description: Gradle, NDK, SDK, APK generation, signing, reproducibility, CI/CD
---

# Android Build & Release Skill

## Purpose
Configure, optimize, and verify Android builds — Gradle, NDK, SDK, APK generation, signing, reproducibility, CI/CD pipelines.

## Activation Conditions
- Modifying build.gradle files
- Gradle/AGP/NDK/SDK version changes
- APK signing configuration
- CI/CD workflow changes
- Build reproducibility issues
- ABI split configuration

## Technical Reference

### Build Configuration
| File | Purpose |
|------|---------|
| `build.gradle` (root) | AGP 8.13.2, repositories |
| `app/build.gradle` | App module config, signing, splits, NDK |
| `terminal-emulator/build.gradle` | Terminal emulator module |
| `terminal-view/build.gradle` | Terminal view module |
| `termux-shared/build.gradle` | Shared module |
| `gradle.properties` | SDK/NDK versions, JVM args |
| `gradle/wrapper/gradle-wrapper.properties` | Gradle 9.2.1 |

### Key Settings
- `compileSdk 36`, `minSdk 21`, `targetSdk 28`
- `ndkVersion 29.0.14206865`
- `packageVariant = apt-android-7` (TERMUX_PACKAGE_VARIANT)
- ABI splits: x86, x86_64, armeabi-v7a, arm64-v8a
- `universalApk true`
- `useLegacyPackaging true` (uncompressed JNI libs)
- `shrinkResources false` (reproducible builds)

### Build Commands
```bash
# Debug (ARM64 split)
export JAVA_HOME=/data/data/com.termux/files/usr/lib/jvm/java-21-openjdk
export ANDROID_HOME=/data/data/com.termux/files/usr/opt/android-sdk
export ANDROID_SDK_ROOT=/data/data/com.termux/files/usr/opt/android-sdk
TERMUX_PACKAGE_VARIANT=apt-android-7 TERMUX_SPLIT_APKS_FOR_DEBUG_BUILDS=1 ./gradlew :app:assembleDebug --no-daemon

# Release
TERMUX_PACKAGE_VARIANT=apt-android-7 ./gradlew :app:assembleRelease
```

### Signing
- Debug: `testkey_untrusted.jks` (in repo, NOT for production)
- Release: user-owned keystore outside repository
- `minifyEnabled true`, `shrinkResources false` for release

### CI/CD (GitHub Actions)
- `.github/workflows/` — Gradle validation, unit tests, native compilation, lint, debug APK builds
- Dependency reporting, security review, APK integrity, SHA-256 checksums
- Separate development and release builds

### Reproducibility
- Fixed SDK/NDK/JDK versions
- `shrinkResources false`
- Deterministic builds
- Document build environment

## Required Tools
- Gradle 9.2.1, AGP 8.13.2
- JDK 17+ (JDK 21 on this host)
- Android SDK 36, NDK 29
- GitHub Actions

## Reusable Workflow
1. Update versions in `gradle.properties` and `build.gradle`
2. Sync Gradle, verify no version conflicts
3. Run full test matrix: `testDebugUnitTest`, `lintDebug`, `assembleDebug`
4. Verify APK: ABI splits, size, SHA-256
5. Update CI/CD if version changes
6. Document changes in CHANGELOG

## Verification Procedure
```bash
./gradlew testDebugUnitTest
./gradlew lintDebug
./gradlew :app:assembleDebug
sha256sum app/build/outputs/apk/debug/*.apk
```

## Common Failure Modes
- AGP/JDK version mismatch (AGP 8.13.2 requires JDK 17+)
- NDK host binary architecture mismatch
- Missing SDK platform (compileSdk not installed)
- Gradle wrapper version incompatible with AGP
- Signing key issues (debug vs release)
- ABI split configuration errors
- Non-reproducible builds (timestamps, non-deterministic ordering)
